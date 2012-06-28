# vi:set ts=8 sw=8 noet ai nocindent:

testscriptdir = /usr/bin

testscript_DATA =							\
	org-genivi-LegacyAppHandler1-test-service.py

testconfdir = /etc/dbus-1/system.d

testconf_DATA =								\
	org.genivi.LegacyAppHandler1.test.conf

servicedir = /lib/systemd/system

service_in_files =							\
	org.genivi.LegacyAppHandler1.test1.service.in				\
	org.genivi.LegacyAppHandler1.test2.service.in				\
	org.genivi.LegacyAppHandler1.test3.service.in

service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	$(service_DATA)							\
	$(testconf_DATA)						\
	$(testscript_DATA)

EXTRA_DIST =								\
	$(service_in_files)						\
	$(testconf_DATA)						\
	$(testscript_DATA)
