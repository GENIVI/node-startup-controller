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

#include <boot-manager/boot-manager-service.h>



typedef struct _BootManagerServiceJob BootManagerServiceJob;



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_SYSTEMD_MANAGER,
};



static void                   boot_manager_service_finalize              (GObject                       *object);
static void                   boot_manager_service_constructed           (GObject                       *object);
static void                   boot_manager_service_get_property          (GObject                       *object,
                                                                          guint                          prop_id,
                                                                          GValue                        *value,
                                                                          GParamSpec                    *pspec);
static void                   boot_manager_service_set_property          (GObject                       *object,
                                                                          guint                          prop_id,
                                                                          const GValue                  *value,
                                                                          GParamSpec                    *pspec);
static gboolean               boot_manager_service_handle_start          (BootManager                   *interface,
                                                                          GDBusMethodInvocation         *invocation,
                                                                          const gchar                   *unit,
                                                                          BootManagerService            *service);
static void                   boot_manager_service_handle_start_finish   (BootManagerService            *service,
                                                                          const gchar                   *unit,
                                                                          const gchar                   *result,
                                                                          GError                        *error,
                                                                          gpointer                       user_data);
static void                   boot_manager_service_start_unit_reply      (GObject                       *object,
                                                                          GAsyncResult                  *result,
                                                                          gpointer                       user_data);
static gboolean               boot_manager_service_handle_stop           (BootManager                   *interface,
                                                                          GDBusMethodInvocation         *invocation,
                                                                          const gchar                   *unit,
                                                                          BootManagerService            *service);
static void                   boot_manager_service_handle_stop_finish    (BootManagerService            *service,
                                                                          const gchar                   *unit,
                                                                          const gchar                   *result,
                                                                          GError                        *error,
                                                                          gpointer                       user_data);
static void                   boot_manager_service_stop_unit_reply       (GObject                       *object,
                                                                          GAsyncResult                  *result,
                                                                          gpointer                       user_data);
static gboolean               boot_manager_service_handle_kill           (BootManager                   *interface,
                                                                          GDBusMethodInvocation         *invocation,
                                                                          const gchar                   *unit,
                                                                          BootManagerService            *service);
static void                   boot_manager_service_handle_kill_finish    (BootManagerService            *service,
                                                                          const gchar                   *unit,
                                                                          const gchar                   *result,
                                                                          GError                        *error,
                                                                          gpointer                       user_data);
static void                   boot_manager_service_kill_unit_reply       (GObject                       *object,
                                                                          GAsyncResult                  *result,
                                                                          gpointer                       user_data);
static gboolean               boot_manager_service_handle_restart        (BootManager                   *interface,
                                                                          GDBusMethodInvocation         *invocation,
                                                                          const gchar                   *unit,
                                                                          BootManagerService            *service);
static void                   boot_manager_service_handle_restart_finish (BootManagerService            *service,
                                                                          const gchar                   *unit,
                                                                          const gchar                   *result,
                                                                          GError                        *error,
                                                                          gpointer                       user_data);
static void                   boot_manager_service_restart_unit_reply    (GObject                       *object,
                                                                          GAsyncResult                  *result,
                                                                          gpointer                       user_data);
static gboolean               boot_manager_service_handle_isolate        (BootManager                   *interface,
                                                                          GDBusMethodInvocation         *invocation,
                                                                          const gchar                   *unit,
                                                                          BootManagerService            *service);
static void                   boot_manager_service_handle_isolate_finish (BootManagerService            *service,
                                                                          const gchar                   *unit,
                                                                          const gchar                   *result,
                                                                          GError                        *error,
                                                                          gpointer                       user_data);
static void                   boot_manager_service_isolate_unit_reply    (GObject                       *object,
                                                                          GAsyncResult                  *result,
                                                                          gpointer                       user_data);
static gboolean               boot_manager_service_handle_list           (BootManager                   *interface,
                                                                          GDBusMethodInvocation         *invocation,
                                                                          BootManagerService            *service);
static void                   boot_manager_service_handle_list_finish    (BootManagerService            *service,
                                                                          const gchar *const            *result,
                                                                          GError                        *error,
                                                                          gpointer                       user_data);
static void                   boot_manager_service_list_units_reply      (GObject                       *object,
                                                                          GAsyncResult                  *result,
                                                                          gpointer                       user_data);
