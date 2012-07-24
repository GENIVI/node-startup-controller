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

#include <dlt/dlt.h>

#include <boot-manager/boot-manager-application.h>
#include <boot-manager/boot-manager-dbus.h>
#include <boot-manager/boot-manager-service.h>
#include <boot-manager/la-handler-service.h>
#include <boot-manager/systemd-manager-dbus.h>
#include <boot-manager/target-startup-monitor.h>



DLT_DECLARE_CONTEXT (boot_manager_context);
DLT_DECLARE_CONTEXT (la_handler_context);



static void
unregister_dlt (void)
{
  DLT_UNREGISTER_CONTEXT (boot_manager_context);
  DLT_UNREGISTER_CONTEXT (la_handler_context);
  DLT_UNREGISTER_APP ();
}



int
main (int    argc,
      char **argv)
{
  BootManagerApplication *application;
  TargetStartupMonitor   *target_startup_monitor;
  BootManagerService     *boot_manager_service;
  LAHandlerService       *la_handler_service;
  GDBusConnection        *connection;
  SystemdManager         *systemd_manager;
  JobManager             *job_manager;
  GMainLoop              *main_loop;
  GError                 *error = NULL;

  /* register the application and context in DLT */
  DLT_REGISTER_APP ("BMGR", "GENIVI Boot Manager");
  DLT_REGISTER_CONTEXT (boot_manager_context, "MGR",
                        "Context of the boot manager itself");
  DLT_REGISTER_CONTEXT (la_handler_context, "LAH",
                        "Context of the legacy application handler that hooks legacy "
                        "applications up with the shutdown concept of the Node State "
                        "Manager");

  /* have DLT unregistered at exit */
  atexit (unregister_dlt);

  /* initialize the GType type system */
  g_type_init ();

  /* attempt to connect to D-Bus */
  connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (connection == NULL)
    {
      g_warning ("Failed to connect to the system bus: %s", error->message);

      /* clean up */
      g_error_free (error);

      return EXIT_FAILURE;
    }

  /* attempt to connect to the systemd manager */
  systemd_manager =
    systemd_manager_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            "org.freedesktop.systemd1",
                                            "/org/freedesktop/systemd1",
                                            NULL, &error);
  if (systemd_manager == NULL)
    {
      g_warning ("Failed to connect to the systemd manager: %s", error->message);

      /* clean up */
      g_error_free (error);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* subscribe to the systemd manager */
  if (!systemd_manager_call_subscribe_sync (systemd_manager, NULL, &error))
    {
      g_warning ("Failed to subscribe to the systemd manager: %s", error->message);

      /* clean up */
      g_error_free (error);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* instantiate the boot manager service implementation */
  boot_manager_service = boot_manager_service_new (connection);

  /* instantiate the job manager */
  job_manager = job_manager_new (connection, systemd_manager);

  /* start up the target startup monitor */
  target_startup_monitor = target_startup_monitor_new (systemd_manager);

  /* instantiate the legacy app handler */
  la_handler_service = la_handler_service_new (connection, job_manager);

  /* create the main loop */
  main_loop = g_main_loop_new (NULL, FALSE);

  /* create and run the main application */
  application = boot_manager_application_new (main_loop, connection, job_manager,
                                              la_handler_service, boot_manager_service);

  /* run the main loop */
  g_main_loop_run (main_loop);
  g_main_loop_unref (main_loop);

  /* release allocated objects */
  g_object_unref (application);
  g_object_unref (target_startup_monitor);
  g_object_unref (systemd_manager);
  g_object_unref (job_manager);
  g_object_unref (boot_manager_service);
  g_object_unref (connection);

  return EXIT_SUCCESS;
}
