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

#include <common/nsm-consumer-dbus.h>
#include <common/nsm-enum-types.h>
#include <common/shutdown-client.h>
#include <common/shutdown-consumer-dbus.h>
#include <common/watchdog-client.h>

#include <node-startup-controller/boot-manager-application.h>
#include <node-startup-controller/node-startup-controller-service.h>
#include <node-startup-controller/job-manager.h>
#include <node-startup-controller/la-handler-service.h>
#include <node-startup-controller/luc-starter.h>




DLT_IMPORT_CONTEXT (boot_manager_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_JOB_MANAGER,
  PROP_LUC_STARTER,
  PROP_LA_HANDLER,
  PROP_MAIN_LOOP,
  PROP_NODE_STARTUP_CONTROLLER,
};



static void boot_manager_application_finalize                     (GObject                *object);
static void boot_manager_application_constructed                  (GObject                *object);
static void boot_manager_application_get_property                 (GObject                *object,
                                                                   guint                   prop_id,
                                                                   GValue                 *value,
                                                                   GParamSpec             *pspec);
static gboolean boot_manager_application_handle_lifecycle_request (ShutdownConsumer       *interface,
                                                                   GDBusMethodInvocation  *invocation,
                                                                   NSMShutdownType         request,
                                                                   guint                   request_id,
                                                                   BootManagerApplication *application);
static void boot_manager_application_handle_register_finish       (GObject                *object,
                                                                   GAsyncResult           *res,
                                                                   gpointer                user_data);
static void boot_manager_application_handle_unregister_finish     (GObject                *object,
                                                                   GAsyncResult           *res,
                                                                   gpointer                user_data);
static void boot_manager_application_set_property                 (GObject                *object,
                                                                   guint                   prop_id,
                                                                   const GValue           *value,
                                                                   GParamSpec             *pspec);
static void boot_manager_application_luc_groups_started           (LUCStarter             *starter,
                                                                   BootManagerApplication *application);



struct _BootManagerApplicationClass
{
  GObjectClass __parent__;
};

struct _BootManagerApplication
{
  GObject                       __parent__;

  /* the connection to D-Bus */
  GDBusConnection              *connection;

  /* systemd watchdog client that repeatedly asks systemd to update
   * the watchdog timestamp */
  WatchdogClient               *watchdog_client;

  /* manager of unit start and stop operations */
  JobManager                   *job_manager;

  /* implementation of the node startup controller service */
  NodeStartupControllerService *node_startup_controller;

  /* LUC starter to restore the LUC */
  LUCStarter                   *luc_starter;

  /* Legacy App Handler to register apps with the Node State Manager */
  LAHandlerService             *la_handler;

  /* the application's main loop */
  GMainLoop                    *main_loop;

  /* identifier for the registered bus name */
  guint                         bus_name_id;

  /* shutdown client for the boot manager itself */
  ShutdownClient               *client;
};



G_DEFINE_TYPE (BootManagerApplication, boot_manager_application, G_TYPE_OBJECT);



