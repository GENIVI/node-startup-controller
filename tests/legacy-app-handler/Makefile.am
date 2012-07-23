# vi:set ts=8 sw=8 noet ai nocindent:

testscriptdir = $(libdir)/boot-manager-$(BOOT_MANAGER_VERSION_API)

testscript_DATA =							\
	legacy-app-handler-test

testconfdir = $(sysconfdir)/dbus-1/system.d

testconf_DATA =								\
	legacy-app-handler-test.conf

servicedir = $(libdir)/systemd/system

service_in_files =							\
	legacy-app-handler-test1.service.in				\
	legacy-app-handler-test2.service.in				\
	legacy-app-handler-test3.service.in

service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	$(service_DATA)

EXTRA_DIST =								\
	$(service_in_files)						\
	$(testconf_DATA)						\
	$(testscript_DATA)
