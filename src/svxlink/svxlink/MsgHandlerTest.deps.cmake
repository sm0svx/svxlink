# MsgHandlerTest exercises MsgHandler directly, so compile it in. MsgHandler is
# an Async::AudioSource (asyncaudio) and uses the GSM codec for .gsm clips.
set(MsgHandlerTest_EXTRA_SRCS MsgHandler.cpp)
set(MsgHandlerTest_EXTRA_LIBS asyncaudio ${GSM_LIBRARY})
