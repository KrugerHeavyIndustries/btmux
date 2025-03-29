ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Werror -Wfatal-errors -fsanitize=address
AM_CPPFLAGS = -Werror -Wfatal-errors -fsanitize=address

SUBDIRS = . btech src
dist_doc_DATA = README

# Our SVN-based magic for downloading and installing data files.
install-data-local:
	$(SHELL) $(srcdir)/getdata.sh
