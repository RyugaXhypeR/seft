m4_define([SEFT_VERSION], [1.0])

AC_INIT([seft], [SEFT_VERSION], [])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_CONFIG_HEADERS([seft_config.h])

AC_CHECK_LIB([ssh], [ssh_new], [], [AC_MSG_ERROR([Missing lib: libssh])])
AC_CHECK_HEADERS(
    [argp.h fcntl.h libssh/libssh.h libssh/sftp.h sys/stat.h sys/types.h unistd.h],
    [], [AC_MSG_ERROR([Missing headers])]
)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
