/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>
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
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __LIBMILTER_MFAPI_H__
#define __LIBMILTER_MFAPI_H__

#include <sys/types.h>
#include <sys/socket.h>

#include "mfdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SECTION: mfapi
 * @title: libmilter/mfapi.h
 * @short_description: libmilter compatible API.
 *
 * libmilter/mfapi.h provides Sendmail's libmilter
 * compatible API. You can use this library instead of
 * Sendmail's libmilter. See also <ulink
 * url="https://www.milter.org/developers/api/">API
 * Documentation on milter.org</ulink>.
 */

/**
 * SMFI_VERSION:
 *
 * libmilter version number.
 */
#ifndef SMFI_VERSION
#  define SMFI_VERSION	0x01000001
#endif

/**
 * SM_LM_VRS_MAJOR:
 * @version: the version number.
 *
 * Extracts major version number from @version.
 *
 * Returns: major version number.
 */
#define SM_LM_VRS_MAJOR(version)	(((version) & 0x7f000000) >> 24)

/**
 * SM_LM_VRS_MINOR:
 * @version: the version number.
 *
 * Extracts minor version number from @version.
 *
 * Returns: minor version number.
 */
#define SM_LM_VRS_MINOR(version)	(((version) & 0x007fff00) >> 8)

/**
 * SM_LM_VRS_PLVL:
 * @version: the version number.
 *
 * Extracts patch level from @version.
 *
 * Returns: patch level.
 */
#define SM_LM_VRS_PLVL(version)		((version) & 0x0000007f)

/**
 * _SOCK_ADDR:
 *
 * %_SOCK_ADDR is just an alias of 'struct sockaddr'.
 */
#ifndef _SOCK_ADDR
#  define _SOCK_ADDR	struct sockaddr
#endif

/**
 * MI_SUCCESS:
 *
 * Indicates an operation is done successfully.
 */
#define MI_SUCCESS	0

/**
 * MI_FAILURE:
 *
 * Indicates an operation is failed.
 */
#define MI_FAILURE	(-1)

/**
 * SMFICTX:
 *
 * Holds information for a milter session. %SMFICTX is
 * created for each milter session. %SMFICTX is the most
 * important object in libmilter API.
 */
typedef struct smfi_str  SMFICTX;

/**
 * SMFICTX_PTR:
 *
 * The pointer type of %SMFICTX.
 */
typedef struct smfi_str *SMFICTX_PTR;

/**
 * smfiDesc_str:
 *
 * Holds information for the milter. %smfiDesc_str is
 * used by smfi_register().
 */
typedef struct smfiDesc smfiDesc_str;

/**
 * smfiDesc_ptr:
 *
 * The pointer type of %smfiDesc_str.
 */
typedef struct smfiDesc	*smfiDesc_ptr;

/**
 * sfsistat:
 *
 * Indicates response status returned by callback.
 *
 * <rd>
 * Available response status is one of the followings:
 *
 *   * %SMFIS_CONTINUE
 *   * %SMFIS_REJECT
 *   * %SMFIS_DISCARD
 *   * %SMFIS_ACCEPT
 *   * %SMFIS_TEMPFAIL
 *   * %SMFIS_NOREPLY
 *   * %SMFIS_SKIP
 *   * %SMFIS_ALL_OPTS
 * </rd>
 */
typedef int sfsistat;

#if HAVE_STDBOOL_H
#  include <stdbool.h>
#else
#  ifndef __cplusplus
#    ifndef bool
/**
 * bool:
 *
 * The boolean type.
 */
#      define bool int
#    endif
#    ifndef true
/**
 * true:
 *
 * The true value.
 */
#      define true 1
#    endif
#    ifndef false
/**
 * false:
 *
 * The false value.
 */
#      define false 0
#    endif
#    ifndef __bool_true_false_are_defined
#      define __bool_true_false_are_defined 1
#    endif
#  endif
#endif

