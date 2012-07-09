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

#include <boot-manager/boot-manager-command-line.h>
#include <boot-manager/la-handler-dbus.h>


DLT_IMPORT_CONTEXT (la_handler_context);



gint
boot_manager_handle_command_line (int              argc,
                                  char           **argv,
                                  GDBusConnection *connection)
{
  GOptionContext *context = g_option_context_new (NULL);
  LAHandler      *legacy_app_handler = NULL;
  gboolean        do_register = FALSE;
  gboolean        do_deregister = FALSE;
  GError         *error = NULL;
  gchar          *unit = NULL;
  gchar          *log_message = NULL;
  gchar          *mode = NULL;
  gint            timeout = 0;
  int             exit_status;

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

      exit_status = EXIT_FAILURE;
      goto finish;
    }

  /* validate the argument data */
  if (unit == NULL || *unit == '\0')
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Invalid arguments: unit must be defined"));
      exit_status = EXIT_FAILURE;
      goto finish;
    }

  if (!(do_register ^ do_deregister))
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Invalid arguments: Please select either --register or "
                           "--deregister"));
      exit_status = EXIT_FAILURE;
      goto finish;
    }

  if (do_register && timeout < 0)
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Invalid arguments: Timeout must be non-negative"));
      exit_status = EXIT_FAILURE;
      goto finish;
    }

  /* get a legacy app handler interface */
  legacy_app_handler =
    la_handler_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE,
                               "org.genivi.BootManager1",
                               "/org/genivi/BootManager1/LegacyAppHandler", NULL,
                               &error);
  if (error != NULL)
    {
      /* failed to connect to the legacy app handler */
      log_message =
        g_strdup_printf ("Error occurred connecting to legacy app handler "
                         "interface: %s", error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

      g_free (log_message);
      g_error_free (error);

      exit_status = EXIT_FAILURE;
      goto finish;
    }

  if (do_register)
    {
      /* call the legacy app handler's Register() method */
      la_handler_call_register_sync (legacy_app_handler, unit, mode ? mode : "normal",
                                     (guint) timeout, NULL, &error);

      if (error != NULL)
        {
          /* failed to register the legacy app */
          log_message = g_strdup_printf ("Error occurred registering legacy app: %s",
                                         error->message);
          DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

          exit_status = EXIT_FAILURE;
        }
      else
        {
          exit_status = EXIT_SUCCESS;
        }
      goto finish;
    }
  else if (do_deregister)
    {
      /* call the legacy app handler's Deregister() method */
      la_handler_call_deregister_sync (legacy_app_handler, unit, NULL, &error);
      if (error != NULL)
        {
          /* failed to deregister the legacy app */
          log_message = g_strdup_printf ("Error occurred deregistering legacy app: %s",
                                         error->message);
          DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_message));

          exit_status = EXIT_FAILURE;
        }
      else
        {
          exit_status = EXIT_SUCCESS;
        }
      goto finish;
    }
  else
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING ("unexpected input"));
      exit_status = EXIT_FAILURE;
      goto finish;
    }

  finish:
  g_option_context_free (context);
  if (legacy_app_handler != NULL)
    g_object_unref (legacy_app_handler);
  if (error != NULL)
    g_error_free (error);
  g_free (unit);
  g_free (log_message);
  g_free (mode);
  return exit_status;
}
