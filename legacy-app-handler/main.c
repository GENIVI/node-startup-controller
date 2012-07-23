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

#include <common/la-handler-dbus.h>
#include <common/nsm-enum-types.h>



static gchar *unit = NULL;
static gint   timeout = 0;
static gchar *shutdown_mode = NULL;



static GOptionEntry entries[] =
{
  { "unit", 'u', 0, G_OPTION_ARG_STRING, &unit, "Legacy application unit", NULL },
  { "timeout", 't', 0, G_OPTION_ARG_INT, &timeout, "Shutdown timeout in seconds", NULL },
  { "shutdown-mode", 'm', 0, G_OPTION_ARG_STRING, &shutdown_mode, "Shutdown mode (none, normal, fast)", NULL },
  { NULL },
};



DLT_DECLARE_CONTEXT (la_handler_context);



static void
unregister_dlt (void)
{
  DLT_UNREGISTER_CONTEXT (la_handler_context);
  DLT_UNREGISTER_APP ();
}



static gboolean
translate_shutdown_mode (const gchar     *str,
                         NSMShutdownType *mode)
{
  g_return_val_if_fail (mode != NULL, FALSE);

  /* try parsing the shutdown mode option into the NSMShutdownType enum */
  if (g_strcmp0 (str, "none") == 0)
    {
      *mode = NSM_SHUTDOWN_TYPE_NOT;
      return TRUE;
    }
  else if (g_strcmp0 (str, "normal") == 0)
    {
      *mode = NSM_SHUTDOWN_TYPE_NORMAL;
      return TRUE;
    }
  else if (g_strcmp0 (str, "fast") == 0)
    {
      *mode = NSM_SHUTDOWN_TYPE_FAST;
      return TRUE;
    }

  return FALSE;
}



int
main (int    argc,
      char **argv)
{
  GOptionContext *context;
  NSMShutdownType real_shutdown_mode;
  LAHandler      *service;
  GError         *error = NULL;
  gchar          *msg;

  /* register the application and context with the DLT */
  DLT_REGISTER_APP ("BMGR", "GENIVI Boot Manager");
  DLT_REGISTER_CONTEXT (la_handler_context, "LAH", "Legacy Application Handler");

  /* make sure to unregister the DLT at exit */
  atexit (unregister_dlt);

  /* initialize the GType type system */
  g_type_init ();

  /* prepare command line option parsing */
  context = g_option_context_new (NULL);
  g_option_context_set_help_enabled (context, TRUE);
  g_option_context_add_main_entries (context, entries, NULL);

  /* try to parse command line options */
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      /* parsing failed, exit with an error */
      msg = g_strdup_printf ("Failed to parse command line options: %s\n",
                             error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (msg));
      g_free (msg);

      /* clean up */
      g_option_context_free (context);
      g_error_free (error);
      g_free (unit);
      g_free (shutdown_mode);

      return EXIT_FAILURE;
    }
  g_option_context_free (context);

  /* abort if no unit file was specified */
  if (unit == NULL || *unit == '\0')
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to register legacy application: no unit specified"));

      /* free command line options */
      g_free (unit);
      g_free (shutdown_mode);

      return EXIT_FAILURE;
    }

  /* validate the shutdown mode */
  if (!translate_shutdown_mode (shutdown_mode, &real_shutdown_mode))
    {
      msg = g_strdup_printf ("Failed to register legacy application: "
                             "invalid shutdown mode \"%s\"", shutdown_mode);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (msg));
      g_free (msg);

      /* free command line options */
      g_free (unit);
      g_free (shutdown_mode);

      return EXIT_FAILURE;
    }

  /* validate the timeout */
  if (timeout < 0)
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to register legacy application: "
                           "shutdown timeout must be non-negative"));

      /* free command line options */
      g_free (unit);
      g_free (shutdown_mode);

      return EXIT_FAILURE;
    }

  /* create a proxy to talk to the legacy app handler D-Bus service */
  service =
    la_handler_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       "org.genivi.BootManager1",
                                       "/org/genivi/BootManager1/LegacyAppHandler",
                                       NULL, &error);

  /* abort if the proxy could not be created */
  if (service == NULL)
    {
      msg = g_strdup_printf ("Failed to register legacy application: %s", error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (msg));
      g_free (msg);

      /* clean up */
      g_error_free (error);

      /* free command line options */
      g_free (unit);
      g_free (shutdown_mode);

      return EXIT_FAILURE;
    }

  /* forward the register request to the legacy app handler D-Bus service */
  if (!la_handler_call_register_sync (service, unit, real_shutdown_mode, timeout,
                                      NULL, &error))
    {
      msg = g_strdup_printf ("Failed to register legacy application: %s", error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (msg));
      g_free (msg);

      /* clean up */
      g_error_free (error);
      g_object_unref (service);

      /* free command line options */
      g_free (unit);
      g_free (shutdown_mode);

      return EXIT_FAILURE;
    }

  /* release the legacy app handler proxy */
  g_object_unref (service);

  /* free command line options */
  g_free (unit);
  g_free (shutdown_mode);

  return EXIT_SUCCESS;
}
