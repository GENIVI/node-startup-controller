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
#include <legacy-app-handler/la-handler-dbus.h>
#include <legacy-app-handler/la-handler-service.h>



DLT_DECLARE_CONTEXT (la_handler_context);



static void
dlt_cleanup (void)
{
  DLT_UNREGISTER_CONTEXT (la_handler_context);
  DLT_UNREGISTER_APP ();
}



static gboolean
handle_command_line (int              argc,
                     char           **argv,
                     GDBusConnection *connection)
{
  GOptionContext *context = g_option_context_new (NULL);
  LAHandler      *legacy_app_handler;
  gboolean        do_register = FALSE;
  gboolean        do_deregister = FALSE;
  GError         *error = NULL;
  gchar          *unit = NULL;
  gchar          *log_message = NULL;
  gchar          *mode = NULL;
  gint            timeout = 0;

  GOptionEntry entries[] = {
    {"deregister",    0, 0, G_OPTION_ARG_NONE,   &do_deregister, NULL, NULL},
    {"register",      0, 0, G_OPTION_ARG_NONE,   &do_register,   NULL, NULL},
    {"unit",          0, 0, G_OPTION_ARG_STRING, &unit,          NULL, NULL},
    {"timeout",       0, 0, G_OPTION_ARG_INT,    &timeout,       NULL, NULL},
    {"shutdown-mode", 0, 0, G_OPTION_ARG_STRING, &mode,          NULL, NULL},
    {NULL},
  };

  /* set up the option context */
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_add_main_entries (context, entries, NULL);

  /* parse the arguments into argument data */
  if (!g_option_context_parse (context, &argc, &argv, &error) || error != NULL)
    {
      /* an error occurred */
      log_message =
        g_strdup_printf ("Error occurred parsing arguments: %s\n", error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

      g_error_free (error);
      g_free (log_message);

      return FALSE;
    }
  else if (do_register && !do_deregister)
    {
      if (unit == NULL || *unit == '\0' || timeout < 0)
        {
          /* register was called incorrectly */
          log_message =
            g_strdup_printf ("Invalid arguments for --register. A unit must be specified"
                             " and the timeout must be positive.");
          DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

          g_free (log_message);

          return FALSE;
        }

      /* get a legacy app handler interface */
      legacy_app_handler =
        la_handler_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE,
                                   "org.genivi.LegacyAppHandler1",
                                   "/org/genivi/LegacyAppHandler1", NULL, &error);
      if (error != NULL)
        {
          /* failed to connect to the legacy app handler */
          log_message =
            g_strdup_printf ("Error occurred connecting to legacy app handler "
                             "interface: %s", error->message);
          DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

          g_free (log_message);
          g_error_free (error);

          return FALSE;
        }

      /* call the legacy app handler's Register() method */
      la_handler_call_register_sync (legacy_app_handler, unit,
                                     mode ? mode : "normal", (guint) timeout, NULL,
                                     &error);
      if (error != NULL)
        {
          /* failed to register the legacy app */
          log_message = g_strdup_printf ("Error occurred registering legacy app: %s",
                                         error->message);
          DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

          g_object_unref (legacy_app_handler);
          g_free (log_message);
          g_error_free (error);

          return FALSE;
        }

      g_object_unref (legacy_app_handler);

      return TRUE;

    }
  else if (do_deregister && !do_register)
    {
      if (unit == NULL || *unit == '\0')
        {
          /* deregister was called incorrectly */
          log_message =
            g_strdup_printf ("Invalid arguments for --deregister. A unit must be "
                             "specified.");
          DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

          g_free (log_message);

          return FALSE;
        }

      /* get a legacy app handler interface */
      legacy_app_handler =
        la_handler_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE,
                                   "org.genivi.LegacyAppHandler1",
                                   "/org/genivi/LegacyAppHandler1", NULL, &error);
      if (error != NULL)
        {
          log_message =
            g_strdup_printf ("Error occurred connecting to legacy app handler "
                             "interface: %s", error->message);
          DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

          g_free (log_message);
          g_error_free (error);

          return FALSE;
        }

      /* call the legacy app handler's Deregister() method */
      la_handler_call_deregister_sync (legacy_app_handler, unit, NULL, &error);
      if (error != NULL)
        {
          log_message = g_strdup_printf ("Error occurred deregistering legacy "
                                         "app: %s", error->message);
          DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

          g_object_unref (legacy_app_handler);
          g_free (log_message);
          g_error_free (error);

          return FALSE;
        }

      g_object_unref (legacy_app_handler);

      return TRUE;
    }
  else if (do_register && do_deregister)
    {
      log_message =
        g_strdup_printf ("Invalid arguments. Please choose either --register or "
                         "--deregister.");
        DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

        g_free (log_message);

        return FALSE;
    }
  else
    {
        DLT_LOG (la_handler_context, DLT_LOG_ERROR,
                 DLT_STRING ("No arguments recognised"));
        return FALSE;
    }
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

  if (is_remote)
    {
      if (!handle_command_line (argc, argv, connection))
        exit_status = EXIT_FAILURE;
      else
        exit_status = EXIT_SUCCESS;
    }
  else
    {
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
      application =
        la_handler_application_new (service, G_APPLICATION_IS_SERVICE);

      exit_status = g_application_run (G_APPLICATION (application), argc, argv);
      g_object_unref (application);

      /* release allocated objects */
      g_object_unref (service);
    }

  g_object_unref (connection);

  return exit_status;
}
