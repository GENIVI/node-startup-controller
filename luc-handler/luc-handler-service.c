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

#include <common/glib-extensions.h>

#include <luc-handler/luc-handler-dbus.h>
#include <luc-handler/luc-handler-service.h>



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void      luc_handler_service_finalize              (GObject               *object);
static void      luc_handler_service_get_property          (GObject               *object,
                                                            guint                  prop_id,
                                                            GValue                *value,
                                                            GParamSpec            *pspec);
static void      luc_handler_service_set_property          (GObject               *object,
                                                            guint                  prop_id,
                                                            const GValue          *value,
                                                            GParamSpec            *pspec);
static gboolean  luc_handler_service_handle_register       (LUCHandler            *interface,
                                                            GDBusMethodInvocation *invocation,
                                                            GVariant              *apps,
                                                            LUCHandlerService     *service);
static gboolean  luc_handler_service_handle_deregister     (LUCHandler            *interface,
                                                            GDBusMethodInvocation *invocation,
                                                            GVariant              *apps,
                                                            LUCHandlerService     *service);
static gboolean  luc_handler_service_get_last_user_context (GValue                *value,
                                                            GVariant              *variant,
                                                            gpointer               user_data);
static GVariant *luc_handler_service_set_last_user_context (const GValue          *value,
                                                            const GVariantType    *expected_type,
                                                            gpointer               user_data);



struct _LUCHandlerServiceClass
{
  GObjectClass __parent__;
};

struct _LUCHandlerService
{
  GObject          __parent__;

  GDBusConnection *connection;
  LUCHandler      *interface;
  GSettings       *settings;
};



G_DEFINE_TYPE (LUCHandlerService, luc_handler_service, G_TYPE_OBJECT);



