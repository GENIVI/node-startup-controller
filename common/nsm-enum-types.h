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

typedef enum /*< enum >*/
{
  NSM_SHUTDOWN_TYPE_NOT    = 0x00000000U, /* Client not registered for any shutdown */
  NSM_SHUTDOWN_TYPE_NORMAL = 0x00000001U, /* Client registered for normal shutdown  */
  NSM_SHUTDOWN_TYPE_FAST   = 0x00000002U, /* Client registered for fast shutdown    */
  NSM_SHUTDOWN_TYPE_RUNUP  = 0x80000000U, /* The shutdown type "run up" can not be used
                                             for registration. Clients which are
                                             registered and have been shut down, will
                                             automatically be informed about the "runup",
                                             when the shut down is cancelled*/
} NSMShutdownType;

GType nsm_shutdown_type_get_type (void);

#define TYPE_NSM_ERROR_STATUS (nsm_error_status_get_type ())

typedef enum /*< enum >*/
{
  NSM_ERROR_STATUS_NOT_SET,          /* Initial value when error type is not set       */
  NSM_ERROR_STATUS_OK,               /* Value when no error occurred                   */
  NSM_ERROR_STATUS_ERROR,            /* A general, non-specific error occurred         */
  NSM_ERROR_STATUS_DBUS,             /* Error in D-Bus communication                   */
  NSM_ERROR_STATUS_INTERNAL,         /* Internal error (memory alloc. failed, etc.)    */
  NSM_ERROR_STATUS_PARAMETER,        /* A passed parameter was incorrect               */
  NSM_ERROR_STATUS_WRONG_SESSION,    /* The requested session is unknown.              */
  NSM_ERROR_STATUS_RESPONSE_PENDING, /* Command accepted, return value delivered asynch. */
  NSM_ERROR_STATUS_LAST,             /* Last error value to identify valid errors.     */
} NSMErrorStatus;

GType nsm_error_status_get_type (void);

G_END_DECLS

#endif /* !__NSM_ENUM_TYPES_H__ */
