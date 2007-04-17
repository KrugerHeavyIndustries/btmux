noinst_LTLIBRARIES = libbtfi.la

libbtfi_la_SOURCES = sax.c vocab.c encalg.c stream.c common.c \
	sax.h vocab.h encalg.h stream.h common.h

check_PROGRAMS = saxtest

TESTS = saxtest

saxtest_SOURCES = tests/saxtest.c
saxtest_LDADD = libbtfi.la
