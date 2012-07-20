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

#include <dlt/dlt.h>

#include <common/nsm-consumer-dbus.h>
#include <common/shutdown-consumer-dbus.h>

#include <nsm-dummy/nsm-consumer-service.h>


DLT_IMPORT_CONTEXT (nsm_dummy_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



typedef struct _NSMShutdownClient       NSMShutdownClient;



static void     nsm_consumer_service_finalize                          (GObject               *object);
static void     nsm_consumer_service_get_property                      (GObject               *object,
                                                                        guint                  prop_id,
                                                                        GValue                *value,
                                                                        GParamSpec            *pspec);
static void     nsm_consumer_service_set_property                      (GObject               *object,
                                                                        guint                  prop_id,
                                                                        const GValue          *value,
                                                                        GParamSpec            *pspec);
static gboolean nsm_consumer_service_handle_register_shutdown_client   (NSMConsumer           *object,
                                                                        GDBusMethodInvocation *invocation,
                                                                        const gchar           *bus_name,
                                                                        const gchar           *object_name,
                                                                        gint                   shutdown_mode,
                                                                        guint                  timeout_ms,
                                                                        NSMConsumerService    *service);
static gboolean nsm_consumer_service_handle_unregister_shutdown_client (NSMConsumer           *object,
                                                                        GDBusMethodInvocation *invocation,
                                                                        const gchar           *bus_name,
                                                                        const gchar           *object_name,
                                                                        gint                   shutdown_mode,
                                                                        NSMConsumerService    *service);
static void     nsm_shutdown_client_release                            (NSMShutdownClient     *shutdown_client);



struct _NSMConsumerServiceClass
{
  GObjectClass __parent__;
};

struct _NSMConsumerService
{
  GObject          __parent__;

  NSMConsumer     *interface;
  GDBusConnection *connection;

  GList           *shutdown_clients;
};

struct _NSMShutdownClient
{
  gchar *bus_name;
  gchar *object_name;
  guint  timeout_ms;
  gint   shutdown_mode;
};



G_DEFINE_TYPE (NSMConsumerService, nsm_consumer_service, G_TYPE_OBJECT);



static void
nsm_consumer_service_class_init (NSMConsumerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = nsm_consumer_service_finalize;
  gobject_class->get_property = nsm_consumer_service_get_property;
  gobject_class->set_property = nsm_consumer_service_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
nsm_consumer_service_init (NSMConsumerService *service)
{
  /* create a list of shutdown consumers; we will use this to shut down
   * the consumers in reverse order of consumer registrations */
  service->shutdown_clients = NULL;

  service->interface = nsm_consumer_skeleton_new ();

  /* implement the RegisterShutdownClient() handler */
  g_signal_connect (service->interface, "handle-register-shutdown-client",
                    G_CALLBACK (nsm_consumer_service_handle_register_shutdown_client),
                    service);

  /* implement the UnRegisterShutdownClient() handler */
  g_signal_connect (service->interface, "handle-un-register-shutdown-client",
                    G_CALLBACK (nsm_consumer_service_handle_unregister_shutdown_client),
                    service);
}



static void
nsm_consumer_service_finalize (GObject *object)
{
  NSMConsumerService *service = NSM_CONSUMER_SERVICE (object);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  /* release the list of shutdown clients */
  if (service->shutdown_clients != NULL)
    g_list_free_full (service->shutdown_clients,
                      (GDestroyNotify) nsm_shutdown_client_release);

  (*G_OBJECT_CLASS (nsm_consumer_service_parent_class)->finalize) (object);
}



static void
nsm_consumer_service_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  NSMConsumerService *service = NSM_CONSUMER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, service->connection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
nsm_consumer_service_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  NSMConsumerService *service = NSM_CONSUMER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->connection = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
nsm_consumer_service_handle_register_shutdown_client (NSMConsumer           *object,
                                                      GDBusMethodInvocation *invocation,
                                                      const gchar           *bus_name,
                                                      const gchar           *object_name,
                                                      gint                   shutdown_mode,
                                                      guint                  timeout_ms,
                                                      NSMConsumerService    *service)
{
  NSMShutdownClient *shutdown_client = NULL;
  gchar             *message;

  g_return_val_if_fail (IS_NSM_CONSUMER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);

  /* create the shutdown client */
  shutdown_client = g_slice_new0 (NSMShutdownClient);
  shutdown_client->bus_name = g_strdup (bus_name);
  shutdown_client->object_name = g_strdup (object_name);
  shutdown_client->timeout_ms = timeout_ms;
  shutdown_client->shutdown_mode = shutdown_mode;

  /* register the shutdown client */
  service->shutdown_clients = g_list_append (service->shutdown_clients,
                                             (gpointer) shutdown_client);

  /* log the registered shutdown client */
  message = g_strdup_printf ("registered client information: Bus %s : Object: %s\
                              Shutdown mode: %d Timeout: %d", shutdown_client->bus_name,
                              shutdown_client->object_name,
                              shutdown_client->shutdown_mode,
                              shutdown_client->timeout_ms);

  DLT_LOG (nsm_dummy_context, DLT_LOG_INFO, DLT_STRING (message));
  g_slice_free (gchar, message);

  /* notify the caller that we have handled the register request */
  nsm_consumer_complete_register_shutdown_client (object, invocation, 0);
  return TRUE;
}



static gboolean
nsm_consumer_service_handle_unregister_shutdown_client (NSMConsumer           *object,
                                                        GDBusMethodInvocation *invocation,
                                                        const gchar           *bus_name,
                                                        const gchar           *object_name,
                                                        gint                   shutdown_mode,
                                                        NSMConsumerService    *service)
{
  NSMShutdownClient *shutdown_client = NULL;
  GList             *clients;
  gchar             *message;

  g_return_val_if_fail (IS_NSM_CONSUMER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);

  /* find the shutdown client in the list */
  for (clients = service->shutdown_clients;
       shutdown_client == NULL && clients != NULL;
       clients = clients->next)
    {
      if ((g_strcmp0 (((NSMShutdownClient *) clients->data)->bus_name, bus_name) == 0)
          && (g_strcmp0 (((NSMShutdownClient *) clients->data)->object_name, object_name) == 0)
          && (((NSMShutdownClient *) clients->data)->shutdown_mode == shutdown_mode))
        {
          shutdown_client = clients->data;
        }
    }

  /* unregister the shutdown client */
  if (shutdown_client != NULL)
    {
        /* log the unregistered shutdown client */
        message = 
            g_strdup_printf ("unregistered client information: Bus %s : Object: %s\
                              Shutdown mode: %d", shutdown_client->bus_name,
                              shutdown_client->object_name,
                              shutdown_client->shutdown_mode);

        DLT_LOG (nsm_dummy_context, DLT_LOG_INFO, DLT_STRING (message));
        g_slice_free (gchar, message);

       /* take out the client from the list */
       service->shutdown_clients = 
         g_list_remove (service->shutdown_clients, shutdown_client);

       /* release the client */
      nsm_shutdown_client_release (shutdown_client);
    }
  else
    {
      /* notify the caller that we have handled the unregister request */
      nsm_consumer_complete_un_register_shutdown_client (object, invocation, -1);
      return TRUE;
    }

  /* notify the caller that we have handled the unregister request */
  nsm_consumer_complete_un_register_shutdown_client (object, invocation, 0);
  return TRUE;
}



NSMConsumerService *
nsm_consumer_service_new (GDBusConnection *connection)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  return g_object_new (NSM_CONSUMER_TYPE_SERVICE, "connection", connection, NULL);
}



gboolean
nsm_consumer_service_start (NSMConsumerService *service,
                            GError            **error)
{
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the Consumer service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/com/contiautomotive/NodeStateManager/Consumer",
                                           error);
}



