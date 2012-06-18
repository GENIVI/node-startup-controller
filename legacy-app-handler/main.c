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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include <legacy-app-handler/la-handler-application.h>
#include <legacy-app-handler/la-handler-service.h>



int
main (int    argc,
      char **argv)
{
  LAHandlerApplication *application;
  LAHandlerService     *service;
  GDBusConnection      *connection;
  GError               *error = NULL;
  gint                  exit_status = EXIT_SUCCESS;

  /* initialize the GType type system */
  g_type_init ();

  /* attempt to connect to D-Bus */
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (connection == NULL)
    {
      g_warning ("Failed to connect to D-Bus: %s", error->message);

      /* clean up */
      g_error_free (error);

      return EXIT_FAILURE;
    }

  /* instantiate the LegacyAppHandler service implementation */
  service = la_handler_service_new (connection);
  if (!la_handler_service_start (service, &error))
    {
      g_warning ("Failed to start the LegacyAppHandler service: %s", error->message);

      /* clean up */
      g_error_free (error);
      g_object_unref (service);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* create and run the main application */
  application = la_handler_application_new (service);
  exit_status = g_application_run (G_APPLICATION (application), 0, NULL);
  g_object_unref (application);

  /* release allocated objects */
  g_object_unref (service);
  g_object_unref (connection);

  return exit_status;
}
