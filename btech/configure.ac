####################
# Autoconf options #
####################

AC_PREREQ([2.50])

AC_INIT([BattleTech Extensions], [0.7])
AC_CONFIG_AUX_DIR([confutil])

AM_INIT_AUTOMAKE([1.9])
AM_CONFIG_HEADER([autoconf.h])


#########################
# Configuration options #
#########################

dnl Check the various enable code sections
AC_MSG_CHECKING([whether to enable graphical HUD support])
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
                   
AC_MSG_CHECKING([whether to enable the advanced econ system])
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

AC_MSG_CHECKING([whether to enable MW3-like stats and skills])
AC_ARG_ENABLE([bt-mw3stats],
    AS_HELP_STRING([--disable-bt-mw3stats],
                   [Disable MW3-like stats and skills [[default=enabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_DEFINE([BT_EXILE_MW3STATS], [1], [Should MW3-like stats and skills be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for bt-mw3stats option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([yes])
        AC_DEFINE([BT_EXILE_MW3STATS], [1], [Should MW3-like stats and skills be enabled])
    ]
)

AC_MSG_CHECKING([whether to enable advanced movement modes])
AC_ARG_ENABLE([bt-movement-modes],
    AS_HELP_STRING([--disable-bt-movement-modes],
                   [Disable advanced movement modes [[default=enabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_DEFINE([BT_MOVEMENT_MODES], [1], [Should the advanced movement modes be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for bt-movement-modes option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([yes])
        AC_DEFINE([BT_MOVEMENT_MODES], [1], [Should the advanced movement modes be enabled])
    ]
)

AC_MSG_CHECKING([whether to scale BTH on infrared with increasing heat])
AC_ARG_ENABLE([bt-scaled-infrared],
    AS_HELP_STRING([--disable-bt-scaled-infrared],
                   [Disable scaled infrared [[default=enabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes)
                AC_DEFINE([BT_MOVEMENT_MODES], [1], [Should heat-scaled infrared be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for bt-scaled-infrared option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([yes])
        AC_DEFINE([BT_SCALED_INFRARED], [1], [Should heat-scaled infrared be enabled])
    ]
)


AC_MSG_CHECKING([whether to copy freqs to MW on disembark])
AC_ARG_ENABLE([bt-mwfreqs],
    AS_HELP_STRING([--disable-bt-mwfreqs],
                   [Disable MW freq copying [[default=enabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_DEFINE([COPY_CHANS_ON_EJECT], [1], [Should copying freqs to MWs be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for bt-mwfreqs option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([yes])
        AC_DEFINE([COPY_CHANS_ON_EJECT], [1], [Should copying freqs to MWs be enabled])
    ]
)

AC_MSG_CHECKING([whether to enable complex repairs])
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

AC_MSG_CHECKING([whether to allow mech overspeeding])
AC_ARG_ENABLE([bt-overspeeding],
    AS_HELP_STRING([--enable-bt-overspeeding],
                   [Enable mech overspeeding [[default=disabled]]])
    AS_HELP_STRING([ ], [ ]),
    [
        case "${enableval}" in
            yes) 
                AC_DEFINE([WEIGHT_OVERSPEEDING], [1], [Should mech overspeeding be enabled])
                AC_MSG_RESULT([yes])
                ;;
            no)
                AC_MSG_RESULT([no])
                ;;
            *)
                AC_MSG_ERROR(bad value "${enableval}" for bt-overspeeding option)
                ;;
        esac
    ],
    [
        AC_MSG_RESULT([no])
    ]
)

AC_MSG_CHECKING([whether to enable freetechtime])
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

dnl Select our BTPR type.
AC_ARG_WITH([btpr],
            AS_HELP_STRING([--with-btpr=LAYER],
                           [Select the BTPR layer to use. LAYER is one of:
                            mux1, penn. Default is mux1.]),
            [ac_cv_use_btpr=${withval}], [ac_cv_use_btpr=mux1])

AC_MSG_CHECKING([which BTPR layer to use])

case "${ac_cv_use_btpr}" in
mux1)
        AC_MSG_RESULT([using TinyMUX 1.x])
        AC_DEFINE([BTPR_MUX1], [], [Use TinyMUX 1.x BTPR layer])
        ;;
penn)   AC_MSG_RESULT([using PennMUSH])
        AC_DEFINE([BTPR_PENN], [], [Use PennMUSH BTPR layer])
        ;;
*)
        AC_MSG_ERROR([bad argument to --with-btpr: ${ac_cv_use_btpr}])
        ;;
esac

AC_SUBST([btpr_dir], ["\$(top_srcdir)/btpr"])
AM_CONDITIONAL([BTPR_MUX1], [test ${ac_cv_use_btpr} = mux1])
AM_CONDITIONAL([BTPR_PENN], [test ${ac_cv_use_btpr} = penn])

#######################
# Configuration tests #
#######################

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

dnl These checks were taken more or less directly from BTMUX's configure.in.  A
dnl large number of them are probably unnecessary, or MUX-specific.

dnl Check for libraries.
AC_CHECK_LIB([m], [tan])

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h errno.h fcntl.h limits.h memory.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h time.h unistd.h stdint.h])

dnl Checks for typedefs, structures, and compiler characteristics.
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

dnl Checks for library functions.
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
AC_CHECK_FUNCS([bzero dup2 floor getdtablesize gethostbyaddr gethostbyname getrusage gettimeofday inet_ntoa isascii localtime memmove memset mktime pow regcomp rint select socket sqrt stpcpy strcasecmp strchr strcspn strdup strerror strncasecmp strndup strnlen strrchr strlcpy strlcat strstr strtol timelocal tzset sigaction posix_memalign])


#########################
# Generate output files #
#########################

AC_CONFIG_FILES([Makefile btpr/Makefile btfi/Makefile src/Makefile])
AC_OUTPUT
