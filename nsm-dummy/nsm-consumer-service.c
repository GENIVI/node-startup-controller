/* vi:set et ai sw=2 sts=2 ts=2: */
/* SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2012, GENIVI
 *
 * This file is part of node-startup-controller.
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 *
 * List of changes:
 * 2015-04-30, Jonathan Maw, List of changes started
 *
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



typedef struct _ShutdownQueue ShutdownQueue;



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
static void     nsm_consumer_service_shut_down_next_client_in_queue    (NSMConsumerService    *service);
static void     nsm_consumer_service_lifecycle_request_finish          (GObject               *object,
                                                                        GAsyncResult          *res,
                                                                        gpointer               user_data);
static gboolean nsm_consumer_service_shut_down_client_timeout          (gpointer               user_data);



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

  ShutdownQueue   *shutdown_queue;
};

struct _ShutdownQueue
{
  NSMConsumerService *service;
  NSMShutdownType     current_mode;
  GList              *remaining_clients;
  guint               timeout_id;
  guint               timeout_request;
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
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (service->interface));
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
  NSMShutdownType   current_shutdown_mode;
  ShutdownClient   *current_client = NULL;
  ShutdownClient   *shutdown_client = NULL;
  const gchar      *current_bus_name;
  const gchar      *current_object_path;
  GError           *error = NULL;
  GList            *lp;

  g_return_val_if_fail (IS_NSM_CONSUMER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);

  /* try to find this shutdown client in the list of registered clients */
  for (lp = service->shutdown_clients;
       shutdown_client == NULL && lp != NULL;
       lp = lp->next)
    {
      current_client = SHUTDOWN_CLIENT (lp->data);
      current_bus_name = shutdown_client_get_bus_name (current_client);
      current_object_path = shutdown_client_get_object_path (current_client);

      if (g_strcmp0 (current_bus_name, bus_name) == 0
          && g_strcmp0 (current_object_path, object_path) == 0)
        {
          shutdown_client = current_client;
        }
    }

  /* check if we already have this shutdown client registered */
  if (shutdown_client != NULL)
    {
      /* update the client's shutdown mode */
      current_shutdown_mode = shutdown_client_get_shutdown_mode (shutdown_client);
      shutdown_client_set_shutdown_mode (shutdown_client,
                                         current_shutdown_mode | shutdown_mode);

      /* update the client's timeout */
      shutdown_client_set_timeout (shutdown_client, timeout);
      consumer = shutdown_client_get_consumer (shutdown_client);
      g_dbus_proxy_set_default_timeout (G_DBUS_PROXY (consumer), timeout);

      /* log information about the re-registration */
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Re-registered shutdown client:"),
               DLT_STRING ("bus name"), DLT_STRING (bus_name),
               DLT_STRING ("object path"), DLT_STRING (object_path),
               DLT_STRING ("new shutdown mode"),
               DLT_UINT (shutdown_client_get_shutdown_mode (shutdown_client)),
               DLT_STRING ("new timeout"),
               DLT_UINT (shutdown_client_get_timeout (shutdown_client)));
    }
  else
    {
      /* this is a new registration, create a proxy for this new shutdown consumer */
      consumer = shutdown_consumer_proxy_new_sync (service->connection,
                                                   G_DBUS_PROXY_FLAGS_NONE,
                                                   bus_name, object_path,
                                                   NULL, &error);
      if (error != NULL)
        {
          /* log the error */
          DLT_LOG (nsm_dummy_context, DLT_LOG_ERROR,
                   DLT_STRING ("Failed to register shutdown client:"),
                   DLT_STRING ("object path"), DLT_STRING (object_path),
                   DLT_STRING ("error message"), DLT_STRING (error->message));
          g_error_free (error);

          /* report the error back to the caller */
          nsm_consumer_complete_register_shutdown_client (object, invocation,
                                                          NSM_ERROR_STATUS_DBUS);
          return TRUE;
        }

      /* create the shutdown client */
      shutdown_client = shutdown_client_new (bus_name, object_path,
                                             shutdown_mode, timeout);
      shutdown_client_set_consumer (shutdown_client, consumer);

      /* set the D-Bus timeout for the interaction with the shutdown consumer */
      g_dbus_proxy_set_default_timeout (G_DBUS_PROXY (consumer), timeout);

      /* add the client to the list of registered clients */
      service->shutdown_clients = g_list_append (service->shutdown_clients,
                                                 shutdown_client);

      /* log the registered shutdown client */
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Shutdown client registered:"),
               DLT_STRING ("bus name"), DLT_STRING (bus_name),
               DLT_STRING ("object path"), DLT_STRING (object_path),
               DLT_STRING ("shutdown mode"), DLT_UINT (shutdown_mode),
               DLT_STRING ("timeout"), DLT_UINT (timeout));

      /* release the consumer proxy; the client owns it now */
      g_object_unref (consumer);
    }

  /* notify the caller that we have handled the register request */
  nsm_consumer_complete_register_shutdown_client (object, invocation,
                                                  NSM_ERROR_STATUS_OK);
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
  NSMShutdownType current_shutdown_mode;
  ShutdownClient *current_client;
  ShutdownClient *shutdown_client = NULL;
  GList          *lp;
  const gchar    *current_bus_name;
  const gchar    *current_object_path;

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
      current_bus_name = shutdown_client_get_bus_name (current_client);
      current_object_path = shutdown_client_get_object_path (current_client);

      /* check whether this is the client corresponding to the unregister request */
      if (g_strcmp0 (current_bus_name, bus_name) == 0
          && g_strcmp0 (current_object_path, object_path) == 0)
        {
          /* yes, this is the client we have been looking for */
          shutdown_client = current_client;
        }
    }

  /* if we have a shutdown client, unregister it for the requested shutdown modes */
  if (shutdown_client != NULL)
    {
      /* remove the shutdown modes to unregister from the shutdown client */
      current_shutdown_mode = shutdown_client_get_shutdown_mode (shutdown_client);
      shutdown_client_set_shutdown_mode (shutdown_client,
                                         current_shutdown_mode & ~(shutdown_mode));

      /* unregister only if the client is not registered for any modes anymore */
      if (shutdown_client_get_shutdown_mode (shutdown_client) == 0)
        {
          /* log the unregistration now */
          DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
                   DLT_STRING ("Shutdown client unregistered:"),
                   DLT_STRING ("bus name"), DLT_STRING (bus_name),
                   DLT_STRING ("object path"), DLT_STRING (object_path));

          /* remove the client from the list of registered clients */
          service->shutdown_clients = g_list_remove (service->shutdown_clients,
                                                     shutdown_client);

          /* release the client and free its resources */
          g_object_unref (shutdown_client);
        }

      /* notify the caller that we have handled the unregister request successfully */
      nsm_consumer_complete_un_register_shutdown_client (object, invocation,
                                                         NSM_ERROR_STATUS_OK);
    }
  else
    {
      /* notify the caller that we could not handle the unregister request properly */
      nsm_consumer_complete_un_register_shutdown_client (object, invocation,
                                                         NSM_ERROR_STATUS_ERROR);
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
  g_return_val_if_fail (IS_NSM_CONSUMER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);

  DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
           DLT_STRING ("Finished shutting down client:"),
           DLT_STRING ("request id"), DLT_UINT (request_id),
           DLT_STRING ("status"), DLT_UINT (status));

  nsm_consumer_complete_lifecycle_request_complete (object, invocation,
                                                    NSM_ERROR_STATUS_OK);

  /* check if we are currently processing the shutdown queue */
  if (service->shutdown_queue != NULL)
    {
      /* check if we have been waiting for a LifecycleRequestComplete call */
      if (service->shutdown_queue->timeout_id > 0)
        {
          /* check if we have waited for this client */
          if (request_id == service->shutdown_queue->timeout_request)
            {
              /* remove the client we just finished shutting down from the queue */
              service->shutdown_queue->remaining_clients =
                g_list_delete_link (service->shutdown_queue->remaining_clients,
                                    service->shutdown_queue->remaining_clients);

              /* drop the coresponding wait time out */
              g_source_remove (service->shutdown_queue->timeout_id);
              service->shutdown_queue->timeout_id = 0;
              service->shutdown_queue->timeout_request = 0;

              /* continue shutting down the next client */
              nsm_consumer_service_shut_down_next_client_in_queue (service);
            }
          else
            {
              /* no we haven't; log this as a warning */
              DLT_LOG (nsm_dummy_context, DLT_LOG_WARN,
                       DLT_STRING ("Waiting for lifecycle request"),
                       DLT_UINT (service->shutdown_queue->timeout_request),
                       DLT_STRING ("to be completed but received completion of"),
                       DLT_UINT (request_id),
                       DLT_STRING ("instead "));
            }
        }
      else
        {
          /* the timeout is no longer active, we might have missed
           * the time window; log this now */
          DLT_LOG (nsm_dummy_context, DLT_LOG_WARN,
                   DLT_STRING ("Lifecycle request"), DLT_UINT (request_id),
                   DLT_STRING ("completed too late"));
        }
    }

  return TRUE;
}



