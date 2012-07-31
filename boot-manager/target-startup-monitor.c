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

#include <boot-manager/boot-manager-dbus.h>
#include <boot-manager/target-startup-monitor.h>
#include <boot-manager/systemd-unit-dbus.h>



DLT_IMPORT_CONTEXT (boot_manager_context);



/* property identifier */
enum
{
  PROP_0,
  PROP_SYSTEMD_MANAGER,
};



typedef struct _GetUnitData GetUnitData;



static void     target_startup_monitor_finalize                (GObject              *object);
static void     target_startup_monitor_constructed             (GObject              *object);
static void     target_startup_monitor_get_property            (GObject              *object,
                                                                guint                 prop_id,
                                                                GValue               *value,
                                                                GParamSpec           *pspec);
static void     target_startup_monitor_set_property            (GObject              *object,
                                                                guint                 prop_id,
                                                                const GValue         *value,
                                                                GParamSpec           *pspec);
static void     target_startup_monitor_get_unit_finish         (GObject              *object,
                                                                GAsyncResult         *res,
                                                                gpointer              user_data);
static void     target_startup_monitor_unit_proxy_new_finish   (GObject              *object,
                                                                GAsyncResult         *res,
                                                                gpointer              user_data);
static void     target_startup_monitor_unit_properties_changed (GDBusProxy           *proxy,
                                                                GVariant             *changed_properties,
                                                                GStrv                *invalidated_properties,
                                                                TargetStartupMonitor *monitor);
static void     target_startup_monitor_set_node_state          (TargetStartupMonitor *monitor,
                                                                NSMNodeState          state);
static void     target_startup_monitor_set_node_state_finish   (GObject              *object,
                                                                GAsyncResult         *res,
                                                                gpointer              user_data);



struct _TargetStartupMonitorClass
{
  GObjectClass __parent__;
};

struct _TargetStartupMonitor
{
  GObject              __parent__;

  SystemdManager      *systemd_manager;

  NSMLifecycleControl *nsm_lifecycle_control;

  /* list of systemd units for the targets we are interested in */
  GList               *units;

  /* map of systemd target names to corresponding node states */
  GHashTable          *targets_to_states;
};

struct _GetUnitData
{
  TargetStartupMonitor *monitor;
  const gchar          *unit_name;
  gchar                *object_path;
};



G_DEFINE_TYPE (TargetStartupMonitor, target_startup_monitor, G_TYPE_OBJECT);



