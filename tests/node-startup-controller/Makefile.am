# vi:set ts=8 sw=8 noet ai nocindent:

installcheck-local: $(noinst_SCRIPTS) $(noinst_PROGRAMS)
	@echo "============================="
	@echo "Running test for LUC handling"
	@if ./test-luc-handler; then					\
	    echo "Test for LUC handling passed";			\
	else								\
	    echo "Test for LUC handling failed";			\
	fi
	@echo "============================="

noinst_SCRIPTS =							\
	test-luc-handler

EXTRA_DIST =								\
	test-luc-handler

export LUC_PATH = last-user-context

export BOOT_MANAGER_CMD =						\
	$(libdir)/boot-manager-$(BOOT_MANAGER_VERSION_API)/boot-manager

noinst_PROGRAMS =							\
	gvariant-writer

gvariant_writer_SOURCES =						\
	gvariant-writer.c

gvariant_writer_CFLAGS =						\
	-DG_LOG_DOMAIN=\"gvariant-writer\"				\
	-I$(top_srcdir)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

gvariant_writer_LDFLAGS =						\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

gvariant_writer_LDADD =							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)
