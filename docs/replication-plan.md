# Replication Design & Implementation Plan

Status: **Draft / Proposal**
Scope: добавить асинхронную репликацию (leader → followers) в StructureDB.

## 1. Цель и гарантии

Добавить **асинхронную log-shipping репликацию**: один **leader** принимает записи,
один или несколько **followers** в режиме read-only тянут поток WAL-событий и
применяют их у себя.

**Гарантии (v1): асинхронная репликация.**

- Подтверждение записи клиенту происходит **до** отгрузки события репликам.
- Возможна потеря «неотгруженного хвоста» WAL при потере leader'а
  (последние записи, не успевшие уйти на follower).
- Реплики **eventually consistent**: догоняют leader с задержкой.
- Followers строго read-only; запись через них запрещена.

Semi-sync / кворум / автоматический failover — **вне scope v1**, см. §8.

## 2. На что опираемся (уже есть в коде)

- `proto/replication_service.proto` — каркас: `service Replication { rpc GetEvents(...) returns (stream WalEvent) }`.
  Сервис **ещё не зарегистрирован** в `main.cpp`, реализации нет.
- WAL как готовый replication log:
  - `wal::Writer` (`src/server/wal/writer.*`) — пишет `Event` в сегменты
    (страницы по `kWalPageSize = 4096`), `FSync` после каждой записи.
  - `wal::Reader` / `ReaderStrategy` (`src/server/wal/reader.*`) — постраничное чтение сегментов.
  - `wal::Recover` (`src/server/wal/recovery.*`) — проигрывает события через `Event::Apply(db)`.
    **Применение событий на реплике — это, по сути, recovery в реальном времени.**
- События: `LsmStorageUpsertEvent`, `TxStorageCommitEvent` (`src/server/wal/events/`).
- **DDL реплицируется «бесплатно»**: `Catalog` (`src/server/database/catalog.hpp`) —
  это обычная таблица `sys_tables`; `CREATE TABLE` идёт обычным upsert-путём и
  попадает в WAL как `LsmStorageUpsertEvent` с `storage_id` каталога.
- Фоновые задачи через `database/jobs/*` + `JobLauncher` — готовый паттерн для
  follower-корутины.
- `WalCleaner` (`database/jobs/wal_cleaner.*`) — удаляет старые сегменты после
  персиста. **Конфликтует с репликацией** (см. §5.6).

## 3. Архитектура

Pull-модель: follower подключается к leader и просит события начиная с позиции.
Leader не обязан знать о followers заранее — ложится на существующий `GetEvents` stream.

```
        WRITE                          GetEvents(stream)
 client ──▶ LsmEngine ──▶ Writer ──fsync──▶ WAL files
                              │                 │
                              └─ publisher ─────┼──▶ ReplicationService ──▶ gRPC stream
                                  (live tail)   │            ▲
                                                └ catch-up (Reader by position)
                                                             │
   follower:  ReplicationFollower ◀── stream ────────────────┘
                  │ write to own WAL → Event::Apply(db) → own LSM
                  └ persist applied position (resume on reconnect)
```

## 4. Протокол (изменения в `proto/replication_service.proto`)

**Решение: физический log-shipping (пересылка сырых WAL-страниц), а не логический
oneof по типам событий.**

Обоснование:
- WAL уже сериализует события (`FlushEvent`) и десериализует (`ParseEvent`, `Recover`).
  Пересылая сырые страницы, мы переиспользуем этот код целиком и **не дублируем**
  каждый тип события в proto и в конвертерах туда-обратно.
- Поля `Event` приватны и без геттеров — логическая конвертация на leader потребовала бы
  ломать инкапсуляцию. Сырые байты этого не требуют.
- Новые типы событий и DDL реплицируются автоматически (DDL = upsert в `sys_tables`).
- Follower записывает полученные страницы в свой WAL «как есть» → его recovery на рестарте
  работает тем же кодом, что и обычный recovery.

Курсор для возобновления: `wal::Position{segment_no, page_no}` (при `kMaxPagesInSegment = 1`
сегмент = одна страница). Гранулярность возобновления — страница; повторная отправка
страницы при реконнекте безопасна за счёт идемпотентности `Apply` (см. §6).

```proto
message ReplPosition {
  int64 segment_no = 1;
  int64 page_no    = 2;
}

message GetEventsRequest {
  ReplPosition from = 1;   // было пусто; follower возобновляется с этой позиции
}

message WalPage {
  ReplPosition position = 1;   // позиция страницы в WAL
  bytes        data     = 2;   // сырые байты страницы (sdb-формат, как в WAL-файле)
}

service Replication {
  rpc GetEvents(GetEventsRequest) returns (stream WalPage) {}
}
```

Транзакционные границы сохраняются автоматически: страница содержит события (включая
`TxStorageCommitEvent`) в том же порядке и тех же байтах, что и в WAL leader'а.

