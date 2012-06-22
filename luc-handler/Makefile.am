# vi:set ts=8 sw=8 noet ai nocindent:

luc_handlerdir =							\
	$(libdir)/luc-handler-$(BOOT_MANAGER_VERSION_API)

luc_handler_PROGRAMS =							\
	luc-handler

luc_handler_built_headers =						\
	luc-handler-dbus.h

luc_handler_built_sources =						\
	luc-handler-dbus.c						\
	$(luc_handler_built_headers)

luc_handler_SOURCES =							\
	../common/glib-extensions.c					\
	../common/glib-extensions.h					\
	../common/watchdog-client.c					\
	../common/watchdog-client.h					\
	luc-handler-application.c					\
	luc-handler-application.h					\
	luc-handler-service.c						\
	luc-handler-service.h						\
	main.c								\
	$(luc_handler_built_sources)

luc_handler_CFLAGS =							\
	-DG_LOG_DOMAIN=\"luc-handler\"					\
	-I$(top_srcdir)							\
	$(DLT_CFLAGS)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)						\
	$(SYSTEMD_DAEMON_CFLAGS)

luc_handler_LDFLAGS =							\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

luc_handler_LDADD =							\
	$(DLT_LIBS)							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(SYSTEMD_DAEMON_LIBS)

servicedir = $(datadir)/dbus-1/services
service_in_files =							\
	org.genivi.LUCHandler1.service.in

service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

gsettingsschema_in_files =						\
	org.genivi.LUCHandler1.gschema.xml.in

gsettings_SCHEMAS =							\
	$(gsettingsschema_in_files:.xml.in=.xml)

.PRECIOUS: $(gsettings_SCHEMAS)

@GSETTINGS_RULES@

CLEANFILES =								\
	$(service_DATA)

EXTRA_DIST =								\
	$(service_in_files)						\
	$(gsettingsschema_in_files)					\
	luc-handler-dbus.xml

DISTCLEANFILES =							\
	$(luc_handler_built_sources)					\
	$(gsettings_SCHEMAS)

BUILT_SOURCES =								\
	$(luc_handler_built_headers)

luc-handler-dbus.h: luc-handler-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN)					\
	    --interface-prefix org.genivi				\
	    --c-namespace ""						\
	    --generate-c-code luc-handler-dbus				\
	    --annotate org.genivi.LUCHandler1 org.gtk.GDBus.C.Name	\
	      LUC_Handler $<