static void
target_startup_monitor_class_init (TargetStartupMonitorClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = target_startup_monitor_finalize;
  gobject_class->constructed = target_startup_monitor_constructed;
  gobject_class->get_property = target_startup_monitor_get_property;
  gobject_class->set_property = target_startup_monitor_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_SYSTEMD_MANAGER,
                                   g_param_spec_object ("systemd-manager",
                                                        "systemd-manager",
                                                        "systemd-manager",
                                                        TYPE_SYSTEMD_MANAGER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
target_startup_monitor_init (TargetStartupMonitor *monitor)
{
  GError      *error = NULL;
  gchar       *log_text;

  /* create proxy to talk to the Node State Manager's lifecycle control */
  monitor->nsm_lifecycle_control =
    nsm_lifecycle_control_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                  "com.contiautomotive.NodeStateManager",
                                                  "/com/contiautomotive/NodeStateManager/LifecycleControl",
                                                  NULL, &error);
  if (error != NULL)
    {
     log_text = g_strdup_printf ("Failed to connect to the NSM lifecycle control: %s",
                                  error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (error);
    }

  /* set the initial state to base running, which means that
   * the mandatory.target has been started (this is done before the
   * boot manager itself is brought up) */
  target_startup_monitor_set_node_state (monitor, NSM_NODE_STATE_BASE_RUNNING);

  /* create the table of targets and their node states */
  monitor->targets_to_states = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (monitor->targets_to_states, "focussed.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_LUC_RUNNING));
  g_hash_table_insert (monitor->targets_to_states, "unfocussed.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_FULLY_RUNNING));
  g_hash_table_insert (monitor->targets_to_states, "lazy.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_FULLY_OPERATIONAL));
}



static void
target_startup_monitor_constructed (GObject *object)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);
  GetUnitData          *data;

  data = g_slice_new0 (GetUnitData);
  data->monitor = g_object_ref (monitor);
  data->unit_name = "focussed.target";

  systemd_manager_call_get_unit (monitor->systemd_manager, "focussed.target", NULL,
                                 target_startup_monitor_get_unit_finish, data);

  data = g_slice_new0 (GetUnitData);
  data->monitor = g_object_ref (monitor);
  data->unit_name = "unfocussed.target";

  systemd_manager_call_get_unit (monitor->systemd_manager, "unfocussed.target", NULL,
                                 target_startup_monitor_get_unit_finish, data);

  data = g_slice_new0 (GetUnitData);
  data->monitor = g_object_ref (monitor);
  data->unit_name = "lazy.target";

  systemd_manager_call_get_unit (monitor->systemd_manager, "lazy.target", NULL,
                                 target_startup_monitor_get_unit_finish, data);
}



static void
target_startup_monitor_finalize (GObject *object)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);
  GList                *lp;

  /* disconnect from all the unit proxies and release them */
  for (lp = monitor->units; lp != NULL; lp = lp->next)
    {
      g_signal_handlers_disconnect_matched (lp->data, G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, monitor);
      g_object_unref (lp->data);
    }
  g_list_free (lp->data);

  /* release the mapping of systemd targets to node states */
  g_hash_table_destroy (monitor->targets_to_states);

  /* release the systemd manager */
  g_signal_handlers_disconnect_matched (monitor->systemd_manager,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, monitor);
  g_object_unref (monitor->systemd_manager);

  if (monitor->nsm_lifecycle_control != NULL)
    g_object_unref (monitor->nsm_lifecycle_control);

  (*G_OBJECT_CLASS (target_startup_monitor_parent_class)->finalize) (object);
}



static void
target_startup_monitor_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);

  switch (prop_id)
    {
    case PROP_SYSTEMD_MANAGER:
      g_value_set_object (value, monitor->systemd_manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
target_startup_monitor_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);

  switch (prop_id)
    {
    case PROP_SYSTEMD_MANAGER:
      monitor->systemd_manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
target_startup_monitor_get_unit_finish (GObject      *object,
                                        GAsyncResult *res,
                                        gpointer      user_data)
{
  GetUnitData *data = user_data;
  GError      *error = NULL;
  gchar       *message;
  gchar       *object_path;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (data != NULL);

  if (!systemd_manager_call_get_unit_finish (SYSTEMD_MANAGER (object), &object_path,
                                             res, &error))
    {
      message = g_strdup_printf ("Failed to get unit \"%s\" from systemd: %s",
                                 data->unit_name, error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (message));
      g_free (message);
      g_error_free (error);

      /* release the get unit data */
      g_object_unref (data->monitor);
      g_slice_free (GetUnitData, data);
    }
  else
    {
      message = g_strdup_printf ("Creating D-Bus proxy for unit \"%s\"", object_path);
      DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (message));
      g_free (message);

      /* remember the object path */
      data->object_path = object_path;

      systemd_unit_proxy_new (g_dbus_proxy_get_connection (G_DBUS_PROXY (data->monitor->systemd_manager)),
                              G_DBUS_PROXY_FLAGS_GET_INVALIDATED_PROPERTIES,
                              "org.freedesktop.systemd1",
                              object_path,
                              NULL,
                              target_startup_monitor_unit_proxy_new_finish,
                              data);
    }
}



static void
target_startup_monitor_unit_proxy_new_finish (GObject      *object,
                                              GAsyncResult *res,
                                              gpointer      user_data)
{
  GetUnitData *data = user_data;
  SystemdUnit *unit;
  GError      *error = NULL;
  gchar       *message;

  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (data != NULL);

  unit = systemd_unit_proxy_new_finish (res, &error);

  if (error != NULL)
    {
      message = g_strdup_printf ("Failed to create a D-Bus proxy for unit \"%s\": %s",
                                 data->object_path, error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (message));
      g_error_free (error);
    }
  else
    {
      g_signal_connect (unit, "g-properties-changed",
                        G_CALLBACK (target_startup_monitor_unit_properties_changed),
                        data->monitor);

      data->monitor->units = g_list_append (data->monitor->units, unit);
    }

  /* free the get unit data */
  g_object_unref (data->monitor);
  g_free (data->object_path);
  g_slice_free (GetUnitData, data);
}



static void
target_startup_monitor_unit_properties_changed (GDBusProxy           *proxy,
                                                GVariant             *changed_properties,
                                                GStrv                *invalidated_properties,
                                                TargetStartupMonitor *monitor)
{
  SystemdUnit *unit = SYSTEMD_UNIT (proxy);
  const gchar *state;
  const gchar *unit_name;
  gpointer     node_state;
  gchar       *message;

  g_return_if_fail (IS_SYSTEMD_UNIT (unit));
  g_return_if_fail (changed_properties != NULL);
  g_return_if_fail (IS_TARGET_STARTUP_MONITOR (monitor));

  /* get the name of the unit */
  unit_name = systemd_unit_get_id (unit);

  message = g_strdup_printf ("Properties of unit \"%s\" changed", unit_name);
  DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (message));
  g_free (message);

  /* log the contents of the changed properties dict for debugging */
  message = g_variant_print (changed_properties, TRUE);
  DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (message));
  g_free (message);

  /* log the invalidated properties for debugging */
  message = g_strjoinv (", ", (gchar **)invalidated_properties);
  DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (message));
  g_free (message);

  /* read the new state from the changed properties */
  if (g_variant_lookup (changed_properties, "ActiveState", "%s", &state))
    {
      /* log the the active state has changed */
      message = g_strdup_printf ("Active state of unit \"%s\" changed to %s",
                                 unit_name, state);
      DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (message));
      g_free (message);

      /* check if the new state is active */
      if (g_strcmp0 (state, "active") == 0)
        {
          /* look up the node state corresponding to this unit, if there is one */
          if (g_hash_table_lookup_extended (monitor->targets_to_states, unit_name,
                                            NULL, &node_state))
            {
              /* we do have a state for this unit, so apply it now */
              target_startup_monitor_set_node_state (monitor, GPOINTER_TO_UINT (node_state));
            }
        }
    }
}



