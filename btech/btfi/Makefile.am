noinst_LTLIBRARIES = libbtfi.la

libbtfi_la_SOURCES = \
	sax.cc Codec.cc                 Document.cc Element.cc \
	sax.h  Codec.hh Serializable.hh Document.hh Element.hh Attributes.hh \
	Name.cc         Value.cc          MutableAttributes.cc \
	Name.hh names.h Value.hh values.h MutableAttributes.hh attribs.h \
	Vocabulary.cc VocabSimple.cc \
	Vocabulary.hh VocabSimple.hh VocabTable.hh \
	encalg.c stream.c errors.c \
	encalg.h stream.h errors.h common.h fiptypes.h \
	ObjectPool.hh Exception.hh

check_PROGRAMS = saxtest vocabtest

TESTS = saxtest vocabtest

saxtest_SOURCES = tests/saxtest.cc tests/saxwrite.c tests/saxread.c
saxtest_LDADD = libbtfi.la

vocabtest_SOURCES = tests/vocabtest.cc
vocabtest_LDADD = libbtfi.la
