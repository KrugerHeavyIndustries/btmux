####################
# Autoconf options #
####################

AC_PREREQ([2.50])

AC_INIT([BattleTech MUX], [0.6])
AC_CONFIG_AUX_DIR([confutil])
AC_PREFIX_DEFAULT([`pwd`/game.run])

AM_INIT_AUTOMAKE([1.9])
AM_CONFIG_HEADER([autoconf.h])


#########################
# Configuration options #
#########################

# Check the various enable code sections
AC_MSG_CHECKING([Should we enable support for graphical HUDs [[default=yes]]])
AC_ARG_ENABLE([hudinfo-support],
    AS_HELP_STRING([--disable-hudinfo-support],
                   [Disable the code to support graphical HUDs [[default=enabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_DEFINE([HUDINFO_SUPPORT], [1], [Should the HUDINFO code be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for hudinfo-support option)
                ;;
        esac
    ],
    [
        AC_DEFINE([HUDINFO_SUPPORT], [1], [Should the HUDINFO code be enabled])
        AC_MSG_RESULT([yes])
    ]
)
                   
AC_MSG_CHECKING([Should we enable the advanced econ system [[default=yes]]])
AC_ARG_ENABLE([bt-advanced-econ],
    AS_HELP_STRING([--disable-bt-advanced-econ],
                   [Disable the advanced econ system [[default=enabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_DEFINE([BT_ADVANCED_ECON], [1], [Should the advanced econ system be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for bt-advanced-econ option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([yes])
        AC_DEFINE([BT_ADVANCED_ECON], [1], [Should the advanced econ system be enabled])
    ]
)

AC_MSG_CHECKING([Should we enable enhanced logging [[default=yes]]])
AC_ARG_ENABLE([enhanced-logging],
    AS_HELP_STRING([--disable-enhanced-logging],
                   [Disable support for arbitrary server logs [[default=enabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_MSG_RESULT([yes])
                AC_DEFINE([ARBITRARY_LOGFILES], [1], [Should Enhanced Logging be enabled])
               ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for enhanced-logging option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([yes])
        AC_DEFINE([ARBITRARY_LOGFILES], [1], [Should Enhanced Logging be enabled])
	enable_enhanced_logging="yes"
    ]
)

AM_CONDITIONAL([MUX_LOGCACHE], [test x"${enable_enhanced_logging}" = x"yes"])

AC_MSG_CHECKING([Should we enable the code for complex repairs [[default=no]]])
AC_ARG_ENABLE([bt-complexrepairs],
    AS_HELP_STRING([--enable-bt-complexrepairs],
                   [Enable code for complex repairs [[default=disabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_DEFINE([BT_COMPLEXREPAIRS], [1], [Should complex repairs be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for bt-complexrepairs option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([no])
    ]
)

AC_MSG_CHECKING([Should we enable the code for freetechtime [[default=no]]])
AC_ARG_ENABLE([bt-freetechtime],
    AS_HELP_STRING([--enable-bt-freetechtime],
                   [Enable code to give near instant techtime [[default=disabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_DEFINE([BT_FREETECHTIME], [1], [Should the freetechtime code be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for bt-freetechtime option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([no])
    ]
)

AC_MSG_CHECKING([Should we enable sql support [[default=no]]])
AC_ARG_ENABLE([sql-support],
    AS_HELP_STRING([--enable-sql-support],
                   [Enable support for sql.  Needs libdbi with mysql driver [[default=disabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_MSG_RESULT([yes])
                AC_CHECK_HEADER([dbi/dbi.h])
                AC_CHECK_LIB(dbi, dbi_conn_new)
                if test "$ac_cv_header_dbi_dbi_h" = "yes" && 
                        test "$ac_cv_lib_dbi_dbi_conn_new" = "yes" ; then
                    AC_DEFINE([SQL_SUPPORT], [1], [Should SQL Support be enabled])
                else
                    AC_MSG_ERROR([Was unable to locate ldbi])
                fi
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for sql-support option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([no])
    ]
)

AM_CONDITIONAL([MUX_SQLCHILD], [test x"${enable_sql_support}" = x"yes"])

AC_MSG_CHECKING([Should we rename the 'help' command to 'muxhelp' [[default=no]]])
AC_ARG_ENABLE([rename-help],
    AS_HELP_STRING([--enable-rename-help],
                   [Move the 'help' command to 'muxhelp' [[default=disabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes)
		AC_DEFINE([MOVE_HELP], [1], [Should we rename the 'help' command to 'muxhelp'])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for rename-help option)
                ;;
        esac    
    ],  
    [
        AC_MSG_RESULT([no])
    ]
)

#######################
# Configuration tests #
#######################

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_PROG_AWK

dnl
dnl TODO: Review all of our configuration tests for necessity.
dnl

dnl Checks for libraries.
AC_CHECK_LIB(bsd, main)
AC_CHECK_LIB(crypt, crypt)
AC_CHECK_LIB(m, tan)
AC_CHECK_LIB(nsl, main)
AC_CHECK_LIB(event, event_init, [],
    AC_MSG_ERROR([Was unable to locate libevent])
)

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h errno.h fcntl.h limits.h memory.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h time.h unistd.h stdint.h])
AC_CHECK_HEADER(event.h, [],
    AC_MSG_ERROR([Was unable to locate event.h])
)

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CHAR_UNSIGNED
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero dup2 floor getdtablesize gethostbyaddr gethostbyname getrusage gettimeofday inet_ntoa isascii localtime memmove memset mktime pow regcomp rint select socket sqrt srandom stpcpy strcasecmp strchr strcspn strdup strerror strncasecmp strndup strnlen strrchr strlcpy strlcat strstr strtol timelocal tzset sigaction posix_memalign])

dnl Check for siginfo_t members
AC_CHECK_MEMBERS([siginfo_t.si_fd],,,[#include <signal.h>])

AC_MSG_CHECKING(for linger)
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
        [#include <sys/socket.h>],
        [struct linger ling; ling.l_onoff = 1;]
    )],
    [AC_DEFINE([HAVE_LINGER], [1], [Define if sys/socket.h has struct linger]) AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)]
)

if test "$ac_cv_func_socket" = "yes"; then
    AC_CHECK_LIB(socket, main)
fi

if test "$ac_cv_func_gethostbyaddr" = "yes"; then
    AC_CHECK_LIB(inet, main)
fi


#########################
# Generate output files #
#########################

AC_CONFIG_SUBDIRS([btech])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
