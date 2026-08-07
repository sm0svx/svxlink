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
