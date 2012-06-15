/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <gio/gio.h>

#include <boot-manager/boot-manager-service.h>
#include <boot-manager/luc-starter.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_BOOT_MANAGER,
};



static void luc_starter_finalize     (GObject      *object);
static void luc_starter_get_property (GObject      *object,
                                      guint         prop_id,
                                      GValue       *value,
                                      GParamSpec   *pspec);
static void luc_starter_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);



struct _LUCStarterClass
{
  GObjectClass __parent__;
};

struct _LUCStarter
{
  GObject             __parent__;

  BootManagerService *boot_manager;
};



G_DEFINE_TYPE (LUCStarter, luc_starter, G_TYPE_OBJECT);



static void
luc_starter_class_init (LUCStarterClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = luc_starter_finalize;
  gobject_class->get_property = luc_starter_get_property;
  gobject_class->set_property = luc_starter_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_BOOT_MANAGER,
                                   g_param_spec_object ("boot-manager",
                                                        "boot-manager",
                                                        "boot-manager",
                                                        BOOT_MANAGER_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
luc_starter_init (LUCStarter *service)
{
}



static void
luc_starter_finalize (GObject *object)
{
  LUCStarter *service = LUC_STARTER (object);

  /* release the boot manager */
  g_object_unref (service->boot_manager);

  (*G_OBJECT_CLASS (luc_starter_parent_class)->finalize) (object);
}



static void
luc_starter_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LUCStarter *service = LUC_STARTER (object);

  switch (prop_id)
    {
    case PROP_BOOT_MANAGER:
      g_value_set_object (value, service->boot_manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
luc_starter_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LUCStarter *service = LUC_STARTER (object);

  switch (prop_id)
    {
    case PROP_BOOT_MANAGER:
      service->boot_manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



LUCStarter *
luc_starter_new (BootManagerService *boot_manager)
{
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (boot_manager), NULL);
  return g_object_new (TYPE_LUC_STARTER, "boot-manager", boot_manager, NULL);
}



void
luc_starter_start_groups (LUCStarter *starter)
{
  g_debug ("restore LUC if desired");
}
