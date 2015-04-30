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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib.h>
#include <gio/gio.h>



static void
print_usage (const char *process_name)
{
  g_print ("Usage: \"%s <variant string> <file>\"\n"
           "i.e.    %s \"{0: ['foo.service']}\" \"temporary_file\"\n",
           process_name, process_name);
}



int
main (int    argc,
      char **argv)
{
  GFileOutputStream *stream;
  const gchar       *format = "a{ias}";
  GVariant          *variant;
  GError            *error = NULL;
  GFile             *outfile;

  g_type_init();

  if (argc != 3)
    {
      print_usage (argv[0]);
      return EXIT_FAILURE;
    }

  outfile = g_file_new_for_path (argv[2]);

  /* First argument is the string to parse, second is the filename to put it in */

  variant = g_variant_parse (G_VARIANT_TYPE (format), argv[1], NULL, NULL, &error);
  if (error != NULL)
    g_error ("Error occurred parsing variant: %s", error->message);

  stream = g_file_create (outfile, G_FILE_CREATE_NONE, NULL, &error);
  if (error != NULL)
    {
      if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_EXISTS)
        g_clear_error (&error);
      else
        g_error ("Error occurred creating file: %s", error->message);
    }
  else
    {
      g_object_unref (stream);
    }

  g_file_replace_contents (outfile, g_variant_get_data (variant),
                           g_variant_get_size (variant), NULL, FALSE, G_FILE_CREATE_NONE,
                           NULL, NULL, &error);
  if (error != NULL)
    g_error ("Error occurred writing variant: %s", error->message);

  g_variant_unref (variant);
  g_object_unref (outfile);

  return EXIT_SUCCESS;
}
