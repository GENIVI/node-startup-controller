/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __LA_HANDLER_APPLICATION_H__
#define __LA_HANDLER_APPLICATION_H__

#include <legacy-app-handler/la-handler-service.h>

G_BEGIN_DECLS

#define LA_HANDLER_TYPE_APPLICATION            (la_handler_application_get_type ())
#define LA_HANDLER_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LA_HANDLER_TYPE_APPLICATION, LAHandlerApplication))
#define LA_HANDLER_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LA_HANDLER_TYPE_APPLICATION, LAHandlerApplicationClass))
#define LA_HANDLER_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LA_HANDLER_TYPE_APPLICATION))
#define LA_HANDLER_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LA_HANDLER_TYPE_APPLICATION)
#define LA_HANDLER_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LA_HANDLER_TYPE_APPLICATION, LAHandlerApplicationClass))

typedef struct _LAHandlerApplicationClass LAHandlerApplicationClass;
typedef struct _LAHandlerApplication      LAHandlerApplication;

GType		       la_handler_application_get_type (void) G_GNUC_CONST;

LAHandlerApplication *la_handler_application_new      (LAHandlerService *service) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__LA_HANDLER_APPLICATION_H__ */

