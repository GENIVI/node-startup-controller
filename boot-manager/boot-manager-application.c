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

#include <common/watchdog-client.h>

#include <boot-manager/boot-manager-dbus.h>
#include <boot-manager/boot-manager-application.h>
#include <boot-manager/boot-manager-service.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_BOOT_MANAGER_SERVICE,
};



static void boot_manager_application_finalize     (GObject      *object);
static void boot_manager_application_get_property (GObject      *object,
                                                   guint         prop_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);
static void boot_manager_application_set_property (GObject      *object,
                                                   guint         prop_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void boot_manager_application_startup      (GApplication *application);



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

  /* service object that implements the boot manager D-Bus interface */
  BootManagerService *service;
};



G_DEFINE_TYPE (BootManagerApplication, boot_manager_application, G_TYPE_APPLICATION);



static void
boot_manager_application_class_init (BootManagerApplicationClass *klass)
{
  GApplicationClass *gapplication_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = boot_manager_application_finalize;
  gobject_class->get_property = boot_manager_application_get_property;
  gobject_class->set_property = boot_manager_application_set_property;

  gapplication_class = G_APPLICATION_CLASS (klass);
  gapplication_class->startup = boot_manager_application_startup;

  g_object_class_install_property (gobject_class,
                                   PROP_BOOT_MANAGER_SERVICE,
                                   g_param_spec_object ("boot-manager-service",
                                                        "boot-manager-service",
                                                        "boot-manager-service",
                                                        BOOT_MANAGER_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}



static void
boot_manager_application_init (BootManagerApplication *application)
{
  /* update systemd's watchdog timestamp every 120 seconds */
  application->watchdog_client = watchdog_client_new (120);
}



static void
boot_manager_application_finalize (GObject *object)
{
  BootManagerApplication *application = BOOT_MANAGER_APPLICATION (object);

  /* release the watchdog client */
  g_object_unref (application->watchdog_client);

  /* release the boot manager service implementation */
  if (application->service != NULL)
    g_object_unref (application->service);

  (*G_OBJECT_CLASS (boot_manager_application_parent_class)->finalize) (object);
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
    case PROP_BOOT_MANAGER_SERVICE:
      g_value_set_object (value, application->service);
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
    case PROP_BOOT_MANAGER_SERVICE:
      application->service = g_value_dup_object (value);
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
}



BootManagerApplication *
boot_manager_application_new (BootManagerService *service)
{
  return g_object_new (BOOT_MANAGER_TYPE_APPLICATION,
                       "application-id", "org.genivi.BootManager1",
                       "flags", G_APPLICATION_IS_SERVICE,
                       "boot-manager-service", service,
                       NULL);
}