/**
 * smfiDesc:
 * @xxfi_name: The name of the milter.
 * @xxfi_version: The version code of the milter.
 * @xxfi_flags: The flags of the milter. Available
 *              values are SMFIF_*.
 * @xxfi_connect: See xxfi_connect().
 * @xxfi_helo: See xxfi_helo().
 * @xxfi_envfrom: See xxfi_envfrom().
 * @xxfi_envrcpt: See xxfi_envrcpt().
 * @xxfi_header: See xxfi_header().
 * @xxfi_eoh: See xxfi_eoh().
 * @xxfi_body: See xxfi_body().
 * @xxfi_eom: See xxfi_eom().
 * @xxfi_abort: See xxfi_abort().
 * @xxfi_close: See xxfi_close().
 * @xxfi_unknown: See xxfi_unknown().
 * @xxfi_data: See xxfi_data().
 * @xxfi_negotiate: See xxfi_negotiate().
 *
 * %smfiDesc is used by smfi_register() to register a milter.
 *
 * If @xxfi_name is %NULL, "Unknown" is used as default
 * milter name.
 *
 * @xxfi_version must be specified %SMFI_VERSION.
 *
 * <rd>
 * Here are the available @xxfi_flags values:
 *
 *   * %SMFIF_ADDHDRS: The milter may call smfi_addheader().
 *   * %SMFIF_CHGHDRS: The milter may call smfi_chgheader().
 *   * %SMFIF_CHGBODY: The milter may call smfi_chgbody().
 *   * %SMFIF_ADDRCPT: The milter may call smfi_addrcpt().
 *   * %SMFIF_ADDRCPT_PAR: The milter may call smfi_addrcpt_par().
 *   * %SMFIF_DELRCPT: The milter may call smfi_delrcpt().
 *   * %SMFIF_QUARANTINE: The milter may call smfi_quarantine().
 *   * %SMFIF_CHGFROM: The milter may call smfi_chgfrom().
 *   * %SMFIF_SETSMLIST: The milter may call smfi_setsymlist().
 *
 * They can be used together by bitwise OR.
 * </rd>
 *
 * All callbacks (e.g. xxfi_helo(), xxfi_envfrom() and so
 * on) may be %NULL. If a callback is %NULL, the event is
 * just ignored the milter.
 */
struct smfiDesc
{
    char          *xxfi_name;
    int            xxfi_version;
    unsigned long  xxfi_flags;