static void
nsm_consumer_service_shut_down_next_client_in_queue (NSMConsumerService *service)
{
  ShutdownConsumer *consumer;
  ShutdownClient   *client;

  g_return_if_fail (NSM_CONSUMER_IS_SERVICE (service));
  g_return_if_fail (service->shutdown_queue != NULL);

  DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
           DLT_STRING ("Shutting down next client in queue"));

  /* check if we have processed all clients in the queue */
  if (service->shutdown_queue->remaining_clients == NULL)
    {
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Processed all items in the queue for this mode"));

      /* check if we have processed all shutdown modes */
      if (service->shutdown_queue->current_mode == NSM_SHUTDOWN_TYPE_NORMAL)
        {
          DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
                   DLT_STRING ("All clients have been shut down"));

          /* fast and normal have been processed, we are finished */
          g_slice_free (ShutdownQueue, service->shutdown_queue);
          service->shutdown_queue = NULL;

          /* release the reference on the service to allow it to be destroyed */
          g_object_unref (service);

          return;
        }
      else if (service->shutdown_queue->current_mode == NSM_SHUTDOWN_TYPE_FAST)
        {
          DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
                   DLT_STRING ("Transitioning to normal shutdown mode"));

          /* move on to normal shutdown mode and reset clients to be processed */
          service->shutdown_queue->current_mode = NSM_SHUTDOWN_TYPE_NORMAL;
          service->shutdown_queue->remaining_clients =
            g_list_reverse (g_list_copy (service->shutdown_clients));
        }
      else
        {
          /* this point will only be reached if the transition from
           * fast to normal shutdown modes is implemented incorrectly */
          g_assert_not_reached ();
        }
    }

  /* get the current client from the queue */
  client =
    SHUTDOWN_CLIENT (g_list_first (service->shutdown_queue->remaining_clients)->data);

  /* check if it is registered for the current shutdown mode */
  if ((shutdown_client_get_shutdown_mode (client)
       & service->shutdown_queue->current_mode) != 0)
    {
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Shutting down a client:"),
               DLT_STRING ("bus name"),
               DLT_STRING (shutdown_client_get_bus_name (client)),
               DLT_STRING ("object path"),
               DLT_STRING (shutdown_client_get_object_path (client)),
               DLT_STRING ("shutdown mode"),
               DLT_UINT (shutdown_client_get_shutdown_mode (client)),
               DLT_STRING ("timeout"),
               DLT_UINT (shutdown_client_get_timeout (client)),
               DLT_STRING ("request id"),
               DLT_UINT (GPOINTER_TO_UINT (client)));

      /* get the consumer associated with the shutdown client */
      consumer = shutdown_client_get_consumer (client);

      /* call the shutdown method */
      shutdown_consumer_call_lifecycle_request (consumer,
                                                service->shutdown_queue->current_mode,
                                                GPOINTER_TO_UINT (client),
                                                NULL,
                                                nsm_consumer_service_lifecycle_request_finish,
                                                service);
    }
  else
    {
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Skipping client"),
               DLT_STRING (shutdown_client_get_object_path (client)),
               DLT_STRING ("as it is not registered for shutdown mode"),
               DLT_UINT (service->shutdown_queue->current_mode));

      /* it isn't, so remove it from the queue */
      service->shutdown_queue->remaining_clients =
        g_list_delete_link (service->shutdown_queue->remaining_clients,
                            service->shutdown_queue->remaining_clients);

      /* continue with the next client */
      nsm_consumer_service_shut_down_next_client_in_queue (service);
    }
}



