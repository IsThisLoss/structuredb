#include "blocking_executor.hpp"

namespace structuredb::server::io {

BlockingExecutor::BlockingExecutor(size_t threads_num)
  : thread_pool_{threads_num}
{}

}