    /**
     * xxfi_connect:
     * @context: the context for the current milter session.
     * @host_name: the host name of the SMTP client.
     * @address: the address of the SMTP client.
     *
     * This callback is called at the start of each milter
     * session.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current connection.
     *
     * : %SMFIS_REJECT
     *    Rejects the current connection.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current connection without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current connection with a temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_CONN flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_connect">xxfi_connect
     * </ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_connect)   (SMFICTX     *context,
                                   char        *host_name,
                                   _SOCK_ADDR  *address);

    /**
     * xxfi_helo:
     * @context: the context for the current milter session.
     * @fqdn: the FQDN in SMTP's "HELO"/"EHLO" command.
     *
     * This callback is called on SMTP's "HELO"/"EHLO"
     * command.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current connection.
     *
     * : %SMFIS_REJECT
     *    Rejects the current connection.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current connection without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current connection with a temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_HELO flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_helo">
     * xxfi_helo</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_helo)      (SMFICTX    *context,
                                   char       *fqdn);

    /**
     * xxfi_envfrom:
     * @context: the context for the current milter session.
     * @arguments: the SMTP's "MAIL FROM" command
     *             arguments. The first element is sender
     *             address. %NULL-terminated.
     *
     * This callback is called on SMTP's "MAIL FROM" command.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current message.
     *
     * : %SMFIS_REJECT
     *    Rejects the current envelope from address and
     *    message. A new envelope from may be specified.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current envelope from address and
     *    message with temporary failure. (i.e. 4xx
     *    status code in SMTP) A new envelope from address
     *    may be specified.
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_MAIL flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_envfrom">
     * xxfi_envfrom</ulink>
     * on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_envfrom)   (SMFICTX    *context,
                                   char      **arguments);

    /**
     * xxfi_envrcpt:
     * @context: the context for the current milter session.
     * @arguments: the SMTP's "RCPT TO" command
     *             arguments. The first element is recipient
     *             address. %NULL-terminated.
     *
     * This callback is called on SMTP's "RCPT TO" command.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current message.
     *
     * : %SMFIS_REJECT
     *    Rejects the current envelope recipient
     *    address. Processing the current messages is
     *    continued.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current envelope recipient.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current envelope recipient address
     *    with temporary failure. (i.e. 4xx status code in
     *    SMTP) Processing the current message is continued.
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_RCPT flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_envrcpt">
     * xxfi_envrcpt</ulink>
     * on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_envrcpt)   (SMFICTX    *context,
                                   char      **arguments);

    /**
     * xxfi_header:
     * @context: the context for the current milter session.
     * @name: the header name.
     * @value: the header value. @value may include folded
     *         white space.
     *
     * This callback is called on each header. If
     * %SMFIP_HDR_LEADSPC flag is set to
     * %smfiDesc::xxfi_flags, @value have spaces after
     * header name and value separator ":".
     *
     * Example:
     *
     * |[
     * From: from &lt;from@example.com&gt;
     * To: recipient &lt;recipient@example.com&gt;
     * Subject:a subject
     * ]|
     *
     * With %SMFIP_HDR_LEADSPC:
     *
     * |[
     * "From", " from &lt;from@example.com&gt;"
     * "To", " recipient &lt;recipient@example.com&gt;"
     * "Subject", "a subject"
     * ]|
     *
     * Without %SMFIP_HDR_LEADSPC:
     *
     * |[
     * "From", "from &lt;from@example.com&gt;"
     * "To", "recipient &lt;recipient@example.com&gt;"
     * "Subject", "a subject"
     * ]|
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current message.
     *
     * : %SMFIS_REJECT
     *    Rejects the current message.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_HDR flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_header">
     * xxfi_header</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_header)    (SMFICTX    *context,
                                   char       *name,
                                   char       *value);

    /**
     * xxfi_eoh:
     * @context: the context for the current milter session.
     *
     * This callback is called on all headers are processed.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current message.
     *
     * : %SMFIS_REJECT
     *    Rejects the current message.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_EOH flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_eof">
     * xxfi_eof</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_eoh)       (SMFICTX    *context);

    /**
     * xxfi_body:
     * @context: the context for the current milter session.
     * @data: the body chunk.
     * @data_size: the size of @data.
     *
     * This callback is called on body data is received. This
     * callback is called zero or more times between
     * xxfi_eoh() and xxfi_eom().
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current message.
     *
     * : %SMFIS_REJECT
     *    Rejects the current message.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_SKIP
     *    Skips further body
     *    processing. xxfi_eom() is called.
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_BODY flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_body">
     * xxfi_body</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_body)      (SMFICTX    *context,
                                   unsigned char *data,
                                   size_t     data_size);

    /**
     * xxfi_eom:
     * @context: the context for the current milter session.
     *
     * This callback is called after all xxfi_body() are
     * called. All message modifications can be done only in
     * this callback. The modifications can be done with
     * smfi_addheader(), smfi_chgfrom() and so on.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current message.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_eom">
     * xxfi_eom</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_eom)       (SMFICTX    *context);

    /**
     * xxfi_abort:
     * @context: the context for the current milter session.
     *
     * This callback may be called at any time between
     * xxfi_envfrom() and xxfi_eom(). This callback is only
     * called if the milter causes an internal error and the
     * message processing isn't completed. For example, if
     * the milter has already returned %SMFIS_ACCEPT,
     * %SMFIS_REJECT, %SMFIS_DISCARD and %SMFIS_TEMPFAIL,
     * this callback will not be called.
     *
     * If the milter has any resources allocated for the
     * message between xxfi_envfrom() and xxfi_eom(), should
     * be freed in this callback. But any resources
     * allocated for the connection should not be freed in
     * this callback. It should be freed in xxfi_close().
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current message.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_abort">
     * xxfi_abort</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_abort)     (SMFICTX    *context);

    /**
     * xxfi_close:
     * @context: the context for the current milter session.
     *
     * This callback is called at the end of each miler
     * session. If the milter has any resources allocated
     * for the session free, should be freed in this
     * callback.
     *
     * All response statuses are ignored. Use %SMFIS_CONTINUE.
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_close">
     * xxfi_close</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_close)     (SMFICTX    *context);

    /**
     * xxfi_unknown:
     * @context: the context for the current milter session.
     * @command: the unknown SMTP command.
     *
     * This callback is called on unknown or unimplemented
     * SMTP command is sent.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_REJECT
     *    Rejects the current message.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_UNKN flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * Note that the unknown or unimplemented SMTP command
     * will always be rejected by MTA.
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_unknown">
     * xxfi_unknown</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_unknown)   (SMFICTX    *context,
                                   const char *command);

    /**
     * xxfi_data:
     * @context: the context for the current milter session.
     *
     * This callback is called on SMTP's "DATA" command.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_CONTINUE
     *    Continues processing the current message.
     *
     * : %SMFIS_REJECT
     *    Rejects the current message.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current envelope recipient.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA.
     *
     *    The milter must set %SMFIP_NR_DATA flag to
     *    %smfiDesc::xxfi_flags.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_data">
     * xxfi_data</ulink> on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_data)      (SMFICTX    *context);

    /**
     * xxfi_negotiate:
     * @context: the context for the current milter session.
     * @actions: the actions received from MTA.
     * @steps: the milter protocol steps offered from MTA.
     * @unused0: unused.
     * @unused1: unused.
     * @actions_output: the actions requested to MTA.
     * @steps_output: the milter protocol steps requested to MTA.
     * @unused0_output: unused.
     * @unused1_output: unused.
     *
     * This callback is called on negotiate request from
     * MTA.  If you want to change received @actions and
     * @steps from MTA, you set @actions_output and
     * @steps_output and returns %SMFIS_CONTINUE. If you
     * don't need to change @actions and @steps, you can
     * just return %SMFIS_ALL_OPTS.
     *
     * All available response statuses are the followings:
     *
     * <rd>
     * : %SMFIS_ALL_OPTS
     *    Enables all available actions and steps.
     *
     * : %SMFIS_REJECT
     *    Rejects the current session.
     *
     * : %SMFIS_CONTINUE
     *    Continues processing the current session with
     *    @actions_output and @steps_output.
     * </rd>
     *
     * See also <ulink
     * url="https://www.milter.org/developers/api/xxfi_negotiate">
     * xxfi_negotiate</ulink>
     * on milter.org.
     *
     * Returns: response status.
     */
    sfsistat	(*xxfi_negotiate) (SMFICTX    *context,
                                   unsigned long  actions,
                                   unsigned long  steps,
                                   unsigned long  unused0,
                                   unsigned long  unused1,
                                   unsigned long *actions_output,
                                   unsigned long *steps_output,
                                   unsigned long *unused0_output,
                                   unsigned long *unused1_output);
};