static void                   boot_manager_service_job_removed           (SystemdManager                *manager,
                                                                          guint                          id,
                                                                          const gchar                   *job_name,
                                                                          const gchar                   *result,
                                                                          BootManagerService            *service);
static BootManagerServiceJob *boot_manager_service_job_new               (BootManagerService            *service,
                                                                          const gchar                   *unit,
                                                                          GCancellable                  *cancellable,
                                                                          BootManagerServiceCallback     callback,
                                                                          BootManagerServiceListCallback list_callback,
                                                                          gpointer                       user_data);
static void                   boot_manager_service_job_unref             (BootManagerServiceJob         *job);
static void                   boot_manager_service_remember_job          (BootManagerService            *service,
                                                                          const gchar                   *job_name,
                                                                          BootManagerServiceJob         *job);
static void                   boot_manager_service_forget_job            (BootManagerService            *service,
                                                                          const gchar                   *job_name);
static void                   boot_manager_service_cancel_task           (gpointer                       key,
                                                                          GCancellable                  *cancellable,
                                                                          gpointer                       user_data);



struct _BootManagerServiceClass
{
  GObjectClass __parent__;
};

struct _BootManagerService
{
  GObject          __parent__;

  GDBusConnection *connection;
  BootManager     *interface;
  SystemdManager  *systemd_manager;

  GHashTable      *jobs;
  GHashTable      *cancellables;
};

struct _BootManagerServiceJob
{
  BootManagerService            *service;
  gchar                         *unit;
  GCancellable                  *cancellable;
  BootManagerServiceCallback     callback;
  BootManagerServiceListCallback list_callback;
  gpointer                       user_data;
  gchar                         *name;
};



G_DEFINE_TYPE (BootManagerService, boot_manager_service, G_TYPE_OBJECT);



static void
boot_manager_service_class_init (BootManagerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = boot_manager_service_finalize;
  gobject_class->constructed = boot_manager_service_constructed;
  gobject_class->get_property = boot_manager_service_get_property;
  gobject_class->set_property = boot_manager_service_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

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
boot_manager_service_init (BootManagerService *service)
{
  /* create a mapping of systemd job names to job objects; we will use this
   * to remember jobs that we started */
  service->jobs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                         (GDestroyNotify) boot_manager_service_job_unref);

  service->interface = boot_manager_skeleton_new ();

  /* implement the Start() method handler */
  g_signal_connect (service->interface, "handle-start",
                    G_CALLBACK (boot_manager_service_handle_start), service);

  /* implement the Stop() method handler */
  g_signal_connect (service->interface, "handle-stop",
                    G_CALLBACK (boot_manager_service_handle_stop), service);

  /* implement the Kill() method handler */
  g_signal_connect (service->interface, "handle-kill",
                    G_CALLBACK (boot_manager_service_handle_kill), service);
  /* implement the Restart() method handler */
  g_signal_connect (service->interface, "handle-restart",
                    G_CALLBACK (boot_manager_service_handle_restart), service);
  /* implement the Isolate() method handler */
  g_signal_connect (service->interface, "handle-isolate",
                    G_CALLBACK (boot_manager_service_handle_isolate), service);
  /* implement the List() method handler */
  g_signal_connect (service->interface, "handle-list",
                    G_CALLBACK (boot_manager_service_handle_list), service);

  /* create a mapping of method calls to units and cancellables; we will use this to 
   * cancel jobs that are in the middle of being started */
  service->cancellables =
    g_hash_table_new_full (g_direct_hash, g_direct_equal, (GDestroyNotify) g_object_unref,
                           (GDestroyNotify) g_object_unref);

}



static void
boot_manager_service_finalize (GObject *object)
{
  BootManagerService *service = BOOT_MANAGER_SERVICE (object);

  /* release all the jobs we have remembered */
  g_hash_table_unref (service->jobs);

  /* release the D-Bus connection object */
  g_object_unref (service->connection);

  /* release the systemd manager */
  g_signal_handlers_disconnect_matched (service->systemd_manager,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->systemd_manager);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  /* release the cancellables hash table */
  g_hash_table_unref (service->cancellables);

  (*G_OBJECT_CLASS (boot_manager_service_parent_class)->finalize) (object);
}



