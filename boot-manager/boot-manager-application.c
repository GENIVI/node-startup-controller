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
#include <glib-unix.h>

#include <common/boot-manager-dbus.h>
#include <common/watchdog-client.h>
#include <luc-handler/luc-handler-dbus.h>

#include <boot-manager/boot-manager-application.h>
#include <boot-manager/boot-manager-service.h>
#include <boot-manager/luc-starter.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_BOOT_MANAGER,
  PROP_LUC_HANDLER,
  PROP_LUC_STARTER,
};



static void     boot_manager_application_finalize     (GObject      *object);
static void     boot_manager_application_constructed  (GObject      *object);
static void     boot_manager_application_get_property (GObject      *object,
                                                       guint         prop_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static void     boot_manager_application_set_property (GObject      *object,
                                                       guint         prop_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void     boot_manager_application_startup      (GApplication *application);
static gboolean boot_manager_application_int_handler  (GApplication *application);



struct _BootManagerApplicationClass
{
  GApplicationClass __parent__;
};

struct _BootManagerApplication
{
  GApplication       __parent__;

  /* systemd watchdog client that repeatedly asks systemd to update
   * the watchdog timestamp */
  WatchdogClient    *watchdog_client;

  /* implementation of the boot manager D-Bus interface */
  BootManagerService *boot_manager;

  /* LUC starter to restore the LUC */
  LUCHandler         *luc_handler;
  LUCStarter         *luc_starter;

  /* signal handler IDs */
  guint               sigint_id;
};



G_DEFINE_TYPE (BootManagerApplication, boot_manager_application, G_TYPE_APPLICATION);



static void
boot_manager_application_class_init (BootManagerApplicationClass *klass)
{
  GApplicationClass *gapplication_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = boot_manager_application_finalize;
  gobject_class->constructed = boot_manager_application_constructed;
  gobject_class->get_property = boot_manager_application_get_property;
  gobject_class->set_property = boot_manager_application_set_property;

  gapplication_class = G_APPLICATION_CLASS (klass);
  gapplication_class->startup = boot_manager_application_startup;

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

  g_object_class_install_property (gobject_class,
                                   PROP_LUC_STARTER,
                                   g_param_spec_object ("luc-starter",
                                                        "luc-starter",
                                                        "luc-starter",
                                                        TYPE_LUC_STARTER,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
boot_manager_application_init (BootManagerApplication *application)
{
  /* update systemd's watchdog timestamp every 120 seconds */
  application->watchdog_client = watchdog_client_new (120);

  /* install the signal handler */
  application->sigint_id =
    g_unix_signal_add (SIGINT, (GSourceFunc) boot_manager_application_int_handler,
                       application);
}



static void
boot_manager_application_finalize (GObject *object)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  /* release the signal handler */
  g_source_remove (application->sigint_id);

  /* release the watchdog client */
  g_object_unref (application->watchdog_client);

  /* release the LUC handler/starter */
  g_object_unref (application->luc_handler);
  g_object_unref (application->luc_starter);

  /* release the boot manager implementation */
  if (application->boot_manager != NULL)
    g_object_unref (application->boot_manager);

  (*G_OBJECT_CLASS (boot_manager_application_parent_class)->finalize) (object);
}



static void
boot_manager_application_constructed (GObject *object)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  /* instantiate the LUC starter */
  application->luc_starter = luc_starter_new (application->boot_manager,
                                              application->luc_handler);
}



static void
boot_manager_application_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_BOOT_MANAGER:
      g_value_set_object (value, application->boot_manager);
      break;
    case PROP_LUC_HANDLER:
      g_value_set_object (value, application->luc_handler);
      break;
    case PROP_LUC_STARTER:
      g_value_set_object (value, application->luc_starter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
boot_manager_application_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_BOOT_MANAGER:
      application->boot_manager = g_value_dup_object (value);
      break;
    case PROP_LUC_HANDLER:
      application->luc_handler = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
boot_manager_application_startup (GApplication *app)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (app);

  /* chain up to the parent class */
  (*G_APPLICATION_CLASS (boot_manager_application_parent_class)->startup) (app);

  /* restore the LUC if desired */
  luc_starter_start_groups (application->luc_starter);
}



static gboolean
boot_manager_application_int_handler (GApplication *app)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (app);

  luc_starter_cancel (application->luc_starter);
  boot_manager_service_cancel (application->boot_manager);

  return TRUE;
}



BootManagerApplication *
boot_manager_application_new (BootManagerService *boot_manager,
                              LUCHandler         *luc_handler)
{
  g_return_val_if_fail (BOOT_MANAGER_IS_SERVICE (boot_manager), NULL);
  g_return_val_if_fail (IS_LUC_HANDLER (luc_handler), NULL);

  return g_object_new (BOOT_MANAGER_TYPE_APPLICATION,
                       "application-id", "org.genivi.BootManager1",
                       "flags", G_APPLICATION_IS_SERVICE,
                       "boot-manager", boot_manager,
                       "luc-handler", luc_handler,
                       NULL);
}
