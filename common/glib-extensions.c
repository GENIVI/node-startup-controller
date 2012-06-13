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

#include <common/glib-extensions.h>



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
