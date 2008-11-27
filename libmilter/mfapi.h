/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __LIBMILTER_MFAPI_H__
#define __LIBMILTER_MFAPI_H__

#include <milter/client.h>

G_BEGIN_DECLS

#ifndef SMFI_VERSION
#  define SMFI_VERSION	0x01000001
#endif

#define SM_LM_VRS_MAJOR(v)	(((v) & 0x7f000000) >> 24)
#define SM_LM_VRS_MINOR(v)	(((v) & 0x007fff00) >> 8)
#define SM_LM_VRS_PLVL(v)	((v) & 0x0000007f)

#include <sys/types.h>
#include <sys/socket.h>

#include "mfdef.h"

#ifndef _SOCK_ADDR
#  define _SOCK_ADDR	struct sockaddr
#endif

#define MI_SUCCESS	0
#define MI_FAILURE	(-1)

typedef struct smfi_str  SMFICTX;
typedef struct smfi_str *SMFICTX_PTR;

typedef struct smfiDesc  smfiDesc_str;
typedef struct smfiDesc	*smfiDesc_ptr;

typedef int sfsistat;

#if HAVE_STDBOOL_H
#  include <stdbool.h>
#else
#  ifndef __cplusplus
#    ifndef bool
#      define bool int
#    endif
#    ifndef true
#      define true 1
#    endif
#    ifndef false
#      define false 0
#    endif
#    ifndef __bool_true_false_are_defined
#      define __bool_true_false_are_defined 1
#    endif
#  endif
#endif

struct smfiDesc
{
    char          *xxfi_name;    /* filter name */
    int            xxfi_version; /* version code -- do not change */
    unsigned long  xxfi_flags;   /* flags */

    sfsistat	(*xxfi_connect)   (SMFICTX     *context,
                                   char        *host_name,
                                   _SOCK_ADDR  *address);
    sfsistat	(*xxfi_helo)      (SMFICTX    *context,
                                   char       *fqdn);
    sfsistat	(*xxfi_envfrom)   (SMFICTX    *context,
                                   char      **addresses);
    sfsistat	(*xxfi_envrcpt)   (SMFICTX    *context,
                                   char      **addresses);
    sfsistat	(*xxfi_header)    (SMFICTX    *context,
                                   char       *name,
                                   char       *value);
    sfsistat	(*xxfi_eoh)       (SMFICTX    *context);
    sfsistat	(*xxfi_body)      (SMFICTX    *context,
                                   unsigned char *data,
                                   size_t     data_size);
    sfsistat	(*xxfi_eom)       (SMFICTX    *context);
    sfsistat	(*xxfi_abort)     (SMFICTX    *context);
    sfsistat	(*xxfi_close)     (SMFICTX    *context);
    sfsistat	(*xxfi_unknown)   (SMFICTX    *context,
                                   const char *command);
    sfsistat	(*xxfi_data)      (SMFICTX    *context);
    sfsistat	(*xxfi_negotiate) (SMFICTX    *context,
                                   unsigned long  flag0,
                                   unsigned long  flag1,
                                   unsigned long  flag2,
                                   unsigned long  flag3,
                                   unsigned long *flag0_output,
                                   unsigned long *flag1_output,
                                   unsigned long *flag2_output,
                                   unsigned long *flag3_output);
};

int smfi_opensocket (bool             remove_socket);
int smfi_register   (struct smfiDesc  description);
int smfi_main       (void);
int smfi_setbacklog (int              backlog);
int smfi_setdbg     (int              level);
int smfi_settimeout (int              timeout);
int smfi_setconn    (char            *connection_spec);
int smfi_stop       (void);
int smfi_version    (unsigned int    *major,
                     unsigned int    *minor,
                     unsigned int    *patch_level);

