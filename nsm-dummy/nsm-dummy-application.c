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
#include <common/nsm-lifecycle-control-dbus.h>
#include <common/watchdog-client.h>

#include <nsm-dummy/nsm-consumer-service.h>
#include <nsm-dummy/nsm-dummy-application.h>
#include <nsm-dummy/nsm-lifecycle-control-service.h>



DLT_IMPORT_CONTEXT (nsm_dummy_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_CONSUMER_SERVICE,
  PROP_LIFECYCLE_CONTROL_SERVICE,
  PROP_MAIN_LOOP,
};



static void     nsm_dummy_application_constructed   (GObject      *object);
static void     nsm_dummy_application_finalize      (GObject      *object);
static void     nsm_dummy_application_get_property  (GObject      *object,
                                                     guint         prop_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);
static void     nsm_dummy_application_set_property  (GObject      *object,
                                                     guint         prop_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static gboolean nsm_dummy_application_handle_sighup (gpointer      user_data);



struct _NSMDummyApplicationClass
{
  GObjectClass __parent__;
};

struct _NSMDummyApplication
{
  GObject                     __parent__;

  /* the connection to D-Bus */
  GDBusConnection            *connection;

  /* systemd watchdog client that repeatedly asks systemd to update
   * the watchdog timestamp */
  WatchdogClient             *watchdog_client;

  /* service objects that implements the NSM Dummy D-Bus interfaces */
  NSMLifecycleControlService *lifecycle_control_service;
  NSMConsumerService         *consumer_service;

  /* the main loop of the application */
  GMainLoop                  *main_loop;

  /* signal handler IDs */
  guint                       sighup_id;

  /* Identifier for the registered bus name */
  guint                       bus_name_id;
};



G_DEFINE_TYPE (NSMDummyApplication, nsm_dummy_application, G_TYPE_OBJECT);



static void
nsm_dummy_application_class_init (NSMDummyApplicationClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = nsm_dummy_application_finalize;
  gobject_class->constructed = nsm_dummy_application_constructed;
  gobject_class->get_property = nsm_dummy_application_get_property;
  gobject_class->set_property = nsm_dummy_application_set_property;

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
                                   PROP_CONSUMER_SERVICE,
                                   g_param_spec_object ("nsm-consumer-service",
                                                        "nsm-consumer-service",
                                                        "nsm-consumer-service",
                                                        NSM_CONSUMER_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
 g_object_class_install_property (gobject_class,
                                  PROP_LIFECYCLE_CONTROL_SERVICE,
                                  g_param_spec_object ("nsm-lifecycle-control-service",
                                                       "nsm-lifecycle-control-service",
                                                       "nsm-lifecycle-control-service",
                                                        NSM_LIFECYCLE_CONTROL_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
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
}



static void
nsm_dummy_application_init (NSMDummyApplication *application)
{
  const gchar *watchdog_str;
  guint64      watchdog_usec = 0;
  guint        watchdog_sec = 0;

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
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Updating the systemd watchdog timestamp every"),
               DLT_UINT (watchdog_sec), DLT_STRING ("seconds"));
    }

  /* install the signal handler */
  application->sighup_id = g_unix_signal_add (SIGHUP, nsm_dummy_application_handle_sighup,
                                              application);
}



static void
nsm_dummy_application_finalize (GObject *object)
{
  NSMDummyApplication *application = NSM_DUMMY_APPLICATION (object);

  /* release the D-Bus connection object */
  if (application->connection != NULL)
    g_object_unref (application->connection);

  /* release the bus name */
  g_bus_unown_name (application->bus_name_id);

  /* release the signal handler */
  g_source_remove (application->sighup_id);

  /* release the watchdog client */
  if (application->watchdog_client != NULL)
    g_object_unref (application->watchdog_client);

  /* release the main loop */
  g_main_loop_unref (application->main_loop);

  /* release the NSM Dummy service implementations */
  if (application->consumer_service != NULL)
    g_object_unref (application->consumer_service);

  if (application->lifecycle_control_service != NULL)
    g_object_unref (application->lifecycle_control_service);

  (*G_OBJECT_CLASS (nsm_dummy_application_parent_class)->finalize) (object);
}



static void
nsm_dummy_application_constructed (GObject *object)
{
  NSMDummyApplication *application = NSM_DUMMY_APPLICATION (object);

  /* get a bus name on the given connection */
  application->bus_name_id =
    g_bus_own_name_on_connection (application->connection,
                                  "com.contiautomotive.NodeStateManager",
                                  G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL, NULL, NULL);

  /* inform systemd that this process has started */
  sd_notify (0, "READY=1");
}



static void
nsm_dummy_application_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  NSMDummyApplication *application = NSM_DUMMY_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, application->connection);
      break;
    case PROP_CONSUMER_SERVICE:
      g_value_set_object (value, application->consumer_service);
      break;
    case PROP_LIFECYCLE_CONTROL_SERVICE:
      g_value_set_object (value, application->lifecycle_control_service);
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
nsm_dummy_application_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  NSMDummyApplication *application = NSM_DUMMY_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      application->connection = g_value_dup_object (value);
      break;
    case PROP_CONSUMER_SERVICE:
      application->consumer_service = g_value_dup_object (value);
      break;
    case PROP_LIFECYCLE_CONTROL_SERVICE:
      application->lifecycle_control_service = g_value_dup_object (value);
      break;
    case PROP_MAIN_LOOP:
      application->main_loop = g_main_loop_ref (g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
nsm_dummy_application_handle_sighup (gpointer user_data)
{
  NSMDummyApplication *application = NSM_DUMMY_APPLICATION (user_data);

  /* call the shutdown consumer method */
  nsm_consumer_service_shutdown_consumers (application->consumer_service);

  return TRUE;
}



NSMDummyApplication *
nsm_dummy_application_new (GMainLoop                  *main_loop,
                           GDBusConnection            *connection,
                           NSMConsumerService         *consumer_service,
                           NSMLifecycleControlService *lifecycle_control_service)
{
  g_return_val_if_fail (main_loop != NULL, NULL);
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (consumer_service), NULL);
  g_return_val_if_fail (NSM_LIFECYCLE_CONTROL_IS_SERVICE (lifecycle_control_service), NULL);

  return g_object_new (NSM_DUMMY_TYPE_APPLICATION,
                       "main-loop", main_loop,
                       "connection", connection,
                       "nsm-consumer-service", consumer_service,
                       "nsm-lifecycle-control-service", lifecycle_control_service,
                       NULL);
}
