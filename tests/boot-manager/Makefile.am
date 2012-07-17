# vi:set ts=8 sw=8 noet ai nocindent:

TESTS =									\
	test-luc-handler

EXTRA_DIST =								\
	test-luc-handler

luc_dir =								\
	$(datadir)/org.genivi.BootManager$(BOOT_MANAGER_VERSION_API)

export luc_path =							\
	$(luc_dir)/last-user-context

noinst_PROGRAMS =							\
	gvariant-writer

gvariant_writer_SOURCES =						\
	gvariant_writer.c

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
