# vi:set ts=8 sw=8 noet ai nocindent:

boot_managerdir =							\
	$(libdir)/boot-manager-$(BOOT_MANAGER_VERSION_API)

boot_manager_PROGRAMS =							\
	boot-manager

boot_manager_built_headers =						\
	systemd-manager-dbus.h

boot_manager_built_sources =						\
	systemd-manager-dbus.c						\
	$(boot_manager_built_headers)

boot_manager_SOURCES =							\
	$(top_srcdir)/luc-handler/luc-handler-dbus.c			\
	$(top_srcdir)/luc-handler/luc-handler-dbus.h			\
	boot-manager-application.c					\
	boot-manager-application.h					\
	boot-manager-service.c						\
	boot-manager-service.h						\
	luc-starter.c							\
	luc-starter.h							\
	target-startup-monitor.c					\
	target-startup-monitor.h					\
	main.c								\
	$(boot_manager_built_sources)

boot_manager_CFLAGS =							\
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

servicedir = $(datadir)/dbus-1/services
service_in_files =							\
	org.genivi.BootManager1.service.in

service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	$(service_DATA)

EXTRA_DIST =								\
	$(service_in_files)						\
	systemd-manager-dbus.xml

DISTCLEANFILES =							\
	$(boot_manager_built_sources)

BUILT_SOURCES =								\
	$(boot_manager_built_headers)

systemd-manager-dbus.h: systemd-manager-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.freedesktop.systemd1			\
	    --c-namespace ""						\
	    --generate-c-code systemd-manager-dbus			\
	    --annotate org.freedesktop.systemd1.Manager org.gtk.GDBus.C.Name \
	      SystemdManager $<
