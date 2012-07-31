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



DLT_IMPORT_CONTEXT (boot_manager_context);



/* property identifier */
enum
{
  PROP_0,
  PROP_SYSTEMD_MANAGER,
};



typedef struct _TargetStartupMonitorData TargetStartupMonitorData;



static void     target_startup_monitor_finalize                         (GObject              *object);
static void     target_startup_monitor_constructed                      (GObject              *object);
static void     target_startup_monitor_get_property                     (GObject              *object,
                                                                         guint                 prop_id,
                                                                         GValue               *value,
                                                                         GParamSpec           *pspec);
static void     target_startup_monitor_set_property                     (GObject              *object,
                                                                         guint                 prop_id,
                                                                         const GValue         *value,
                                                                         GParamSpec           *pspec);
static gboolean target_startup_monitor_job_removed                      (SystemdManager       *manager,
                                                                         guint                 id,
                                                                         const gchar          *job_name,
                                                                         const gchar          *unit,
                                                                         const gchar          *result,
                                                                         TargetStartupMonitor *monitor);
static void     target_startup_monitor_set_node_state                   (TargetStartupMonitor *monitor,
                                                                         NSMNodeState          state);
static void     target_startup_monitor_set_node_state_finish            (GObject              *object,
                                                                         GAsyncResult         *res,
                                                                         gpointer              user_data);
static void     target_startup_monitor_set_state_if_is_start_job        (TargetStartupMonitor *monitor,
                                                                         NSMNodeState          state,
                                                                         const gchar          *job_name);
static void     target_startup_monitor_set_state_if_is_start_job_finish (GObject              *object,
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

  /* map of systemd targets to corresponding node states */
  GHashTable          *watched_targets;
};

struct _TargetStartupMonitorData
{
  TargetStartupMonitor *monitor;
  NSMNodeState          state;
  gchar                *job_name;
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
  monitor->watched_targets = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (monitor->watched_targets, "focussed.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_LUC_RUNNING));
  g_hash_table_insert (monitor->watched_targets, "unfocussed.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_FULLY_RUNNING));
  g_hash_table_insert (monitor->watched_targets, "lazy.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_FULLY_OPERATIONAL));
}



static void
target_startup_monitor_constructed (GObject *object)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);

  /* connect to systemd's "JobRemoved" signal so that we are notified
   * whenever a job is finished */
  g_signal_connect (monitor->systemd_manager, "job-removed",
                    G_CALLBACK (target_startup_monitor_job_removed), monitor);
}



static void
target_startup_monitor_finalize (GObject *object)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);

  /* release the mapping of systemd targets to node states */
  g_hash_table_destroy (monitor->watched_targets);

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



static gboolean
target_startup_monitor_job_removed (SystemdManager       *manager,
                                    guint                 id,
                                    const gchar          *job_name,
                                    const gchar          *unit,
                                    const gchar          *result,
                                    TargetStartupMonitor *monitor)
{
  gpointer state;
  gchar   *message;

  g_return_val_if_fail (IS_SYSTEMD_MANAGER (manager), FALSE);
  g_return_val_if_fail (job_name != NULL && *job_name != '\0', FALSE);
  g_return_val_if_fail (unit != NULL && *unit != '\0', FALSE);
  g_return_val_if_fail (result != NULL && *result != '\0', FALSE);
  g_return_val_if_fail (IS_TARGET_STARTUP_MONITOR (monitor), FALSE);

  message = g_strdup_printf ("A systemd job was removed: "
                             "id %u, job name %s, unit %s, result %s",
                             id, job_name, unit, result);
  DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (message));
  g_free (message);

  /* we are only interested in successful JobRemoved signals */
  if (g_strcmp0 (result, "done") != 0)
    return TRUE;

  /* check if the unit corresponds to a node state */
  if (g_hash_table_lookup_extended (monitor->watched_targets, unit, NULL, &state))
    {
      /* apply the state only if the job is a start job */
      target_startup_monitor_set_state_if_is_start_job (monitor,
                                                        GPOINTER_TO_UINT (state),
                                                        job_name);
    }
  return TRUE;
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



