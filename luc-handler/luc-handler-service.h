/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __LUC_HANDLER_SERVICE_H__
#define __LUC_HANDLER_SERVICE_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define LUC_HANDLER_TYPE_SERVICE            (luc_handler_service_get_type ())
#define LUC_HANDLER_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LUC_HANDLER_TYPE_SERVICE, LUCHandlerService))
#define LUC_HANDLER_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LUC_HANDLER_TYPE_SERVICE, LUCHandlerServiceClass))
#define LUC_HANDLER_IS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LUC_HANDLER_TYPE_SERVICE))
#define LUC_HANDLER_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LUC_HANDLER_TYPE_SERVICE)
#define LUC_HANDLER_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LUC_HANDLER_TYPE_SERVICE, LUCHandlerServiceClass))

typedef struct _LUCHandlerServiceClass LUCHandlerServiceClass;
typedef struct _LUCHandlerService      LUCHandlerService;

GType              luc_handler_service_get_type (void) G_GNUC_CONST;

LUCHandlerService *luc_handler_service_new      (GDBusConnection   *connection) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean           luc_handler_service_start    (LUCHandlerService *service,
                                                 GError           **error);

G_END_DECLS

#endif /* !__LUC_HANDLER_SERVICE_H__ */

