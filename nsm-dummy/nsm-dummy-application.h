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

#ifndef __NSM_DUMMY_APPLICATION_H__
#define __NSM_DUMMY_APPLICATION_H__

#include <nsm-dummy/nsm-consumer-service.h>
#include <nsm-dummy/nsm-lifecycle-control-service.h>

G_BEGIN_DECLS

#define NSM_DUMMY_TYPE_APPLICATION            (nsm_dummy_application_get_type ())
#define NSM_DUMMY_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NSM_DUMMY_TYPE_APPLICATION, NSMDummyApplication))
#define NSM_DUMMY_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NSM_DUMMY_TYPE_APPLICATION, NSMDummyApplicationClass))
#define NSM_DUMMY_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NSM_DUMMY_TYPE_APPLICATION))
#define NSM_DUMMY_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NSM_DUMMY_TYPE_APPLICATION)
#define NSM_DUMMY_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NSM_DUMMY_TYPE_APPLICATION, NSMDummyApplicationClass))

typedef struct _NSMDummyApplicationClass NSMDummyApplicationClass;
typedef struct _NSMDummyApplication      NSMDummyApplication;

GType                nsm_dummy_application_get_type (void) G_GNUC_CONST;

NSMDummyApplication *nsm_dummy_application_new      (GMainLoop                  *main_loop,
                                                     GDBusConnection            *connection,
                                                     NSMConsumerService         *consumer_service,
                                                     NSMLifecycleControlService *lifecycle_control_service) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__NSM_DUMMY_APPLICATION_H__ */
