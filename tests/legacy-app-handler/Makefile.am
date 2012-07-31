# vi:set ts=8 sw=8 noet ai nocindent:

service_in_files =							\
	legacy-app-handler-test1.service.in				\
	legacy-app-handler-test2.service.in				\
	legacy-app-handler-test3.service.in

noinst_DATA =								\
	legacy-app-handler-test						\
	legacy-app-handler-test.conf					\
	$(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@NODE_STARTUP_CONTROLLER_VERSION_API\@,$(NODE_STARTUP_CONTROLLER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	*.service

EXTRA_DIST =								\
	legacy-app-handler-test						\
	legacy-app-handler-test.conf					\
	$(service_in_files)
