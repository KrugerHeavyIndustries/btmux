ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Werror -Wfatal-errors
AM_CPPFLAGS = -Werror -Wfatal-errors

SUBDIRS = . btech src

# Our SVN-based magic for downloading and installing data files.
install-data-local:
	$(SHELL) $(srcdir)/getdata.sh

# Our SVN update target.  Kinda silly, really.
update:
	svn update
