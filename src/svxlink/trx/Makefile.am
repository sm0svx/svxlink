# Import some general items
include $(top_srcdir)/common.mk

lib_LTLIBRARIES = libtrx.la

# Sources needed to build the library
DIST_CPP = \
		DtmfDecoder.cpp \
		DtmfEncoder.cpp \
		HwDtmfDecoder.cpp \
		LocalRx.cpp \
		LocalTx.cpp \
		MultiTx.cpp \
		NetRx.cpp \
		NetTrxTcpClient.cpp \
		NetTx.cpp \
		PttCtrl.cpp \
		Rx.cpp \
		S54sDtmfDecoder.cpp \
		Sel5Decoder.cpp \
		SigLevDetNoise.cpp \
		SigLevDetTone.cpp \
		SquelchEvDev.cpp \
		SquelchVox.cpp \
		SwDtmfDecoder.cpp \
		SwSel5Decoder.cpp \
		ToneDetector.cpp \
		Tx.cpp \
		Voter.cpp

DIST_H_PUBLIC = \
		NetTrxMsg.h \
		Rx.h \
		Tx.h

DIST_H_PRIVATE = \
		DtmfDecoder.h \
		DtmfEncoder.h \
		Goertzel.h \
		HwDtmfDecoder.h \
		LocalRx.h \
		LocalTx.h \
		multirate_filter_coeff.h \
		MultiTx.h \
		NetRx.h \
		NetTrxTcpClient.h \
		NetTx.h \
		PttCtrl.h \
		S54sDtmfDecoder.h \
		Sel5Decoder.h \
		SigLevDet.h \
		SigLevDetNoise.h \
		SigLevDetTone.h \
		SquelchCtcss.h \
		SquelchEvDev.h \
		Squelch.h \
		SquelchSerial.h \
		SquelchSigLev.h \
		SquelchVox.h \
		SwDtmfDecoder.h \
		SwSel5Decoder.h \
		ToneDetector.h \
		Voter.h

DIST_H = $(DIST_H_PUBLIC) $(DIST_H_PRIVATE)

libtrx_la_SOURCES = $(DIST_CPP) $(DIST_H)

AM_CPPFLAGS += $(TRX_CFLAGS)

pkginclude_HEADERS = $(DIST_H_PUBLIC)

# SpanDtmfDecoder is distributed but not built
EXTRA_DIST = \
		SpanDtmfDecoder.cpp \
		SpanDtmfDecoder.h \
		Makefile.default


# not sure where this came from, but it does not seem to be needed
#AM_CXXFLAGS += -D__STDC_LIMIT_MACROS

