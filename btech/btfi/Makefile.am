noinst_LTLIBRARIES = libbtfi.la

libbtfi_la_SOURCES = sax.cc sax.h \
	Document.cc Element.cc Attributes.cc MutableAttributes.cc \
	Document.hh Element.hh Attributes.hh MutableAttributes.hh \
	Name.cc Value.cc generic.cc \
	Name.hh Value.hh generic.h \
	vocab.cc encalg.c encutil.cc stream.c common.c \
	vocab.hh encalg.h encutil.hh stream.h common.h

check_PROGRAMS = saxtest vocabtest

TESTS = saxtest vocabtest

saxtest_SOURCES = tests/dummy.cc tests/saxtest.c
saxtest_LDADD = libbtfi.la

vocabtest_SOURCES = tests/vocabtest.cc
vocabtest_LDADD = libbtfi.la
