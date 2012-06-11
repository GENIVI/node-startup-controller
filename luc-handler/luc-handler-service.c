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

#include <luc-handler/luc-handler-dbus.h>
#include <luc-handler/luc-handler-service.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void     luc_handler_service_finalize          (GObject               *object);
static void     luc_handler_service_get_property      (GObject               *object,
                                                       guint                  prop_id,
                                                       GValue                *value,
                                                       GParamSpec            *pspec);
static void     luc_handler_service_set_property      (GObject               *object,
                                                       guint                  prop_id,
                                                       const GValue          *value,
                                                       GParamSpec            *pspec);
static gboolean luc_handler_service_handle_register   (LUCHandler            *interface,
                                                       GDBusMethodInvocation *invocation,
                                                       LUCHandlerService     *service);
static gboolean luc_handler_service_handle_deregister (LUCHandler            *interface,
                                                       GDBusMethodInvocation *invocation,
                                                       LUCHandlerService     *service);



struct _LUCHandlerServiceClass
{
  GObjectClass __parent__;
};

struct _LUCHandlerService
{
  GObject          __parent__;

  GDBusConnection *connection;
  LUCHandler      *interface;
};



G_DEFINE_TYPE (LUCHandlerService, luc_handler_service, G_TYPE_OBJECT);



static void
luc_handler_service_class_init (LUCHandlerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = luc_handler_service_finalize; 
  gobject_class->get_property = luc_handler_service_get_property;
  gobject_class->set_property = luc_handler_service_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}



static void
luc_handler_service_init (LUCHandlerService *service)
{
  service->interface = luc_handler_skeleton_new ();

  /* TODO read LUC content from disk and set the "content" property of the 
   * skeleton instance */

  g_signal_connect (service->interface, "handle-register",
                    G_CALLBACK (luc_handler_service_handle_register),
                    service);

  g_signal_connect (service->interface, "handle-deregister",
                    G_CALLBACK (luc_handler_service_handle_deregister),
                    service);
}



static void
luc_handler_service_finalize (GObject *object)
{
  LUCHandlerService *service = LUC_HANDLER_SERVICE (object);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  (*G_OBJECT_CLASS (luc_handler_service_parent_class)->finalize) (object);
}



static void
luc_handler_service_get_property (GObject      *object,
                                      guint         prop_id,
                                      GValue       *value,
                                      GParamSpec   *pspec)
{
  LUCHandlerService *service = LUC_HANDLER_SERVICE (object);

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



static void luc_handler_service_set_property (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec)
{
  LUCHandlerService *service = LUC_HANDLER_SERVICE (object);

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
luc_handler_service_handle_register (LUCHandler            *object,
                                     GDBusMethodInvocation *invocation,
                                     LUCHandlerService     *service)
{
  g_return_val_if_fail (IS_LUC_HANDLER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (LUC_HANDLER_IS_SERVICE (service), FALSE);

  g_debug ("Register called");

  /* TODO read the apps parameter and update the "content" property of
   * the skeleton */

  g_dbus_method_invocation_return_value (invocation, NULL);

  return TRUE;
}



static gboolean
luc_handler_service_handle_deregister (LUCHandler            *object,
                                       GDBusMethodInvocation *invocation,
                                       LUCHandlerService     *service)
{
  g_return_val_if_fail (IS_LUC_HANDLER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (LUC_HANDLER_IS_SERVICE (service), FALSE);

  g_debug ("Deregister called");

  /* TODO read the apps parameter and update the "content" property of
   * the skeleton */

  g_dbus_method_invocation_return_value (invocation, NULL);

  return TRUE;
}



LUCHandlerService *
luc_handler_service_new (GDBusConnection *connection)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  return g_object_new (LUC_HANDLER_TYPE_SERVICE, "connection", connection, NULL);
}



gboolean
luc_handler_service_start (LUCHandlerService *service,
                           GError           **error)
{
  g_return_val_if_fail (LUC_HANDLER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the org.genivi.LUCHandler1 service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/org/genivi/LUCHandler1",
                                           error);
}
