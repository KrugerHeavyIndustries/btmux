if BTPR_MUX1
BTPR_INCLUDE = -I$(top_srcdir)/../include -I$(top_srcdir)/../src
endif

if BTPR_PENN
MAYBE_BTPR = btpr
BTPR_INCLUDE = -I$(top_srcdir)/btpr/penn/include -I$(top_srcdir)/btpr/include
endif

AM_CPPFLAGS = $(BTPR_INCLUDE) -I$(top_srcdir)/src/include -I$(top_srcdir)/src

check_PROGRAMS = #asn1tlv

TESTS = #asn1tlv

#asn1tlv_SOURCES = asn1tlv.c
#asn1tlv_LDADD = ../src/asn1tlv.lo
