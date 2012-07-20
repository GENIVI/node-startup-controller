# vi:set ts=8 sw=8 noet ai nocindent:

bus_servicedir = $(datadir)/dbus-1/system-services

bus_service_in_files =							\
	com.contiautomotive.NodeStateManager.service.in

bus_service_DATA = $(bus_service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	$(bus_service_DATA)

EXTRA_DIST =								\
	$(bus_service_in_files)