static void
boot_manager_service_constructed (GObject *object)
{
  BootManagerService *service = BOOT_MANAGER_SERVICE (object);

  /* connect to systemd's "JobRemoved" signal so that we are notified
   * whenever a job is finished */
  g_signal_connect (service->systemd_manager, "job-removed",
                    G_CALLBACK (boot_manager_service_job_removed), service);
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
    case PROP_SYSTEMD_MANAGER:
      g_value_set_object (value, service->systemd_manager);
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
    case PROP_SYSTEMD_MANAGER:
      service->systemd_manager = g_value_dup_object (value);
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
  GCancellable *cancellable;

  g_return_val_if_fail (IS_BOOT_MANAGER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (unit != NULL, FALSE);
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);

  /* create a new cancellable so that we can cancel this start call */
  cancellable = g_cancellable_new ();

  /* store the cancellable in the cancellables GHashTable */
  g_hash_table_insert (service->cancellables, g_object_ref (invocation), cancellable);

  /* ask systemd to start the unit for us, send a D-Bus reply in the finish callback */
  boot_manager_service_start (service, unit, cancellable,
                              boot_manager_service_handle_start_finish, invocation);

  return TRUE;
}



static void
boot_manager_service_handle_start_finish (BootManagerService *service,
                                          const gchar        *unit,
                                          const gchar        *result,
                                          GError             *error,
                                          gpointer            user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (user_data);

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));

  /* log any potential errors */
  if (error != NULL)
    g_warning ("there was an error: %s", error->message);

  /* remove the cancellable associated with this invocation now the job is finished */
  g_hash_table_remove (service->cancellables, invocation);
  g_object_unref (invocation);

  /* report the result back to the boot manager client */
  boot_manager_complete_start (service->interface, invocation, result);
}



static void
boot_manager_service_start_unit_reply (GObject      *object,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  BootManagerServiceJob *job = user_data;
  GError                *error = NULL;
  gchar                 *job_name = NULL;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (user_data != NULL);

  /* finish the start unit call */
  if (!systemd_manager_call_start_unit_finish (job->service->systemd_manager,
                                               &job_name, result, &error))
    {
      /* there was an error; let the caller know */
      job->callback (job->service, job->unit, "failed", error, job->user_data);
      g_error_free (error);
      g_free (job_name);

      /* finish the job immediately */
      boot_manager_service_job_unref (job);
    }
  else
    {
      /* remember the job so that we can finish it in the "job-removed" signal
       * handler. the service takes ownership of the job so we don't need to
       * unref it here */
      boot_manager_service_remember_job (job->service, job_name, job);
    }
}



static gboolean
boot_manager_service_handle_stop (BootManager           *interface,
                                  GDBusMethodInvocation *invocation,
                                  const gchar           *unit,
                                  BootManagerService    *service)
{
  GCancellable *cancellable;

  g_return_val_if_fail (IS_BOOT_MANAGER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (unit != NULL, FALSE);
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);

  /* create a new cancellable so that we can cancel this stop call */
  cancellable = g_cancellable_new ();

  /* store the cancellable in the cancellables GHashTable */
  g_hash_table_insert (service->cancellables, g_object_ref (invocation), cancellable);

  /* ask systemd to stop the unit for us, send a D-Bus reply in the finish callback */
  boot_manager_service_stop (service, unit, cancellable,
                             boot_manager_service_handle_stop_finish, invocation);

  return TRUE;
}



static void
boot_manager_service_handle_stop_finish (BootManagerService *service,
                                         const gchar        *unit,
                                         const gchar        *result,
                                         GError             *error,
                                         gpointer            user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (user_data);

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));

  /* log any potential errors */
  if (error != NULL)
    g_warning ("there was an error: %s", error->message);

  /* remove the cancellable associated with this invocation now the job is finished */
  g_hash_table_remove (service->cancellables, invocation);
  g_object_unref (invocation);

  /* report the result back to the boot manager client */
  boot_manager_complete_stop (service->interface, invocation, result);
}



