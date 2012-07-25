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

#include <common/la-handler-dbus.h>
#include <common/nsm-consumer-dbus.h>
#include <common/nsm-enum-types.h>
#include <common/shutdown-consumer-dbus.h>

#include <boot-manager/job-manager.h>
#include <boot-manager/la-handler-service.h>



DLT_IMPORT_CONTEXT (la_handler_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_JOB_MANAGER,
};



typedef struct _LAHandlerServiceData LAHandlerServiceData;



static void                  la_handler_service_constructed                              (GObject               *object);
static void                  la_handler_service_finalize                                 (GObject               *object);
static void                  la_handler_service_get_property                             (GObject               *object,
                                                                                          guint                  prop_id,
                                                                                          GValue                *value,
                                                                                          GParamSpec            *pspec);
static void                  la_handler_service_set_property                             (GObject               *object,
                                                                                          guint                  prop_id,
                                                                                          const GValue          *value,
                                                                                          GParamSpec            *pspec);
static gboolean              la_handler_service_handle_register                          (LAHandler             *interface,
                                                                                          GDBusMethodInvocation *invocation,
                                                                                          const gchar           *unit,
                                                                                          NSMShutdownType        mode,
                                                                                          guint                  timeout,
                                                                                          LAHandlerService      *service);
static void                  la_handler_service_handle_register_finish                   (GObject               *object,
                                                                                          GAsyncResult          *res,
                                                                                          gpointer               user_data);
static void                  la_handler_service_handle_consumer_lifecycle_request        (ShutdownConsumer      *interface,
                                                                                          GDBusMethodInvocation *invocation,
                                                                                          guint                  request,
                                                                                          guint                  request_id,
                                                                                          LAHandlerService      *service);
static void                  la_handler_service_handle_consumer_lifecycle_request_finish (JobManager            *manager,
                                                                                          const gchar           *unit,
                                                                                          const gchar           *result,
                                                                                          GError                *error,
                                                                                          gpointer               user_data);
static LAHandlerServiceData *la_handler_service_data_new                                 (LAHandlerService      *service,
                                                                                          GDBusMethodInvocation *invocation,
                                                                                          guint                  request_id);
static void                  la_handler_service_data_unref                               (LAHandlerServiceData  *data);



struct _LAHandlerServiceClass
{
  GObjectClass __parent__;
};

struct _LAHandlerService
{
  GObject          __parent__;

  GDBusConnection *connection;
  LAHandler       *interface;
  JobManager      *job_manager;

  /* Associations of shutdown consumers and their units */
  GHashTable      *units_to_consumers;
  GHashTable      *consumers_to_units;

  const gchar     *prefix;
  guint            index;
  guint            bus_name_id;

  /* connection to the NSM consumer interface */
  NSMConsumer     *nsm_consumer;
};

struct _LAHandlerServiceData
{
  GDBusMethodInvocation *invocation;
  LAHandlerService      *service;
  guint                  request_id;
};



G_DEFINE_TYPE (LAHandlerService, la_handler_service, G_TYPE_OBJECT);



static void
la_handler_service_class_init (LAHandlerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = la_handler_service_constructed;
  gobject_class->finalize = la_handler_service_finalize;
  gobject_class->get_property = la_handler_service_get_property;
  gobject_class->set_property = la_handler_service_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
                                   PROP_JOB_MANAGER,
                                   g_param_spec_object ("job-manager",
                                                        "Job Manager",
                                                        "The internal handler of Start()"
                                                        " and Stop() jobs",
                                                        TYPE_JOB_MANAGER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
la_handler_service_constructed (GObject *object)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);
  GError           *error = NULL;
  gchar            *log_text;

  /* connect to the node state manager */
  service->nsm_consumer =
    nsm_consumer_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
                                         "com.contiautomotive.NodeStateManager",
                                         "/com/contiautomotive/NodeStateManager/Consumer",
                                          NULL, &error);
  if (error != NULL)
    {
      log_text = g_strdup_printf ("Error occurred connecting to NSM Consumer: %s",
                                  error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (error);
    }
}