static void
target_startup_monitor_set_node_state (TargetStartupMonitor *monitor,
                                       NSMNodeState          state)
{
  g_return_if_fail (IS_TARGET_STARTUP_MONITOR (monitor));

  /* set node state in the Node State Manager */
  nsm_lifecycle_control_call_set_node_state (monitor->nsm_lifecycle_control,
                                             (gint) state, NULL,
                                             target_startup_monitor_set_node_state_finish,
                                             NULL);
}



static void
target_startup_monitor_set_node_state_finish (GObject      *object,
                                              GAsyncResult *res,
                                              gpointer      user_data)
{
  NSMLifecycleControl *nsm_lifecycle_control = NSM_LIFECYCLE_CONTROL (object);
  GError              *error = NULL;
  gchar               *log_text;
  NSMErrorStatus       error_code = NSM_ERROR_STATUS_OK;

  g_return_if_fail (IS_NSM_LIFECYCLE_CONTROL (nsm_lifecycle_control));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));

  /* finish setting the node state in the NSM */
  if (!nsm_lifecycle_control_call_set_node_state_finish (nsm_lifecycle_control,
                                                         (gint *) &error_code, res,
                                                         &error))
    {
      log_text = g_strdup_printf ("Failed to set the node state: %s", error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (error);
    }
  else if (error_code != NSM_ERROR_STATUS_OK)
    {
      log_text = g_strdup_printf ("Failed to set the node state: error code %d",
                                  error_code);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
    }
}



TargetStartupMonitor *
target_startup_monitor_new (SystemdManager *systemd_manager)
{
  g_return_val_if_fail (IS_SYSTEMD_MANAGER (systemd_manager), NULL);

  return g_object_new (TYPE_TARGET_STARTUP_MONITOR,
                       "systemd-manager", systemd_manager,
                       NULL);
}