/**
 * smfi_opensocket:
 * @remove_socket: Whether or not trying to remove existing
 *                 UNIX domain socket before creating a new
 *                 socket.
 *
 * Creates the socket that is used to connect from MTA.
 *
 * Normally, smfi_opensocket() isn't needed to call
 * explicitly. The socket is created in smfi_main()
 * implicitly.
 *
 * <rd>
 * Here are the fail conditions:
 *   * smfi_register() hasn't called successfully.
 *   * smfi_setconn() hasn't called successfully.
 *   * smfi_opensocket() fails to remove existing UNIX
 *     domain socket if connection spec is for UNIX domain
 *     socket and @remove_socket is true.
 *   * smfi_opensocket() fails to create the new socket.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_opensocket">
 * smfi_opensocket</ulink>
 * on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 */
int smfi_opensocket (bool             remove_socket);

/**
 * smfi_register:
 * @description: The milter description.
 *
 * Registers the milter implementation as callbacks.
 *
 * <rd>
 * Here are the fail conditions:
 *   * incompatible xxfi_version.
 *   * illegal xxfi_flags value.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register</ulink>
 * on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 */
int smfi_register   (struct smfiDesc  description);

/**
 * smfi_main:
 *
 * Enters event loop. The milter should be initialized
 * with smfi_register(), smfi_setconn() and so on before
 * smfi_main() is called.
 *
 * <rd>
 * Here are the fail conditions:
 *   * failed to create the socket.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_main">
 * smfi_main</ulink>
 * on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 */
int smfi_main       (void);

/**
 * smfi_setbacklog:
 * @backlog: The maximum length of the pending connections queue.
 *
 * Sets the milters' backlog value that is used for
 * listen(2).
 *
 * <rd>
 * Here are the fail conditions:
 *   * @backlog <= 0.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_setbacklog">
 * smfi_setbacklog</ulink>
 * on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 */
int smfi_setbacklog (int              backlog);

/**
 * smfi_setdbg:
 * @level: The log level.
 *
 * Sets the log level. If @level is 0, turns off any log
 * message. The greater value is specified, more log
 * messages are output.
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_setdbg">
 * smfi_setdbg</ulink>
 * on milter.org.
 *
 * Returns: always %MI_SUCCESS.
 */
int smfi_setdbg     (int              level);

/**
 * smfi_settimeout:
 * @timeout: The timeout value in seconds.
 *
 * Sets the I/O timeout value in seconds. The default value
 * is 7210 seconds. @timeout == 0 means no wait, not "wait
 * forever".
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_settimeout">
 * smfi_settimeout</ulink>
 * on milter.org.
 *
 * Returns: always %MI_SUCCESS.
 */
int smfi_settimeout (int              timeout);

/**
 * smfi_setconn:
 * @connection_spec: The connection spec for communicating MTA.
 *
 * Sets the connection spec.
 *
 * <rd>
 * @connection_spec format is one of them:
 *   * "unix:/PATH/TO/SOCKET": UNIX domain socket.
 *   * "inet:PORT", "inet:PORT&commat;HOST_NAME" or
 *     "inet:PORT&commat;IP_ADDRESS": IPv4.
 *   * "inet6:PORT", "inet6:PORT&commat;HOST_NAME" or
 *     "inet6:PORT&commat;IP_ADDRESS": IPv6.
 * </rd>
 *
 * <rd>
 * Here are the fail conditions:
 *   * invalid format.
 *   * @connection_spec is %NULL.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_setconn">
 * smfi_setconn</ulink>
 * on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 */
int smfi_setconn    (char            *connection_spec);

/**
 * smfi_stop:
 *
 * Stops the milter. No more connections are accepted but
 * processing connections are continued until they are
 * finished.
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_stop">
 * smfi_stop</ulink>
 * on milter.org.
 *
 * Returns: always %MI_SUCCESS.
 */
int smfi_stop       (void);

/**
 * smfi_version:
 * @major: return location for major version.
 * @minor: return location for minor version.
 * @patch_level: return location for patch level.
 *
 * Gets the libmilter version. The version is for using
 * libmilter not built libmilter.
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_version">
 * smfi_version</ulink>
 * on milter.org.
 *
 * Returns: always %MI_SUCCESS.
 */
