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
#include <common/nsm-enum-types.h>
#include <common/shutdown-client.h>
#include <common/shutdown-consumer-dbus.h>

#include <nsm-dummy/nsm-consumer-service.h>



DLT_IMPORT_CONTEXT (nsm_dummy_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



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
                                                                        const gchar           *object_path,
                                                                        NSMShutdownType        shutdown_mode,
                                                                        guint                  timeout,
                                                                        NSMConsumerService    *service);
static gboolean nsm_consumer_service_handle_unregister_shutdown_client (NSMConsumer           *object,
                                                                        GDBusMethodInvocation *invocation,
                                                                        const gchar           *bus_name,
                                                                        const gchar           *object_path,
                                                                        NSMShutdownType        shutdown_mode,
                                                                        NSMConsumerService    *service);
static gboolean nsm_consumer_service_handle_lifecycle_request_complete (NSMConsumer           *object,
                                                                        GDBusMethodInvocation *invocation,
                                                                        guint                  request_id,
                                                                        NSMErrorStatus         status,
                                                                        NSMConsumerService    *service);



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
  guint            request_id;
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

  /* implement the RegisterShutdownClient method */
  g_signal_connect (service->interface, "handle-register-shutdown-client",
                    G_CALLBACK (nsm_consumer_service_handle_register_shutdown_client),
                    service);

  /* implement the UnRegisterShutdownClient method */
  g_signal_connect (service->interface, "handle-un-register-shutdown-client",
                    G_CALLBACK (nsm_consumer_service_handle_unregister_shutdown_client),
                    service);

  /* implement the LifecycleRequestComplete method */
  g_signal_connect (service->interface, "handle-lifecycle-request-complete",
                    G_CALLBACK (nsm_consumer_service_handle_lifecycle_request_complete),
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
    g_list_free_full (service->shutdown_clients, (GDestroyNotify) g_object_unref);

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
                                                      const gchar           *object_path,
                                                      NSMShutdownType        shutdown_mode,
                                                      guint                  timeout,
                                                      NSMConsumerService    *service)
{
  ShutdownConsumer *consumer;
  ShutdownClient   *shutdown_client;
  GError           *error = NULL;
  gchar            *message;

  g_return_val_if_fail (IS_NSM_CONSUMER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);

  /* create a proxy for the shutdown consumer to be registered */
  consumer = shutdown_consumer_proxy_new_sync (service->connection,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               bus_name, object_path,
                                               NULL, &error);
  if (error != NULL)
    {
      /* log the error */
      message = g_strdup_printf ("Failed to register shutdown client %s: %s",
                                 object_path, error->message);
      DLT_LOG (nsm_dummy_context, DLT_LOG_ERROR, DLT_STRING (message));
      g_free (message);
      g_error_free (error);

      /* report the error back to the caller */
      nsm_consumer_complete_register_shutdown_client (object, invocation,
                                                      NSM_ERROR_STATUS_ERROR);
      return TRUE;
    }

  /* create the shutdown client */
  shutdown_client = shutdown_client_new (bus_name, object_path, shutdown_mode, timeout);
  shutdown_client_set_consumer (shutdown_client, consumer);

  /* register the shutdown client */
  service->shutdown_clients = g_list_append (service->shutdown_clients, shutdown_client);

  /* log the registered shutdown client */
  message = g_strdup_printf ("Shutdown client registered: bus name %s, "
                             "object path %s, shutdown mode: %d, timeout: %d",
                             bus_name, object_path, shutdown_mode, timeout);
  DLT_LOG (nsm_dummy_context, DLT_LOG_INFO, DLT_STRING (message));
  g_free (message);

  /* notify the caller that we have handled the register request */
  nsm_consumer_complete_register_shutdown_client (object, invocation, 0);
  return TRUE;
}



static gboolean
nsm_consumer_service_handle_unregister_shutdown_client (NSMConsumer           *object,
                                                        GDBusMethodInvocation *invocation,
                                                        const gchar           *bus_name,
                                                        const gchar           *object_path,
                                                        NSMShutdownType        shutdown_mode,
                                                        NSMConsumerService    *service)
{
  NSMShutdownType client_shutdown_mode;
  ShutdownClient *current_client;
  ShutdownClient *shutdown_client = NULL;
  GList          *lp;
  const gchar    *client_bus_name;
  const gchar    *client_object_path;
  gchar          *message;

  g_return_val_if_fail (IS_NSM_CONSUMER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);

  /* try to find the shutdown client in the list of registered clients */
  for (lp = service->shutdown_clients;
       shutdown_client == NULL && lp != NULL;
       lp = lp->next)
    {
      current_client = SHUTDOWN_CLIENT (lp->data);

      /* extract information from the current client */
      client_bus_name = shutdown_client_get_bus_name (current_client);
      client_object_path = shutdown_client_get_object_path (current_client);
      client_shutdown_mode = shutdown_client_get_shutdown_mode (current_client);

      /* check whether this is the client corresponding to the unregister request */
      if (g_strcmp0 (client_bus_name, bus_name) == 0
          && g_strcmp0 (client_object_path, object_path) == 0
          && client_shutdown_mode == shutdown_mode)
        {
          /* yes, this is the client we have been looking for */
          shutdown_client = current_client;
        }
    }

  /* unregister the shutdown client now */
  if (shutdown_client != NULL)
    {
      /* log the unregistered shutdown client */
      message = g_strdup_printf ("Shutdown client unregistered: bus name %s, "
                                 "object path %s, shutdown mode: %d",
                                 bus_name, object_path, shutdown_mode);
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO, DLT_STRING (message));
      g_free (message);

      /* remove the client from the list of registered clients */
      service->shutdown_clients = g_list_remove (service->shutdown_clients,
                                                 shutdown_client);

      /* release the client and free its resources */
      g_object_unref (shutdown_client);

      /* notify the caller that we have handled the unregister request successfully */
      nsm_consumer_complete_un_register_shutdown_client (object, invocation, 0);
    }
  else
    {
      /* notify the caller that we could not handle the unregister request properly */
      nsm_consumer_complete_un_register_shutdown_client (object, invocation, -1);
    }

  return TRUE;
}



