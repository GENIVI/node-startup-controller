/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SHUTDOWN_CONSUMER_SERVICE_H__
#define __SHUTDOWN_CONSUMER_SERVICE_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define TYPE_SHUTDOWN_CONSUMER_SERVICE            (shutdown_consumer_service_get_type())
#define SHUTDOWN_CONSUMER_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SHUTDOWN_CONSUMER_SERVICE, ShutdownConsumerService))
#define SHUTDOWN_CONSUMER_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SHUTDOWN_CONSUMER_SERVICE, ShutdownConsumerServiceClass))
#define IS_SHUTDOWN_CONSUMER_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SHUTDOWN_CONSUMER_SERVICE))
#define IS_SHUTDOWN_CONSUMER_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SHUTDOWN_CONSUMER_SERVICE))
#define SHUTDOWN_CONSUMER_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SHUTDOWN_CONSUMER_SERVICE, ShutdownConsumerServiceClass))

typedef struct _ShutdownConsumerServiceClass ShutdownConsumerServiceClass;
typedef struct _ShutdownConsumerService      ShutdownConsumerService;

GType                    shutdown_consumer_service_get_type        (void) G_GNUC_CONST;

const gchar             *shutdown_consumer_service_get_object_path (ShutdownConsumerService *service);
const gchar             *shutdown_consumer_service_get_unit_name   (ShutdownConsumerService *service);
ShutdownConsumerService *shutdown_consumer_service_new             (GDBusConnection         *connection,
                                                                    const gchar             *object_path,
                                                                    const gchar             *unit_name) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean                 shutdown_consumer_service_start           (ShutdownConsumerService *service,
                                                                    GError                 **error);

#endif /* !__SHUTDOWN_CONSUMER_SERVICE_H__ */