int smfi_version    (unsigned int    *major,
                     unsigned int    *minor,
                     unsigned int    *patch_level);

/**
 * SMFIF_ADDHDRS:
 *
 * The milter may call smfi_addheader().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_ADDHDRS     0x00000001L

/**
 * SMFIF_CHGBODY:
 *
 * The milter may call smfi_chgbody().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_CHGBODY     0x00000002L

/**
 * SMFIF_MODBODY:
 *
 * Same as %SMFIF_CHGBODY.
 */
#define SMFIF_MODBODY     SMFIF_CHGBODY

/**
 * SMFIF_ADDRCPT:
 *
 * The milter may call smfi_addrcpt().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_ADDRCPT     0x00000004L

/**
 * SMFIF_DELRCPT:
 *
 * The milter may call smfi_delrcpt().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_DELRCPT     0x00000008L

/**
 * SMFIF_CHGHDRS:
 *
 * The milter may call smfi_chgheader().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_CHGHDRS     0x00000010L

/**
 * SMFIF_QUARANTINE:
 *
 * The milter may call smfi_quarantine().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_QUARANTINE  0x00000020L

/**
 * SMFIF_CHGFROM:
 *
 * The milter may call smfi_chgfrom().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_CHGFROM     0x00000040L

/**
 * SMFIF_ADDRCPT_PAR:
 *
 * The milter may call smfi_addrcpt_par().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_ADDRCPT_PAR 0x00000080L

/**
 * SMFIF_SETSYMLIST:
 *
 * The milter may call smfi_setsymlist().
 *
 * See also %smfiDesc, xxfi_negotiate() and <ulink
 * url="https://www.milter.org/developers/api/smfi_register">
 * smfi_register on milter.org</ulink>.
 */
#define SMFIF_SETSYMLIST  0x00000100L


/**
 * SMFIS_CONTINUE:
 *
 * Continues the current process.
 *
 * See each callback (xxfi_connect(), xxfi_helo() and so
 * on) and <ulink
 * url="https://www.milter.org/developers/api/">
 * callback return status description</ulink> on milter.org.
 */
#define SMFIS_CONTINUE    0

/**
 * SMFIS_REJECT:
 *
 * Rejects the current processing target.
 *
 * See each callback (xxfi_connect(), xxfi_helo() and so
 * on) and <ulink
 * url="https://www.milter.org/developers/api/">
 * callback return status description</ulink> on milter.org.
 */
#define SMFIS_REJECT      1

/**
 * SMFIS_DISCARD:
 *
 * Accepts the current processing target and discards it
 * silently.
 *
 * See each callback (xxfi_envfrom(), xxfi_envrcpt() and so
 * on) and <ulink
 * url="https://www.milter.org/developers/api/">
 * callback return status description</ulink> on milter.org.
 */
#define SMFIS_DISCARD     2

/**
 * SMFIS_ACCEPT:
 *
 * Accepts the current processing target.
 *
 * See each callback (xxfi_connect(), xxfi_helo() and so
 * on) and <ulink
 * url="https://www.milter.org/developers/api/">
 * callback return status description</ulink> on milter.org.
 */
#define SMFIS_ACCEPT      3

/**
 * SMFIS_TEMPFAIL:
 *
 * Replies a temporary failure status for the current
 * processing target.
 *
 * See each callback (xxfi_connect(), xxfi_helo() and so
 * on) and <ulink
 * url="https://www.milter.org/developers/api/">
 * callback return status description</ulink> on milter.org.
 */
#define SMFIS_TEMPFAIL    4

/**
 * SMFIS_NOREPLY:
 *
 * Doesn't reply to the MTA.
 *
 * See each callback (xxfi_connect(), xxfi_helo() and so
 * on) and <ulink
 * url="https://www.milter.org/developers/api/">
 * callback return status description</ulink> on milter.org.
 */
#define SMFIS_NOREPLY     7

/**
 * SMFIS_SKIP:
 *
 * Skips the rest body chunks. This can be used only in
 * xxfi_body().
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/">
 * callback return status description</ulink> on milter.org.
 */
#define SMFIS_SKIP        8

/**
 * SMFIS_ALL_OPTS:
 *
 * Uses the all negotiate options received from the
 * MTA. This can be used only in xxfi_negotiate().
 */
#define SMFIS_ALL_OPTS    10

/**
 * SMFIM_CONNECT:
 *
 * Indicates the protocol stage for xxfi_connect().
 *
 * See smfi_setsymlist().
 **/
#define SMFIM_CONNECT	  0

/**
 * SMFIM_HELO:
 *
 * Indicates the protocol stage for xxfi_helo().
 *
 * See smfi_setsymlist().
 **/
#define SMFIM_HELO	  1

/**
 * SMFIM_ENVFROM:
 *
 * Indicates the protocol stage for xxfi_envfrom().
 *
 * See smfi_setsymlist().
 **/
