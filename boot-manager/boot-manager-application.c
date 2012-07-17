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

#include <glib-object.h>
#include <gio/gio.h>
#include <glib-unix.h>

#include <systemd/sd-daemon.h>

#include <dlt/dlt.h>

#include <boot-manager/boot-manager-application.h>
#include <boot-manager/boot-manager-service.h>
#include <boot-manager/job-manager.h>
#include <boot-manager/la-handler-service.h>
#include <boot-manager/luc-starter.h>
#include <boot-manager/watchdog-client.h>



DLT_IMPORT_CONTEXT (boot_manager_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_JOB_MANAGER,
  PROP_BOOT_MANAGER_SERVICE,
  PROP_LUC_STARTER,
  PROP_LA_HANDLER,
};



static void     boot_manager_application_finalize     (GObject      *object);
static void     boot_manager_application_constructed  (GObject      *object);
static void     boot_manager_application_get_property (GObject      *object,
                                                       guint         prop_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static void     boot_manager_application_set_property (GObject      *object,
                                                       guint         prop_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void     boot_manager_application_startup      (GApplication *application);
static gboolean boot_manager_application_int_handler  (GApplication *application);



struct _BootManagerApplicationClass
{
  GApplicationClass __parent__;
};

struct _BootManagerApplication
{
  GApplication        __parent__;

  /* the connection to D-Bus */
  GDBusConnection    *connection;

  /* systemd watchdog client that repeatedly asks systemd to update
   * the watchdog timestamp */
  WatchdogClient     *watchdog_client;

  /* internal handler of Start() and Stop() jobs */
  JobManager         *job_manager;

  /* boot manager service */
  BootManagerService *boot_manager_service;

  /* LUC starter to restore the LUC */
  LUCStarter         *luc_starter;

  /* signal handler IDs */
  guint               sigint_id;

  /* Legacy App Handler to register apps with the Node State Manager */
  LAHandlerService   *la_handler;

  /* Identifier for the registered bus name */
  guint               bus_name_id;
};



G_DEFINE_TYPE (BootManagerApplication, boot_manager_application, G_TYPE_APPLICATION);



static void
boot_manager_application_class_init (BootManagerApplicationClass *klass)
{
  GApplicationClass *gapplication_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = boot_manager_application_finalize;
  gobject_class->constructed = boot_manager_application_constructed;
  gobject_class->get_property = boot_manager_application_get_property;
  gobject_class->set_property = boot_manager_application_set_property;

  gapplication_class = G_APPLICATION_CLASS (klass);
  gapplication_class->startup = boot_manager_application_startup;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "D-Bus Connection",
                                                        "The connection to D-Bus",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_JOB_MANAGER,
                                   g_param_spec_object ("job-manager",
                                                        "Job Manager",
                                                        "The internal handler of Start()"
                                                        " and Stop() jobs",
                                                        TYPE_JOB_MANAGER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_LA_HANDLER,
                                   g_param_spec_object ("la-handler",
                                                        "LA Handler",
                                                        "Legacy Application Handler",
                                                        LA_HANDLER_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_BOOT_MANAGER_SERVICE,
                                   g_param_spec_object ("boot-manager-service",
                                                        "boot-manager-service",
                                                        "boot-manager-service",
                                                        BOOT_MANAGER_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_LUC_STARTER,
                                   g_param_spec_object ("luc-starter",
                                                        "luc-starter",
                                                        "luc-starter",
                                                        TYPE_LUC_STARTER,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
boot_manager_application_init (BootManagerApplication *application)
{
  /* update systemd's watchdog timestamp every 120 seconds */
  application->watchdog_client = watchdog_client_new (120);

  /* install the signal handler */
  application->sigint_id =
    g_unix_signal_add (SIGINT, (GSourceFunc) boot_manager_application_int_handler,
                       application);
}



static void
boot_manager_application_finalize (GObject *object)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  /* release the bus name */
  g_bus_unown_name (application->bus_name_id);

  /* release the D-Bus connection object */
  if (application->connection != NULL)
    g_object_unref (application->connection);

  /* release the signal handler */
  g_source_remove (application->sigint_id);

  /* release the watchdog client */
  g_object_unref (application->watchdog_client);

  /* release the boot manager */
  g_object_unref (application->boot_manager_service);

  /* release the LUC starter */
  g_object_unref (application->luc_starter);

  /* release the legacy app handler */
  g_object_unref (application->la_handler);

  /* release the job manager */
  g_object_unref (application->job_manager);

  (*G_OBJECT_CLASS (boot_manager_application_parent_class)->finalize) (object);
}



static void
boot_manager_application_constructed (GObject *object)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  /* get a bus name on the given connection */
  application->bus_name_id =
    g_bus_own_name_on_connection (application->connection, "org.genivi.BootManager1",
                                  G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL, NULL, NULL);

  /* instantiate the LUC starter */
  application->luc_starter = luc_starter_new (application->job_manager,
                                              application->boot_manager_service);
}



static void
boot_manager_application_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, application->connection);
      break;
    case PROP_JOB_MANAGER:
      g_value_set_object (value, application->job_manager);
      break;
    case PROP_BOOT_MANAGER_SERVICE:
      g_value_set_object (value, application->boot_manager_service);
    case PROP_LA_HANDLER:
      g_value_set_object (value, application->la_handler);
      break;
    case PROP_LUC_STARTER:
      g_value_set_object (value, application->luc_starter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
boot_manager_application_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      application->connection = g_value_dup_object (value);
      break;
    case PROP_JOB_MANAGER:
      application->job_manager = g_value_dup_object (value);
      break;
    case PROP_BOOT_MANAGER_SERVICE:
      application->boot_manager_service = g_value_dup_object (value);
      break;
    case PROP_LA_HANDLER:
      application->la_handler = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
boot_manager_application_startup (GApplication *app)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (app);
  GError                 *error = NULL;
  gchar                  *log_text;

  /* chain up to the parent class */
  (*G_APPLICATION_CLASS (boot_manager_application_parent_class)->startup) (app);

  /* start the boot manager service */
  boot_manager_service_start_up (application->boot_manager_service, &error);

  if (error != NULL)
    {
      log_text = g_strdup_printf ("Error starting boot manager service: %s",
                                  error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
    }

  /* restore the LUC if desired */
  luc_starter_start_groups (application->luc_starter);

  /* start the legacy app handler */
  la_handler_service_start (application->la_handler, &error);

  if (error != NULL)
    {
      log_text =
        g_strdup_printf ("Boot Manager Application failed to start the legacy app "
                         "handler: %s", error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
    }

  /* inform systemd that this process has started */
  sd_notify (0, "READY=1");

  /* hold the application so that it persists */
  g_application_hold (app);
}


static gboolean
boot_manager_application_int_handler (GApplication *app)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (app);

  luc_starter_cancel (application->luc_starter);

  return TRUE;
}



BootManagerApplication *
boot_manager_application_new (GDBusConnection         *connection,
                              JobManager              *job_manager,
                              LAHandlerService        *la_handler,
                              BootManagerService      *boot_manager_service)
{
  g_return_val_if_fail (IS_JOB_MANAGER (job_manager), NULL);
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (boot_manager_service), NULL);

  return g_object_new (BOOT_MANAGER_TYPE_APPLICATION,
                       "application-id", "org.genivi.BootManager1",
                       "flags", G_APPLICATION_IS_SERVICE,
                       "connection", connection,
                       "job-manager", job_manager,
                       "boot-manager-service", boot_manager_service,
                       "la-handler", la_handler,
                       NULL);
}
