/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __NSM_ENUM_TYPES_H__
#define __NSM_ENUM_TYPES_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TYPE_NSM_SHUTDOWN_TYPE (nsm_shutdown_type_get_type ())

/**
 * NSMShutdownType:
 * @NSM_SHUTDOWN_TYPE_NOT    : Client not registered for any shutdown.
 * @NSM_SHUTDOWN_TYPE_NORMAL : Client registered for normal shutdown.
 * @NSM_SHUTDOWN_TYPE_FAST   : Client registered for fast shutdown.
 * @NSM_SHUTDOWN_TYPE_RUNUP  : The shutdown type "run up" can not be used
 *                             for registration. Clients which are
 *                             registered and have been shut down, will
 *                             automatically be informed about the "runup",
 *                             when the shutdown is cancelled.
 *
 * Shutdown modes supported by the Node State Manager.
 */
typedef enum /*< flags >*/
{
  NSM_SHUTDOWN_TYPE_NOT    = 0x00000000U,
  NSM_SHUTDOWN_TYPE_NORMAL = 0x00000001U,
  NSM_SHUTDOWN_TYPE_FAST   = 0x00000002U,
  NSM_SHUTDOWN_TYPE_RUNUP  = 0x80000000U, 
} NSMShutdownType;

GType nsm_shutdown_type_get_type (void) G_GNUC_CONST;



#define TYPE_NSM_ERROR_STATUS (nsm_error_status_get_type ())

/**
 * NSMErrorStatus:
 * @NSM_ERROR_STATUS_NOT_SET          : Initial value when error type is not set.
 * @NSM_ERROR_STATUS_OK               : Value when no error occurred.
 * @NSM_ERROR_STATUS_ERROR            : A general, non-specific error occurred.
 * @NSM_ERROR_STATUS_DBUS             : Error in D-Bus communication.
 * @NSM_ERROR_STATUS_INTERNAL         : Internal error (memory alloc. failed, etc.).
 * @NSM_ERROR_STATUS_PARAMETER        : A passed parameter was incorrect.
 * @NSM_ERROR_STATUS_WRONG_SESSION    : The requested session is unknown.
 * @NSM_ERROR_STATUS_RESPONSE_PENDING : Command accepted, return value delivered asynch.
 * @NSM_ERROR_STATUS_LAST             : Last error value to identify valid errors.
 *
 * Error codes supported by the Node State Manager.
 */
typedef enum /*< enum >*/
{
  NSM_ERROR_STATUS_NOT_SET,
  NSM_ERROR_STATUS_OK,
  NSM_ERROR_STATUS_ERROR,
  NSM_ERROR_STATUS_DBUS,
  NSM_ERROR_STATUS_INTERNAL,
  NSM_ERROR_STATUS_PARAMETER,
  NSM_ERROR_STATUS_WRONG_SESSION,
  NSM_ERROR_STATUS_RESPONSE_PENDING,
  NSM_ERROR_STATUS_LAST,
} NSMErrorStatus;

GType nsm_error_status_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* !__NSM_ENUM_TYPES_H__ */
