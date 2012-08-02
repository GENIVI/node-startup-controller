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

#include <dlt/dlt.h>

#include <common/nsm-enum-types.h>
#include <common/nsm-lifecycle-control-dbus.h>

#include <nsm-dummy/nsm-lifecycle-control-service.h>



DLT_IMPORT_CONTEXT (nsm_dummy_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void     nsm_lifecycle_control_service_finalize                  (GObject                    *object);
static void     nsm_lifecycle_control_service_get_property              (GObject                    *object,
                                                                         guint                       prop_id,
                                                                         GValue                     *value,
                                                                         GParamSpec                 *pspec);
static void     nsm_lifecycle_control_service_set_property              (GObject                    *object,
                                                                         guint                       prop_id,
                                                                         const GValue               *value,
                                                                         GParamSpec                 *pspec);
static gboolean nsm_lifecycle_control_service_handle_set_node_state     (NSMLifecycleControl        *object,
                                                                         GDBusMethodInvocation      *invocation,
                                                                         gint                        node_state_id,
                                                                         NSMLifecycleControlService *service);
static gboolean nsm_lifecycle_control_service_handle_check_luc_required (NSMLifecycleControl        *object,
                                                                         GDBusMethodInvocation      *invocation,
                                                                         NSMLifecycleControlService *service);



struct _NSMLifecycleControlServiceClass
{
  GObjectClass __parent__;
};

struct _NSMLifecycleControlService
{
  GObject              __parent__;

  NSMLifecycleControl *interface;
  GDBusConnection     *connection;

  /* this variable show if the node state manager mode has been set */
  gboolean             accept_state;

  /* this variable show if LUC should be restored */
  gboolean             luc_required;
};



G_DEFINE_TYPE (NSMLifecycleControlService, nsm_lifecycle_control_service, G_TYPE_OBJECT);



static void
nsm_lifecycle_control_service_class_init (NSMLifecycleControlServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = nsm_lifecycle_control_service_finalize;
  gobject_class->get_property = nsm_lifecycle_control_service_get_property;
  gobject_class->set_property = nsm_lifecycle_control_service_set_property;

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
nsm_lifecycle_control_service_init (NSMLifecycleControlService *service)
{
  service->interface = nsm_lifecycle_control_skeleton_new ();
  service->accept_state = TRUE;
  service->luc_required = TRUE;

  /* implement the SetNodeState() handler */
  g_signal_connect (service->interface, "handle-set-node-state",
                    G_CALLBACK (nsm_lifecycle_control_service_handle_set_node_state),
                    service);

  /* implement the CheckLucRequired() handler */
  g_signal_connect (service->interface, "handle-check-luc-required",
                    G_CALLBACK (nsm_lifecycle_control_service_handle_check_luc_required),
                    service);
}



static void
nsm_lifecycle_control_service_finalize (GObject *object)
{
  NSMLifecycleControlService *service = NSM_LIFECYCLE_CONTROL_SERVICE (object);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (service->interface));
  g_object_unref (service->interface);

  (*G_OBJECT_CLASS (nsm_lifecycle_control_service_parent_class)->finalize) (object);
}



static void
nsm_lifecycle_control_service_get_property (GObject    *object,
                                            guint       prop_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  NSMLifecycleControlService *service = NSM_LIFECYCLE_CONTROL_SERVICE (object);

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
nsm_lifecycle_control_service_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  NSMLifecycleControlService *service = NSM_LIFECYCLE_CONTROL_SERVICE (object);

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
nsm_lifecycle_control_service_handle_set_node_state (NSMLifecycleControl        *object,
                                                     GDBusMethodInvocation      *invocation,
                                                     gint                        node_state_id,
                                                     NSMLifecycleControlService *service)
{
  gint error_code;

  g_return_val_if_fail (IS_NSM_LIFECYCLE_CONTROL (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_LIFECYCLE_CONTROL_IS_SERVICE (service), FALSE);

  /* check whether this is a valid node state */
  if (node_state_id >= NSM_NODE_STATE_NOT_SET && node_state_id <= NSM_NODE_STATE_LAST)
    {
      /* log how we handled the node state */
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Node state"), DLT_INT (node_state_id),
               DLT_STRING ("applied:"),
               DLT_STRING (service->accept_state ? "yes" : "no"));

      /* alternate return value between successful(0) and fail(-1) with every handled call.
       * We are temporarily assuming that 0 is success and -1 is failure */
      if (service->accept_state)
        error_code = NSM_ERROR_STATUS_OK;
      else
        error_code = NSM_ERROR_STATUS_ERROR;
      service->accept_state = !service->accept_state;
    }
  else
    {
      /* log how we handled the node state */
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Received an invalid node state:"), DLT_INT (node_state_id));

      /* let the caller know that it sent an invalid parameter */
      error_code = NSM_ERROR_STATUS_PARAMETER;
    }

  /* notify the caller that we have handled the register request */
  nsm_lifecycle_control_complete_set_node_state (object, invocation, error_code);
  return TRUE;
}



static gboolean
nsm_lifecycle_control_service_handle_check_luc_required (NSMLifecycleControl        *object,
                                                         GDBusMethodInvocation      *invocation,
                                                         NSMLifecycleControlService *service)
{
  g_return_val_if_fail (IS_NSM_LIFECYCLE_CONTROL (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_LIFECYCLE_CONTROL_IS_SERVICE (service), FALSE);

  /* alternate return value between true and false with every handled call */
  service->luc_required = !service->luc_required;

  /* notify the caller that we have handled the register request */
  nsm_lifecycle_control_complete_check_luc_required (object,
                                                     invocation,
                                                     service->luc_required);
  return TRUE;
}



NSMLifecycleControlService *
nsm_lifecycle_control_service_new (GDBusConnection *connection)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  return g_object_new (NSM_LIFECYCLE_CONTROL_TYPE_SERVICE, "connection", connection, NULL);
}



gboolean
nsm_lifecycle_control_service_start (NSMLifecycleControlService *service,
                                     GError                    **error)
{
  g_return_val_if_fail (NSM_LIFECYCLE_CONTROL_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the LifecycleControl service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/com/contiautomotive/NodeStateManager/LifecycleControl",
                                           error);
}
