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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib-object.h>
#include <gio/gio.h>

#include <dlt/dlt.h>

#include <common/watchdog-client.h>

#include <legacy-app-handler/la-handler-dbus.h>
#include <legacy-app-handler/la-handler-application.h>
#include <legacy-app-handler/la-handler-service.h>



DLT_IMPORT_CONTEXT (la_handler_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_LA_HANDLER_SERVICE,
};



static void la_handler_application_finalize     (GObject                 *object);
static void la_handler_application_get_property (GObject                 *object,
                                                 guint                    prop_id,
                                                 GValue                  *value,
                                                 GParamSpec              *pspec);
static void la_handler_application_set_property (GObject                 *object,
                                                 guint                    prop_id,
                                                 const GValue            *value,
                                                 GParamSpec              *pspec);
static void la_handler_application_startup      (GApplication            *application);



struct _LAHandlerApplicationClass
{
  GApplicationClass __parent__;
};

struct _LAHandlerApplication
{
  GApplication       __parent__;

  /* systemd watchdog client that repeatedly asks systemd to update
   * the watchdog timestamp */
  WatchdogClient    *watchdog_client;

  /* service object that implements the Legacy App Handler D-Bus interface */
  LAHandlerService *service;
};



G_DEFINE_TYPE (LAHandlerApplication, la_handler_application, G_TYPE_APPLICATION);



static void
la_handler_application_class_init (LAHandlerApplicationClass *klass)
{
  GApplicationClass *gapplication_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = la_handler_application_finalize;
  gobject_class->get_property = la_handler_application_get_property;
  gobject_class->set_property = la_handler_application_set_property;

  gapplication_class = G_APPLICATION_CLASS (klass);
  gapplication_class->startup = la_handler_application_startup;

  g_object_class_install_property (gobject_class,
                                   PROP_LA_HANDLER_SERVICE,
                                   g_param_spec_object ("la-handler-service",
                                                        "la-handler-service",
                                                        "la-handler-service",
                                                        LA_HANDLER_TYPE_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
la_handler_application_init (LAHandlerApplication *application)
{
}



static void
la_handler_application_finalize (GObject *object)
{
  LAHandlerApplication *application = LA_HANDLER_APPLICATION (object);

  /* release the watchdog client */
  if (application->watchdog_client != NULL)
    g_object_unref (application->watchdog_client);

  /* release the Legacy App Handler service implementation */
  if (application->service != NULL)
    g_object_unref (application->service);

  (*G_OBJECT_CLASS (la_handler_application_parent_class)->finalize) (object);
}



static void
la_handler_application_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LAHandlerApplication *application = LA_HANDLER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_LA_HANDLER_SERVICE:
      g_value_set_object (value, application->service);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
la_handler_application_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LAHandlerApplication *application = LA_HANDLER_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_LA_HANDLER_SERVICE:
      application->service = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
la_handler_application_startup (GApplication *app)
{
  LAHandlerApplication *application = LA_HANDLER_APPLICATION (app);

  /* chain up to the parent class */
  (*G_APPLICATION_CLASS (la_handler_application_parent_class)->startup) (app);

  /* update systemd's watchdog timestamp every 120 seconds */
  application->watchdog_client = watchdog_client_new (120);

  /* the Legacy Application Handler should keep running until it is shut down by the Node
   * State Manager. */
  g_application_hold (app);
}



LAHandlerApplication *
la_handler_application_new (LAHandlerService *service,
                            GApplicationFlags flags)
{
  return g_object_new (LA_HANDLER_TYPE_APPLICATION,
                       "application-id", "org.genivi.LegacyAppHandler1",
                       "flags", flags,
                       "la-handler-service", service,
                       NULL);
}
