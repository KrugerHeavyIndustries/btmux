# FIXME: We should eliminate the need to include btech/src/btech somehow.
AM_CPPFLAGS = -I$(top_srcdir)/include \
	-I$(top_srcdir)/btech/src/include -I$(top_srcdir)/btech/src/btech

# TODO: Fiddle with the default compiler options.
#CFLAGS = -Wall -Wno-unused
#ifdef DEBUG
#CFLAGS += -g3
#CPPFLAGS += -DDEBUG
#OBJSUFFIX = .debug
#else
#CFLAGS += -g -O3
#OBJSUFFIX =
#endif

bin_PROGRAMS = netmux mkindx

# TODO: Need to list all the headers for the dist target.
netmux_SOURCES = dummy.cc \
	../misc/myfifo.c ../event/event.c \
	boolexp.c bsd.c commac.c command.c compat.c comsys.c conf.c cque.c \
	create.c db.c db_xml.c db_xdr.c db_rw.c dllist.c dnschild.c eval.c \
	file_c.c flags.c funceval.c functions.c game.c help.c log.c look.c \
	macro.c mail.c match.c mguests.c move.c mmdb.c nametab.c netcommon.c \
	nrbtab.c object.c player.c player_c.c powers.c predicates.c python.c \
	rbtree.c rbtab.c rob.c set.c signal.c speech.c stringutil.c timer.c \
	unparse.c vattr.c walkdb.c wild.c wiz.c $(netmux_OPTIONAL) \
	version.c

netmux_LDADD = ../btech/src/libbtech.la

if MUX_LOGCACHE
netmux_SOURCES += logcache.c
endif

if MUX_SQLCHILD
netmux_SOURCES += sqlchild.c
endif

# TODO: Need to list all the headers for the dist target.
mkindx_SOURCES = mkindx.c

# Support for autogenerating version.h.
BUILT_SOURCES = version.h

version.h:
	$(SHELL) $(srcdir)/version.sh

# XXX: A hack to ensure the version.h isn't present on our next build.  Build
# numbers will increment based on the number of times you install, rather than
# the number of times you compile.  This makes more sense to me, anyway.
#
# You can hook all or something if you want different behavior, or have
# version.h depend on a .PHONY target.
install-exec-hook:
	rm -f version.h

# TODO: Is this really used anyway? In any case, these should all be configure
# options/additional programs, as convenient as it is to simply be able to
# build/link to different targets.
#mudflap:
#	$(CC) -g3 -DDEBUG -DSVN_REVISION="\"`svnversion .`\"" $(ALLFLAGS) $(VERSION_FLAGS) -fmudflap -o src/netmux.mudflap $(ALLSRCS) src/version.c $(LIBS) -lmudflap

# TODO: I think we should just drop this target.
#dbconvert src/dbconvert:
#	@echo "dbconvert is no longer supported." && exit 1
