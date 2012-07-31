/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __TARGET_STARTUP_MONITOR_H__
#define __TARGET_STARTUP_MONITOR_H__

#include <node-startup-controller/systemd-manager-dbus.h>

G_BEGIN_DECLS

#define TYPE_TARGET_STARTUP_MONITOR            (target_startup_monitor_get_type ())
#define TARGET_STARTUP_MONITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_TARGET_STARTUP_MONITOR, TargetStartupMonitor))
#define TARGET_STARTUP_MONITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_TARGET_STARTUP_MONITOR, TargetStartupMonitorClass))
#define IS_TARGET_STARTUP_MONITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_TARGET_STARTUP_MONITOR))
#define IS_TARGET_STARTUP_MONITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_TARGET_STARTUP_MONITOR))
#define TARGET_STARTUP_MONITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_TARGET_STARTUP_MONITOR, TargetStartupMonitorClass))

typedef struct _TargetStartupMonitor      TargetStartupMonitor;
typedef struct _TargetStartupMonitorClass TargetStartupMonitorClass;

GType                 target_startup_monitor_get_type (void) G_GNUC_CONST;
TargetStartupMonitor *target_startup_monitor_new      (SystemdManager *systemd_manager) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__TARGET_STARTUP_MONITOR_H__ */
