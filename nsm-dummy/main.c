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

#include <dlt/dlt.h>

#include <nsm-dummy/nsm-consumer-service.h>
#include <nsm-dummy/nsm-dummy-application.h>
#include <nsm-dummy/nsm-lifecycle-control-service.h>



DLT_DECLARE_CONTEXT (nsm_dummy_context);



static void
unregister_dlt (void)
{
  DLT_UNREGISTER_CONTEXT (nsm_dummy_context);
  DLT_UNREGISTER_APP ();
}



int
main (int    argc,
      char **argv)
{
  NSMLifecycleControlService *lifecycle_control_service;
  NSMDummyApplication        *application;
  NSMConsumerService         *consumer_service;
  GDBusConnection            *connection;
  GMainLoop                  *main_loop;
  GError                     *error = NULL;

  /* register the application and context in DLT */
  DLT_REGISTER_APP ("NSMD", "GENIVI Node State Manager Dummy");
  DLT_REGISTER_CONTEXT (nsm_dummy_context, "NSMC",
                        "Context of the node state manager dummy itself");

  /* have DLT unregistered at exit */
  atexit (unregister_dlt);

  /* initialize the GType type system */
  g_type_init ();

  /* attempt to connect to D-Bus */
  connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (connection == NULL)
    {
      DLT_LOG (nsm_dummy_context, DLT_LOG_FATAL,
               DLT_STRING ("Failed to connect to D-Bus:"),
               DLT_STRING (error->message));

      /* clean up */
      g_error_free (error);

      return EXIT_FAILURE;
    }

  /* instantiate the NSMLifecycleControlService implementation */
  lifecycle_control_service = nsm_lifecycle_control_service_new (connection);
  if (!nsm_lifecycle_control_service_start (lifecycle_control_service, &error))
    {
      DLT_LOG (nsm_dummy_context, DLT_LOG_FATAL,
               DLT_STRING ("Failed to start the lifecycle control service:"),
               DLT_STRING (error->message));

      /* clean up */
      g_error_free (error);
      g_object_unref (lifecycle_control_service);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* instantiate the NSMConsumerService implementation */
  consumer_service = nsm_consumer_service_new (connection);
  if (!nsm_consumer_service_start (consumer_service, &error))
    {
      DLT_LOG (nsm_dummy_context, DLT_LOG_FATAL,
               DLT_STRING ("Failed to start the consumer service:"),
               DLT_STRING (error->message));

      /* clean up */
      g_error_free (error);
      g_object_unref (consumer_service);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* create the main loop */
  main_loop = g_main_loop_new (NULL, FALSE);

  /* create the main application */
  application = nsm_dummy_application_new (main_loop, connection, consumer_service,
                                           lifecycle_control_service);

  /* run the main loop */
  g_main_loop_run (main_loop);
  g_main_loop_unref (main_loop);

  /* release allocated objects */
  g_object_unref (application);
  g_object_unref (lifecycle_control_service);
  g_object_unref (consumer_service);
  g_object_unref (connection);

  return EXIT_SUCCESS;
}
