# vi:set ts=8 sw=8 noet ai nocindent:

noinst_LTLIBRARIES = libcommon.la

libcommon_la_built_headers =						\
	boot-manager-dbus.h						\
	shutdown-consumer-dbus.h

libcommon_la_built_sources =						\
	boot-manager-dbus.c						\
	shutdown-consumer-dbus.c					\
	$(libcommon_la_built_headers)

libcommon_la_SOURCES =							\
	glib-extensions.c						\
	glib-extensions.h						\
	shutdown-consumer-service.c					\
	shutdown-consumer-service.h					\
	watchdog-client.c						\
	watchdog-client.h						\
	$(libcommon_la_built_sources)

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

EXTRA_DIST =								\
	boot-manager-dbus.xml
	shutdown-consumer-dbus.xml

DISTCLEANFILES =							\
	$(libcommon_la_built_sources)

BUILT_SOURCES =								\
	$(libcommon_la_built_headers)

boot-manager-dbus.h: boot-manager-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.genivi				\
	    --c-namespace ""						\
	    --generate-c-code boot-manager-dbus				\
	    --annotate org.genivi.BootManager1 org.gtk.GDBus.C.Name	\
	      BootManager $<

shutdown-consumer-dbus.h: shutdown-consumer-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.genivi				\
	    --c-namespace ""						\
	    --generate-c-code shutdown-consumer-dbus			\
	    --annotate org.genivi.lifecycle.ShutdownConsumer1 org.gtk.GDBus.C.Name	\
	      Shutdown_Consumer $<
