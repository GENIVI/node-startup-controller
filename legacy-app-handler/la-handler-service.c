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

#include <common/glib-extensions.h>

#include <legacy-app-handler/la-handler-dbus.h>
#include <legacy-app-handler/la-handler-service.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void      la_handler_service_finalize              (GObject               *object);
static void      la_handler_service_get_property          (GObject               *object,
                                                            guint                  prop_id,
                                                            GValue                *value,
                                                            GParamSpec            *pspec);
static void      la_handler_service_set_property          (GObject               *object,
                                                           guint                  prop_id,
                                                           const GValue          *value,
                                                           GParamSpec            *pspec);
static gboolean  la_handler_service_handle_register       (LAHandler            *interface,
                                                           GDBusMethodInvocation *invocation,
                                                           GVariant              *apps,
                                                           LAHandlerService     *service);
static gboolean  la_handler_service_handle_deregister     (LAHandler            *interface,
                                                           GDBusMethodInvocation *invocation,
                                                           GVariant              *apps,
                                                           LAHandlerService     *service);



struct _LAHandlerServiceClass
{
  GObjectClass __parent__;
};

struct _LAHandlerService
{
  GObject          __parent__;

  GDBusConnection *connection;
  LAHandler      *interface;
};



G_DEFINE_TYPE (LAHandlerService, la_handler_service, G_TYPE_OBJECT);



static void
la_handler_service_class_init (LAHandlerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = la_handler_service_finalize;
  gobject_class->get_property = la_handler_service_get_property;
  gobject_class->set_property = la_handler_service_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
la_handler_service_init (LAHandlerService *service)
{
  service->interface = la_handler_skeleton_new ();

  /* implement the Register() handler */
  g_signal_connect (service->interface, "handle-register",
                    G_CALLBACK (la_handler_service_handle_register),
                    service);

  /* implement the Deregister() handler */
  g_signal_connect (service->interface, "handle-deregister",
                    G_CALLBACK (la_handler_service_handle_deregister),
                    service);
}




static void
la_handler_service_finalize (GObject *object)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  (*G_OBJECT_CLASS (la_handler_service_parent_class)->finalize) (object);
}



static void
la_handler_service_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, service->connection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
la_handler_service_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->connection = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
la_handler_service_handle_register (LAHandler            *object,
                                    GDBusMethodInvocation *invocation,
                                    GVariant              *apps,
                                    LAHandlerService     *service)
{
  g_debug ("Register called:");

  /* Notify the caller that we have handled the registration request */
  g_dbus_method_invocation_return_value (invocation, NULL);
  return TRUE;
}



static gboolean
la_handler_service_handle_deregister (LAHandler            *object,
                                      GDBusMethodInvocation *invocation,
                                      GVariant              *apps,
                                      LAHandlerService     *service)
{
  g_debug ("Deregister called:");

  /* Notify the caller that we have handled the registration request */
  g_dbus_method_invocation_return_value (invocation, NULL);
  return TRUE;
}



LAHandlerService *
la_handler_service_new (GDBusConnection *connection)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  return g_object_new (LA_HANDLER_TYPE_SERVICE, "connection", connection, NULL);
}



gboolean
la_handler_service_start (LAHandlerService *service,
                           GError           **error)
{
  g_return_val_if_fail (LA_HANDLER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the org.genivi.LegacyAppHandler1 service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/org/genivi/LegacyAppHandler1",
                                           error);
}
