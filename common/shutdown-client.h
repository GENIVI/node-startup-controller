/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SHUTDOWN_CLIENT_H__
#define __SHUTDOWN_CLIENT_H__

#include <common/nsm-enum-types.h>
#include <common/shutdown-consumer-dbus.h>

G_BEGIN_DECLS

#define TYPE_SHUTDOWN_CLIENT            (shutdown_client_get_type ())
#define SHUTDOWN_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SHUTDOWN_CLIENT, ShutdownClient))
#define SHUTDOWN_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SHUTDOWN_CLIENT, ShutdownClientClass))
#define IS_SHUTDOWN_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SHUTDOWN_CLIENT))
#define IS_SHUTDOWN_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SHUTDOWN_CLIENT)
#define SHUTDOWN_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SHUTDOWN_CLIENT, ShutdownClientClass))

typedef struct _ShutdownClientClass ShutdownClientClass;
typedef struct _ShutdownClient      ShutdownClient;

GType             shutdown_client_get_type          (void) G_GNUC_CONST;

ShutdownClient   *shutdown_client_new               (const gchar      *bus_name,
                                                     const gchar      *object_path,
                                                     NSMShutdownType   shutdown_mode,
                                                     guint             timeout) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

const gchar      *shutdown_client_get_bus_name      (ShutdownClient   *client);
const gchar      *shutdown_client_get_object_path   (ShutdownClient   *client);
NSMShutdownType   shutdown_client_get_shutdown_mode (ShutdownClient   *client);
guint             shutdown_client_get_timeout       (ShutdownClient   *client);
ShutdownConsumer *shutdown_client_get_consumer      (ShutdownClient   *client);
void              shutdown_client_set_consumer      (ShutdownClient   *client,
                                                     ShutdownConsumer *consumer);

G_END_DECLS

#endif /* !__SHUTDOWN_CLIENT_H__ */

