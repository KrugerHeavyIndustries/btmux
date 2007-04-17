noinst_LTLIBRARIES = libbtfi.la

libbtfi_la_SOURCES = sax.c vocab.cc encalg.c stream.c common.c \
	sax.h vocab.hh encalg.h stream.h common.h

check_PROGRAMS = saxtest vocabtest

TESTS = saxtest vocabtest

saxtest_SOURCES = tests/saxtest.c
saxtest_LDADD = libbtfi.la

vocabtest_SOURCES = tests/vocabtest.cc
vocabtest_LDADD = libbtfi.la
