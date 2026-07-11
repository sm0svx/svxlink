# The Opus encoder is only built when the Opus library is found, so skip
# this test otherwise (see the <Name>_SKIP variable in SvxLinkAddTests.cmake).
if(NOT Opus_FOUND)
  set(AudioEncoderOpusTest_SKIP TRUE)
endif()
