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

#include <boot-manager/job-manager.h>
#include <boot-manager/systemd-manager-dbus.h>



typedef struct _JobManagerJob JobManagerJob;



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_SYSTEMD_MANAGER,
};



static void           job_manager_constructed      (GObject           *object);
static void           job_manager_finalize         (GObject           *object);
static void           job_manager_get_property     (GObject           *object,
                                                    guint              prop_id,
                                                    GValue            *value,
                                                    GParamSpec        *pspec);
static void           job_manager_set_property     (GObject           *object,
                                                    guint              prop_id,
                                                    const GValue      *value,
                                                    GParamSpec        *pspec);
static void           job_manager_start_unit_reply (GObject           *object,
                                                    GAsyncResult      *result,
                                                    gpointer           user_data);
static void           job_manager_stop_unit_reply  (GObject           *object,
                                                    GAsyncResult      *result,
                                                    gpointer           user_data);
static void           job_manager_job_removed      (SystemdManager    *systemd_manager,
                                                    guint              id,
                                                    const gchar       *job_name,
                                                    const gchar       *unit,
                                                    const gchar       *result,
                                                    JobManager        *job_manager);
static JobManagerJob *job_manager_job_new          (JobManager        *manager,
                                                    const gchar       *unit,
                                                    GCancellable      *cancellable,
                                                    JobManagerCallback callback,
                                                    gpointer           user_data);
static void           job_manager_job_unref        (JobManagerJob     *job);
static void           job_manager_remember_job     (JobManager        *manager,
                                                    const gchar       *job_name,
                                                    JobManagerJob     *job);
static void           job_manager_forget_job       (JobManager        *manager,
                                                    const gchar       *job_name);



struct _JobManagerClass
{
  GObjectClass __parent__;
};

struct _JobManager
{
  GObject __parent__;

  GDBusConnection *connection;
  SystemdManager  *systemd_manager;

  GHashTable      *jobs;
};

struct _JobManagerJob
{
  JobManager        *manager;
  gchar             *unit;
  GCancellable      *cancellable;
  JobManagerCallback callback;
  gpointer           user_data;
};



G_DEFINE_TYPE (JobManager, job_manager, G_TYPE_OBJECT);



static void
job_manager_class_init (JobManagerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = job_manager_finalize;
  gobject_class->constructed = job_manager_constructed;
  gobject_class->get_property = job_manager_get_property;
  gobject_class->set_property = job_manager_set_property;

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
job_manager_init (JobManager *manager)
{
  /* create a mapping of systemd job names to job objects; we will use this
   * to remember jobs that we started */
  manager->jobs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                        (GDestroyNotify) job_manager_job_unref);

}



static void
job_manager_finalize (GObject *object)
{
  JobManager *manager = JOB_MANAGER (object);

  /* release all the jobs we have remembered */
  g_hash_table_unref (manager->jobs);

  /* release the D-Bus connection */
  g_object_unref (manager->connection);

  /* release the systemd manager */
  g_signal_handlers_disconnect_matched (manager->systemd_manager,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, manager);
  g_object_unref (manager->systemd_manager);

  /* chain up to finalize parent class */
  (*G_OBJECT_CLASS (job_manager_parent_class)->finalize) (object);
}



static void
job_manager_constructed (GObject *object)
{
  JobManager *manager = JOB_MANAGER (object);

  /* connect to systemd's "JobRemoved" signal so that we are notified
   * whenever a job is finished */
  g_signal_connect (manager->systemd_manager, "job-removed",
                    G_CALLBACK (job_manager_job_removed), manager);
}



