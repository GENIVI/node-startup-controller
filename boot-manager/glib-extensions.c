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

#include <boot-manager/glib-extensions.h>



GVariant *
g_variant_lookup_value_with_int_key (GVariant           *dictionary,
                                     const gint          key,
                                     const GVariantType *expected_type)
{
  GVariantIter iter;
  GVariant    *entry;
  GVariant    *entry_key;
  GVariant    *tmp;
  GVariant    *value;
  gboolean     matches;

  g_return_val_if_fail (g_variant_is_of_type (dictionary, G_VARIANT_TYPE ("a{i*}")),
                        NULL);

  g_variant_iter_init (&iter, dictionary);

  while ((entry = g_variant_iter_next_value (&iter)))
    {
      entry_key = g_variant_get_child_value (entry, 0);
      matches = (g_variant_get_int32(entry_key) == key);
      g_variant_unref (entry_key);

      if (matches)
        break;

      g_variant_unref (entry);
    }

  if (entry == NULL)
    return NULL;

  value = g_variant_get_child_value (entry, 1);
  g_variant_unref (entry);

  if (g_variant_is_of_type (value, G_VARIANT_TYPE_VARIANT))
    {
      tmp = g_variant_get_variant (value);
      g_variant_unref (value);

      if (expected_type && !g_variant_is_of_type (tmp, expected_type))
        {
          g_variant_unref (tmp);
          tmp = NULL;
        }

      value = tmp;
    }

  g_return_val_if_fail (expected_type == NULL || value == NULL ||
                        g_variant_is_of_type (value, expected_type), NULL);

  return value;
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