#define SMFIM_ENVFROM	  2

/**
 * SMFIM_ENVRCPT:
 *
 * Indicates the protocol stage for xxfi_envrcpt().
 *
 * See smfi_setsymlist().
 **/
#define SMFIM_ENVRCPT	  3

/**
 * SMFIM_DATA:
 *
 * Indicates the protocol stage for xxfi_data().
 *
 * See smfi_setsymlist().
 **/
#define SMFIM_DATA	  4

/**
 * SMFIM_EOM:
 *
 * Indicates the protocol stage for xxfi_eom().
 *
 * See smfi_setsymlist().
 **/
#define SMFIM_EOM	  5

/**
 * SMFIM_EOH:
 *
 * Indicates the protocol stage for xxfi_eoh().
 *
 * See smfi_setsymlist().
 **/
#define SMFIM_EOH	  6

/**
 * SMFIP_NOCONNECT:
 *
 * Indicates that the MTA should not send information for
 * xxfi_connect().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NOCONNECT       0x00000001L

/**
 * SMFIP_NOHELO:
 *
 * Indicates that the MTA should not send information for
 * xxfi_helo().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NOHELO          0x00000002L

/**
 * SMFIP_NOMAIL:
 *
 * Indicates that the MTA should not send information for
 * xxfi_mail().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NOMAIL          0x00000004L

/**
 * SMFIP_NORCPT:
 *
 * Indicates that the MTA should not send information for
 * xxfi_rcpt().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NORCPT          0x00000008L

/**
 * SMFIP_BODY:
 *
 * Indicates that the MTA should not send information for
 * xxfi_body().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NOBODY          0x00000010L

/**
 * SMFIP_NOHDRS:
 *
 * Indicates that the MTA should not send information for
 * xxfi_header().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NOHDRS          0x00000020L

/**
 * SMFIP_NOEOH:
 *
 * Indicates that the MTA should not send information for
 * xxfi_eoh().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NOEOH           0x00000040L

/**
 * SMFIP_NR_HDR:
 *
 * Indicates that the milter don't reply on xxfi_header().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_HDR          0x00000080L

/**
 * SMFIP_NOHREPL:
 *
 * Same as %SMFIP_NR_HDR.
 **/
#define SMFIP_NOHREPL         SMFIP_NR_HDR

/**
 * SMFIP_NOUNKNOWN:
 *
 * Indicates that the MTA should not send information for
 * xxfi_unknown().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NOUNKNOWN       0x00000100L

/**
 * SMFIP_NODATA:
 *
 * Indicates that the MTA should not send information for
 * xxfi_data().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NODATA          0x00000200L

/**
 * SMFIP_SKIP:
 *
 * Indicates that the MTA supports %SMFIS_SKIP in xxfi_body().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_SKIP            0x00000400L

/**
 * SMFIP_RCPT_REJ:
 *
 * Indicates that the MTA should send rejected envelope
 * recipients and xxfi_envrcpt() is called for them.
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_RCPT_REJ        0x00000800L

/**
 * SMFIP_NR_CONN:
 *
 * Indicates that the milter don't reply on xxfi_connect().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_CONN         0x00001000L

/**
 * SMFIP_NR_HELO:
 *
 * Indicates that the milter don't reply on xxfi_helo().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_HELO         0x00002000L

/**
 * SMFIP_NR_MAIL:
 *
 * Indicates that the milter don't reply on xxfi_envfrom().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_MAIL         0x00004000L

/**
 * SMFIP_NR_RCPT:
 *
 * Indicates that the milter don't reply on xxfi_envrcpt().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_RCPT         0x00008000L

/**
 * SMFIP_NR_DATA:
 *
 * Indicates that the milter don't reply on xxfi_data().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_DATA         0x00010000L

/**
 * SMFIP_NR_UNKN:
 *
 * Indicates that the milter don't reply on xxfi_unknown().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_UNKN         0x00020000L

/**
 * SMFIP_NR_EOH:
 *
 * Indicates that the milter don't reply on xxfi_eoh().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_EOH          0x00040000L

/**
 * SMFIP_NR_BODY:
 *
 * Indicates that the milter don't reply on xxfi_body().
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_NR_BODY         0x00080000L

/**
 * SMFIP_HDR_LEADSPC:
 *
 * Indicates that xxfi_header() callback is received a
 * header value including spaces after ':'. See
 * xxfi_header() for examples.
 *
 * This flag can be got/set to @steps_output of xxfi_negotiate().
 **/
#define SMFIP_HDR_LEADSPC     0x00100000L

