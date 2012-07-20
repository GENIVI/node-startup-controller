/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __NSM_CONSUMER_SERVICE_H__
#define __NSM_CONSUMER_SERVICE_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define NSM_CONSUMER_TYPE_SERVICE            (nsm_consumer_service_get_type ())
#define NSM_CONSUMER_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NSM_CONSUMER_TYPE_SERVICE, NSMConsumerService))
#define NSM_CONSUMER_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NSM_CONSUMER_TYPE_SERVICE, NSMConsumerServiceClass))
#define NSM_CONSUMER_IS_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NSM_CONSUMER_TYPE_SERVICE))
#define NSM_CONSUMER_IS_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NSM_CONSUMER_TYPE_SERVICE)
#define NSM_CONSUMER_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NSM_CONSUMER_TYPE_SERVICE, NSMConsumerServiceClass))

typedef struct _NSMConsumerServiceClass NSMConsumerServiceClass;
typedef struct _NSMConsumerService      NSMConsumerService;

GType               nsm_consumer_service_get_type           (void) G_GNUC_CONST;

NSMConsumerService *nsm_consumer_service_new                (GDBusConnection    *connection) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean            nsm_consumer_service_start              (NSMConsumerService *service,
                                                             GError            **error);
void                nsm_consumer_service_shutdown_consumers (NSMConsumerService *service);

G_END_DECLS

#endif /* !__NSM_CONSUMER_SERVICE_H__ */
