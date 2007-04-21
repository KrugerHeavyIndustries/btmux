noinst_LTLIBRARIES = libbtfi.la

libbtfi_la_SOURCES = sax.cc sax.h \
	Document.cc vocab.cc encalg.c stream.c common.c \
	Document.hh vocab.hh encalg.h stream.h common.h

check_PROGRAMS = saxtest vocabtest

TESTS = saxtest vocabtest

saxtest_SOURCES = tests/dummy.cc tests/saxtest.c
saxtest_LDADD = libbtfi.la

vocabtest_SOURCES = tests/vocabtest.cc
vocabtest_LDADD = libbtfi.la
