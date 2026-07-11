# Exec's constructor sets up an Async::FdWatch (for the SIGCHLD pipe),
# which requires an Async::Application instance to exist. Link asynccpp
# for Async::CppApplication.
set(AsyncExecTest_EXTRA_LIBS asynccpp)