static void
boot_manager_application_class_init (BootManagerApplicationClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = boot_manager_application_finalize;
  gobject_class->constructed = boot_manager_application_constructed;
  gobject_class->get_property = boot_manager_application_get_property;
  gobject_class->set_property = boot_manager_application_set_property;

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
                                   PROP_LUC_STARTER,
                                   g_param_spec_object ("luc-starter",
                                                        "luc-starter",
                                                        "luc-starter",
                                                        TYPE_LUC_STARTER,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_MAIN_LOOP,
                                   g_param_spec_boxed ("main-loop",
                                                       "main-loop",
                                                       "main-loop",
                                                       G_TYPE_MAIN_LOOP,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_NODE_STARTUP_CONTROLLER,
                                   g_param_spec_object ("node-startup-controller",
                                                        "node-startup-controller",
                                                        "node-startup-controller",
                                                        TYPE_NODE_STARTUP_CONTROLLER_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
boot_manager_application_init (BootManagerApplication *application)
{
  const gchar *watchdog_str;
  guint64      watchdog_usec = 0;
  guint        watchdog_sec = 0;
  gchar       *message;

  /* read the WATCHDOG_USEC environment variable and parse it
   * into an unsigned integer */
  watchdog_str = g_getenv ("WATCHDOG_USEC");
  if (watchdog_str != NULL)
    watchdog_usec = g_ascii_strtoull (watchdog_str, NULL, 10);

  /* only create the watchdog client if a timeout was specified */
  if (watchdog_usec > 0)
    {
      /* halve the watchdog timeout because we need to notify systemd
       * twice in every interval; also, convert it to seconds */
      watchdog_sec = (guint) ((watchdog_usec / 2) / 1000000);

      /* update systemd's watchdog timestamp in regular intervals */
      application->watchdog_client = watchdog_client_new (watchdog_sec);

      /* log information about the watchdog timeout using DLT */
      message = g_strdup_printf ("Updating systemd's watchdog timestamp every %d seconds",
                                 watchdog_sec);
      DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (message));
      g_free (message);
    }
}



static void
boot_manager_application_finalize (GObject *object)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);
  ShutdownConsumer       *consumer;

  /* disconnect from the shutdown consumer */
  consumer = shutdown_client_get_consumer (application->client);
  g_signal_handlers_disconnect_matched (consumer, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL,
                                        application);

  /* release the shutdown client */
  g_object_unref (application->client);

  /* release the bus name */
  g_bus_unown_name (application->bus_name_id);

  /* release the D-Bus connection object */
  if (application->connection != NULL)
    g_object_unref (application->connection);

  /* release the watchdog client */
  if (application->watchdog_client != NULL)
    g_object_unref (application->watchdog_client);

  /* release the node startup controller */
  g_object_unref (application->node_startup_controller);

  /* release the LUC starter */
  g_object_unref (application->luc_starter);

  /* release the legacy app handler */
  g_object_unref (application->la_handler);

  /* release the job manager */
  g_object_unref (application->job_manager);

  /* release the main loop */
  g_main_loop_unref (application->main_loop);

  (*G_OBJECT_CLASS (boot_manager_application_parent_class)->finalize) (object);
}



static void
boot_manager_application_constructed (GObject *object)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);
  ShutdownConsumer       *consumer;
  NSMShutdownType         shutdown_mode;
  NSMConsumer            *nsm_consumer;
  GError                 *error = NULL;
  gchar                  *bus_name = "org.genivi.BootManager1";
  gchar                  *log_text;
  gchar                  *object_path;
  gint                    timeout;

  /* instantiate the LUC starter */
  application->luc_starter = luc_starter_new (application->job_manager,
                                              application->node_startup_controller);

  /* be notified when LUC groups have started so that we can hand
   * control over to systemd again */
  g_signal_connect (application->luc_starter, "luc-groups-started",
                    G_CALLBACK (boot_manager_application_luc_groups_started),
                    application);

  /* restore the LUC if desired */
  luc_starter_start_groups (application->luc_starter);

  /* get a bus name on the given connection */
  application->bus_name_id =
    g_bus_own_name_on_connection (application->connection, "org.genivi.BootManager1",
                                  G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL, NULL, NULL);

  /* create a shutdown client for the boot manager itself */
  object_path = "/org/genivi/BootManager1/ShutdownConsumer/0";
  shutdown_mode = NSM_SHUTDOWN_TYPE_NORMAL;
  timeout = 1000;
  application->client = shutdown_client_new (bus_name, object_path, shutdown_mode,
                                             timeout);

  /* create a shutdown consumer and implement its LifecycleRequest method */
  consumer = shutdown_consumer_skeleton_new ();
  shutdown_client_set_consumer (application->client, consumer);
  g_signal_connect (consumer, "handle-lifecycle-request",
                    G_CALLBACK (boot_manager_application_handle_lifecycle_request),
                    application);

  /* export the shutdown consumer on the bus */
  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (consumer),
                                         application->connection, object_path, &error))
    {
      log_text = g_strdup_printf ("Failed to export shutdown consumer on the bus: %s",
                                  error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_clear_error (&error);
    }

  /* register boot manager as a shutdown consumer */
  nsm_consumer = la_handler_service_get_nsm_consumer (application->la_handler);
  nsm_consumer_call_register_shutdown_client (nsm_consumer, bus_name, object_path,
                                              shutdown_mode, timeout, NULL,
                                              boot_manager_application_handle_register_finish,
                                              NULL);

  /* release the shutdown consumer */
  g_object_unref (consumer);
}



static void
boot_manager_application_handle_register_finish (GObject      *object,
                                                 GAsyncResult *res,
                                                 gpointer      user_data)
{
  NSMConsumer *nsm_consumer = NSM_CONSUMER (object);
  GError      *error = NULL;
  gchar       *log_text;
  gint         error_code = NSM_ERROR_STATUS_OK;

  g_return_if_fail (IS_NSM_CONSUMER (nsm_consumer));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));

  /* finish registering boot manager as a shutdown client */
  if (!nsm_consumer_call_register_shutdown_client_finish (nsm_consumer, &error_code, res,
                                                          &error))
    {
      log_text = g_strdup_printf ("Failed to register the boot manager as a shutdown "
                                  "consumer: %s", error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (error);
    }
  else if (error_code == NSM_ERROR_STATUS_OK)
    {
      log_text = g_strdup_printf ("The boot manager has registered as a shutdown "
                                  "consumer");
      DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (log_text));
      g_free (log_text);
    }
  else
    {
      log_text = g_strdup_printf ("Failed to register the boot manager as a shutdown "
                                  "consumer: error status %d", error_code);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
    }
}



