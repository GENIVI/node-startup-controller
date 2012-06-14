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

#include <boot-manager/boot-manager-dbus.h>
#include <boot-manager/boot-manager-service.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void     boot_manager_service_finalize     (GObject               *object);
static void     boot_manager_service_get_property (GObject               *object,
                                                   guint                  prop_id,
                                                   GValue                *value,
                                                   GParamSpec            *pspec);
static void     boot_manager_service_set_property (GObject               *object,
                                                   guint                  prop_id,
                                                   const GValue          *value,
                                                   GParamSpec            *pspec);
static gboolean boot_manager_service_handle_start (BootManager           *interface,
                                                   GDBusMethodInvocation *invocation,
                                                   const gchar           *unit,
                                                   BootManagerService    *service);



struct _BootManagerServiceClass
{
  GObjectClass __parent__;
};

struct _BootManagerService
{
  GObject          __parent__;

  GDBusConnection *connection;
  BootManager     *interface;
};



G_DEFINE_TYPE (BootManagerService, boot_manager_service, G_TYPE_OBJECT);



static void
boot_manager_service_class_init (BootManagerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = boot_manager_service_finalize;
  gobject_class->get_property = boot_manager_service_get_property;
  gobject_class->set_property = boot_manager_service_set_property;

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
boot_manager_service_init (BootManagerService *service)
{
  service->interface = boot_manager_skeleton_new ();

  /* implement the Start() handler */
  g_signal_connect (service->interface, "handle-start",
                    G_CALLBACK (boot_manager_service_handle_start), service);
}



static void
boot_manager_service_finalize (GObject *object)
{
  BootManagerService *service = BOOT_MANAGER_SERVICE (object);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  (*G_OBJECT_CLASS (boot_manager_service_parent_class)->finalize) (object);
}



static void
boot_manager_service_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BootManagerService *service = BOOT_MANAGER_SERVICE (object);

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
boot_manager_service_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BootManagerService *service = BOOT_MANAGER_SERVICE (object);

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
boot_manager_service_handle_start (BootManager           *interface,
                                   GDBusMethodInvocation *invocation,
                                   const gchar           *unit,
                                   BootManagerService    *service)
{
  g_return_val_if_fail (IS_BOOT_MANAGER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (unit != NULL, FALSE);
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);

  g_debug ("handle start of %s", unit);

  boot_manager_complete_start (service->interface, invocation, "done");

  return TRUE;
}



BootManagerService *
boot_manager_service_new (GDBusConnection *connection)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  return g_object_new (BOOT_MANAGER_TYPE_SERVICE, "connection", connection, NULL);
}



gboolean
boot_manager_service_start (BootManagerService *service,
                            GError             **error)
{
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the org.genivi.BootManager1 service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/org/genivi/BootManager1",
                                           error);
}