static void
target_startup_monitor_set_state_if_is_start_job (TargetStartupMonitor *monitor,
                                                  NSMNodeState          state,
                                                  const gchar          *job_name)
{
  TargetStartupMonitorData *data;
  gchar                    *message;

  g_return_if_fail (IS_TARGET_STARTUP_MONITOR (monitor));
  g_return_if_fail (job_name != NULL && *job_name != '\0');

  data = g_slice_new0 (TargetStartupMonitorData);
  data->monitor = g_object_ref (monitor);
  data->state = state;
  data->job_name = g_strdup (job_name);

  message = g_strdup_printf ("Querying systemd jobs to see if %s is a start job",
                             job_name);
  DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (message));
  g_free (message);

  /* get the list of the jobs in the system */
  systemd_manager_call_list_jobs (monitor->systemd_manager, NULL,
                                  target_startup_monitor_set_state_if_is_start_job_finish,
                                  data);
}



static void
target_startup_monitor_set_state_if_is_start_job_finish (GObject      *object,
                                                         GAsyncResult *res,
                                                         gpointer      user_data)
{
  TargetStartupMonitorData *data = (TargetStartupMonitorData *)user_data;
  SystemdManager           *manager = SYSTEMD_MANAGER (object);
  GVariantIter              iter;
  const gchar              *job_type;
  const gchar              *job_name;
  GVariant                 *jobs = NULL;
  GError                   *error = NULL;
  gchar                    *log_text;

  g_return_if_fail (IS_SYSTEMD_MANAGER (manager));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (data != NULL);

  /* finish the call to list systemd jobs */
  if (!systemd_manager_call_list_jobs_finish (manager, &jobs, res, &error))
    {
      /* log the error */
      log_text = g_strdup_printf ("Failed to get the list of jobs from systemd: %s",
                                  error->message);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (error);

      /* log that we cannot change the state because we don't know
       * the type of the systemd job that was removed */
      log_text = g_strdup_printf ("Failed to determine type of job %s: "
                                  "will not change the node state to %u",
                                  data->job_name, data->state);
      DLT_LOG (boot_manager_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);

      /* free the callback data */
      g_object_unref (data->monitor);
      g_free (data->job_name);
      g_slice_free (TargetStartupMonitorData, data);
      
      /* we are done here */
      return;
    }

  /* iterate over the list of jobs */
  g_variant_iter_init (&iter, jobs);
  while (g_variant_iter_loop (&iter, "usssoo", NULL, NULL, &job_type, NULL,
                              &job_name, NULL))
    {
      log_text = g_strdup_printf ("Checking job %s, type %s", job_name, job_type);
      DLT_LOG (boot_manager_context, DLT_LOG_INFO, DLT_STRING (log_text));
      g_free (log_text);

      /* check if the job that was removed is in this list and is
       * a start job */
      if (g_strcmp0 (job_name, data->job_name) == 0
          && g_strcmp0 (job_type, "start") == 0)
        {
          /* it is, so set the state now */
          target_startup_monitor_set_node_state (data->monitor, data->state);
          break;
        }
    }

  /* release the variant for the array of jobs */
  g_variant_unref (jobs);

  /* free the callback data */
  g_object_unref (data->monitor);
  g_free (data->job_name);
  g_slice_free (TargetStartupMonitorData, data);
}



TargetStartupMonitor *
target_startup_monitor_new (SystemdManager *systemd_manager)
{
  g_return_val_if_fail (IS_SYSTEMD_MANAGER (systemd_manager), NULL);

  return g_object_new (TYPE_TARGET_STARTUP_MONITOR,
                       "systemd-manager", systemd_manager,
                       NULL);
}