static void
boot_manager_service_stop_unit_reply (GObject      *object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  BootManagerServiceJob *job = user_data;
  GError                *error = NULL;
  gchar                 *job_name = NULL;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (user_data != NULL);

  /* finish the stop unit call */
  if (!systemd_manager_call_stop_unit_finish (job->service->systemd_manager,
                                              &job_name, result, &error))
    {
      /* there was an error; let the caller know */
      job->callback (job->service, job->unit, "failed", error, job->user_data);
      g_error_free (error);
      g_free (job_name);

      /* finish the job immediately */
      boot_manager_service_job_unref (job);
    }
  else
    {
      /* remember the job so that we can finish it in the "job-removed" signal
       * handler. the service takes ownership of the job so we don't need to
       * unref it here */
      boot_manager_service_remember_job (job->service, job_name, job);
    }
}



static gboolean
boot_manager_service_handle_kill (BootManager           *interface,
                                  GDBusMethodInvocation *invocation,
                                  const gchar           *unit,
                                  BootManagerService    *service)
{
  GCancellable *cancellable;

  g_return_val_if_fail (IS_BOOT_MANAGER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (unit != NULL, FALSE);
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);

  /* create a new cancellable so that we can cancel this kill call */
  cancellable = g_cancellable_new ();

  /* store the cancellable in the cancellables GHashTable */
  g_hash_table_insert (service->cancellables, g_object_ref (invocation), cancellable);

  /* ask systemd to kill the unit, send a D-Bus reply in the finish callback */
  boot_manager_service_kill (service, unit, cancellable,
                             boot_manager_service_handle_kill_finish, invocation);

  return TRUE;
}



static void
boot_manager_service_handle_kill_finish (BootManagerService *service,
                                         const gchar        *unit,
                                         const gchar        *result,
                                         GError             *error,
                                         gpointer            user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (user_data);

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit!= NULL);
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));

  /* log any potential errors */
  if (error != NULL)
    g_warning ("there was an error: %s", error->message);

  /* remove the cancellable associated with this invocation now the job is finished */
  g_hash_table_remove (service->cancellables, invocation);
  g_object_unref (invocation);

  /* report the result back to the boot manager client */
  boot_manager_complete_kill (service->interface, invocation, result);
}



static void
boot_manager_service_kill_unit_reply (GObject      *object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  BootManagerServiceJob *job = user_data;
  GError                *error = NULL;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (user_data != NULL);

  /* finish the kill unit call */
  systemd_manager_call_kill_unit_finish (job->service->systemd_manager,
                                         result, &error);

  /* got a reply from systemd; let the caller know we're done */
  job->callback (job->service, job->unit, (error == NULL) ? "done" : "failed", error,
                 job->user_data);
  if (error != NULL)
    g_error_free (error);

  /* finish the job */
  boot_manager_service_job_unref (job);
}



static gboolean
boot_manager_service_handle_restart (BootManager           *interface,
                                     GDBusMethodInvocation *invocation,
                                     const gchar           *unit,
                                     BootManagerService    *service)
{
  GCancellable *cancellable;

  g_return_val_if_fail (IS_BOOT_MANAGER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (unit != NULL, FALSE);
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);

  /* create a new cancellable so that we can cancel this restart call */
  cancellable = g_cancellable_new ();

  /* store the cancellable in the cancellables GHashTable */
  g_hash_table_insert (service->cancellables, g_object_ref (invocation), cancellable);

  /* ask systemd to restart the unit for us, send a D-Bus reply in the finish callback */
  boot_manager_service_restart (service, unit, cancellable,
                                boot_manager_service_handle_restart_finish, invocation);

  return TRUE;
}



static void
boot_manager_service_handle_restart_finish (BootManagerService *service,
                                            const gchar        *unit,
                                            const gchar        *result,
                                            GError             *error,
                                            gpointer            user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (user_data);

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));

  /* log any potential errors */
  if (error != NULL)
    g_warning ("there was an error: %s", error->message);

  /* remove the cancellable associated with this invocation now the job is finished */
  g_hash_table_remove (service->cancellables, invocation);
  g_object_unref (invocation);

  /* report the result back to the boot manager client */
  boot_manager_complete_restart (service->interface, invocation, result);
}



