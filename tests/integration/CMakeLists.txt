# Python integration-test suite, registered with ctest.
#
# The test bootstraps its own virtualenv (run_tests.py) so the only host
# requirement is a Python 3 interpreter. It drives the freshly-built
# structuredb-server binary over gRPC.

option(STRUCTUREDB_INTEGRATION_TESTS "Enable the Python integration test suite" ON)

if(STRUCTUREDB_INTEGRATION_TESTS)
  find_package(Python3 COMPONENTS Interpreter)

  if(Python3_Interpreter_FOUND)
    add_test(
      NAME integration
      COMMAND
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py
        --server-binary $<TARGET_FILE:structuredb-server>
        --venv ${CMAKE_CURRENT_BINARY_DIR}/.venv
        -- -v
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    # The suite spawns servers and waits on background jobs/replication.
    set_tests_properties(integration PROPERTIES
      LABELS "integration"
      TIMEOUT 600
    )
  else()
    message(WARNING "Python3 not found; integration test suite disabled")
  endif()
endif()
