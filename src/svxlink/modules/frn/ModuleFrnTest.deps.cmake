# Extra sources/libs needed to link ModuleFrnTest.
#
# ModuleFrn.cpp requires QsoFrn.cpp/Utils.cpp (its module siblings) and
# Module.cpp (the svxlink core base class it derives from). Module.cpp
# in turn makes a few direct (non-virtual) calls into Logic:: and touches
# the LinkManager singleton pointer; ModuleFrnTestLinkStubs.cpp provides
# trivial link-only stand-ins for those so the real Logic/LinkManager
# subsystem (and the Async event loop / application wiring it needs)
# does not have to be pulled in for this standalone test.
set(ModuleFrnTest_EXTRA_SRCS
  ModuleFrn.cpp
  QsoFrn.cpp
  Utils.cpp
  ModuleFrnTestLinkStubs.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../svxlink/Module.cpp
  )
set(ModuleFrnTest_EXTRA_LIBS
  asynccpp asynccore asyncaudio svxmisc ${GSM_LIBRARY}
  )

# Module.cpp is compiled in only so the ModuleFrn base class links; it is
# not under test. With -fsanitize=undefined its vptr checks reference the
# RTTI of Logic and Rx, whose key functions live in the svxlink executable
# and the trx library, neither of which this test links. Turn just those
# checks off for Module.cpp (a no-op without a sanitizer) so the test also
# links in UBSan builds; ModuleFrn.cpp itself keeps full instrumentation.
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/../../svxlink/Module.cpp
  APPEND_STRING PROPERTY COMPILE_FLAGS " -fno-sanitize=vptr")
