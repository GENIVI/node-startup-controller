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

#ifndef __NODE_STARTUP_CONTROLLER_SERVICE_H__
#define __NODE_STARTUP_CONTROLLER_SERVICE_H__

#include <gio/gio.h>

#include <node-startup-controller/systemd-manager-dbus.h>

G_BEGIN_DECLS

#define TYPE_NODE_STARTUP_CONTROLLER_SERVICE            (node_startup_controller_service_get_type ())
#define NODE_STARTUP_CONTROLLER_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_NODE_STARTUP_CONTROLLER_SERVICE, NodeStartupControllerService))
#define NODE_STARTUP_CONTROLLER_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_NODE_STARTUP_CONTROLLER_SERVICE, NodeStartupControllerServiceClass))
#define IS_NODE_STARTUP_CONTROLLER_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_NODE_STARTUP_CONTROLLER_SERVICE))
#define IS_NODE_STARTUP_CONTROLLER_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_NODE_STARTUP_CONTROLLER_SERVICE))
#define NODE_STARTUP_CONTROLLER_SERVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_NODE_STARTUP_CONTROLLER_SERVICE, NodeStartupControllerServiceClass))

typedef struct _NodeStartupControllerServiceClass NodeStartupControllerServiceClass;
typedef struct _NodeStartupControllerService      NodeStartupControllerService;



GType                         node_startup_controller_service_get_type  (void) G_GNUC_CONST;

NodeStartupControllerService *node_startup_controller_service_new       (GDBusConnection              *connection) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
gboolean                      node_startup_controller_service_start_up  (NodeStartupControllerService *service,
                                                                         GError                      **error);
GVariant                     *node_startup_controller_service_read_luc  (NodeStartupControllerService *service,
                                                                         GError                      **error);
void                          node_startup_controller_service_write_luc (NodeStartupControllerService *service,
                                                                         GError                      **error);


G_END_DECLS

#endif /* !__NODE_STARTUP_CONTROLLER_SERVICE_H__ */

