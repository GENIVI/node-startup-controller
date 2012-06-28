# vi:set ts=8 sw=8 noet ai nocindent:

noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES =							\
	glib-extensions.c						\
	glib-extensions.h						\
	watchdog-client.c						\
	watchdog-client.h

libcommon_la_CFLAGS =							\
	-DG_LOG_DOMAIN=\"common\"					\
	-I$(top_srcdir)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)						\
	$(SYSTEMD_DAEMON_CFLAGS)

libcommon_la_LDFLAGS =							\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

libcommon_la_LIBADD =							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(SYSTEMD_DAEMON_LIBS)

