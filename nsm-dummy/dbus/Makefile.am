# vi:set ts=8 sw=8 noet ai nocindent:

servicedir = $(datadir)/dbus-1/system-services

service_in_files =							\
	com.contiautomotive.NodeStateManager.Consumer.service.in	\
	com.contiautomotive.NodeStateManager.LifecycleControl.service.in

service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	$(service_DATA)

EXTRA_DIST =								\
	$(service_in_files)
