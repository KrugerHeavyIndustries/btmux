noinst_LTLIBRARIES = libbtfi.la

libbtfi_la_SOURCES = \
	sax.cc encutil.cc Document.cc Element.cc \
	sax.h  encutil.hh Document.hh Element.hh Attributes.hh \
	Name.cc         Value.cc          MutableAttributes.cc \
	Name.hh names.h Value.hh values.h MutableAttributes.hh attribs.h \
	Vocabulary.cc                            encalg.c stream.c common.c \
	Vocabulary.hh VocabTable.hh Exception.hh encalg.h stream.h common.h

check_PROGRAMS = saxtest vocabtest

TESTS = saxtest vocabtest

saxtest_SOURCES = tests/dummy.cc tests/saxtest.c
saxtest_LDADD = libbtfi.la

vocabtest_SOURCES = tests/vocabtest.cc
vocabtest_LDADD = libbtfi.la
