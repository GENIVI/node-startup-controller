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

#include <common/boot-manager-dbus.h>

#include <boot-manager/target-startup-monitor.h>



/* property identifier */
enum
{
  PROP_0,
  PROP_SYSTEMD_MANAGER,
};



static void target_startup_monitor_finalize     (GObject              *object);
static void target_startup_monitor_constructed  (GObject              *object);
static void target_startup_monitor_get_property (GObject              *object,
                                                 guint                 prop_id,
                                                 GValue               *value,
                                                 GParamSpec           *pspec);
static void target_startup_monitor_set_property (GObject              *object,
                                                 guint                 prop_id,
                                                 const GValue         *value,
                                                 GParamSpec           *pspec);
static void target_startup_monitor_job_removed  (SystemdManager       *manager,
                                                 guint                 id,
                                                 const gchar          *job_name,
                                                 const gchar          *result,
                                                 TargetStartupMonitor *monitor);



struct _TargetStartupMonitorClass
{
  GObjectClass __parent__;
};

struct _TargetStartupMonitor
{
  GObject         __parent__;

  SystemdManager *systemd_manager;

  /* map of targets to their NSM states */
  GHashTable     *watched_targets;
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
  /* create the table of targets and their NSM states */
  monitor->watched_targets = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (monitor->watched_targets, "focussed.target", "LUC_RUNNING");
  g_hash_table_insert (monitor->watched_targets, "unfocussed.target", "FULLY_RUNNING");
  g_hash_table_insert (monitor->watched_targets, "lazy.target", "FULLY_OPERATIONAL");
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

  /* release the watched_targets table */
  g_hash_table_destroy (monitor->watched_targets);

  /* release the systemd manager */
  g_signal_handlers_disconnect_matched (monitor->systemd_manager,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, monitor);
  g_object_unref (monitor->systemd_manager);

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
target_startup_monitor_job_removed (SystemdManager       *manager,
                                    guint                 id,
                                    const gchar          *job_name,
                                    const gchar          *result,
                                    TargetStartupMonitor *monitor)
{
  g_return_if_fail (IS_SYSTEMD_MANAGER (manager));
  g_return_if_fail (job_name != NULL && *job_name != '\0');
  g_return_if_fail (result != NULL && *result != '\0');
  g_return_if_fail (IS_TARGET_STARTUP_MONITOR (monitor));

  /* TODO: The current version of systemd does not return the unit's name.
   * later versions will, so finishing this handler is postponed until we
   * have a newer version of systemd */
}



TargetStartupMonitor *
target_startup_monitor_new (SystemdManager *systemd_manager)
{
  g_return_val_if_fail (IS_SYSTEMD_MANAGER (systemd_manager), NULL);

  return g_object_new (TYPE_TARGET_STARTUP_MONITOR,
                       "systemd-manager", systemd_manager,
                       NULL);
}