static void
nsm_shutdown_client_release (NSMShutdownClient *shutdown_client)
{
  g_return_if_fail (shutdown_client != NULL);

  /* release the client fields */
  g_slice_free (gchar, shutdown_client->bus_name);
  g_slice_free (gchar, shutdown_client->object_name);

  /* release the client */
  g_slice_free (NSMShutdownClient, shutdown_client);
}



void
nsm_shutdown_consumers (NSMConsumerService *service)
{
  ShutdownConsumer *proxy               = NULL;
  GError           *error               = NULL;
  gchar            *message;
  GList            *clients;
  GList            *shutdown_consumers;
  gint              error_code;

  g_return_if_fail (NSM_CONSUMER_IS_SERVICE (service));

  /* reverse the list of clients */
  shutdown_consumers = g_list_reverse (service->shutdown_clients);

  for (clients = shutdown_consumers;
       clients != NULL;
       clients = clients->next)
    {
      /* create a synchronous proxy; the NSM has to wait
       * the shutting down reponse from Boot Manager */
      proxy =
        shutdown_consumer_proxy_new_sync (service->connection,
                                          G_DBUS_PROXY_FLAGS_NONE,
                                          ((NSMShutdownClient *) clients->data)->bus_name,
                                          ((NSMShutdownClient *) clients->data)->object_name,
                                          NULL,
                                          &error);
      if (error != NULL)
        {
          message =
            g_strdup_printf ("Failed to connect to nsm consumer service: %s",
                             error->message);
          DLT_LOG (nsm_dummy_context, DLT_LOG_ERROR, DLT_STRING (message));
          g_slice_free (gchar, message);
          g_error_free (error);
        }

      /* call the shutdown method (temporarily using bald numbers instead of enums) */
      shutdown_consumer_call_lifecycle_request_sync (proxy, 1, 1234, &error_code, NULL,
                                                     &error);
      if (error != NULL)
        {
          message = 
            g_strdup_printf ("Failed to call shutdown method: %s",
                             error->message);
          DLT_LOG (nsm_dummy_context, DLT_LOG_ERROR, DLT_STRING (message));
          g_slice_free (gchar, message);
          g_error_free (error);
        }

      /* log the shutdown client */
        message =
            g_strdup_printf ("Shutdown client information: Bus %s : Object: %s\
                              Shutdown mode: %d", 
                             ((NSMShutdownClient *) clients->data)->bus_name,
                             ((NSMShutdownClient *) clients->data)->object_name,
                             ((NSMShutdownClient *) clients->data)->shutdown_mode);

        DLT_LOG (nsm_dummy_context, DLT_LOG_INFO, DLT_STRING (message));
        g_slice_free (gchar, message);

      /* take out the shutdown client from the list and release it */
        service->shutdown_clients = 
          g_list_remove (service->shutdown_clients, (NSMShutdownClient *) clients->data);

        nsm_shutdown_client_release ((NSMShutdownClient *) clients->data);

      /* release the proxy */
        g_object_unref (proxy);
    }
}
