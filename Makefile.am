# vi:set ts=8 sw=8 noet ai nocindent:

SUBDIRS =								\
	common								\
	boot-manager							\
	nsm-dummy							\
	tests

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

DISTCHECK_CONFIGURE_FLAGS =						\
	--enable-gtk-doc						\
	--enable-debug=no
