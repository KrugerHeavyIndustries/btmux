noinst_LTLIBRARIES = libbtfi.la

libbtfi_la_SOURCES = sax.cc sax.h \
	Document.cc Element.cc \
	Document.hh Element.hh Attributes.hh \
	Name.cc         Value.cc          MutableAttributes.cc \
	Name.hh names.h Value.hh values.h MutableAttributes.hh attribs.h \
	vocab.cc encalg.c encutil.cc stream.c common.c \
	vocab.hh encalg.h encutil.hh stream.h common.h Exception.hh

check_PROGRAMS = saxtest vocabtest

TESTS = saxtest vocabtest

saxtest_SOURCES = tests/dummy.cc tests/saxtest.c
saxtest_LDADD = libbtfi.la

vocabtest_SOURCES = tests/vocabtest.cc
vocabtest_LDADD = libbtfi.la
