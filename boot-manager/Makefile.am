# vi:set ts=8 sw=8 noet ai nocindent:

SUBDIRS =								\
	busconf

boot_managerdir =							\
	$(libdir)/boot-manager-$(BOOT_MANAGER_VERSION_API)

boot_manager_PROGRAMS =							\
	boot-manager

boot_manager_built_headers =						\
	boot-manager-dbus.h						\
	systemd-manager-dbus.h

boot_manager_built_sources =						\
	boot-manager-dbus.c						\
	systemd-manager-dbus.c						\
	$(boot_manager_built_headers)

boot_manager_SOURCES =							\
	boot-manager-application.c					\
	boot-manager-application.h					\
	boot-manager-service.c						\
	boot-manager-service.h						\
	glib-extensions.c						\
	glib-extensions.h						\
	job-manager.c							\
	job-manager.h							\
	la-handler-service.c						\
	la-handler-service.h						\
	luc-starter.c							\
	luc-starter.h							\
	target-startup-monitor.c					\
	target-startup-monitor.h					\
	main.c								\
	$(boot_manager_built_sources)

boot_manager_CFLAGS =							\
	-DLUC_PATH=\"$(sysconfdir)/boot-manager/last-user-context\"	\
	-DG_LOG_DOMAIN=\"boot-manager\"					\
	-I$(top_srcdir)							\
	$(DLT_CFLAGS)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)						\
	$(SYSTEMD_DAEMON_CFLAGS)

boot_manager_LDFLAGS =							\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

boot_manager_DEPENDENCIES =						\
	$(top_builddir)/common/libcommon.la

boot_manager_LDADD =							\
	$(DLT_LIBS)							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(SYSTEMD_DAEMON_LIBS)						\
	$(top_builddir)/common/libcommon.la

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

dbus_servicedir = $(datadir)/dbus-1/system-services

dbus_service_in_files =							\
	org.genivi.BootManager1.service.in

dbus_service_DATA = $(dbus_service_in_files:.service.in=.service)

systemd_servicedir = $(libdir)/systemd/system

systemd_service_in_files =						\
	boot-manager.service.in

systemd_service_DATA = $(systemd_service_in_files:.service.in=.service)

CLEANFILES =								\
	$(dbus_service_DATA)						\
	$(systemd_service_DATA)						\
	boot-manager-dbus-doc-*.xml

EXTRA_DIST =								\
	boot-manager-dbus.xml						\
	systemd-manager-dbus.xml					\
	$(dbus_service_in_files)					\
	$(systemd_service_in_files)

DISTCLEANFILES =							\
	$(boot_manager_built_sources)

BUILT_SOURCES =								\
	$(boot_manager_built_headers)

boot-manager-dbus.h: boot-manager-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.genivi				\
	    --c-namespace ""						\
	    --generate-c-code boot-manager-dbus				\
	    --generate-docbook boot-manager-dbus-doc			\
	    --annotate org.genivi.BootManager1.BootManager org.gtk.GDBus.C.Name	\
	      Boot_Manager $<

systemd-manager-dbus.h: systemd-manager-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.freedesktop.systemd1			\
	    --c-namespace ""						\
	    --generate-c-code systemd-manager-dbus			\
	    --annotate org.freedesktop.systemd1.Manager org.gtk.GDBus.C.Name \
	      SystemdManager $<
