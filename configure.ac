AC_PREREQ(2.60)
AC_INIT([dt-utils],
        [2021.03.0],
        [oss-tools@pengutronix.de],
        [dt-utils],
        [https://git.pengutronix.de/cgit/tools/dt-utils])
AC_CONFIG_SRCDIR([src/libdt.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([check-news foreign 1.11 -Wall -Wno-portability silent-rules tar-pax no-dist-gzip dist-xz subdir-objects])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])
LT_INIT([disable-static pic-only])
AC_PREFIX_DEFAULT([/usr])

AC_PROG_SED
AC_PROG_MKDIR_P

AC_ARG_ENABLE([state-backward-compatibility],
        AS_HELP_STRING([--enable-state-backward-compatibility], [barebox-state: when using the 'direct' storage backend, keep the on-disk format readable by barebox <= v2016.08.0 @<:@default=disabled@:>@]),
        [], [enable_state_backward_compatibility=no])
AS_IF([test "x${enable_state_backward_compatibility}" = "xyes"], [
        AC_DEFINE(CONFIG_STATE_BACKWARD_COMPATIBLE, [1], ['direct' storage backend backward compatibility.])
], [
        AC_DEFINE(CONFIG_STATE_BACKWARD_COMPATIBLE, [0])
])

AC_DEFINE(CONFIG_MTD, [1], [Statically define to be enabled to harmonize barebox' & dt-utils' code base.])

AC_DEFINE(CONFIG_STATE, [1], [Statically define to be enabled to harmonize barebox' & dt-utils' code base.])

AC_CHECK_FUNCS([__secure_getenv secure_getenv])

my_CFLAGS="-Wall \
-Wmissing-declarations -Wmissing-prototypes \
-Wnested-externs -Wsign-compare -Wchar-subscripts \
-Wstrict-prototypes -Wshadow \
-Wformat-security -Wtype-limits \
-fno-strict-aliasing"
AC_SUBST([my_CFLAGS])

PKG_CHECK_MODULES(UDEV, [libudev])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
        Makefile
])

AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE $VERSION
        =====

        prefix:                         ${prefix}
        sysconfdir:                     ${sysconfdir}
        libdir:                         ${libdir}
        includedir:                     ${includedir}

        compiler:                       ${CC}
        cflags:                         ${CFLAGS}
        ldflags:                        ${LDFLAGS}

        state-backward-compatibility:   ${enable_state_backward_compatibility}
])
