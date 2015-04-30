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

#ifndef __NODE_STARTUP_CONTROLLER_APPLICATION_H__
#define __NODE_STARTUP_CONTROLLER_APPLICATION_H__

#include <gio/gio.h>

#include <node-startup-controller/node-startup-controller-service.h>
#include <node-startup-controller/job-manager.h>
#include <node-startup-controller/la-handler-service.h>

G_BEGIN_DECLS

#define TYPE_NODE_STARTUP_CONTROLLER_APPLICATION            (node_startup_controller_application_get_type ())
#define NODE_STARTUP_CONTROLLER_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_NODE_STARTUP_CONTROLLER_APPLICATION, NodeStartupControllerApplication))
#define NODE_STARTUP_CONTROLLER_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_NODE_STARTUP_CONTROLLER_APPLICATION, NodeStartupControllerApplicationClass))
#define IS_NODE_STARTUP_CONTROLLER_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_NODE_STARTUP_CONTROLLER_APPLICATION))
#define IS_NODE_STARTUP_CONTROLLER_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_NODE_STARTUP_CONTROLLER_APPLICATION))
#define NODE_STARTUP_CONTROLLER_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_NODE_STARTUP_CONTROLLER_APPLICATION, NodeStartupControllerApplicationClass))

typedef struct _NodeStartupControllerApplicationClass NodeStartupControllerApplicationClass;
typedef struct _NodeStartupControllerApplication      NodeStartupControllerApplication;

GType		                          node_startup_controller_application_get_type (void) G_GNUC_CONST;

NodeStartupControllerApplication *node_startup_controller_application_new      (GMainLoop                    *main_loop,
                                                                                GDBusConnection              *connection,
                                                                                JobManager                   *job_manager,
                                                                                LAHandlerService             *la_handler,
                                                                                NodeStartupControllerService *node_startup_controller) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__NODE_STARTUP_CONTROLLER_APPLICATION_H__ */

