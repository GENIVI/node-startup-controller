# vi:set ts=8 sw=8 noet ai nocindent:

legacy_app_handlerdir =							\
	$(libdir)/legacy-app-handler-$(BOOT_MANAGER_VERSION_API)

legacy_app_handler_PROGRAMS =							\
	legacy-app-handler

legacy_app_handler_built_headers =						\
	la-handler-dbus.h

legacy_app_handler_built_sources =						\
	la-handler-dbus.c						\
	$(legacy_app_handler_built_headers)

legacy_app_handler_SOURCES =							\
	../common/glib-extensions.c					\
	../common/glib-extensions.h					\
	../common/watchdog-client.c					\
	../common/watchdog-client.h					\
	la-handler-application.c					\
	la-handler-application.h					\
	la-handler-service.c						\
	la-handler-service.h						\
	main.c								\
	$(legacy_app_handler_built_sources)

legacy_app_handler_CFLAGS =							\
	-DG_LOG_DOMAIN=\"legacy-app-handler\"					\
	-I$(top_srcdir)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)						\
	$(SYSTEMD_DAEMON_CFLAGS)

legacy_app_handler_LDFLAGS =							\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

legacy_app_handler_LDADD =							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(SYSTEMD_DAEMON_LIBS)

servicedir = $(datadir)/dbus-1/services
service_in_files =							\
	org.genivi.LAHandler1.service.in

service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	$(service_DATA)

EXTRA_DIST =								\
	$(service_in_files)						\
	$(gsettingsschema_in_files)					\
	la-handler-dbus.xml

DISTCLEANFILES =							\
	$(legacy_app_handler_built_sources)					\
	$(gsettings_SCHEMAS)

BUILT_SOURCES =								\
	$(legacy_app_handler_built_headers)

la-handler-dbus.h: la-handler-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.genivi				\
	    --c-namespace ""						\
	    --generate-c-code la-handler-dbus				\
	    --annotate org.genivi.LegacyAppHandler1 org.gtk.GDBus.C.Name	\
	      LA_Handler $<