static void
nsm_consumer_service_lifecycle_request_finish (GObject      *object,
                                               GAsyncResult *res,
                                               gpointer      user_data)
{
  NSMConsumerService *service = NSM_CONSUMER_SERVICE (user_data);
  ShutdownConsumer   *consumer = SHUTDOWN_CONSUMER (object);
  ShutdownClient     *client;
  NSMErrorStatus      error_code;
  GError             *error = NULL;

  g_return_if_fail (IS_SHUTDOWN_CONSUMER (consumer));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (NSM_CONSUMER_IS_SERVICE (service));

  /* get the current client from the queue */
  client =
    SHUTDOWN_CLIENT (g_list_first (service->shutdown_queue->remaining_clients)->data);

  if (!shutdown_consumer_call_lifecycle_request_finish (consumer, (gint *)&error_code,
                                                        res, &error))
    {
      /* log the error */
      DLT_LOG (nsm_dummy_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to shut down a client:"),
               DLT_STRING ("object path"),
               DLT_STRING (shutdown_client_get_object_path (client)),
               DLT_STRING ("error message"), DLT_STRING (error->message));
      g_clear_error (&error);

      /* remove the client it from the shutdown queue */
      service->shutdown_queue->remaining_clients =
        g_list_delete_link (service->shutdown_queue->remaining_clients,
                            service->shutdown_queue->remaining_clients);

      /* continue shutting down the next client */
      nsm_consumer_service_shut_down_next_client_in_queue (service);
    }
  else if (error_code == NSM_ERROR_STATUS_OK)
    {
      /* log the successful shutdown */
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Successfully shut down a client:"),
               DLT_STRING ("bus name"),
               DLT_STRING (shutdown_client_get_bus_name (client)),
               DLT_STRING ("object path"),
               DLT_STRING (shutdown_client_get_object_path (client)),
               DLT_STRING ("shutdown mode"),
               DLT_UINT (service->shutdown_queue->current_mode));

      /* remove the client it from the shutdown queue */
      service->shutdown_queue->remaining_clients =
        g_list_delete_link (service->shutdown_queue->remaining_clients,
                            service->shutdown_queue->remaining_clients);

      /* continue shutting down the next client */
      nsm_consumer_service_shut_down_next_client_in_queue (service);
    }
  else if (error_code == NSM_ERROR_STATUS_RESPONSE_PENDING)
    {
      /* log that we are waiting for the client to finish its shutdown */
      DLT_LOG (nsm_dummy_context, DLT_LOG_INFO,
               DLT_STRING ("Waiting for client to shut down:"),
               DLT_STRING ("request id"),
               DLT_UINT (GPOINTER_TO_UINT (client)),
               DLT_STRING ("bus name"),
               DLT_STRING (shutdown_client_get_bus_name (client)),
               DLT_STRING ("object path"),
               DLT_STRING (shutdown_client_get_object_path (client)),
               DLT_STRING ("shutdown mode"),
               DLT_UINT (service->shutdown_queue->current_mode));

      /* start a timeout to wait for LifecycleComplete to be called by the
       * client we just asked to shut down */
      g_assert (service->shutdown_queue->timeout_id == 0);
      service->shutdown_queue->timeout_request = GPOINTER_TO_UINT (client);
      service->shutdown_queue->timeout_id =
        g_timeout_add_full (G_PRIORITY_DEFAULT,
                            shutdown_client_get_timeout (client),
                            nsm_consumer_service_shut_down_client_timeout,
                            g_object_ref (service),
                            (GDestroyNotify) g_object_unref);
    }
  else
    {
      /* log that shutting down this client failed */
      DLT_LOG (nsm_dummy_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed shutting down a client:"),
               DLT_STRING ("request id"),
               DLT_UINT (GPOINTER_TO_UINT (client)),
               DLT_STRING ("bus name"),
               DLT_STRING (shutdown_client_get_bus_name (client)),
               DLT_STRING ("object path"),
               DLT_STRING (shutdown_client_get_object_path (client)),
               DLT_STRING ("shutdown mode"),
               DLT_UINT (service->shutdown_queue->current_mode),
               DLT_STRING ("error status"),
               DLT_UINT (error_code));

      /* remove the client it from the shutdown queue */
      service->shutdown_queue->remaining_clients =
        g_list_delete_link (service->shutdown_queue->remaining_clients,
                            service->shutdown_queue->remaining_clients);

      /* continue shutting down the next client */
      nsm_consumer_service_shut_down_next_client_in_queue (service);
    }
}



