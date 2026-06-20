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

Нужен монотонный курсор для возобновления после реконнекта. Базируемся на
существующем `wal::Position{segment_no, page_no}`, расширенном смещением внутри страницы.

```proto
message ReplPosition {
  int64 segment_no = 1;
  int64 page_no    = 2;
  int64 offset     = 3;   // смещение внутри страницы
}

message GetEventsRequest {
  ReplPosition from = 1;   // было пусто; follower возобновляется с этой позиции
}

message WalEvent {
  ReplPosition position = 1;   // НОВОЕ: курсор события
  oneof event {
    LsmStorageUpsertEvent lsm_storage_upsert_event = 2;
    TxStorageCommitEvent  tx_storage_commit_event  = 3;   // НОВОЕ: сохранить tx-границы
  }
}
```

Добавление `TxStorageCommitEvent` в `oneof` обязательно: без него теряются
транзакционные границы. Реплика применяет upsert'ы и commit в том же порядке, что и leader.

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

1. **MVP single-follower:** `ReplPosition` + `TxStorageCommitEvent` в proto;
   `ReplicationService` (только catch-up чтением файлов, поллинг новых сегментов);
   `ReplicationFollower` job; read-only режим; ручной bootstrap копированием `root`;
   cleaner временно отключён на leader.
2. **Live-tail + retention:** publisher в `Writer`; учёт min-позиции реплик в `WalCleaner`.
3. **Snapshot RPC** для автоматического bootstrap нескольких followers.
4. **Failover** (ручной промоушн follower → leader).

## 8. Вне scope v1 (на будущее)

- Semi-sync / кворумная репликация, синхронные подтверждения.
- Автоматический failover / выбор лидера (consensus).
- Автоматический snapshot-bootstrap (`Snapshot` RPC).
- Каскадная репликация (follower-of-follower).
</content>
</invoke>