/**
 * smfi_getsymval:
 * @context: the context for the current milter session.
 * @name: the name of a macro.
 *
 * Gets a value of the macro named @name in the current milter
 * session context. smfi_getsymval() can be called in
 * xxfi_XXX callbacks. (e.g. xxfi_connect(), xxfi_helo(),
 * ...)
 *
 * @name should be enclosed in braces ("{" and "}") like
 * "{if_name}" except @name contains a character like "i".
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_getsymval">
 * smfi_getsymval</ulink> on milter.org. Sendmail's default
 * macros are also shown in the page.
 *
 * Returns: a value of the macro named @name if it exists,
 *          %NULL otherwise.
 **/
char   *smfi_getsymval   (SMFICTX        *context,
                          char           *name);

/**
 * smfi_setreply:
 * @context: the context for the current milter session.
 * @return_code: the three-digit SMTP error reply
 *               code. (RFC 2821) Only 4xx and 5xx are
 *               accepted.
 * @extended_code: the extended reply code (RFC 1893/2034),
 *                 or %NULL. Only 4.x.x and 5.x.x are
 *                 available.
 * @message: the text part of the SMTP reply, or %NULL.
 *
 * Sets the error reply code. 4xx @return_code is used on
 * %SMFIS_TEMPFAIL. 5xx @return_code is used on
 * %SMFIS_REJECT.
 *
 * <rd>
 * Here are the fail conditions:
 *   * @return_code is neither 4xx nor 5xx.
 *   * @extended_code is neither 4.x.x nor 5.x.x.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_setreply">
 * smfi_setreply</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_setreply    (SMFICTX        *context,
                          char           *return_code,
                          char           *extended_code,
                          char           *message);

/**
 * smfi_setmlreply:
 * @context: the context for the current milter session.
 * @return_code: the three-digit SMTP error reply
 *               code. (RFC 2821) Only 4xx and 5xx are
 *               accepted.
 * @extended_code: the extended reply code (RFC 1893/2034),
 *                 or %NULL. Only 4.x.x and 5.x.x are
 *                 available.
 * @...: the single lines of text part of the SMTP reply,
 *       terminated by %NULL.
 *
 * Sets the error reply code. 4xx @return_code is used on
 * %SMFIS_TEMPFAIL. 5xx @return_code is used on
 * %SMFIS_REJECT.
 *
 * <rd>
 * Here are the fail conditions:
 *   * @return_code is neither 4xx nor 5xx.
 *   * @extended_code is neither 4.x.x nor 5.x.x.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_setmlreply">
 * smfi_setmlreply</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_setmlreply  (SMFICTX        *context,
                          const char     *return_code,
                          const char     *extended_code,
                          ...);

/**
 * smfi_addheader:
 * @context: the context for the current milter session.
 * @name: the header name.
 * @value: the header value.
 *
 * Adds a header to the current message's header
 * list. smfi_addheader() can be called in xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @name is %NULL.
 *   * @value is %NULL.
 *   * called in except xxfi_eom().
 *   * %SMFIF_ADDHDRS flag isn't set in smfi_register() or
 *     xxfi_negotiate().
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_addheader">
 * smfi_addheader</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_addheader   (SMFICTX        *context,
                          char           *name,
                          char           *value);

/**
 * smfi_chgheader:
 * @context: the context for the current milter session.
 * @name: the header name.
 * @index: the index of headers that all of them are named
 *         @name. (1-based)
 * @value: the header value. Use %NULL to delete the target
 *         header.
 *
 * Changes a header that is located at @index in headers
 * that all of them are named @name. If @value is %NULL, the
 * header is deleted. smfi_chgheader() can be called in
 * xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @name is %NULL.
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * %SMFIF_CHGHDRS flag isn't set in smfi_register() or
 *     xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_chgheader">
 * smfi_chgheader</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_chgheader   (SMFICTX        *context,
                          char           *name,
                          int             index,
                          char           *value);

/**
 * smfi_insheader:
 * @context: the context for the current milter session.
 * @index: the index of inserted header. 0 means that the
 *         header is prepended.
 * @name: the header name.
 * @value: the header value.
 *
 * Inserts a header to @index in headers. smfi_insheader()
 * can be called in xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @name is %NULL.
 *   * @value is %NULL. FIXME: not-implemented yet.
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * %SMFIF_ADDHDRS flag isn't set in smfi_register() or
 *     xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_insheader">
 * smfi_insheader</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_insheader   (SMFICTX        *context,
                          int             index,
                          char           *name,
                          char           *value);

/**
 * smfi_chgfrom:
 * @context: the context for the current milter session.
 * @mail: the new sender address.
 * @arguments: the extra arguments for ESMTP.
 *
 * Changes a sender address. smfi_chgfrom() can be called in
 * xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @mail is %NULL. FIXME: not-implemented yet.
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * %SMFIF_CHGFROM flag isn't set in smfi_register() or
 *     xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_chgfrom">
 * smfi_chgfrom</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_chgfrom     (SMFICTX        *context,
                          char           *mail,
                          char           *arguments);

/**
 * smfi_addrcpt:
 * @context: the context for the current milter session.
 * @recipient: the new recipient address.
 *
 * Adds a recipient address. smfi_addrcpt() can be called in
 * xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @recipient is %NULL. FIXME: not-implemented yet.
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * %SMFIF_ADDRCPT flag isn't set in smfi_register() or
 *     xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_addrcpt">
 * smfi_addrcpt</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_addrcpt     (SMFICTX        *context,
                          char           *recipient);

/**
 * smfi_addrcpt_par:
 * @context: the context for the current milter session.
 * @recipient: the recipient address.
 * @arguments: the extra arguments for ESMTP.
 *
 * Adds a recipient address with extra ESMTP
 * arguments. smfi_addrcpt_par() can be called in
 * xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @recipient is %NULL. FIXME: not-implemented yet.
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * %SMFIF_ADDRCPT_PAR flag isn't set in smfi_register() or
 *     xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_addrcpt_par">
 * smfi_addrcpt_par</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_addrcpt_par (SMFICTX        *context,
                          char           *recipient,
                          char           *arguments);

/**
 * smfi_delrcpt:
 * @context: the context for the current milter session.
 * @recipient: the recipient address.
 *
 * Deletes a recipient address. smfi_delrcpt() can be called in
 * xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @recipient is %NULL. FIXME: not-implemented yet.
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * %SMFIF_DELRCPT flag isn't set in smfi_register() or
 *     xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_delrcpt">
 * smfi_delrcpt</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_delrcpt     (SMFICTX        *context,
                          char           *recipient);

/**
 * smfi_progress:
 * @context: the context for the current milter session.
 *
 * Keeps the current connection. smfi_progress() can be called in
 * xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_progress">
 * smfi_progress</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_progress    (SMFICTX        *context);

/**
 * smfi_replacebody:
 * @context: the context for the current milter session.
 * @new_body: the new body data.
 * @new_body_size: the size of @new_body.
 *
 * Replaces the current body data with
 * @new_body. smfi_replacebody() can be called in
 * xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @new_body == %NULL and @new_body_size > 0. FIXME:
 *     not-implemented yet.
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * %SMFIF_CHGBODY flag isn't set in smfi_register() or
 *     xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_replacebody">
 * smfi_replacebody</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_replacebody (SMFICTX        *context,
                          unsigned char  *new_body,
                          int             new_body_size);

/**
 * smfi_quarantine:
 * @context: the context for the current milter session.
 * @reason: the quarantine reason.
 *
 * Quarantines the current message with
 * @reason. smfi_quarantine() can be called in xxfi_eom().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @reason is %NULL or empty. FIXME: not-implemented yet.
 *   * called in except xxfi_eom(). FIXME: not-implemented yet.
 *   * %SMFIF_QUARANTINE flag isn't set in smfi_register() or
 *     xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_quarantine">
 * smfi_quarantine</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_quarantine  (SMFICTX        *context,
                          char           *reason);

/**
 * smfi_setpriv:
 * @context: the context for the current milter session.
 * @data: the private data.
 *
 * Sets the private data.
 *
 * <rd>
 * Here are the fail conditions:
 *   * @context is invalid. FIXME: not-implemented yet.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_setpriv">
 * smfi_setpriv</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_setpriv     (SMFICTX        *context,
                          void           *data);

/**
 * smfi_getpriv:
 * @context: the context for the current milter session.
 *
 * Gets the private data.
 *
 * <rd>
 * Here are the fail conditions:
 *   * @context is invalid. FIXME: not-implemented yet.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_getpriv">
 * smfi_getpriv</ulink> on milter.org.
 *
 * Returns: the private data set by smfi_setpriv() or %NULL.
 **/
void   *smfi_getpriv     (SMFICTX        *context);

/**
 * smfi_setsymlist:
 * @context: the context for the current milter session.
 * @state: the state defined as SMFIM_XXX like %SMFIM_CONNECT.
 * @macros: the space separated macro names like
 *          "{rcpt_mailer} {rcpt_host}".
 *
 * Sets the list of requested macros. smfi_setsymlist() can
 * be called in xxfi_negotiate().
 *
 * <rd>
 * Here are the fail conditions:
 *   * @state is not a valid value. FIXME: not-implemented yet.
 *   * @macros is %NULL or empty. FIXME: not-implemented yet.
 *   * the macro list for @state has been set before. FIXME:
 *     not-implemented yet.
 *   * called in except xxfi_negotiate(). FIXME: not-implemented yet.
 *   * network error is occurred.
 * </rd>
 *
 * See also <ulink
 * url="https://www.milter.org/developers/api/smfi_setsymlist">
 * smfi_setsymlist</ulink> on milter.org.
 *
 * Returns: %MI_SUCCESS if success, %MI_FAILURE otherwise.
 **/
int     smfi_setsymlist  (SMFICTX        *context,
                          int             state,
                          char           *macros);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMILTER_MFAPI_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
