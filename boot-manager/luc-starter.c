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
#include <luc-handler/luc-handler-dbus.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_BOOT_MANAGER,
  PROP_LUC_HANDLER,
};



static void luc_starter_constructed  (GObject      *object);
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
  LUCHandler         *luc_handler;

  gchar             **prioritised_types;
};



G_DEFINE_TYPE (LUCStarter, luc_starter, G_TYPE_OBJECT);



static void
luc_starter_class_init (LUCStarterClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = luc_starter_constructed;
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

  g_object_class_install_property (gobject_class,
                                   PROP_LUC_HANDLER,
                                   g_param_spec_object ("luc-handler",
                                                        "luc-handler",
                                                        "luc-handler",
                                                        TYPE_LUC_HANDLER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
luc_starter_init (LUCStarter *starter)
{
}



static void
luc_starter_constructed (GObject *object)
{
  LUCStarter *starter = LUC_STARTER (object);

  /* parse the prioritised LUC types defined at build-time */
  starter->prioritised_types = g_strsplit (PRIORITISED_LUC_TYPES, ",", -1);
}



static void
luc_starter_finalize (GObject *object)
{
  LUCStarter *starter = LUC_STARTER (object);

  /* free the prioritised types array */
  g_strfreev (starter->prioritised_types);

  /* release the boot manager */
  g_object_unref (starter->boot_manager);

  /* release the LUC handler */
  g_object_unref (starter->luc_handler);

  (*G_OBJECT_CLASS (luc_starter_parent_class)->finalize) (object);
}



static void
luc_starter_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LUCStarter *starter = LUC_STARTER (object);

  switch (prop_id)
    {
    case PROP_BOOT_MANAGER:
      g_value_set_object (value, starter->boot_manager);
      break;
    case PROP_LUC_HANDLER:
      g_value_set_object (value, starter->luc_handler);
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
  LUCStarter *starter = LUC_STARTER (object);

  switch (prop_id)
    {
    case PROP_BOOT_MANAGER:
      starter->boot_manager = g_value_dup_object (value);
      break;
    case PROP_LUC_HANDLER:
      starter->luc_handler = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



LUCStarter *
luc_starter_new (BootManagerService *boot_manager,
                 LUCHandler         *luc_handler)
{
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (boot_manager), NULL);
  g_return_val_if_fail (IS_LUC_HANDLER (luc_handler), NULL);

  return g_object_new (TYPE_LUC_STARTER,
                       "boot-manager", boot_manager,
                       "luc-handler", luc_handler,
                       NULL);
}



void
luc_starter_start_groups (LUCStarter *starter)
{
  guint n;

  g_debug ("start LUC types in the following order:");

  for (n = 0; starter->prioritised_types[n] != NULL; n++)
    g_debug ("  %s",  starter->prioritised_types[n]);
}
