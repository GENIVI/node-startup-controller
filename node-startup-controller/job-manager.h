/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __JOB_MANAGER_H__
#define __JOB_MANAGER_H__

#include <node-startup-controller/systemd-manager-dbus.h>

G_BEGIN_DECLS

#define TYPE_JOB_MANAGER            (job_manager_get_type())
#define JOB_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_JOB_MANAGER, JobManager))
#define JOB_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_JOB_MANAGER, JobManagerClass))
#define IS_JOB_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_JOB_MANAGER))
#define IS_JOB_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass, TYPE_JOB_MANAGER))
#define JOB_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj, TYPE_JOB_MANAGER, JobManagerClass))

typedef struct _JobManagerClass JobManagerClass;
typedef struct _JobManager      JobManager;

/**
 * JobManagerCallback:
 * @manager:   The #JobManager object.
 * @unit:      The name of the systemd unit to be started or stopped.
 * @result:    The result of trying to start or stop the unit. Usually %success or %failed.
 * @error:     The error (if any) raised by the start or stop method. %NULL if none 
 *             occurred.
 * @user_data: The user_data passed into the start or stop methods.
 * 
 * The JobManagerCallback is called when job_manager_start() or job_manager_stop()
 * finishes. 
 */
typedef void (*JobManagerCallback) (JobManager  *manager,
                                    const gchar *unit,
                                    const gchar *result,
                                    GError      *error,
                                    gpointer     user_data);

GType       job_manager_get_type (void) G_GNUC_CONST;
JobManager *job_manager_new      (GDBusConnection   *connection,
                                  SystemdManager    *systemd_manager) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void        job_manager_start    (JobManager        *manager,
                                  const gchar       *unit,
                                  GCancellable      *cancellable,
                                  JobManagerCallback callback,
                                  gpointer           user_data);
void        job_manager_stop     (JobManager        *manager,
                                  const gchar       *unit,
                                  GCancellable      *cancellable,
                                  JobManagerCallback callback,
                                  gpointer           user_data);

G_END_DECLS

#endif /* !__JOB_MANAGER_H__ */
