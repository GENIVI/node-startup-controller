/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BOOT_MANAGER_APPLICATION_H__
#define __BOOT_MANAGER_APPLICATION_H__

#include <gio/gio.h>

#include <boot-manager/boot-manager-service.h>
#include <boot-manager/job-manager.h>
#include <boot-manager/la-handler-service.h>

G_BEGIN_DECLS

#define BOOT_MANAGER_TYPE_APPLICATION            (boot_manager_application_get_type ())
#define BOOT_MANAGER_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOOT_MANAGER_TYPE_APPLICATION, BootManagerApplication))
#define BOOT_MANAGER_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOOT_MANAGER_TYPE_APPLICATION, BootManagerApplicationClass))
#define BOOT_MANAGER_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOOT_MANAGER_TYPE_APPLICATION))
#define BOOT_MANAGER_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOOT_MANAGER_TYPE_APPLICATION)
#define BOOT_MANAGER_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOOT_MANAGER_TYPE_APPLICATION, BootManagerApplicationClass))

typedef struct _BootManagerApplicationClass BootManagerApplicationClass;
typedef struct _BootManagerApplication      BootManagerApplication;

GType		                boot_manager_application_get_type (void) G_GNUC_CONST;

BootManagerApplication *boot_manager_application_new      (GMainLoop          *main_loop,
                                                           GDBusConnection    *connection,
                                                           JobManager         *job_manager,
                                                           LAHandlerService   *la_handler,
                                                           BootManagerService *boot_manager_service) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__BOOT_MANAGER_APPLICATION_H__ */