#define SMFIF_NONE        MILTER_ACTION_NONE
#define SMFIF_ADDHDRS     MILTER_ACTION_ADD_HEADERS
#define SMFIF_CHGBODY     MILTER_ACTION_CHANGE_BODY
#define SMFIF_MODBODY     SMFIF_CHGBODY
#define SMFIF_ADDRCPT     MILTER_ACTION_ADD_RCPT
#define SMFIF_DELRCPT     MILTER_ACTION_DELETE_RCPT
#define SMFIF_CHGHDRS     MILTER_ACTION_CHANGE_HEADERS
#define SMFIF_QUARANTINE  MILTER_ACTION_QUARANTINE
#define SMFIF_CHGFROM     MILTER_ACTION_CHANGE_FROM
#define SMFIF_ADDRCPT_PAR MILTER_ACTION_ADD_RCPT_WITH_PARAMETERS
#define SMFIF_SETSYMLIST  MILTER_ACTION_SET_SYMBOL_LIST

#define SMFIS_CONTINUE    MILTER_STATUS_CONTINUE
#define SMFIS_REJECT      MILTER_STATUS_REJECT
#define SMFIS_DISCARD     MILTER_STATUS_DISCARD
#define SMFIS_ACCEPT      MILTER_STATUS_ACCEPT
#define SMFIS_TEMPFAIL    MILTER_STATUS_TEMPORARY_FAILURE
#define SMFIS_NOREPLY     MILTER_STATUS_NO_REPLY
#define SMFIS_SKIP        MILTER_STATUS_SKIP
#define SMFIS_ALL_OPTS    MILTER_STATUS_ALL_OPTIONS

#define SMFIM_FIRST	0
#define SMFIM_CONNECT	MILTER_MACRO_STAGE_CONNECT
#define SMFIM_HELO	MILTER_MACRO_STAGE_HELO
#define SMFIM_ENVFROM	MILTER_MACRO_STAGE_MAIL
#define SMFIM_ENVRCPT	MILTER_MACRO_STAGE_RCPT
#define SMFIM_DATA	MILTER_MACRO_STATE_DATA
#define SMFIM_EOM	MILTER_MACRO_STAGE_END_OF_MESSAGE
#define SMFIM_EOH	MILTER_MACRO_STAGE_END_OF_HEADER
#define SMFIM_LAST	MILTER_MACRO_STAGE_END_OF_HEADER

char   *smfi_getsymval   (SMFICTX        *context,
                          char           *name);
int     smfi_setreply    (SMFICTX        *context,
                          char           *return_code,
                          char           *extended_code,
                          char           *message);
int     smfi_setmlreply  (SMFICTX        *context,
                          const char     *return_code,
                          const char     *extended_code,
                          ...);
int     smfi_addheader   (SMFICTX        *context,
                          char           *name,
                          char           *value);
int     smfi_chgheader   (SMFICTX        *context,
                          char           *name,
                          int             index,
                          char           *value);
int     smfi_insheader   (SMFICTX        *context,
                          int             index,
                          char           *name,
                          char           *value);
int     smfi_chgfrom     (SMFICTX        *context,
                          char           *mail,
                          char           *args);
int     smfi_addrcpt     (SMFICTX        *context,
                          char           *recipient);
int     smfi_addrcpt_par (SMFICTX        *context,
                          char           *recipient,
                          char           *args);
int     smfi_delrcpt     (SMFICTX        *context,
                          char           *recipient);
int     smfi_progress    (SMFICTX        *context);
int     smfi_replacebody (SMFICTX        *context,
                          unsigned char  *new_body,
                          int             new_body_size);
int     smfi_quarantine  (SMFICTX        *context,
                          char           *reason);
int     smfi_setpriv     (SMFICTX        *context,
                          void           *data);
void   *smfi_getpriv     (SMFICTX        *context);
int     smfi_setsymlist  (SMFICTX        *context,
                          int             where,
                          char           *macros);

G_END_DECLS

#endif /* __LIBMILTER_MFAPI_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
