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

#include <glib.h>

#include <node-startup-controller/glib-extensions.h>



GVariant *
g_variant_lookup_value_with_int_key (GVariant           *dictionary,
                                     const gint          key,
                                     const GVariantType *expected_type)
{
  GVariantIter iter;
  GVariant    *value = NULL;
  gint32       current_key;


  g_return_val_if_fail (dictionary != NULL, NULL);
  g_return_val_if_fail (expected_type != NULL, NULL);
  
  if (!g_variant_is_of_type (dictionary, G_VARIANT_TYPE ("a{ias}")))
    return NULL;

  g_variant_iter_init (&iter, dictionary);
  while (g_variant_iter_loop (&iter, "{i@as}", &current_key, &value))
    {
      if (current_key == key)
        {
          if (value != NULL && g_variant_is_of_type (value, expected_type))
            return value;
          else
            return NULL;
        }
    }

  return NULL;
}



gboolean
g_variant_string_array_has_string (GVariant    *array,
                                   const gchar *str)
{
  gboolean found = FALSE;
  gchar   *current_str;
  guint    n;

  for (n = 0; array != NULL && !found && n < g_variant_n_children (array); n++)
    {
      g_variant_get_child (array, n, "&s", &current_str);
      if (g_strcmp0 (str, current_str) == 0)
        found = TRUE;
    }

  return found;
}


gint
g_int_pointer_compare (gconstpointer a, gconstpointer b)
{
  return GPOINTER_TO_INT (a) - GPOINTER_TO_INT (b);
}
