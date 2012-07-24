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

#include <common/nsm-enum-types.h>
#include <common/shutdown-client.h>
#include <common/shutdown-consumer-dbus.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_BUS_NAME,
  PROP_OBJECT_PATH,
  PROP_SHUTDOWN_MODE,
  PROP_TIMEOUT,
  PROP_CONSUMER,
};



static void     shutdown_client_finalize     (GObject      *object);
static void     shutdown_client_get_property (GObject      *object,
                                              guint         prop_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static void     shutdown_client_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);



struct _ShutdownClientClass
{
  GObjectClass __parent__;
};

struct _ShutdownClient
{
  GObject           __parent__;

  gchar            *bus_name;
  gchar            *object_path;
  NSMShutdownType   shutdown_mode;
  guint             timeout;

  ShutdownConsumer *consumer;
};



G_DEFINE_TYPE (ShutdownClient, shutdown_client, G_TYPE_OBJECT);



static void
shutdown_client_class_init (ShutdownClientClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = shutdown_client_finalize;
  gobject_class->get_property = shutdown_client_get_property;
  gobject_class->set_property = shutdown_client_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_BUS_NAME,
                                   g_param_spec_string ("bus-name",
                                                        "bus-name",
                                                        "bus-name",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        "object-path",
                                                        "object-path",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SHUTDOWN_MODE,
                                   g_param_spec_enum ("shutdown-mode",
                                                      "shutdown-mode",
                                                      "shutdown-mode",
                                                      TYPE_NSM_SHUTDOWN_TYPE,
                                                      NSM_SHUTDOWN_TYPE_NOT,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_TIMEOUT,
                                   g_param_spec_uint ("timeout",
                                                      "timeout",
                                                      "timeout",
                                                      0, G_MAXUINT, 120,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_CONSUMER,
                                   g_param_spec_object ("consumer",
                                                        "consumer",
                                                        "consumer",
                                                        TYPE_SHUTDOWN_CONSUMER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

}



static void
shutdown_client_init (ShutdownClient *client)
{
}



static void
shutdown_client_finalize (GObject *object)
{
  ShutdownClient *client = SHUTDOWN_CLIENT (object);

  /* free bus name and object path */
  g_free (client->bus_name);
  g_free (client->object_path);

  /* release the consumer, if we have one */
  if (client->consumer != NULL)
    g_object_unref (client->consumer);

  (*G_OBJECT_CLASS (shutdown_client_parent_class)->finalize) (object);
}



static void
shutdown_client_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ShutdownClient *client = SHUTDOWN_CLIENT (object);

  switch (prop_id)
    {
    case PROP_BUS_NAME:
      g_value_set_string (value, client->bus_name);
      break;
    case PROP_OBJECT_PATH:
      g_value_set_string (value, client->object_path);
      break;
    case PROP_SHUTDOWN_MODE:
      g_value_set_enum (value, client->shutdown_mode);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint (value, client->timeout);
      break;
    case PROP_CONSUMER:
      g_value_set_object (value, client->consumer);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
shutdown_client_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ShutdownClient *client = SHUTDOWN_CLIENT (object);

  switch (prop_id)
    {
    case PROP_BUS_NAME:
      client->bus_name = g_value_dup_string (value);
      break;
    case PROP_OBJECT_PATH:
      client->object_path = g_value_dup_string (value);
      break;
    case PROP_SHUTDOWN_MODE:
      client->shutdown_mode = g_value_get_enum (value);
      break;
    case PROP_TIMEOUT:
      client->timeout = g_value_get_uint (value);
      break;
    case PROP_CONSUMER:
      shutdown_client_set_consumer (client, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



ShutdownClient *
shutdown_client_new (const gchar    *bus_name,
                     const gchar    *object_path,
                     NSMShutdownType shutdown_mode,
                     guint           timeout)
{
  return g_object_new (TYPE_SHUTDOWN_CLIENT,
                       "bus-name", bus_name,
                       "object-path", object_path,
                       "shutdown-mode", shutdown_mode,
                       "timeout", timeout,
                       NULL);
}



const gchar *
shutdown_client_get_bus_name (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), NULL);
  return client->bus_name;
}



const gchar *
shutdown_client_get_object_path (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), NULL);
  return client->object_path;
}



NSMShutdownType
shutdown_client_get_shutdown_mode (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), NSM_SHUTDOWN_TYPE_NOT);
  return client->shutdown_mode;
}



guint
shutdown_client_get_timeout (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), 0);
  return client->timeout;
}



ShutdownConsumer *
shutdown_client_get_consumer (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), NULL);
  return client->consumer;
}



void
shutdown_client_set_consumer (ShutdownClient   *client,
                              ShutdownConsumer *consumer)
{
  g_return_if_fail (IS_SHUTDOWN_CLIENT (client));
  g_return_if_fail (consumer == NULL || IS_SHUTDOWN_CONSUMER (consumer));

  /* release the existing consumer if we have one */
  if (client->consumer != NULL)
    {
      /* also, if we are trying to reset the same consumer, ignore */
      if (client->consumer == consumer)
        return;

      /* release and reset the consumer member */
      g_object_unref (client->consumer);
      client->consumer = NULL;
    }

  /* grab a reference on the new consumer */
  if (consumer != NULL)
    client->consumer = g_object_ref (consumer);

  /* notify others that the property has changed */
  g_object_notify (G_OBJECT (client), "consumer");
}
