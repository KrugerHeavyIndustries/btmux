AM_CPPFLAGS = -I $(srcdir)/include

noinst_LTLIBRARIES = libbtfi.la

libbtfi_la_SOURCES = \
	sax.cc                 Codec.cc Document.cc Element.cc \
	       Serializable.hh Codec.hh Document.hh Element.hh Attributes.hh \
	Name.cc Value.cc MutableAttributes.cc \
	Name.hh Value.hh MutableAttributes.hh \
	Vocabulary.cc VocabSimple.cc \
	Vocabulary.hh VocabSimple.hh VocabTable.hh \
	encalg.c stream.c errors.c \
	encalg.h stream.h common.h \
	ObjectPool.hh Exception.hh \
	include/sax.h include/names.h include/values.h include/attribs.h \
	include/vocab.h include/errors.h include/fiptypes.h

check_PROGRAMS = saxtest vocabtest

TESTS = saxtest vocabtest

saxtest_SOURCES = tests/saxtest.cc tests/saxwrite.c tests/saxread.c
saxtest_LDADD = libbtfi.la

vocabtest_SOURCES = tests/vocabtest.cc
vocabtest_LDADD = libbtfi.la
