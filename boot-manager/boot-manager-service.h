/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BOOT_MANAGER_SERVICE_H__
#define __BOOT_MANAGER_SERVICE_H__

#include <boot-manager/systemd-manager-dbus.h>

G_BEGIN_DECLS

#define BOOT_MANAGER_TYPE_SERVICE            (boot_manager_service_get_type ())
#define BOOT_MANAGER_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOOT_MANAGER_TYPE_SERVICE, BootManagerService))
#define BOOT_MANAGER_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOOT_MANAGER_TYPE_SERVICE, BootManagerServiceClass))
#define BOOT_MANAGER_IS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOOT_MANAGER_TYPE_SERVICE))
#define BOOT_MANAGER_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOOT_MANAGER_TYPE_SERVICE)
#define BOOT_MANAGER_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOOT_MANAGER_TYPE_SERVICE, BootManagerServiceClass))

typedef struct _BootManagerServiceClass BootManagerServiceClass;
typedef struct _BootManagerService      BootManagerService;

typedef void (*BootManagerServiceCallback) (BootManagerService *service,
                                            const gchar        *unit,
                                            const gchar        *result,
                                            GError             *error,
                                            gpointer            user_data);

GType               boot_manager_service_get_type (void) G_GNUC_CONST;

BootManagerService *boot_manager_service_new      (GDBusConnection           *connection,
                                                   SystemdManager            *systemd_manager) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean            boot_manager_service_start_up (BootManagerService        *service,
                                                   GError                   **error);
void                boot_manager_service_start    (BootManagerService        *service,
                                                   const gchar               *unit,
                                                   GCancellable              *cancellable,
                                                   BootManagerServiceCallback callback,
                                                   gpointer                   user_data);
void                boot_manager_service_stop     (BootManagerService        *service,
                                                   const gchar               *unit,
                                                   GCancellable              *cancellable,
                                                   BootManagerServiceCallback callback,
                                                   gpointer                   user_data);
void                boot_manager_service_kill     (BootManagerService        *service,
                                                   const gchar               *unit,
                                                   GCancellable              *cancellable,
                                                   BootManagerServiceCallback callback,
                                                   gpointer                   user_data);
void                boot_manager_service_restart  (BootManagerService        *service,
                                                   const gchar               *unit,
                                                   GCancellable              *cancellable,
                                                   BootManagerServiceCallback callback,
                                                   gpointer                   user_data);
void                boot_manager_service_isolate  (BootManagerService        *service,
                                                   const gchar               *unit,
                                                   GCancellable              *cancellable,
                                                   BootManagerServiceCallback callback,
                                                   gpointer                   user_data);

G_END_DECLS

#endif /* !__BOOT_MANAGER_SERVICE_H__ */

