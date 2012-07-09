/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <glib.h>
#include <gio/gio.h>

gint boot_manager_handle_command_line (int              argc,
                                       char           **argv,
                                       GDBusConnection *connection);