static gboolean
nsm_consumer_service_handle_lifecycle_request_complete (NSMConsumer           *object,
                                                        GDBusMethodInvocation *invocation,
                                                        guint                  request_id,
                                                        NSMErrorStatus         status,
                                                        NSMConsumerService    *service)
{
  gchar *message;

  g_return_val_if_fail (IS_NSM_CONSUMER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);

  message = g_strdup_printf ("Finished to shut down a client: "
                             "request id %d, status %d", request_id, status);
  DLT_LOG (nsm_dummy_context, DLT_LOG_INFO, DLT_STRING (message));
  g_free (message);

  nsm_consumer_complete_lifecycle_request_complete (object, invocation,
                                                    NSM_ERROR_STATUS_OK);

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



void
nsm_consumer_service_shutdown_consumers (NSMConsumerService *service)
{
  ShutdownConsumer *consumer;
  NSMShutdownType   current_mode;
  NSMShutdownType   shutdown_mode;
  ShutdownClient   *shutdown_client;
  const gchar      *bus_name;
  const gchar      *object_path;
  GError           *error = NULL;
  gchar            *message;
  GList            *lp;
  gint              error_code;

  g_return_if_fail (NSM_CONSUMER_IS_SERVICE (service));

  /* shutdown mode after mode (fast first, then normal) */
  for (current_mode = NSM_SHUTDOWN_TYPE_FAST;
       current_mode >= NSM_SHUTDOWN_TYPE_NORMAL;
       current_mode--)
    {
      /* shut down all registered clients in reverse order of registration */
      for (lp = g_list_last (service->shutdown_clients); lp != NULL; lp = lp->prev)
        {
          shutdown_client = SHUTDOWN_CLIENT (lp->data);

          /* extract information from the shutdown client */
          consumer = shutdown_client_get_consumer (shutdown_client);
          bus_name = shutdown_client_get_bus_name (shutdown_client);
          object_path = shutdown_client_get_object_path (shutdown_client);
          shutdown_mode = shutdown_client_get_shutdown_mode (shutdown_client);

          /* skip the shutdown consumer if it is not registered for this mode */
          if ((shutdown_mode & current_mode) == 0)
            continue;

          /* call the shutdown method */
          shutdown_consumer_call_lifecycle_request_sync (consumer,
                                                         current_mode,
                                                         service->request_id++,
                                                         &error_code, NULL, &error);
          if (error != NULL)
            {
              message = g_strdup_printf ("Failed to shut down client %s: %s",
                                         object_path, error->message);
              DLT_LOG (nsm_dummy_context, DLT_LOG_ERROR, DLT_STRING (message));
              g_free (message);
              g_error_free (error);
            }
          else if (error_code == NSM_ERROR_STATUS_OK)
            {
              message = g_strdup_printf ("Shutdown client shut down: bus name %s, "
                                         "object path %s, shutdown mode: %d",
                                         bus_name, object_path, current_mode);
              DLT_LOG (nsm_dummy_context, DLT_LOG_INFO, DLT_STRING (message));
              g_free (message);
            }
          else if (error_code == NSM_ERROR_STATUS_RESPONSE_PENDING)
            {
              message = g_strdup_printf ("Started to shut down a client: "
                                         "request id: %d, bus name %s, "
                                         "object path %s, shutdown mode: %d",
                                         service->request_id-1, bus_name, object_path,
                                         current_mode);
              DLT_LOG (nsm_dummy_context, DLT_LOG_INFO, DLT_STRING (message));
              g_free (message);
            }
          else
            {
              message = g_strdup_printf ("Failed to shut down client %s: "
                                         "error status = %d", object_path, error_code);
              DLT_LOG (nsm_dummy_context, DLT_LOG_ERROR, DLT_STRING (message));
              g_free (message);
            }
        }
    }
}
