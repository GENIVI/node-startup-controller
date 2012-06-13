/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __LUC_HANDLER_APPLICATION_H__
#define __LUC_HANDLER_APPLICATION_H__

#include <luc-handler/luc-handler-service.h>

G_BEGIN_DECLS

#define LUC_HANDLER_TYPE_APPLICATION            (luc_handler_application_get_type ())
#define LUC_HANDLER_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LUC_HANDLER_TYPE_APPLICATION, LUCHandlerApplication))
#define LUC_HANDLER_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LUC_HANDLER_TYPE_APPLICATION, LUCHandlerApplicationClass))
#define LUC_HANDLER_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LUC_HANDLER_TYPE_APPLICATION))
#define LUC_HANDLER_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LUC_HANDLER_TYPE_APPLICATION)
#define LUC_HANDLER_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LUC_HANDLER_TYPE_APPLICATION, LUCHandlerApplicationClass))

typedef struct _LUCHandlerApplicationClass LUCHandlerApplicationClass;
typedef struct _LUCHandlerApplication      LUCHandlerApplication;

GType		       luc_handler_application_get_type (void) G_GNUC_CONST;

LUCHandlerApplication *luc_handler_application_new      (LUCHandlerService *service) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__LUC_HANDLER_APPLICATION_H__ */

