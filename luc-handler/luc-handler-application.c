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

#include <luc-handler/luc-handler-dbus.h>
#include <luc-handler/luc-handler-application.h>
#include <luc-handler/luc-handler-service.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_LUC_HANDLER_SERVICE,
};



static void luc_handler_application_finalize     (GObject      *object);
static void luc_handler_application_get_property (GObject      *object,
                                                  guint         prop_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void luc_handler_application_set_property (GObject      *object,
                                                  guint         prop_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void luc_handler_application_startup      (GApplication *application);



struct _LUCHandlerApplicationClass
{
  GApplicationClass __parent__;
};

struct _LUCHandlerApplication
{
  GApplication       __parent__;

  /* systemd watchdog client that repeatedly asks systemd to update
   * the watchdog timestamp */
  WatchdogClient    *watchdog_client;

  /* service object that implements the LUC Handler D-Bus interface */
  LUCHandlerService *service;
};



G_DEFINE_TYPE (LUCHandlerApplication, luc_handler_application, G_TYPE_APPLICATION);



static void
luc_handler_application_class_init (LUCHandlerApplicationClass *klass)
{
  GApplicationClass *gapplication_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = luc_handler_application_finalize;
  gobject_class->get_property = luc_handler_application_get_property;
  gobject_class->set_property = luc_handler_application_set_property;

  gapplication_class = G_APPLICATION_CLASS (klass);
  gapplication_class->startup = luc_handler_application_startup;

  g_object_class_install_property (gobject_class,
                                   PROP_LUC_HANDLER_SERVICE,
                                   g_param_spec_object ("luc-handler-service",
                                                        "luc-handler-service",
                                                        "luc-handler-service",
                                                        LUC_HANDLER_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
luc_handler_application_init (LUCHandlerApplication *application)
{
}



static void
luc_handler_application_finalize (GObject *object)
{
  LUCHandlerApplication *application = LUC_HANDLER_APPLICATION (object);

  /* release the watchdog client */
  g_object_unref (application->watchdog_client);

  /* release the LUC Handler service implementation */
  if (application->service != NULL)
    g_object_unref (application->service);

  (*G_OBJECT_CLASS (luc_handler_application_parent_class)->finalize) (object);
}



static void
luc_handler_application_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  LUCHandlerApplication *application = LUC_HANDLER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_LUC_HANDLER_SERVICE:
      g_value_set_object (value, application->service);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
luc_handler_application_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  LUCHandlerApplication *application = LUC_HANDLER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_LUC_HANDLER_SERVICE:
      application->service = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
luc_handler_application_startup (GApplication *app)
{
  LUCHandlerApplication *application = LUC_HANDLER_APPLICATION (app);

  /* update systemd's watchdog timestamp every 120 seconds */
  application->watchdog_client = watchdog_client_new (120);
}



LUCHandlerApplication *
luc_handler_application_new (LUCHandlerService *service)
{
  return g_object_new (LUC_HANDLER_TYPE_APPLICATION,
                       "application-id", "org.genivi.LUCHandler1",
                       "flags", G_APPLICATION_IS_SERVICE,
                       "luc-handler-service", service,
                       NULL);
}