static void
la_handler_service_init (LAHandlerService *service)
{
  service->interface = la_handler_skeleton_new ();

  /* the number that follows the prefix in the shutdown consumer's object path,
   * making every shutdown consumer unique */
  service->index = 1;

  /* the string that precedes the index in the shutdown consumer's object path */
  service->prefix = "/org/genivi/BootManager1/ShutdownConsumer";

  /* initialize the association of shutdown consumers to units */
  service->units_to_consumers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                       (GDestroyNotify) g_free,
                                                       (GDestroyNotify) g_object_unref);
  service->consumers_to_units = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                       (GDestroyNotify) g_object_unref,
                                                       (GDestroyNotify) g_free);

  /* implement the Register() handler */
  g_signal_connect (service->interface, "handle-register",
                    G_CALLBACK (la_handler_service_handle_register),
                    service);
}



static void
la_handler_service_finalize (GObject *object)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  /* release the bus name */
  g_bus_unown_name (service->bus_name_id);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  /* release the job manager skeleton */
  g_object_unref (service->job_manager);

  /* release the shutdown consumers */
  g_hash_table_unref (service->units_to_consumers);
  g_hash_table_unref (service->consumers_to_units);

  (*G_OBJECT_CLASS (la_handler_service_parent_class)->finalize) (object);
}



static void
la_handler_service_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, service->connection);
      break;
    case PROP_JOB_MANAGER:
      g_value_set_object (value, service->job_manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
la_handler_service_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->connection = g_value_dup_object (value);
      break;
    case PROP_JOB_MANAGER:
      service->job_manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
la_handler_service_handle_register (LAHandler             *interface,
                                    GDBusMethodInvocation *invocation,
                                    const gchar           *unit,
                                    NSMShutdownType        mode,
                                    guint                  timeout,
                                    LAHandlerService      *service)
{
  ShutdownConsumer *consumer;
  GError           *error = NULL;
  gchar            *log_text;
  gchar            *object_path;

  g_return_val_if_fail (IS_LA_HANDLER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (unit != NULL && *unit != '\0', FALSE);
  g_return_val_if_fail (LA_HANDLER_IS_SERVICE (service), FALSE);

  /* find out if this unit is already registered with a shutdown consumer */
  if (g_hash_table_lookup (service->units_to_consumers, unit))
   {
      /* there already is a shutdown consumer for the unit, so ignore this request */
      la_handler_complete_register (interface, invocation);
      return TRUE;
   }

  /* create a new ShutdownConsumer and implement its LifecycleRequest method */
  consumer = shutdown_consumer_skeleton_new ();
  g_signal_connect (consumer, "handle-lifecycle-request",
                    G_CALLBACK (la_handler_service_handle_consumer_lifecycle_request),
                    service);

  /* associate the shutdown consumer with the unit name */
  g_hash_table_insert (service->units_to_consumers, g_strdup (unit),
                       g_object_ref (consumer));
  g_hash_table_insert (service->consumers_to_units, g_object_ref (consumer),
                       g_strdup (unit));

  /* export the shutdown consumer on the bus */
  object_path = g_strdup_printf ("%s/%u", service->prefix, service->index);
  g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (consumer),
                                    service->connection, object_path, &error);
  if (error != NULL)
    {
      log_text = g_strdup_printf ("Failed to export shutdown consumer on the bus: %s",
                                  error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (error);
    }

  /* temporarily store a reference to the LAHandlerService in the invocation object */
  g_object_set_data_full (G_OBJECT (invocation), "la-handler-service",
                          g_object_ref (service), (GDestroyNotify) g_object_unref);

  /* register the shutdown consumer with the NSM Consumer */
  nsm_consumer_call_register_shutdown_client (service->nsm_consumer,
                                              "org.genivi.BootManager1", object_path,
                                              mode, timeout, NULL,
                                              la_handler_service_handle_register_finish,
                                              invocation);

  g_free (object_path);

  /* increment the counter for our shutdown consumer object paths */
  service->index++;

  return TRUE;
}



static void
la_handler_service_handle_register_finish (GObject      *object,
                                           GAsyncResult *res,
                                           gpointer      user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (user_data);
  LAHandlerService      *service;
  NSMConsumer           *nsm_consumer = NSM_CONSUMER (object);
  GError                *error = NULL;
  gchar                 *log_text;
  gint                   error_code;

  g_return_if_fail (IS_NSM_CONSUMER (nsm_consumer));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));

  /* finish registering the shutdown client */
  nsm_consumer_call_register_shutdown_client_finish (nsm_consumer, &error_code, res,
                                                     &error);
  if (error != NULL)
    {
      log_text = g_strdup_printf ("Failed to register a shutdown consumer: %s",
                                  error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (error);
    }

  /* retrieve the LAHandlerService from the invocation object */
  service = g_object_get_data (G_OBJECT (invocation), "la-handler-service");

  /* notify the caller that we have handled the registration request */
  la_handler_complete_register (service->interface, invocation);
}



static void
la_handler_service_handle_consumer_lifecycle_request (ShutdownConsumer      *consumer,
                                                      GDBusMethodInvocation *invocation,
                                                      guint                  request,
                                                      guint                  request_id,
                                                      LAHandlerService      *service)
{
  LAHandlerServiceData *data;
  gchar                *unit_name;

  g_return_if_fail (IS_SHUTDOWN_CONSUMER (consumer));
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (LA_HANDLER_IS_SERVICE (service));

  /* look up the unit name associated with this shutdown consumer */
  unit_name = g_hash_table_lookup (service->consumers_to_units, consumer);

  if (unit_name != NULL)
    {
      data = la_handler_service_data_new (service, NULL, request_id);

      /* stop this unit now */
      job_manager_stop (service->job_manager, unit_name, NULL,
                        la_handler_service_handle_consumer_lifecycle_request_finish,
                        data);

      /* let the NSM know that we are working on this request */
      shutdown_consumer_complete_lifecycle_request (consumer, invocation, 7);
    }
  else
    {
      /* NSM asked us to shutdown a shutdown consumer we did not register;
       * make it aware by returning an error */
      shutdown_consumer_complete_lifecycle_request (consumer, invocation, 2);
    }
}



static void
la_handler_service_handle_consumer_lifecycle_request_finish (JobManager  *manager,
                                                             const gchar *unit,
                                                             const gchar *result,
                                                             GError      *error,
                                                             gpointer     user_data)
{
  LAHandlerServiceData *data = (LAHandlerServiceData *)user_data;
  GError               *err = NULL;
  gchar                *log_text;
  gint                  status = 1;

  g_return_if_fail (IS_JOB_MANAGER (manager));
  g_return_if_fail (unit != NULL && *unit != '\0');
  g_return_if_fail (result != NULL && *result != '\0');
  g_return_if_fail (data != NULL);

  /* log an error if shutting down the consumer has failed */
  if (error != NULL)
    {
      log_text = g_strdup_printf ("Failed to shutdown a shutdown consumer: %s",
                                  error->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);

      /* send an error back to the NSM */
      status = 2;
    }

  /* log an error if systemd failed to stop the consumer */
  if (g_strcmp0 (result, "failed") == 0)
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to shutdown a shutdown consumer"));

      /* send an error back to the NSM */
      status = 2;
    }

  /* let the NSM know that we have handled the lifecycle request */
  if (!nsm_consumer_call_lifecycle_request_complete_sync (data->service->nsm_consumer,
                                                          data->request_id, status,
                                                          NULL, NULL, &err))
    {
      log_text = g_strdup_printf ("Failed to notify Node State Manager about completed "
                                  "lifecycle request: %s", err->message);
      DLT_LOG (la_handler_context, DLT_LOG_ERROR, DLT_STRING (log_text));
      g_free (log_text);
      g_error_free (err);
    }

  la_handler_service_data_unref (data);
}



