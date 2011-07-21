
# The name of the library
lib_LTLIBRARIES = libasyncaudio.la

DIST_CPP = \
		AsyncAudioCompressor.cpp \
		AsyncAudioDecimator.cpp \
		AsyncAudioDecoder.cpp \
		AsyncAudioDecoderGsm.cpp \
		AsyncAudioDecoderS16.cpp \
		AsyncAudioDelayLine.cpp \
		AsyncAudioDevice.cpp \
		AsyncAudioDeviceFactory.cpp \
		AsyncAudioEncoder.cpp \
		AsyncAudioEncoderGsm.cpp \
		AsyncAudioEncoderS16.cpp \
		AsyncAudioFifo.cpp \
		AsyncAudioFilter.cpp \
		AsyncAudioInterpolator.cpp \
		AsyncAudioIO.cpp \
		AsyncAudioJitterFifo.cpp \
		AsyncAudioMixer.cpp \
		AsyncAudioPacer.cpp \
		AsyncAudioProcessor.cpp \
		AsyncAudioReader.cpp \
		AsyncAudioRecorder.cpp \
		AsyncAudioSelector.cpp \
		AsyncAudioSink.cpp \
		AsyncAudioSource.cpp \
		AsyncAudioSplitter.cpp \
		fidlib.c

DIST_H_PUBLIC = \
		AsyncAudioAmp.h \
		AsyncAudioClipper.h \
		AsyncAudioCompressor.h \
		AsyncAudioDebugger.h \
		AsyncAudioDecimator.h \
		AsyncAudioDecoder.h \
		AsyncAudioDelayLine.h \
		AsyncAudioDevice.h \
		AsyncAudioDeviceFactory.h \
		AsyncAudioEncoder.h \
		AsyncAudioFifo.h \
		AsyncAudioFilter.h \
		AsyncAudioInterpolator.h \
		AsyncAudioIO.h \
		AsyncAudioJitterFifo.h \
		AsyncAudioMixer.h \
		AsyncAudioPacer.h \
		AsyncAudioPassthrough.h \
		AsyncAudioProcessor.h \
		AsyncAudioReader.h \
		AsyncAudioRecorder.h \
		AsyncAudioSelector.h \
		AsyncAudioSink.h \
		AsyncAudioSource.h \
		AsyncAudioSplitter.h \
		AsyncAudioStreamStateDetector.h \
		AsyncAudioValve.h \
		SigCAudioSink.h \
		SigCAudioSource.h

DIST_H_PRIVATE = \
		AsyncAudioDecoderGsm.h \
		AsyncAudioDecoderRaw.h \
		AsyncAudioDecoderS16.h \
		AsyncAudioEncoderGsm.h \
		AsyncAudioEncoderRaw.h \
		AsyncAudioEncoderS16.h \
		fidlib.h \
		fidmkf.h \
		fidrf_cmdlist.h

DIST_H = $(DIST_H_PUBLIC) $(DIST_H_PRIVATE)

ALSA_CPP = AsyncAudioDeviceAlsa.cpp
ALSA_H = AsyncAudioDeviceAlsa.h

OSS_CPP = AsyncAudioDeviceOSS.cpp
OSS_H = AsyncAudioDeviceOSS.h

SPEEX_CPP = AsyncAudioDecoderSpeex.cpp AsyncAudioEncoderSpeex.cpp
SPEEX_H = AsyncAudioDecoderSpeex.h AsyncAudioEncoderSpeex.h

OPTIONAL_CPP =
OPTIONAL_H =

if HAVE_ALSA
OPTIONAL_CPP += $(ALSA_CPP)
OPTIONAL_H += $(ALSA_H)
endif

if HAVE_OSS
OPTIONAL_CPP += $(OSS_CPP)
OPTIONAL_H += $(OSS_H)
endif

if USE_SPEEX
OPTIONAL_CPP += $(SPEEX_CPP)
OPTIONAL_H += $(SPEEX_H)
endif

# Sources needed to build the library
libasyncaudio_la_SOURCES = $(DIST_CPP) $(DIST_H) $(OPTIONAL_CPP) $(OPTIONAL_H)

# Headers that define the API of this library
pkginclude_HEADERS = $(DIST_H_PUBLIC) $(OPTIONAL_H)

# required by automake for optional sources
EXTRA_libasyncaudio_la_SOURCES = $(SPEEX_CPP) $(SPEEX_H) \
		$(ALSA_CPP) $(ALSA_H) $(OSS_CPP) $(OSS_H)

# Extra compiler flags
AM_CPPFLAGS += @ASYNC_AUDIO_CFLAGS@

RELEASE = @LIBASYNC_SHLIB_VERSION@

libasyncaudio_la_LDFLAGS = -release $(RELEASE)

# Files that should be packaged with the sources
EXTRA_DIST = fidlib.txt SampleFifo.cpp SampleFifo.h Makefile.default


install-exec-hook:
	@INSTDIR="$(DESTDIR)@libdir@" ; \
	for lafile in $(lib_LTLIBRARIES) ; \
	do \
	echo "Removing $${lafile} from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${lafile} ; \
	done

uninstall-hook:
	@INSTDIR="$(DESTDIR)@libdir@" ; \
	for lafile in $(lib_LTLIBRARIES) ; \
	do \
	LIBBASE=`echo $${lafile} | ${AWK} -F. '/^.*\.la$$/ {print $$1}'` ; \
	echo "Removing $${LIBBASE}.a from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBBASE}.a ; \
	echo "Removing $${LIBBASE}.so from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBBASE}.so ; \
	echo "Removing $${LIBBASE}-$(RELEASE).so from $${INSTDIR}" ; \
	$(RM) $${INSTDIR}/$${LIBBASE}-$(RELEASE).so ; \
	done
