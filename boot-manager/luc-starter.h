/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __LUC_STARTER_H__
#define __LUC_STARTER_H__

#include <boot-manager/boot-manager-service.h>
#include <luc-handler/luc-handler-dbus.h>

G_BEGIN_DECLS

#define TYPE_LUC_STARTER            (luc_starter_get_type ())
#define LUC_STARTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_LUC_STARTER, LUCStarter))
#define LUC_STARTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_LUC_STARTER, LUCStarterClass))
#define IS_LUC_STARTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_LUC_STARTER))
#define IS_LUC_STARTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_LUC_STARTER)
#define LUC_STARTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_LUC_STARTER, LUCStarterClass))

typedef struct _LUCStarterClass LUCStarterClass;
typedef struct _LUCStarter      LUCStarter;

GType       luc_starter_get_type     (void) G_GNUC_CONST;

LUCStarter *luc_starter_new          (BootManagerService *boot_manager,
                                      LUCHandler         *luc_handler) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void        luc_starter_start_groups (LUCStarter         *starter);

G_END_DECLS

#endif /* !__LUC_STARTER_H__ */

