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

#ifndef __WATCHDOG_CLIENT_H__
#define __WATCHDOG_CLIENT_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define TYPE_WATCHDOG_CLIENT            (watchdog_client_get_type ())
#define WATCHDOG_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_WATCHDOG_CLIENT, WatchdogClient))
#define WATCHDOG_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_WATCHDOG_CLIENT, WatchdogClientClass))
#define IS_WATCHDOG_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_WATCHDOG_CLIENT))
#define IS_WATCHDOG_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_WATCHDOG_CLIENT))
#define WATCHDOG_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_WATCHDOG_CLIENT, WatchdogClientClass))

typedef struct _WatchdogClientClass WatchdogClientClass;
typedef struct _WatchdogClient      WatchdogClient;

GType           watchdog_client_get_type (void) G_GNUC_CONST;

WatchdogClient *watchdog_client_new      (guint timeout) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__WATCHDOG_CLIENT_H__ */

