/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include <dlt/dlt.h>

#include <legacy-app-handler/la-handler-application.h>
#include <legacy-app-handler/la-handler-service.h>



DLT_DECLARE_CONTEXT (la_handler_context);



static void
dlt_cleanup (void)
{
  DLT_UNREGISTER_CONTEXT (la_handler_context);
  DLT_UNREGISTER_APP ();
}



int
main (int    argc,
      char **argv)
{
  LAHandlerApplication *application;
  LAHandlerService     *service;
  GDBusConnection      *connection;
  gboolean              is_remote;
  GError               *error = NULL;
  gchar                *log_text;
  int                   exit_status;

  /* check if this program execution is meant as a remote application.
   * if it is a remote application, then it will be called with command-line arguments. */
  is_remote = (argc > 1) ? TRUE : FALSE;

  /* register the application and context in DLT */
  if (!is_remote)
    {
      DLT_REGISTER_APP ("BMGR", "GENIVI Boot Manager");
      DLT_REGISTER_CONTEXT (la_handler_context, "LAH",
                            "Context of the legacy application handler that hooks legacy "
                            "applications up with the shutdown concept of the Node State "
                            "Manager");
      atexit (dlt_cleanup);
    }

  /* initialize the GType type system */
  g_type_init ();

  /* attempt to connect to D-Bus */
  connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (connection == NULL || error != NULL || !G_IS_DBUS_CONNECTION (connection))
    {
      log_text = g_strdup_printf ("Failed to connect to D-Bus: %s", error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_text));

      /* clean up */
      g_free (log_text);
      g_error_free (error);

      return EXIT_FAILURE;
    }

  /* instantiate the LegacyAppHandler service implementation */
  service = la_handler_service_new (connection);
  if (!la_handler_service_start (service, &error))
    {
      log_text = g_strdup_printf ("Failed to start the legacy app handler service: %s",
                                  error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_text));

      /* clean up */
      g_free (log_text);
      g_error_free (error);
      g_object_unref (service);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* create and run the main application */
  if (is_remote)
    {
      /* an application with the flag G_APPLICATION_IS_SERVICE tries to be the primary
       * instance of the application, and fails if another instance already exists.
       * setting G_APPLICATION_IS_LAUNCHER indicates that it shouldn't try to be the
       * primary instance */
      application =
        la_handler_application_new (service, G_APPLICATION_HANDLES_COMMAND_LINE |
                                             G_APPLICATION_IS_LAUNCHER);
    }
  else
    {
      /* this application is meant to be the primary instance, so
       * G_APPLICATION_IS_LAUNCHER is not set */
      application =
        la_handler_application_new (service, G_APPLICATION_IS_SERVICE);
    }

  /* run the application */
  exit_status = g_application_run (G_APPLICATION (application), argc, argv);
  g_object_unref (application);

  /* release allocated objects */
  g_object_unref (service);
  g_object_unref (connection);

  return exit_status;
}
