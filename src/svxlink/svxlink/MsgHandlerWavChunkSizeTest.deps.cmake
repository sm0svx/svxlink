# MsgHandlerWavChunkSizeTest exercises MsgHandler/WavFileQueueItem directly,
# so compile MsgHandler.cpp in. MsgHandler is an Async::AudioSource
# (asyncaudio) and uses the GSM codec for .gsm clips.
set(MsgHandlerWavChunkSizeTest_EXTRA_SRCS MsgHandler.cpp)
set(MsgHandlerWavChunkSizeTest_EXTRA_LIBS asyncaudio ${GSM_LIBRARY})
