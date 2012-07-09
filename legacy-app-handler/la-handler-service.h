/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __LA_HANDLER_SERVICE_H__
#define __LA_HANDLER_SERVICE_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define LA_HANDLER_TYPE_SERVICE            (la_handler_service_get_type ())
#define LA_HANDLER_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LA_HANDLER_TYPE_SERVICE, LAHandlerService))
#define LA_HANDLER_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LA_HANDLER_TYPE_SERVICE, LAHandlerServiceClass))
#define LA_HANDLER_IS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LA_HANDLER_TYPE_SERVICE))
#define LA_HANDLER_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LA_HANDLER_TYPE_SERVICE)
#define LA_HANDLER_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LA_HANDLER_TYPE_SERVICE, LAHandlerServiceClass))

typedef struct _LAHandlerServiceClass LAHandlerServiceClass;
typedef struct _LAHandlerService      LAHandlerService;

GType             la_handler_service_get_type (void) G_GNUC_CONST;

LAHandlerService *la_handler_service_new      (GDBusConnection  *connection) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean          la_handler_service_start    (LAHandlerService *service,
                                               GError          **error);
void              la_handler_service_register (LAHandlerService *service,
                                               const gchar      *unit,
                                               const gchar      *mode,
                                               guint             timeout);

G_END_DECLS

#endif /* !__LA_HANDLER_SERVICE_H__ */