static void
boot_manager_application_handle_unregister_finish (GObject      *object,
                                                   GAsyncResult *res,
                                                   gpointer      user_data)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (user_data);
  NSMConsumer            *nsm_consumer = NSM_CONSUMER (object);
  GError                 *error = NULL;
  gchar                  *log_text;
  gint                    error_code = NSM_ERROR_STATUS_OK;

  g_return_if_fail (IS_NSM_CONSUMER (nsm_consumer));
  g_return_if_fail (BOOT_MANAGER_IS_APPLICATION (application));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));

  /* finish unregistering boot manager as a shutdown client */
  if (!nsm_consumer_call_un_register_shutdown_client_finish (nsm_consumer, &error_code,
                                                             res, &error))
    {
      log_text = g_strdup_printf ("Failed to unregister the boot manager as a shutdown "
                                  "consumer: %s", error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (error);
    }
  else if (error_code == NSM_ERROR_STATUS_OK)
    {
      log_text = g_strdup_printf ("The boot manager has unregistered as a shutdown "
                                  "consumer");
      DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (log_text));
      g_free (log_text);
    }
  else
    {
      log_text = g_strdup_printf ("Failed to unregister the boot manager as a shutdown "
                                  "consumer: error status %d", error_code);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
    }

  /* quit the application */
  g_main_loop_quit (application->main_loop);
}



static gboolean
boot_manager_application_handle_lifecycle_request (ShutdownConsumer       *consumer,
                                                   GDBusMethodInvocation  *invocation,
                                                   NSMShutdownType         request,
                                                   guint                   request_id,
                                                   BootManagerApplication *application)
{
  NSMConsumer *nsm_consumer;
  const gchar *bus_name;
  const gchar *object_path;
  gint         shutdown_mode;

  g_return_val_if_fail (IS_SHUTDOWN_CONSUMER (consumer), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (BOOT_MANAGER_IS_APPLICATION (application), FALSE);

  /* cancel the LUC startup */
  luc_starter_cancel (application->luc_starter);

  /* deregister the shutdown consumers */
  la_handler_service_deregister_consumers (application->la_handler);

  /* let the NSM know that we have handled the lifecycle request */
  shutdown_consumer_complete_lifecycle_request (consumer, invocation,
                                                NSM_ERROR_STATUS_OK);

  /* deregister the boot manager as a shutdown client itself */
  nsm_consumer = la_handler_service_get_nsm_consumer (application->la_handler);
  bus_name = shutdown_client_get_bus_name (application->client);
  object_path = shutdown_client_get_object_path (application->client);
  shutdown_mode = shutdown_client_get_shutdown_mode (application->client);
  nsm_consumer_call_un_register_shutdown_client (nsm_consumer, bus_name,
                                                 object_path, shutdown_mode, NULL,
                                                 boot_manager_application_handle_unregister_finish,
                                                 application);
  return TRUE;
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
    case PROP_NODE_STARTUP_CONTROLLER:
      g_value_set_object (value, application->node_startup_controller);
    case PROP_LA_HANDLER:
      g_value_set_object (value, application->la_handler);
      break;
    case PROP_LUC_STARTER:
      g_value_set_object (value, application->luc_starter);
      break;
    case PROP_MAIN_LOOP:
      g_value_set_boxed (value, application->main_loop);
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
    case PROP_NODE_STARTUP_CONTROLLER:
      application->node_startup_controller = g_value_dup_object (value);
      break;
    case PROP_LA_HANDLER:
      application->la_handler = g_value_dup_object (value);
      break;
    case PROP_MAIN_LOOP:
      application->main_loop = g_main_loop_ref (g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
boot_manager_application_luc_groups_started (LUCStarter             *starter,
                                             BootManagerApplication *application)
{
  g_return_if_fail (IS_LUC_STARTER (starter));
  g_return_if_fail (BOOT_MANAGER_IS_APPLICATION (application));

  /* notify systemd that we have finished starting the LUC and
   * that it can take over control to start unfocused.target,
   * lazy.target etc. */
  sd_notify (0, "READY=1");
}



BootManagerApplication *
boot_manager_application_new (GMainLoop                    *main_loop,
                              GDBusConnection              *connection,
                              JobManager                   *job_manager,
                              LAHandlerService             *la_handler,
                              NodeStartupControllerService *node_startup_controller)
{
  g_return_val_if_fail (main_loop != NULL, NULL);
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (IS_JOB_MANAGER (job_manager), NULL);
  g_return_val_if_fail (LA_HANDLER_IS_SERVICE (la_handler), NULL);
  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER_SERVICE (node_startup_controller), NULL);

  return g_object_new (BOOT_MANAGER_TYPE_APPLICATION,
                       "connection", connection,
                       "node-startup-controller", node_startup_controller,
                       "job-manager", job_manager,
                       "la-handler", la_handler,
                       "main-loop", main_loop,
                       NULL);
}