static LAHandlerServiceData *
la_handler_service_data_new (LAHandlerService      *service,
                             GDBusMethodInvocation *invocation,
                             guint                  request_id)
{
  LAHandlerServiceData *data;

  data = g_slice_new0 (LAHandlerServiceData);
  if (service != NULL)
    data->service = g_object_ref (service);
  if (invocation != NULL)
    data->invocation = g_object_ref (invocation);
  data->request_id = request_id;

  return data;
}



static void
la_handler_service_data_unref (LAHandlerServiceData *data)
{
  if (data == NULL)
    return;

  if (data->invocation != NULL)
    g_object_unref (data->invocation);
  if (data->service != NULL)
    g_object_unref (data->service);
  g_slice_free (LAHandlerServiceData, data);
}



LAHandlerService *
la_handler_service_new (GDBusConnection *connection,
                        JobManager      *job_manager)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (IS_JOB_MANAGER (job_manager), NULL);

  return g_object_new (LA_HANDLER_TYPE_SERVICE,
                       "connection", connection,
                       "job-manager", job_manager,
                       NULL);
}


gboolean
la_handler_service_start (LAHandlerService *service,
                           GError         **error)
{
  g_return_val_if_fail (LA_HANDLER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the org.genivi.BootManager1.LegacyAppHandler service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/org/genivi/BootManager1/LegacyAppHandler",
                                           error);
}
