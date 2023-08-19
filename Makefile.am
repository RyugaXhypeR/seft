AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = sftp_cli
sftp_cli_SOURCES = sftp.c src/sftp_client.c src/sftp_list.c src/sftp_path.c src/sftp_utils.c
sftp_cli_CFLAGS = $(C_FLAGS)
sftp_cli_LDADD = $(LINK_FLAGS)

# If defined i.e D=DEBUG will display debug.
D = NDEBUG
LINK_FLAGS = -lssh
INC_FLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/include
OPT_FLAG = -O3
IGNORE_FLAGS = -Wno-stringop-truncation
LINTER_FLAGS = -Wall -Wextra -Wpedantic
C_FLAGS = $(LINTER_FLAGS) $(IGNORE_FLAGS) -g $(OPT_FLAG) $(INC_FLAGS) $(LINK_FLAGS) -D$(D)

# Clean up automake-generated files
clean-local:
	-rm -rf autom4te.cache config.h config.h.in~ Makefile.in aclocal.m4 install-sh missing depcomp configure configure\~

# Make "make distcheck" work with non-GNU tar
DISTCHECK_CONFIGURE_FLAGS = --disable-dependency-tracking

EXTRA_DIST = $(top_srcdir)/include/* $(top_srcdir)/src/*
