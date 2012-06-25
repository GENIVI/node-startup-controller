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

#include <common/nsm-consumer-dbus.h>
#include <common/nsm-lifecycle-control-dbus.h>
#include <common/watchdog-client.h>

#include <nsm-dummy/nsm-consumer-service.h>
#include <nsm-dummy/nsm-dummy-application.h>
#include <nsm-dummy/nsm-lifecycle-control-service.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_CONSUMER_SERVICE,
  PROP_LIFECYCLE_CONTROL_SERVICE
};


static void     nsm_dummy_application_constructed  (GObject      *object);
static void     nsm_dummy_application_finalize     (GObject      *object);
static void     nsm_dummy_application_get_property (GObject      *object,
                                                    guint         prop_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);
static void     nsm_dummy_application_set_property (GObject      *object,
                                                    guint         prop_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void     nsm_dummy_application_startup      (GApplication *application);
static gboolean nsm_dummy_application_int_handler  (GApplication *application);



struct _NSMDummyApplicationClass
{
  GApplicationClass __parent__;
};

struct _NSMDummyApplication
{
  GApplication                __parent__;

  /* the connection to D-Bus */
  GDBusConnection            *connection;

  /* systemd watchdog client that repeatedly asks systemd to update
   * the watchdog timestamp */
  WatchdogClient             *watchdog_client;

  /* service objects that implements the NSM Dummy D-Bus interfaces */
  NSMLifecycleControlService *lifecycle_control_service;
  NSMConsumerService         *consumer_service;

  /* signal handler IDs */
  guint                       sigint_id;

  /* Identifier for the registered bus name */
  guint                       bus_name_id;
};



G_DEFINE_TYPE (NSMDummyApplication, nsm_dummy_application, G_TYPE_APPLICATION);



static void
nsm_dummy_application_class_init (NSMDummyApplicationClass *klass)
{
  GApplicationClass *gapplication_class;
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = nsm_dummy_application_finalize;
  gobject_class->constructed = nsm_dummy_application_constructed;
  gobject_class->get_property = nsm_dummy_application_get_property;
  gobject_class->set_property = nsm_dummy_application_set_property;

  gapplication_class = G_APPLICATION_CLASS (klass);
  gapplication_class->startup = nsm_dummy_application_startup;

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

  /* inform systemd that this process has started */
  sd_notify (0, "READY=1");
}



static void
nsm_dummy_application_init (NSMDummyApplication *application)
{
  /* update systemd's watchdog timestamp every 120 seconds */
  application->watchdog_client = watchdog_client_new (120);

  /* install the signal handler */
  application->sigint_id =
    g_unix_signal_add (SIGINT, (GSourceFunc) nsm_dummy_application_int_handler,
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
  g_source_remove (application->sigint_id);

  /* release the watchdog client */
  g_object_unref (application->watchdog_client);

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
    g_bus_own_name_on_connection (application->connection, "com.conti.NodeStateManager",
                                  G_BUS_NAME_OWNER_FLAGS_NONE, NULL, NULL, NULL, NULL);
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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
nsm_dummy_application_int_handler (GApplication *app)
{
  NSMDummyApplication *application = NSM_DUMMY_APPLICATION (app);

  /* call the shutdown consumer method */
  nsm_shutdown_consumers (application->consumer_service);

  return TRUE;
}



static void
nsm_dummy_application_startup (GApplication *app)
{
  NSMDummyApplication *application = NSM_DUMMY_APPLICATION (app);

  /* chain up to the parent class */
  (*G_APPLICATION_CLASS (nsm_dummy_application_parent_class)->startup) (app);

  /* update systemd's watchdog timestamp every 120 seconds */
  application->watchdog_client = watchdog_client_new (120);

  /* increment the reference count holding the application running to test the services */
  g_application_hold (app);
}



NSMDummyApplication *
nsm_dummy_application_new (GDBusConnection            *connection,
                           NSMConsumerService         *consumer_service,
                           NSMLifecycleControlService *lifecycle_control_service)
{
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (consumer_service), NULL);
  g_return_val_if_fail (NSM_LIFECYCLE_CONTROL_IS_SERVICE (lifecycle_control_service), NULL);

  return g_object_new (NSM_DUMMY_TYPE_APPLICATION,
                       "application-id", "com.conti.NodeStateManager",
                       "flags", G_APPLICATION_IS_SERVICE,
                       "connection", connection,
                       "nsm-consumer-service", consumer_service,
                       "nsm-lifecycle-control-service", lifecycle_control_service,
                       NULL);
}