static gboolean
nsm_consumer_service_shut_down_client_timeout (gpointer user_data)
{
  NSMConsumerService *service = NSM_CONSUMER_SERVICE (user_data);
  ShutdownClient     *client;

  g_return_val_if_fail (NSM_CONSUMER_IS_SERVICE (service), FALSE);

  /* drop the timeout if we have finished to process the shutdown
   * queue in the meantime */
  if (service->shutdown_queue == NULL)
    return FALSE;

  /* drop the timeout if there are no further clients to process
   * at the moment; this is rarely going to happen */
  if (service->shutdown_queue->remaining_clients == NULL)
    return FALSE;

  /* get the current/next client in the queue */
  client = SHUTDOWN_CLIENT (service->shutdown_queue->remaining_clients->data);

  /* check if this still is the client we are currently waiting
   * for to shut down */
  if (service->shutdown_queue->timeout_request == GPOINTER_TO_UINT (client))
    {
      DLT_LOG (nsm_dummy_context, DLT_LOG_WARN,
               DLT_STRING ("Received timeout while shutting down a client:"),
               DLT_STRING ("bus name"),
               DLT_STRING (shutdown_client_get_bus_name (client)),
               DLT_STRING ("object path"),
               DLT_STRING (shutdown_client_get_object_path (client)),
               DLT_STRING ("shutdown mode"),
               DLT_UINT (shutdown_client_get_shutdown_mode (client)),
               DLT_STRING ("timeout"),
               DLT_UINT (shutdown_client_get_timeout (client)),
               DLT_STRING ("request id"),
               DLT_UINT (service->shutdown_queue->timeout_request));

      /* it is, so we haven't receive da reply in time; drop the
       * client from the queue and continue with the next right
       * immediately */
      service->shutdown_queue->remaining_clients =
        g_list_delete_link (service->shutdown_queue->remaining_clients,
                            service->shutdown_queue->remaining_clients);

      /* reset the timeout information */
      service->shutdown_queue->timeout_request = 0;
      service->shutdown_queue->timeout_id = 0;

      /* continue shutting down the next client in the queue */
      nsm_consumer_service_shut_down_next_client_in_queue (service);
    }

  return FALSE;
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
                                           "/org/genivi/NodeStateManager/Consumer",
                                           error);
}



void
nsm_consumer_service_shutdown_consumers (NSMConsumerService *service)
{
  g_return_if_fail (NSM_CONSUMER_IS_SERVICE (service));

  /* do nothing if there already is a shutdown queue */
  if (service->shutdown_queue != NULL)
    return;

  /* do nothing if there are no shutdown clients at all */
  if (service->shutdown_clients == NULL)
    return;

  /* grab a reference on the service; this is to avoid that the service object
   * is destroyed while we are processing the shutdown queue */
  g_object_ref (service);

  /* allocate a new shutdown queue */
  service->shutdown_queue = g_slice_new0 (ShutdownQueue);

  /* start with the fast shutdown clients */
  service->shutdown_queue->current_mode = NSM_SHUTDOWN_TYPE_FAST;

  /* reset shutdown clients to be processed */
  service->shutdown_queue->remaining_clients =
    g_list_reverse (g_list_copy (service->shutdown_clients));

  /* shutdown the next client in the queue */
  nsm_consumer_service_shut_down_next_client_in_queue (service);
}