static void
boot_manager_service_restart_unit_reply (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
  BootManagerServiceJob *job = user_data;
  GError                *error = NULL;
  gchar                 *job_name = NULL;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (user_data != NULL);

  /* finish the restart unit call */
  if (!systemd_manager_call_restart_unit_finish (job->service->systemd_manager,
                                                 &job_name, result, &error))
    {
      /* there was an error; let the caller know */
      job->callback (job->service, job->unit, "failed", error, job->user_data);
      g_error_free (error);
      g_free (job_name);

      /* finish the job immediately */
      boot_manager_service_job_unref (job);
    }
  else
    {
      /* remember the job so that we can finish it in the "job-removed" signal
       * handler. the service takes ownership of the job so we don't need to
       * unref it here */
      boot_manager_service_remember_job (job->service, job_name, job);
    }
}



static gboolean
boot_manager_service_handle_isolate (BootManager           *interface,
                                     GDBusMethodInvocation *invocation,
                                     const gchar           *unit,
                                     BootManagerService    *service)
{
  GCancellable *cancellable;

  g_return_val_if_fail (IS_BOOT_MANAGER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (unit != NULL, FALSE);
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);

  /* create a new cancellable so that we can cancel this isolate call */
  cancellable = g_cancellable_new ();

  /* store the cancellable in the cancellables GHashTable */
  g_hash_table_insert (service->cancellables, g_object_ref (invocation), cancellable);

  /* ask systemd to isolate the unit for us, send a D-Bus reply in the finish callback */
  boot_manager_service_isolate (service, unit, cancellable,
                                boot_manager_service_handle_isolate_finish, invocation);

  return TRUE;
}



static void
boot_manager_service_handle_isolate_finish (BootManagerService *service,
                                            const gchar        *unit,
                                            const gchar        *result,
                                            GError             *error,
                                            gpointer            user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (user_data);

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));

  /* log any potential errors */
  if (error != NULL)
    g_warning ("there was an error: %s", error->message);

  /* remove the cancellable associated with this invocation now the job is finished */
  g_hash_table_remove (service->cancellables, invocation);
  g_object_unref (invocation);

  /* report the result back to the boot manager client */
  boot_manager_complete_isolate (service->interface, invocation, result);
}



static void
boot_manager_service_isolate_unit_reply (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
  BootManagerServiceJob *job = user_data;
  GError                *error = NULL;
  gchar                 *job_name = NULL;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (user_data != NULL);

  /* finish the isolate unit call */
  if (!systemd_manager_call_start_unit_finish (job->service->systemd_manager,
                                               &job_name, result, &error))
    {
      /* there was an error; let the caller know */
      job->callback (job->service, job->unit, "failed", error, job->user_data);
      g_error_free (error);
      g_free (job_name);

      /* finish the job immediately */
      boot_manager_service_job_unref (job);
    }
  else
    {
      /* remember the job so that we can finish it in the "job-removed" signal
       * handler. the service takes ownership of the job so we don't need to
       * unref it here */
      boot_manager_service_remember_job (job->service, job_name, job);
    }
}



static gboolean
boot_manager_service_handle_list (BootManager           *interface,
                                  GDBusMethodInvocation *invocation,
                                  BootManagerService    *service)
{
  GCancellable *cancellable;

  g_return_val_if_fail (IS_BOOT_MANAGER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);

  /* create a new cancellable so that we can cancel this list call */
  cancellable = g_cancellable_new ();

  /* store the cancellable in the cancellables GHashTable */
  g_hash_table_insert (service->cancellables, g_object_ref (invocation), cancellable);

  /* ask systemd to list all its units, send a D-Bus reply in the finish callback */
  boot_manager_service_list (service, cancellable, boot_manager_service_handle_list_finish,
                             invocation);

  return TRUE;
}



static void
boot_manager_service_handle_list_finish (BootManagerService *service,
                                         const gchar *const *result,
                                         GError             *error,
                                         gpointer            user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (user_data);

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (result != NULL);
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));

  /* log any potential errors */
  if (error != NULL)
    g_warning ("there was an error: %s", error->message);

  /* remove this cancellable now the job has finished */
  g_hash_table_remove (service->cancellables, invocation);

  /* report the result back to the boot manager client */
  boot_manager_complete_list (service->interface, invocation, result);
}



