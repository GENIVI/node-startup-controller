/* vi:set et ai sw=2 sts=2 ts=2: */
/* SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2012, GENIVI
 *
 * This file is part of node-startup-controller.
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 *
 * List of changes:
 * 2015-04-30, Jonathan Maw, List of changes started
 *
 */

#ifndef __NSM_LIFECYCLE_CONTROL_SERVICE_H__
#define __NSM_LIFECYCLE_CONTROL_SERVICE_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define NSM_LIFECYCLE_CONTROL_TYPE_SERVICE            (nsm_lifecycle_control_service_get_type ())
#define NSM_LIFECYCLE_CONTROL_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NSM_LIFECYCLE_CONTROL_TYPE_SERVICE, NSMLifecycleControlService))
#define NSM_LIFECYCLE_CONTROL_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NSM_LIFECYCLE_CONTROL_TYPE_SERVICE, NSMLifecycleControlServiceClass))
#define NSM_LIFECYCLE_CONTROL_IS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NSM_LIFECYCLE_CONTROL_TYPE_SERVICE))
#define NSM_LIFECYCLE_CONTROL_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NSM_LIFECYCLE_CONTROL_TYPE_SERVICE)
#define NSM_LIFECYCLE_CONTROL_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NSM_LIFECYCLE_CONTROL_TYPE_SERVICE, NSMLifecycleControlServiceClass))

typedef struct _NSMLifecycleControlServiceClass NSMLifecycleControlServiceClass;
typedef struct _NSMLifecycleControlService      NSMLifecycleControlService;

GType                       nsm_lifecycle_control_service_get_type (void) G_GNUC_CONST;

NSMLifecycleControlService *nsm_lifecycle_control_service_new      (GDBusConnection             *connection) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean                    nsm_lifecycle_control_service_start    (NSMLifecycleControlService  *service,
                                                                    GError                     **error);

G_END_DECLS

#endif /* !__NSM_LIFECYCLE_CONTROL_SERVICE_H__ */
