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

/**
 * SECTION: mfapi
 * @title: libmilter/mfapi.h
 * @short_description: libmilter compatible API.
 *
 * libmilter/mfapi.h provides Sendmail's libmilter
 * compatible API. You can use this library instead of
 * Sendmail's libmilter. See also <link
 * linked="https://www.milter.org/developers/api/">API
 * Documentation on milter.org</link>.
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

#include <sys/types.h>
#include <sys/socket.h>

#include "mfdef.h"

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
 * Indicates response status of callback.
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
 * Here are the available @xxfi_flags values.
 *
 *   * %SMFIF_ADDHDRS: The milter may call
 *     smfi_addheader().
 *   * %SMFIF_CHGHDRS: The milter may call
 *     smfi_chgheader().
 *   * %SMFIF_CHGBODY: The milter may call
 *     smfi_chgbody().
 *   * %SMFIF_ADDRCPT: The milter may call
 *     smfi_addrcpt().
 *   * %SMFIF_ADDRCPT_PTR: The milter may call
 *     smfi_addrcpt_ptr().
 *   * %SMFIF_DELRCPT: The milter may call
 *     smfi_delrcpt().
 *   * %SMFIF_QUARANTINE: The milter may call
 *     smfi_quarantine().
 *   * %SMFIF_CHGROM: The milter may call
 *     smfi_chgrom().
 *   * %SMFIF_SETSMLIST: The milter may call
 *     smfi_setsymlist().  ymbolx().
 *
 * All callbacks (e.g. xxfi_helo(), xxfi_envfrom() and so
 * on) may be %NULL. If a callback is %NULL, the event is
 * just ignored the milter.
 *
 * They can be used together by bitwise OR.
 * </rd>
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
     * @address: the host address of the SMTP client.
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
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_CONN flag to
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
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_HELO flag to
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
     * @from: the envelope from address in SMTP's "MAIL
     *        FROM" command.
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
     *    Rejects the current envelope from address. A new
     *    envelope from may be specified.
     *
     * : %SMFIS_DISCARD
     *    Accepts the current message and discards it silently.
     *
     * : %SMFIS_ACCEPT
     *    Accepts the current message without further
     *    more processing.
     *
     * : %SMFIS_TEMPFAIL
     *    Rejects the envelope from address and the current
     *    connection with temporary failure. (i.e. 4xx
     *    status code in SMTP) A new envelope from address
     *    may be specified.
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_MAIL flag to
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
                                   char      **addresses);

    /**
     * xxfi_envrcpt:
     * @context: the context for the current milter session.
     * @recipient: the envelope recipient address in SMTP's
     *             "RCPT TO" command.
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
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_RCPT flag to
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
                                   char      **addresses);

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
     *    Rejects the the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_HDR flag to
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
     *    Rejects the the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_EOH flag to
     *    %smfiDesc_str's xxfi_flags.
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
     *    Rejects the the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_SKIP
     *    Skips further body
     *    processing. xxfi_eom() is called.
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_BODY flag to
     *    %smfiDesc_str's xxfi_flags.
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
     * This callback is called after all xxfi_eom() are
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
     *    Rejects the the current message with temporary
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
     * message between xxfi_envfrom and xxfi_eom(), should
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
     *    Rejects the the current message with temporary
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
     *    Rejects the the current message with temporary
     *    failure. (i.e. 4xx status code in SMTP)
     *
     * : %SMFIS_NOREPLY
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_UNKN flag to
     *    %smfiDesc_str's xxfi_flags.
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
     *    Doesn't send a reply back to MTA. The milter
     *    must set %SMFIP_NR_DATA flag to
     *    %smfiDesc_str's xxfi_flags.
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
     * @context: the context that received the signal.
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
 *   * "inet:PORT", "inet:PORT@HOST_NAME" or
 *     "inet:PORT@IP_ADDRESS": IPv4.
 *   * "inet6:PORT", "inet6:PORT@HOST_NAME" or
 *     "inet6:PORT@IP_ADDRESS": IPv6.
 * </rd>
 *
 * <rd>
 * Here are the fail conditions:
 *   * invalid format.
 *   * @connection_spec is NULL.
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

#define SMFIF_NONE        MILTER_ACTION_NONE
#define SMFIF_ADDHDRS     MILTER_ACTION_ADD_HEADERS
#define SMFIF_CHGBODY     MILTER_ACTION_CHANGE_BODY
#define SMFIF_MODBODY     SMFIF_CHGBODY
#define SMFIF_ADDRCPT     MILTER_ACTION_ADD_ENVELOPE_RECIPIENT
#define SMFIF_DELRCPT     MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT
#define SMFIF_CHGHDRS     MILTER_ACTION_CHANGE_HEADERS
#define SMFIF_QUARANTINE  MILTER_ACTION_QUARANTINE
#define SMFIF_CHGFROM     MILTER_ACTION_CHANGE_ENVELOPE_FROM
#define SMFIF_ADDRCPT_PAR MILTER_ACTION_ADD_ENVELOPE_RECIPIENT_WITH_PARAMETERS
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
