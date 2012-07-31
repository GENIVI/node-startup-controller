# vi:set ts=8 sw=8 noet ai nocindent:

legacy_app_handlerdir =							\
	$(libdir)/node-startup-controller-$(NODE_STARTUP_CONTROLLER_VERSION_API)

legacy_app_handler_PROGRAMS =						\
	legacy-app-handler

legacy_app_handler_SOURCES =						\
	main.c

legacy_app_handler_CFLAGS =						\
	-DG_LOG_DOMAIN=\"legacy-app-handler\"				\
	-I$(top_srcdir)							\
	$(DLT_CFLAGS)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

legacy_app_handler_LDFLAGS =						\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

legacy_app_handler_DEPENDENCIES =					\
	$(top_builddir)/common/libcommon.la

legacy_app_handler_LDADD =						\
	$(DLT_LIBS)							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(top_builddir)/common/libcommon.la
