ACLOCAL_AMFLAGS = -I m4

if BTPR_MUX1
BTPR_INCLUDE = -I$(srcdir)/../include -I$(srcdir)/../src
endif

if BTPR_PENN
MAYBE_BTPR = btpr
BTPR_INCLUDE = -I$(srcdir)/btpr/penn/include -I$(srcdir)/btpr/include
endif

SUBDIRS = $(MAYBE_BTPR) src

AM_CPPFLAGS = $(BTPR_INCLUDE) -I$(srcdir)/src/include -I$(srcdir)/src/btech

check_PROGRAMS =

TESTS =
