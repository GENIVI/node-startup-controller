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

#include <common/shutdown-consumer-dbus.h>

#include <boot-manager/shutdown-consumer-service.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_OBJECT_PATH,
  PROP_UNIT_NAME,
};



/* signal identifiers */
enum
{
  SIGNAL_SHUTDOWN_REQUESTED,
  LAST_SIGNAL,
};



static void     shutdown_consumer_service_finalize        (GObject                 *object);
static void     shutdown_consumer_service_get_property    (GObject                 *object,
                                                           guint                    prop_id,
                                                           GValue                  *value,
                                                           GParamSpec              *pspec);
static void     shutdown_consumer_service_set_property    (GObject                 *object,
                                                           guint                    prop_id,
                                                           const GValue            *value,
                                                           GParamSpec              *pspec);
static gboolean shutdown_consumer_service_handle_shutdown (ShutdownConsumer        *interface,
                                                           GDBusMethodInvocation   *invocation,
                                                           ShutdownConsumerService *service);



struct _ShutdownConsumerServiceClass
{
  GObjectClass __parent__;
};

struct _ShutdownConsumerService
{
  GObject           __parent__;

  /* properties */
  ShutdownConsumer *interface;
  GDBusConnection  *connection;
  gchar            *object_path;
  gchar            *unit_name;
};



G_DEFINE_TYPE (ShutdownConsumerService, shutdown_consumer_service, G_TYPE_OBJECT);



static guint shutdown_consumer_signals[LAST_SIGNAL];



static void
shutdown_consumer_service_class_init (ShutdownConsumerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = shutdown_consumer_service_finalize;
  gobject_class->get_property = shutdown_consumer_service_get_property;
  gobject_class->set_property = shutdown_consumer_service_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "The D-Bus connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        "object path",
                                                        "The object path to the shutdown"
                                                        " consumer",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
                                   PROP_UNIT_NAME,
                                   g_param_spec_string ("unit-name",
                                                        "unit name",
                                                        "The name of the unit the "
                                                        "shutdown consumer is "
                                                        "responsible for",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  /* create the 'shutdown-consumer-reply' signal */
  shutdown_consumer_signals[SIGNAL_SHUTDOWN_REQUESTED] =
    g_signal_new ("shutdown-requested", TYPE_SHUTDOWN_CONSUMER_SERVICE,
                  G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static void
shutdown_consumer_service_init (ShutdownConsumerService *service)
{
  service->interface = shutdown_consumer_skeleton_new ();

  /* implement the Shutdown() handler */
  g_signal_connect (service->interface, "handle-shutdown",
                    G_CALLBACK (shutdown_consumer_service_handle_shutdown),
                    service);
}



static void
shutdown_consumer_service_finalize (GObject *object)
{
  ShutdownConsumerService *service = SHUTDOWN_CONSUMER_SERVICE (object);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the interface skeleton */
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (service->interface));
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  (*G_OBJECT_CLASS (shutdown_consumer_service_parent_class)->finalize) (object);
}



static void
shutdown_consumer_service_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ShutdownConsumerService *service = SHUTDOWN_CONSUMER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, service->connection);
      break;
    case PROP_OBJECT_PATH:
      g_value_set_string (value, service->object_path);
      break;
    case PROP_UNIT_NAME:
      g_value_set_string (value, service->unit_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
shutdown_consumer_service_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ShutdownConsumerService *service = SHUTDOWN_CONSUMER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->connection = g_value_dup_object (value);
      break;
    case PROP_OBJECT_PATH:
      service->object_path = g_value_dup_string (value);
      break;
    case PROP_UNIT_NAME:
      service->unit_name = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
shutdown_consumer_service_handle_shutdown (ShutdownConsumer        *interface,
                                           GDBusMethodInvocation   *invocation,
                                           ShutdownConsumerService *service)
{
  /* Signal that this Shutdown Consumer has been told to shut down */
  g_signal_emit (service, shutdown_consumer_signals[SIGNAL_SHUTDOWN_REQUESTED], 0);

  /* notify the caller that we have handled the shutdown request */
  g_dbus_method_invocation_return_value (invocation, NULL);

  return TRUE;
}



const gchar *
shutdown_consumer_service_get_object_path (ShutdownConsumerService *service)
{
  g_return_val_if_fail (IS_SHUTDOWN_CONSUMER_SERVICE (service), NULL);

  return service->object_path;
}



const gchar *
shutdown_consumer_service_get_unit_name (ShutdownConsumerService *service)
{
  g_return_val_if_fail (IS_SHUTDOWN_CONSUMER_SERVICE (service), NULL);

  return service->unit_name;
}




ShutdownConsumerService *
shutdown_consumer_service_new (GDBusConnection *connection,
                               const gchar     *object_path,
                               const gchar     *unit_name)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (object_path != NULL && *object_path != '\0', NULL);
  g_return_val_if_fail (unit_name != NULL && *unit_name != '\0', NULL);

  return g_object_new (TYPE_SHUTDOWN_CONSUMER_SERVICE,
                       "connection", connection,
                       "object-path", object_path,
                       "unit-name", unit_name,
                       NULL);
}



gboolean
shutdown_consumer_service_start (ShutdownConsumerService *service,
                                 GError                 **error)
{
  g_return_val_if_fail (IS_SHUTDOWN_CONSUMER_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the shutdown consumer on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           service->object_path,
                                           error);
}