## 5. Компоненты для реализации

### 5.1. Позиция/LSN
Ввести `ReplPosition` сквозь WAL: `Writer` должен уметь сообщать позицию только что
записанного события; `Reader`/`ReaderStrategy` уже оперируют `Position` — расширить `offset`.

### 5.2. Leader: отдача потока (гибрид catch-up + live-tail)
Новый `src/server/services/replication_service/`.

- **Catch-up:** по `from`-позиции читаем WAL-файлы существующим `Reader` + `ParseEvent`,
  стримим события клиенту.
- **Live-tail:** добавить в `wal::Writer` лёгкий publisher (список подписчиков);
  после `FSync` рассылать записанное событие + позицию в активные стримы.
  Шлём только durable-данные, без поллинга, низкая задержка.

Регистрация сервиса в `main.cpp` рядом с `table_service` / `transaction_service`.

### 5.3. Follower: применение
Новый `src/server/database/jobs/replication_follower.hpp` (фоновая корутина):

1. gRPC-клиентом подключается к leader, вызывает `GetEvents(from = последняя_применённая_позиция)`.
2. На каждое `WalEvent`: реконструирует `Event` → пишет в **свой** WAL
   (собственная durability/recovery) → `event->Apply(db)`.
3. Персистит применённую позицию для возобновления (в свой WAL или отдельный `repl_state`-файл).
4. Реконнект с backoff при обрыве стрима.

По сути follower = `RecoveryStrategy` поверх сети — большая часть логики уже в `recovery.cpp`.

### 5.4. Read-only режим
При `role: follower` table/transaction-сервисы на запись возвращают ошибку
«read-only replica». Чтения обслуживаются локально.

### 5.5. Bootstrap нового follower (snapshot)
Чистый follower не доедет только по WAL — старые сегменты удалены `WalCleaner`.
Нужен базовый снапшот: скопировать текущие `sdb`-файлы (`root`) + позицию, на которой
снапшот консистентен, и с неё начать стрим.

- **v1 (ручной):** на время копирования приостановить compaction/cleaner, скопировать
  `root`, запомнить позицию, стартовать follower с неё. Копирование `root` целиком
  автоматически сохраняет совпадение `storage_id` таблиц.
- Позже: `Snapshot` RPC для автоматического bootstrap (см. §8).

### 5.6. Retention: `WalCleaner` ↔ репликация
`WalCleaner` не должен удалять сегменты, ещё не отгруженные всем followers.
Ввести **минимальную удерживаемую позицию** = min(подтверждённых offset активных реплик)
и чистить только ниже неё.

### 5.7. Конфигурация (`config.yaml` + `cfg/config.hpp`)
```yaml
replication:
  role: leader            # leader | follower
  leader_address: ""      # для follower: "host:port"
```
При `role: follower` — поднять `ReplicationFollower` job и включить read-only.

## 6. Корректность

- **Идемпотентность Apply:** при реконнекте возможна повторная отправка событий
  вокруг последней позиции — `Event::Apply` / `IsPersistent` должны быть идемпотентны
  по `seq_no`. **Проверить** `LsmStorageUpsertEvent::Apply`.
- **Порядок:** строго сохраняем порядок WAL (включая commit-события).
- **`storage_id`:** follower обязан иметь те же id таблиц, что leader —
  обеспечивается копированием `root` при bootstrap.

## 7. Этапы внедрения

1. **MVP single-follower** — ✅ сделано: physical WAL-page protocol; leader
   `ReplicationService` (polling, шлёт и in-progress страницу); `replication::Follower`
   (зеркалит WAL + apply, resume по позиции); read-only режим; ленивая материализация
   таблиц (DDL). Проверено end-to-end (leader→follower, live, рестарт-resume).
2. **Retention** — ✅ сделано: `FollowerRegistry` + удержание сегментов в `WalCleaner`.
   *Live-tail publisher в `Writer`* — отложено: poll (`poll_interval`, по умолчанию 200мс)
   уже подхватывает незавершённую страницу, отдельный publisher не нужен для MVP.
3. **Snapshot bootstrap** — ручная процедура (копирование `root`), см.
   [`replication.md`](replication.md). Автоматический `Snapshot` RPC — вне scope v1 (§8).
4. **Failover** — ручной промоушн follower → leader сменой роли в конфиге, см.
   [`replication.md`](replication.md). Автоматический failover — вне scope v1 (§8).

## 8. Вне scope v1 (на будущее)

- Semi-sync / кворумная репликация, синхронные подтверждения.
- Автоматический failover / выбор лидера (consensus).
- Автоматический snapshot-bootstrap (`Snapshot` RPC).
- Каскадная репликация (follower-of-follower).
</content>
</invoke>