static void
luc_handler_service_class_init (LUCHandlerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = luc_handler_service_finalize;
  gobject_class->get_property = luc_handler_service_get_property;
  gobject_class->set_property = luc_handler_service_set_property;

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
luc_handler_service_init (LUCHandlerService *service)
{
  service->interface = luc_handler_skeleton_new ();

  /* create GSettings object for the LUC Handler schema */
  service->settings = g_settings_new ("org.genivi.LUCHandler1");

  /* bind the settings key to the D-Bus property so changes are
   * automatically synchronised; we need to use a custom binding
   * here because g_settings_bind() alone does not work with dicts */
  g_settings_bind_with_mapping (service->settings, "last-user-context",
                                service->interface, "last-user-context",
                                G_SETTINGS_BIND_DEFAULT,
                                luc_handler_service_get_last_user_context,
                                luc_handler_service_set_last_user_context,
                                g_object_ref (service),
                                (GDestroyNotify) g_object_unref);

  /* implement the Register() handler */
  g_signal_connect (service->interface, "handle-register",
                    G_CALLBACK (luc_handler_service_handle_register),
                    service);

  /* implement the Deregister() handler */
  g_signal_connect (service->interface, "handle-deregister",
                    G_CALLBACK (luc_handler_service_handle_deregister),
                    service);
}



static void
luc_handler_service_finalize (GObject *object)
{
  LUCHandlerService *service = LUC_HANDLER_SERVICE (object);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  /* release the settings object */
  g_object_unref (service->settings);

  (*G_OBJECT_CLASS (luc_handler_service_parent_class)->finalize) (object);
}



static void
luc_handler_service_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LUCHandlerService *service = LUC_HANDLER_SERVICE (object);

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
luc_handler_service_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LUCHandlerService *service = LUC_HANDLER_SERVICE (object);

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
luc_handler_service_handle_register (LUCHandler            *object,
                                     GDBusMethodInvocation *invocation,
                                     GVariant              *apps,
                                     LUCHandlerService     *service)
{
  GVariantBuilder dict_builder;
  GHashTableIter  hiter;
  GVariantIter    viter;
  GHashTable     *table;
  GPtrArray      *apps_array;
  GVariant       *current_context;
  GVariant       *current_apps;
  GVariant       *new_context;
  GVariant       *new_apps;
  GList          *lp;
  GList          *luc_types;
  gchar          *app;
  gchar          *luc_type;
  guint           n;

  g_return_val_if_fail (IS_LUC_HANDLER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (LUC_HANDLER_IS_SERVICE (service), FALSE);

  /* create a hash table to merge the current context and the newly registered apps */
  table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                 g_free, (GDestroyNotify) g_ptr_array_unref);

  /* obtain the current content of the last user context */
  current_context = luc_handler_get_last_user_context (service->interface);

  /* prepare app lists for all LUC types present in the current context */
  g_variant_iter_init (&viter, current_context);
  while (g_variant_iter_loop (&viter, "{sas}", &luc_type, NULL))
    {
      g_hash_table_insert (table, g_strdup (luc_type),
                           g_ptr_array_new_with_free_func (g_free));
    }

  /* add app lists for LUC types that are needed for the newly registered apps */
  g_variant_iter_init (&viter, apps);
  while (g_variant_iter_loop (&viter, "{sas}", &luc_type, NULL))
    {
      g_hash_table_insert (table, g_strdup (luc_type),
                           g_ptr_array_new_with_free_func (g_free));
    }

  /* we now have a hash table that has all LUC types involved in the
   * current context and in the newly registered apps */

  /* fill the app lists for each LUC type involved, make sure that newly registered
   * apps are added at the end so that they are "prioritized" */
  g_hash_table_iter_init (&hiter, table);
  while (g_hash_table_iter_next (&hiter, (gpointer) &luc_type, (gpointer) &apps_array))
    {
      /* get apps currently registered for the LUC type */
      current_apps = g_variant_lookup_value (current_context, luc_type,
                                             G_VARIANT_TYPE_STRING_ARRAY);

      /* get apps to be registered for the LUC type now */
      new_apps = g_variant_lookup_value (apps, luc_type, G_VARIANT_TYPE_STRING_ARRAY);

      /* add all currently registered apps unless they are to be registered now.
       * this is because we want apps to be registered now to be moved to the end
       * of the lists */
      for (n = 0; current_apps != NULL && n < g_variant_n_children (current_apps); n++)
        {
          g_variant_get_child (current_apps, n, "&s", &app);
          if (!g_variant_string_array_has_string (new_apps, app))
            g_ptr_array_add (apps_array, g_strdup (app));
        }

      /* add all newly registered apps at the end now */
      for (n = 0; new_apps != NULL && n < g_variant_n_children (new_apps); n++)
        {
          g_variant_get_child (new_apps, n, "&s", &app);
          g_ptr_array_add (apps_array, g_strdup (app));
        }

      /* release app lists for this LUC type */
      if (current_apps != NULL)
        g_variant_unref (current_apps);
      if (new_apps != NULL)
        g_variant_unref (new_apps);
    }

  /* construct a new dictionary variant for the new LUC */
  g_variant_builder_init (&dict_builder, G_VARIANT_TYPE ("a{sas}"));

  /* copy LUC types and corresponding apps over to the new context. make
   * sure the order (alphabetic) in which we add LUC types to the context
   * dict is always the same. this is helpful for testing */
  luc_types = g_hash_table_get_keys (table);
  luc_types = g_list_sort (luc_types, (GCompareFunc) g_strcmp0);
  for (lp = luc_types; lp != NULL; lp = lp->next)
    {
      /* get the apps list registered for this LUC type */
      apps_array = g_hash_table_lookup (table, lp->data);

      /* NULL-terminate the pointer so that we can treat it as a gchar ** */
      g_ptr_array_add (apps_array, NULL);

      /* add the LUC type and its apps to the new context */
      g_variant_builder_add (&dict_builder, "{s^as}", lp->data, apps_array->pdata);
    }

  /* free the LUC types and our LUC type to apps mapping */
  g_list_free (luc_types);
  g_hash_table_unref (table);

  /* apply the new last user context */
  new_context = g_variant_builder_end (&dict_builder);
  luc_handler_set_last_user_context (service->interface, new_context);

  /* notify the caller that we have handled the register request */
  g_dbus_method_invocation_return_value (invocation, NULL);
  return TRUE;
}



static gboolean
luc_handler_service_handle_deregister (LUCHandler            *object,
                                       GDBusMethodInvocation *invocation,
                                       GVariant              *apps,
                                       LUCHandlerService     *service)
{
  GVariantBuilder apps_builder;
  GVariantBuilder builder;
  GVariantIter    viter;
  GVariant       *current_context;
  GVariant       *new_context;
  GVariant       *current_apps;
  GVariant       *apps_to_remove;
  gchar          *app;
  gchar          *luc_type;
  guint           num_apps;
  guint           n;

  g_return_val_if_fail (IS_LUC_HANDLER (object), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (LUC_HANDLER_IS_SERVICE (service), FALSE);

  /* obtain the current content of the last user context */
  current_context = luc_handler_get_last_user_context (service->interface);

  /* initialise the builder for the new context */
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sas}"));

  /* copy the current context into the new context, drop all apps that
   * are supposed to be registered */
  g_variant_iter_init (&viter, current_context);
  while (g_variant_iter_loop (&viter, "{s@as}", &luc_type, &current_apps))
    {
      /* get a list of apps to be removed from this LUC type */
      apps_to_remove = g_variant_lookup_value (apps, luc_type,
                                               G_VARIANT_TYPE_STRING_ARRAY);

      if (apps_to_remove == NULL)
        {
          /* there are no apps to be removed from this LUC type, just copy
           * all currently registered apps over */
          g_variant_builder_add (&builder, "{s@as}", luc_type, current_apps);
        }
      else
        {
          /* we need to remove some apps from the current LUC type, so let's
           * build a new string array */
          g_variant_builder_init (&apps_builder, G_VARIANT_TYPE ("as"));

          /* add all apps currently registered for this LUC type to the string
           * array unless they are to be removed */
          for (n = 0, num_apps = 0;
               current_apps != NULL && n < g_variant_n_children (current_apps);
               n++)
            {
              g_variant_get_child (current_apps, n, "&s", &app);
              if (!g_variant_string_array_has_string (apps_to_remove, app))
                {
                  g_variant_builder_add (&apps_builder, "s", app);
                  num_apps++;
                }
            }

          /* add the LUC type and its apps to the new context unless there are none */
          if (num_apps > 0)
            g_variant_builder_add (&builder, "{sas}", luc_type, &apps_builder);

          /* free resources used for building the string array */
          g_variant_builder_clear (&apps_builder);
        }

      /* release the apps to be removed for this LUC type */
      if (apps_to_remove != NULL)
        g_variant_unref (apps_to_remove);
    }

  /* apply the new last user context */
  new_context = g_variant_builder_end (&builder);
  luc_handler_set_last_user_context (service->interface, new_context);

  /* notify the caller that we have handled the deregistration request */
  g_dbus_method_invocation_return_value (invocation, NULL);
  return TRUE;
}



static gboolean
luc_handler_service_get_last_user_context (GValue   *value,
                                           GVariant *variant,
                                           gpointer  user_data)
{
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);
  g_return_val_if_fail (variant != NULL, FALSE);

  g_value_set_variant (value, variant);
  return TRUE;
}



static GVariant *
luc_handler_service_set_last_user_context (const GValue       *value,
                                           const GVariantType *expected_type,
                                           gpointer           user_data)
{
  g_return_val_if_fail (G_IS_VALUE (value), FALSE);
  g_return_val_if_fail (expected_type != G_VARIANT_TYPE_DICTIONARY, FALSE);

  return g_value_dup_variant (value);
}



LUCHandlerService *
luc_handler_service_new (GDBusConnection *connection)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  return g_object_new (LUC_HANDLER_TYPE_SERVICE, "connection", connection, NULL);
}



gboolean
luc_handler_service_start (LUCHandlerService *service,
                           GError           **error)
{
  g_return_val_if_fail (LUC_HANDLER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the org.genivi.LUCHandler1 service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/org/genivi/LUCHandler1",
                                           error);
}