static void
job_manager_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  JobManager *manager = JOB_MANAGER (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, manager->connection);
      break;
    case PROP_SYSTEMD_MANAGER:
      g_value_set_object (value, manager->systemd_manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
job_manager_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  JobManager *manager = JOB_MANAGER (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      manager->connection = g_value_dup_object (value);
      break;
    case PROP_SYSTEMD_MANAGER:
      manager->systemd_manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
job_manager_start_unit_reply (GObject       *object,
                              GAsyncResult *result,
                              gpointer      user_data)
{
  JobManagerJob *job = user_data;
  GError        *error = NULL;
  gchar         *job_name = NULL;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (user_data != NULL);

  /* finish the start unit call */
  if (!systemd_manager_call_start_unit_finish (job->manager->systemd_manager,
                                               &job_name, result, &error))
    {
      /* there was an error. notify the caller */
      job->callback (job->manager, job->unit, "failed", error, job->user_data);
      g_error_free (error);
      g_free (job_name);

      /* finish the job immediately */
      job_manager_job_unref (job);
    }
  else
    {
      /* remember the job so that we can finish it in the "job-removed" signal handler.
       * the service takes ownership of the job so we don't need to unref it here */
      job_manager_remember_job (job->manager, job_name, job);
    }
}



static void
job_manager_stop_unit_reply (GObject       *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  JobManagerJob *job = user_data;
  GError        *error = NULL;
  gchar         *job_name = NULL;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (user_data != NULL);

  /* finish the stop unit call */
  if (!systemd_manager_call_stop_unit_finish (job->manager->systemd_manager,
                                              &job_name, result, &error))
    {
      /* there was an error. notify the caller */
      job->callback (job->manager, job->unit, "failed", error, job->user_data);
      g_error_free (error);
      g_free (job_name);

      /* finish the job immediately */
      job_manager_job_unref (job);
    }
  else
    {
      /* remember the job so that we can finish it in the "job-removed" signal handler.
       * the service takes ownership of the job so we don't need to unref it here */
      job_manager_remember_job (job->manager, job_name, job);
    }
}



static void
job_manager_job_removed (SystemdManager *systemd_manager,
                         guint           id,
                         const gchar    *job_name,
                         const gchar    *unit,
                         const gchar    *result,
                         JobManager     *job_manager)
{
  JobManagerJob *job;

  g_return_if_fail (IS_SYSTEMD_MANAGER (systemd_manager));
  g_return_if_fail (job_name != NULL && *job_name != '\0');
  g_return_if_fail (result != NULL && *result != '\0');
  g_return_if_fail (unit != NULL && *unit != '\0');
  g_return_if_fail (IS_JOB_MANAGER (job_manager));

  /* look up the remembered job for this job name */
  job = g_hash_table_lookup (job_manager->jobs, job_name);

  /* if no job is found, ignore this job-removed signal */
  if (job == NULL)
    return;

  /* finish the job by notifying the caller */
  job->callback (job_manager, job->unit, result, NULL, job->user_data);

  /* forget about this job */
  job_manager_forget_job (job_manager, job_name);
}



static JobManagerJob *
job_manager_job_new (JobManager        *manager,
                     const gchar       *unit,
                     GCancellable      *cancellable,
                     JobManagerCallback callback,
                     gpointer           user_data)
{
  JobManagerJob *job;

  g_return_val_if_fail (IS_JOB_MANAGER (manager), NULL);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

  /* allocate a new job struct */
  job = g_slice_new0 (JobManagerJob);
  job->manager = g_object_ref (manager);
  job->unit = g_strdup(unit);
  if (cancellable != NULL)
    job->cancellable = g_object_ref (cancellable);
  job->callback = callback;
  job->user_data = user_data;

  return job;
}


static void
job_manager_job_unref (JobManagerJob *job)
{
  if (job == NULL)
    return;

  /* release all memory and references held by job */
  if (job->cancellable != NULL)
    g_object_unref (job->cancellable);
  g_free (job->unit);
  g_object_unref (job->manager);
  g_slice_free (JobManagerJob, job);
}



static void
job_manager_remember_job (JobManager    *manager,
                          const char    *job_name,
                          JobManagerJob *job)
{
  JobManagerJob *existing_job;

  g_return_if_fail (IS_JOB_MANAGER (manager));
  g_return_if_fail (job_name != NULL && *job_name != '\0');
  g_return_if_fail (job != NULL);

  /* if the job is already being remembered, there is a programming error that should be
   * notified */
  existing_job = g_hash_table_lookup (manager->jobs, job_name);
  if (existing_job != NULL)
    {
      g_critical ("Trying to remember the same job twice.");
      return;
    }
  /* associate the job name with the job */
  g_hash_table_insert (manager->jobs, g_strdup (job_name), job);
}



static void
job_manager_forget_job (JobManager  *manager,
                        const gchar *job_name)
{
  g_return_if_fail (IS_JOB_MANAGER (manager));
  g_return_if_fail (job_name != NULL && *job_name != '\0');

  g_hash_table_remove (manager->jobs, job_name);
}



JobManager *
job_manager_new (GDBusConnection *connection,
                 SystemdManager  *systemd_manager)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (IS_SYSTEMD_MANAGER (systemd_manager), NULL);

  return g_object_new (TYPE_JOB_MANAGER,
                       "connection", connection,
                       "systemd-manager", systemd_manager,
                       NULL);
}



void
job_manager_start (JobManager        *manager,
                   const gchar       *unit,
                   GCancellable      *cancellable,
                   JobManagerCallback callback,
                   gpointer           user_data)
{
  JobManagerJob *job;

  g_return_if_fail (IS_JOB_MANAGER (manager));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);

  /* create a new job object */
  job = job_manager_job_new (manager, unit, cancellable, callback, user_data);

  /* ask systemd to start the unit asynchronously */
  systemd_manager_call_start_unit (manager->systemd_manager, unit, "fail", cancellable,
                                   job_manager_start_unit_reply, job);
}



void
job_manager_stop (JobManager        *manager,
                  const gchar       *unit,
                  GCancellable      *cancellable,
                  JobManagerCallback callback,
                  gpointer           user_data)
{
  JobManagerJob *job;

  g_return_if_fail (IS_JOB_MANAGER (manager));
  g_return_if_fail (unit != NULL);
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (callback != NULL);

  /* create a new job object */
  job = job_manager_job_new (manager, unit, cancellable, callback, user_data);

  /* ask systemd to stop the unit asynchronously */
  systemd_manager_call_stop_unit (manager->systemd_manager, unit, "fail", cancellable,
                                  job_manager_stop_unit_reply, job);
}