static void
boot_manager_service_list_units_reply (GObject      *object,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  BootManagerServiceJob *job = user_data;
  GVariantIter           iter;
  GVariant              *units = NULL;
  GError                *error = NULL;
  gchar                **output = NULL;
  guint                  n;
  guint                  i;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (user_data != NULL);

  /* finish the list units call */
  if (!systemd_manager_call_list_units_finish (job->service->systemd_manager, &units,
                                               result, &error))
    {
      /* there was an error; build the result as failure response */
      output = g_malloc (sizeof (gchar *));
      output[0] = NULL;
    }
  else
    {
      /* it was successful; build the result from the GVariant units */
      n = g_variant_n_children (units);
      i = 0;

      /* output is a null-terminated strv */
      output = g_malloc (sizeof (gchar *) * (n + 1));

      g_variant_iter_init (&iter, units);

      /* fill output with the first element of every list entry */
      while (g_variant_iter_next (&iter, "(ssssssouso)", &output[i], NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL, NULL, NULL))
        {
          i++;
        }
      /* null-terminate output */
      output[i] = NULL;
    }

  /* let the caller know the result */
  job->list_callback (job->service, (const gchar *const *)output, error, job->user_data);

  /* clean up */
  if (error != NULL)
    g_error_free (error);

  g_strfreev (output);
  boot_manager_service_job_unref (job);
}



static void
boot_manager_service_job_removed (SystemdManager     *manager,
                                  guint               id,
                                  const gchar        *job_name,
                                  const gchar        *result,
                                  BootManagerService *service)
{
  BootManagerServiceJob *job;

  g_return_if_fail (IS_SYSTEMD_MANAGER (manager));
  g_return_if_fail (job_name != NULL && *job_name != '\0');
  g_return_if_fail (result != NULL && *result != '\0');
  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));

  /* lookup the rememebred job for this job name */
  job = g_hash_table_lookup (service->jobs, job_name);

  /* if there is no such job, just ignore this job-removed signal */
  if (job == NULL)
    return;

  /* finish the job by notifying the caller */
  job->callback (service, job->unit, result, NULL, job->user_data);

  /* forget about this job; this will unref the job */
  boot_manager_service_forget_job (service, job_name);
}



static BootManagerServiceJob *
boot_manager_service_job_new (BootManagerService            *service,
                              const gchar                   *unit,
                              GCancellable                  *cancellable,
                              BootManagerServiceCallback     callback,
                              BootManagerServiceListCallback list_callback,
                              gpointer                       user_data)
{
  BootManagerServiceJob *job;

  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), NULL);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

  /* allocate a new job struct */
  job = g_slice_new0 (BootManagerServiceJob);
  job->service = g_object_ref (service);
  job->unit = g_strdup (unit);
  if (cancellable != NULL)
    job->cancellable = g_object_ref (cancellable);
  job->callback = callback;
  job->list_callback = list_callback;
  job->user_data = user_data;

  return job;
}



static void
boot_manager_service_job_unref (BootManagerServiceJob *job)
{
  if (job == NULL)
    return;

  /* release all memory and references held by the job */
  if (job->cancellable != NULL)
    g_object_unref (job->cancellable);
  g_free (job->unit);
  g_object_unref (job->service);
  g_slice_free (BootManagerServiceJob, job);
}



static void
boot_manager_service_remember_job (BootManagerService    *service,
                                   const gchar           *job_name,
                                   BootManagerServiceJob *job)
{
  BootManagerServiceJob *existing_job;

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (job_name != NULL && *job_name != '\0');
  g_return_if_fail (job != NULL);

  /* if the job is already being remembered, then there is a programming
   * mistake and we should make people aware of it */
  existing_job = g_hash_table_lookup (service->jobs, job_name);
  if (existing_job != NULL)
    {
      g_critical ("trying to remember the same job twice");
      return;
    }

  /* associate the job name with the job */
  g_hash_table_insert (service->jobs, g_strdup (job_name), job);
}



static void
boot_manager_service_forget_job (BootManagerService *service,
                                 const gchar        *job_name)
{
  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (job_name != NULL && *job_name != '\0');

  g_hash_table_remove (service->jobs, job_name);
}



static void
boot_manager_service_cancel_task (gpointer      key,
                                  GCancellable *cancellable,
                                  gpointer      user_data)
{
  g_cancellable_cancel (cancellable);
}



BootManagerService *
boot_manager_service_new (GDBusConnection *connection,
                          SystemdManager  *systemd_manager)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (IS_SYSTEMD_MANAGER (systemd_manager), NULL);

  return g_object_new (BOOT_MANAGER_TYPE_SERVICE,
                       "connection", connection,
                       "systemd-manager", systemd_manager,
                       NULL);
}



