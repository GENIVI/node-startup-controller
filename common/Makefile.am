# vi:set ts=8 sw=8 noet ai nocindent:

noinst_LTLIBRARIES = libcommon.la

la_handler_built_sources =						\
	la-handler-dbus.h						\
	la-handler-dbus.c

nsm_consumer_built_sources =						\
	nsm-consumer-dbus.h						\
	nsm-consumer-dbus.c

nsm_lifecycle_control_built_sources =					\
	nsm-lifecycle-control-dbus.h					\
	nsm-lifecycle-control-dbus.c

shutdown_consumer_built_sources =					\
	shutdown-consumer-dbus.h					\
	shutdown-consumer-dbus.c

libcommon_la_built_sources =						\
	$(la_handler_built_sources)					\
	$(nsm_consumer_built_sources)					\
	$(nsm_lifecycle_control_built_sources)				\
	$(shutdown_consumer_built_sources)

libcommon_la_SOURCES =							\
	nsm-enum-types.c						\
	nsm-enum-types.h						\
	shutdown-client.c						\
	shutdown-client.h						\
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
	la-handler-dbus.xml						\
	nsm-consumer-dbus.xml						\
	nsm-lifecycle-control-dbus.xml					\
	shutdown-consumer-dbus.xml

DISTCLEANFILES =							\
	$(libcommon_la_built_sources)

BUILT_SOURCES =								\
	$(libcommon_la_built_sources)

$(la_handler_built_sources): la-handler-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.genivi				\
	    --c-namespace ""						\
	    --generate-c-code la-handler-dbus				\
	    --annotate org.genivi.BootManager1.LegacyAppHandler org.gtk.GDBus.C.Name \
	      LA_Handler $<

$(nsm_consumer_built_sources): nsm-consumer-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix com.contiautomotive.NodeStateManager	\
	    --c-namespace ""						\
	    --generate-c-code nsm-consumer-dbus				\
	    --annotate com.contiautomotive.NodeStateManager.Consumer	\
	      org.gtk.GDBus.C.Name NSM_Consumer $<

$(nsm_lifecycle_control_built_sources): nsm-lifecycle-control-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix com.contiautomotive.NodeStateManager	\
	    --c-namespace ""						\
	    --generate-c-code nsm-lifecycle-control-dbus		\
	    --annotate com.contiautomotive.NodeStateManager.LifecycleControl \
	      org.gtk.GDBus.C.Name NSM_Lifecycle_Control $<

$(shutdown_consumer_built_sources): shutdown-consumer-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.genivi				\
	    --c-namespace ""						\
	    --generate-c-code shutdown-consumer-dbus			\
	    --annotate com.contiautomotive.NodeStateManager.LifeCycleConsumer \
	      org.gtk.GDBus.C.Name Shutdown_Consumer $<
