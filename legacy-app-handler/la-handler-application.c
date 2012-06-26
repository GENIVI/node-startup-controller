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

#include <glib-object.h>
#include <gio/gio.h>

#include <dlt/dlt.h>

#include <common/watchdog-client.h>

#include <legacy-app-handler/la-handler-dbus.h>
#include <legacy-app-handler/la-handler-application.h>
#include <legacy-app-handler/la-handler-service.h>



DLT_IMPORT_CONTEXT (la_handler_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_LA_HANDLER_SERVICE,
};



static void la_handler_application_finalize     (GObject                 *object);
static void la_handler_application_get_property (GObject                 *object,
                                                 guint                    prop_id,
                                                 GValue                  *value,
                                                 GParamSpec              *pspec);
static void la_handler_application_set_property (GObject                 *object,
                                                 guint                    prop_id,
                                                 const GValue            *value,
                                                 GParamSpec              *pspec);
static void la_handler_application_startup      (GApplication            *application);
static int  la_handler_application_command_line (GApplication            *application,
                                                 GApplicationCommandLine *cmdline);



struct _LAHandlerApplicationClass
{
  GApplicationClass __parent__;
};

struct _LAHandlerApplication
{
  GApplication       __parent__;

  /* systemd watchdog client that repeatedly asks systemd to update
   * the watchdog timestamp */
  WatchdogClient    *watchdog_client;

  /* service object that implements the Legacy App Handler D-Bus interface */
  LAHandlerService *service;
};



G_DEFINE_TYPE (LAHandlerApplication, la_handler_application, G_TYPE_APPLICATION);



static void
la_handler_application_class_init (LAHandlerApplicationClass *klass)
{
  GApplicationClass *gapplication_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = la_handler_application_finalize;
  gobject_class->get_property = la_handler_application_get_property;
  gobject_class->set_property = la_handler_application_set_property;

  gapplication_class = G_APPLICATION_CLASS (klass);
  gapplication_class->startup = la_handler_application_startup;
  gapplication_class->command_line = la_handler_application_command_line;

  g_object_class_install_property (gobject_class,
                                   PROP_LA_HANDLER_SERVICE,
                                   g_param_spec_object ("la-handler-service",
                                                        "la-handler-service",
                                                        "la-handler-service",
                                                        LA_HANDLER_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
la_handler_application_init (LAHandlerApplication *application)
{
}



static void
la_handler_application_finalize (GObject *object)
{
  LAHandlerApplication *application = LA_HANDLER_APPLICATION (object);

  /* release the watchdog client */
  if (application->watchdog_client != NULL)
    g_object_unref (application->watchdog_client);

  /* release the Legacy App Handler service implementation */
  if (application->service != NULL)
    g_object_unref (application->service);

  (*G_OBJECT_CLASS (la_handler_application_parent_class)->finalize) (object);
}



static void
la_handler_application_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LAHandlerApplication *application = LA_HANDLER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_LA_HANDLER_SERVICE:
      g_value_set_object (value, application->service);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
la_handler_application_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LAHandlerApplication *application = LA_HANDLER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_LA_HANDLER_SERVICE:
      application->service = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
la_handler_application_startup (GApplication *app)
{
  LAHandlerApplication *application = LA_HANDLER_APPLICATION (app);

  /* chain up to the parent class */
  (*G_APPLICATION_CLASS (la_handler_application_parent_class)->startup) (app);

  /* update systemd's watchdog timestamp every 120 seconds */
  application->watchdog_client = watchdog_client_new (120);

  /* the Legacy Application Handler should keep running until it is shut down by the Node
   * State Manager. */
  g_application_hold (app);
}



static int
la_handler_application_command_line (GApplication            *application,
                                     GApplicationCommandLine *cmdline)
{
  GOptionContext *context;
  gboolean        do_register;
  GError         *error;
  gchar         **args;
  gchar         **argv;
  gchar          *message;
  gchar          *mode = NULL;
  gchar          *unit = NULL;
  gint            argc;
  gint            timeout;
  gint            i;

  GOptionEntry entries[] = {
    {"register",      0, 0, G_OPTION_ARG_NONE,   &do_register, NULL, NULL},
    {"unit",          0, 0, G_OPTION_ARG_STRING, &unit,     NULL, NULL},
    {"timeout",       0, 0, G_OPTION_ARG_INT,    &timeout,  NULL, NULL},
    {"shutdown-mode", 0, 0, G_OPTION_ARG_STRING, &mode,     NULL, NULL},
    {NULL},
  };

  /* keep the application running until we have finished */
  g_application_hold (application);

  /* retrieve the command-line arguments */
  args = g_application_command_line_get_arguments (cmdline, &argc);

  /* copy the args array, because g_option_context_parse() removes elements without
   * freeing them */
  argv = g_new (gchar *, argc + 1);
  for (i = 0; i <= argc; i++)
    argv[i] = args[i];

  /* set up the option context */
  context = g_option_context_new (NULL);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_add_main_entries (context, entries, NULL);

  /* parse the arguments into the argument data */
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      /* an error occurred */
      g_application_command_line_printerr (cmdline, "%s\n", error->message);
      g_error_free (error);
      g_application_command_line_set_exit_status (cmdline, EXIT_FAILURE);
    }
  else if (do_register)
    {
      if (unit != NULL && *unit != '\0' && timeout >= 0)
        {
          /* register was called correctly */
          message =
            g_strdup_printf ("Register application \"%s\" with mode \"%s\"and "
                             "timeout %dms",
                             unit,
                             (mode != NULL) ? mode : "normal",
                             timeout);
          DLT_LOG (la_handler_context, DLT_LOG_INFO, DLT_STRING (message));
          g_free (message);
        }
      else
        {
          /* register was called incorrectly */
          g_application_command_line_printerr (cmdline,
                                               "Invalid arguments. A unit must be "
                                               "specified and the timeout must be "
                                               "positive.\n");
        }
    }

  /* clean up */
  g_free (argv);
  g_strfreev (args);
  g_option_context_free (context);

  g_free (mode);
  g_free (unit);

  /* allow the application to stop */
  g_application_release (application);

  return EXIT_SUCCESS;
}



LAHandlerApplication *
la_handler_application_new (LAHandlerService *service,
                            GApplicationFlags flags)
{
  return g_object_new (LA_HANDLER_TYPE_APPLICATION,
                       "application-id", "org.genivi.LegacyAppHandler1",
                       "flags", flags,
                       "la-handler-service", service,
                       NULL);
}