gboolean
boot_manager_service_start_up (BootManagerService *service,
                               GError            **error)
{
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the org.genivi.BootManager1 service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/org/genivi/BootManager1",
                                           error);
}



void
boot_manager_service_start (BootManagerService        *service,
                            const gchar               *unit,
                            GCancellable              *cancellable,
                            BootManagerServiceCallback callback,
                            gpointer                   user_data)
{
  BootManagerServiceJob *job;

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);

  /* create a new job object */
  job = boot_manager_service_job_new (service, unit, cancellable, callback, NULL,
                                      user_data);

  /* ask systemd to start the unit asynchronously */
  systemd_manager_call_start_unit (service->systemd_manager, unit, "fail", cancellable,
                                   boot_manager_service_start_unit_reply, job);
}



void
boot_manager_service_stop (BootManagerService        *service,
                           const gchar               *unit,
                           GCancellable              *cancellable,
                           BootManagerServiceCallback callback,
                           gpointer                   user_data)
{
  BootManagerServiceJob *job;

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);

  /* create a new job object */
  job = boot_manager_service_job_new (service, unit, cancellable, callback, NULL,
                                      user_data);

  /* ask systemd to stop the unit asynchronously */
  systemd_manager_call_stop_unit (service->systemd_manager, unit, "fail", cancellable,
                                  boot_manager_service_stop_unit_reply, job);
}



void
boot_manager_service_kill (BootManagerService        *service,
                           const gchar               *unit,
                           GCancellable              *cancellable,
                           BootManagerServiceCallback callback,
                           gpointer                   user_data)
{
  BootManagerServiceJob *job;

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);

  /* create a new job object */
  job = boot_manager_service_job_new (service, unit, cancellable, callback, NULL,
                                      user_data);

  /* ask systemd to stop the unit asynchronously */
  systemd_manager_call_kill_unit (service->systemd_manager, unit, "all", "control-group",
                                  SIGKILL, cancellable,
                                  boot_manager_service_kill_unit_reply, job);
}



void
boot_manager_service_restart (BootManagerService        *service,
                              const gchar               *unit,
                              GCancellable              *cancellable,
                              BootManagerServiceCallback callback,
                              gpointer                   user_data)
{
  BootManagerServiceJob *job;

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);

  /* create a new job object */
  job = boot_manager_service_job_new (service, unit, cancellable, callback, NULL,
                                      user_data);

  /* ask systemd to restart the unit asynchronously */
  systemd_manager_call_restart_unit (service->systemd_manager, unit, "fail", cancellable,
                                     boot_manager_service_restart_unit_reply, job);
}



void
boot_manager_service_isolate (BootManagerService        *service,
                              const gchar               *unit,
                              GCancellable              *cancellable,
                              BootManagerServiceCallback callback,
                              gpointer                   user_data)
{
  BootManagerServiceJob *job;

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);

  /* create a new job object */
  job = boot_manager_service_job_new (service, unit, cancellable, callback, NULL,
                                      user_data);

  /* ask systemd to isolate the unit asynchronously */
  systemd_manager_call_start_unit (service->systemd_manager, unit, "isolate",
                                   cancellable, boot_manager_service_isolate_unit_reply,
                                   job);
}



void
boot_manager_service_list (BootManagerService            *service,
                           GCancellable                  *cancellable,
                           BootManagerServiceListCallback list_callback,
                           gpointer                       user_data)
{
  BootManagerServiceJob *job;

  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (list_callback != NULL);

  /* create a new job object */
  job = boot_manager_service_job_new (service, NULL, cancellable, NULL, list_callback,
                                      user_data);

  /* ask systemd to list units asynchronously */
  systemd_manager_call_list_units (service->systemd_manager, cancellable,
                                   boot_manager_service_list_units_reply, job);
}

void
boot_manager_service_cancel (BootManagerService *service)
{
  g_return_if_fail (BOOT_MANAGER_IS_SERVICE (service));

  /* cancel all listed cancellables */
  g_hash_table_foreach (service->cancellables,
                        (GHFunc) boot_manager_service_cancel_task, NULL);
}
