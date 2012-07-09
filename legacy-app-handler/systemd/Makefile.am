# vi:set ts=8 sw=8 noet ai nocindent:

systemd_servicedir = $(libdir)/systemd/system

systemd_service_in_files =							\
	org.genivi.LegacyAppHandler1.service.in

systemd_service_DATA = $(systmd_service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@BOOT_MANAGER_VERSION_API\@,$(BOOT_MANAGER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	$(systemd_service_DATA)

EXTRA_DIST =								\
	$(systemd_service_in_files)


