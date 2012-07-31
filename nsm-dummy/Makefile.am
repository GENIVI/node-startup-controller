# vi:set ts=8 sw=8 noet ai nocindent:

SUBDIRS =								\
	busconf

nsm_dummydir =								\
	$(libdir)/node-startup-controller-$(NODE_STARTUP_CONTROLLER_VERSION_API)

nsm_dummy_PROGRAMS =							\
	nsm-dummy

nsm_dummy_SOURCES =							\
        nsm-consumer-service.c						\
        nsm-consumer-service.h						\
        nsm-dummy-application.c						\
        nsm-dummy-application.h						\
        nsm-lifecycle-control-service.c					\
        nsm-lifecycle-control-service.h					\
	main.c

nsm_dummy_CFLAGS =							\
	-DG_LOG_DOMAIN=\"nsm-dummy\"					\
	-I$(top_srcdir)							\
	$(DLT_CFLAGS)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)						\
	$(SYSTEMD_DAEMON_CFLAGS)

nsm_dummy_LDFLAGS =							\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

nsm_dummy_DEPENDENCIES =						\
	$(top_builddir)/common/libcommon.la

nsm_dummy_LDADD =							\
	$(DLT_LIBS)							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(SYSTEMD_DAEMON_LIBS)						\
	$(top_builddir)/common/libcommon.la

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@NODE_STARTUP_CONTROLLER_VERSION_API\@,$(NODE_STARTUP_CONTROLLER_VERSION_API),g" < $< > $@

dbus_servicedir = $(datadir)/dbus-1/system-services

dbus_service_in_files =							\
	com.contiautomotive.NodeStateManager.Consumer.service.in	\
	com.contiautomotive.NodeStateManager.LifecycleControl.service.in

dbus_service_DATA = $(dbus_service_in_files:.service.in=.service)

systemd_servicedir = ${libdir}/systemd/system

systemd_service_in_files =						\
	nsm-dummy.service.in

systemd_service_DATA = $(systemd_service_in_files:.service.in=.service)

CLEANFILES =								\
	$(dbus_service_DATA)						\
	$(systemd_service_DATA)

EXTRA_DIST =								\
	$(dbus_service_in_files)					\
	$(systemd_service_in_files)
