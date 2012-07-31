# vi:set ts=8 sw=8 noet ai nocindent:

SUBDIRS =								\
	busconf

node_startup_controllerdir =						\
	$(libdir)/node-startup-controller-$(NODE_STARTUP_CONTROLLER_VERSION_API)

node_startup_controller_PROGRAMS =					\
	node-startup-controller

node_startup_controller_built_sources =					\
	node-startup-controller-dbus.h					\
	node-startup-controller-dbus.c

systemd_manager_built_sources =						\
	systemd-manager-dbus.h						\
	systemd-manager-dbus.c

systemd_unit_built_sources =						\
	systemd-unit-dbus.h						\
	systemd-unit-dbus.c

node_startup_controller_SOURCES =					\
	glib-extensions.c						\
	glib-extensions.h						\
	job-manager.c							\
	job-manager.h							\
	la-handler-service.c						\
	la-handler-service.h						\
	luc-starter.c							\
	luc-starter.h							\
	node-startup-controller-application.c				\
	node-startup-controller-application.h				\
	node-startup-controller-service.c				\
	node-startup-controller-service.h				\
	target-startup-monitor.c					\
	target-startup-monitor.h					\
	main.c								\
	$(node_startup_controller_built_sources)			\
	$(systemd_manager_built_sources)				\
	$(systemd_unit_built_sources)

node_startup_controller_CFLAGS =					\
	-DLUC_PATH=\"$(sysconfdir)/node-startup-controller/last-user-context\"	\
	-DG_LOG_DOMAIN=\"node-startup-controller\"			\
	-I$(top_srcdir)							\
	$(DLT_CFLAGS)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)						\
	$(SYSTEMD_DAEMON_CFLAGS)

node_startup_controller_LDFLAGS =					\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

node_startup_controller_DEPENDENCIES =					\
	$(top_builddir)/common/libcommon.la

node_startup_controller_LDADD =						\
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
	org.genivi.NodeStartupController1.service.in

dbus_service_DATA = $(dbus_service_in_files:.service.in=.service)

systemd_servicedir = $(libdir)/systemd/system

systemd_service_in_files =						\
	node-startup-controller.service.in

systemd_service_DATA = $(systemd_service_in_files:.service.in=.service)

CLEANFILES =								\
	$(dbus_service_DATA)						\
	$(systemd_service_DATA)						\
	node-startup-controller-dbus-doc-*.xml

EXTRA_DIST =								\
	node-startup-controller-dbus.xml				\
	systemd-manager-dbus.xml					\
	$(dbus_service_in_files)					\
	$(systemd_service_in_files)

DISTCLEANFILES =							\
	$(node_startup_controller_built_sources)			\
	$(systemd_manager_built_sources)				\
	$(systemd_unit_built_sources)

BUILT_SOURCES =								\
	$(node_startup_controller_built_sources)			\
	$(systemd_manager_built_sources)				\
	$(systemd_unit_built_sources)

$(node_startup_controller_built_sources): node-startup-controller-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.genivi				\
	    --c-namespace ""						\
	    --generate-c-code node-startup-controller-dbus		\
	    --generate-docbook doc					\
	    --annotate							\
	      org.genivi.NodeStartupController1.NodeStartupController	\
	      org.gtk.GDBus.C.Name Node_Startup_Controller $<

$(systemd_manager_built_sources): systemd-manager-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.freedesktop.systemd1			\
	    --c-namespace ""						\
	    --generate-c-code systemd-manager-dbus			\
	    --annotate org.freedesktop.systemd1.Manager			\
	      org.gtk.GDBus.C.Name SystemdManager $<

$(systemd_unit_built_sources): systemd-unit-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.freedesktop.systemd1			\
	    --c-namespace ""						\
	    --generate-c-code systemd-unit-dbus				\
	    --annotate org.freedesktop.systemd1.Unit			\
	      org.gtk.GDBus.C.Name SystemdUnit $<
