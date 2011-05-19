/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <glib.h>

#include "milter-protocol.h"
#include "milter-reply-decoder.h"
#include "milter-enum-types.h"
#include "milter-marshalers.h"
#include "milter-macros-requests.h"
#include "milter-utils.h"
#include "milter-logger.h"

MILTER_IMPLEMENT_REPLY_SIGNALS(reply_init)
G_DEFINE_TYPE_WITH_CODE(MilterReplyDecoder,
                        milter_reply_decoder,
                        MILTER_TYPE_DECODER,
                        G_IMPLEMENT_INTERFACE(MILTER_TYPE_REPLY_SIGNALS,
                                              reply_init))

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static gboolean decode     (MilterDecoder *decoder,
                            GError       **error);


static void
milter_reply_decoder_class_init (MilterReplyDecoderClass *klass)
{
    GObjectClass *gobject_class;
    MilterDecoderClass *decoder_class;

    gobject_class = G_OBJECT_CLASS(klass);
    decoder_class = MILTER_DECODER_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    decoder_class->decode = decode;
}

static void
milter_reply_decoder_init (MilterReplyDecoder *decoder)
{
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_reply_decoder_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_reply_decoder_error_quark (void)
{
    return g_quark_from_static_string("milter-reply-decoder-error-quark");
}

MilterDecoder *
milter_reply_decoder_new (void)
{
    return MILTER_DECODER(g_object_new(MILTER_TYPE_REPLY_DECODER,
                                       NULL));
}

static gboolean
decode_reply_continue (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "CONTINUE reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][continue]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "continue");

    return TRUE;
}

static gint
find_space_character (const gchar *buffer, gint length)
{
    gint i;

    for (i = 0; i < length; i++) {
        if (buffer[i] == ' ')
            return i;
    }

    return -1;
}

static gboolean
decode_reply_reply_code (MilterDecoder *decoder, GError **error)
{
    gint null_character_point;
    gint space_character_point;
    const gchar *buffer;
    gint32 command_length;
    gchar *reply_code_str;
    gchar *extended_code;
    gchar *message = NULL;
    guint reply_code;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    buffer++;
    command_length--;

    if (!milter_decoder_check_command_length(
         buffer, command_length, 3,
         MILTER_DECODER_COMPARE_AT_LEAST, error,
         "Reply code doesn't have SMTP reply code")) {
        return FALSE;
    }

    reply_code_str = g_strndup(buffer, 3);
    if ((buffer[0] != '4' && buffer[0] != '5') ||
        !g_ascii_isdigit(buffer[1]) ||
        !g_ascii_isdigit(buffer[2])) {
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_LONG_COMMAND_LENGTH,
                    "SMTP reply code <%s> should be 4XX or 5XX.",
                    reply_code_str);
        g_free(reply_code_str);
        return FALSE;
    }

    reply_code = atoi(reply_code_str);
    g_free(reply_code_str);

    null_character_point =
        milter_decoder_decode_null_terminated_value(
            buffer, command_length, error,
            "Reply code isn't terminated by NULL on REPLY CODE reply");
    if (null_character_point <= 0)
        return FALSE;

    buffer += 3;
    command_length -= 3;
    if (buffer[0] == ' ') {
        buffer++;
        command_length--;
    }

    space_character_point = find_space_character(buffer, command_length);

    if (space_character_point <0) {
        extended_code = g_strdup(buffer);
    } else {
        extended_code = g_strndup(buffer, space_character_point);
        message = g_strdup(buffer + space_character_point + 1);
    }

    milter_debug("[%u] [reply-decoder][reply-code] <%u %s %s>",
                 milter_decoder_get_tag(decoder),
                 reply_code, extended_code, message);

    g_signal_emit_by_name(decoder, "reply-code",
                          reply_code, extended_code, message);

    if (extended_code)
        g_free(extended_code);
    if (message)
        g_free(message);

    return TRUE;
}

static gboolean
decode_reply_temporary_failure (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "TEMPORARY FAILURE reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][temporary-failure]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "temporary-failure");

    return TRUE;
}

static gboolean
decode_reply_reject (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "REJECT reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][reject]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "reject");

    return TRUE;
}

static gboolean
decode_reply_accept (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "ACCEPT reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][accept]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "accept");

    return TRUE;
}

static gboolean
decode_reply_discard (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "DISCARD reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][discard]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "discard");

    return TRUE;
}

static gboolean
decode_reply_add_header (MilterDecoder *decoder, GError **error)
{
    const gchar *name = NULL, *value = NULL;
    const gchar *buffer;
    gint32 command_length;
    gboolean decoded;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    decoded = milter_decoder_decode_header_content(buffer + 1,
                                                   command_length - 1,
                                                   &name, &value, error);
    if (!decoded)
        return FALSE;

    milter_debug("[%u] [reply-decoder][add-header] <%s>=<%s>",
                 milter_decoder_get_tag(decoder),
                 name, value);

    g_signal_emit_by_name(decoder, "add-header", name, value);

    return TRUE;
}

static gboolean
decode_header_content_with_index (const gchar *buffer, gint length,
                                  gint *index,
                                  const gchar **name,
                                  const gchar **value,
                                  GError **error)
{
    gint null_character_point;
    guint32 normalized_index;
    gchar *error_message;

    if (length < sizeof(guint32)) {
        return FALSE;
    }

    memcpy(&normalized_index, buffer, sizeof(guint32));
    *index = g_ntohl(normalized_index);
    buffer += sizeof(guint32);
    length -= sizeof(guint32);

    error_message = g_strdup_printf("name isn't terminated by NULL "
                                    "on header:");
    null_character_point =
        milter_decoder_decode_null_terminated_value(buffer, length, error,
                                                    error_message);
    g_free(error_message);
    if (null_character_point <= 0)
        return FALSE;

    *name = buffer;

    buffer += (null_character_point + 1);
    length -= (null_character_point + 1);

    error_message = g_strdup_printf("value isn't terminated by NULL "
                                    "on header: <%s>",
                                    *name);
    null_character_point =
        milter_decoder_decode_null_terminated_value(buffer, length, error,
                                                    error_message);
    g_free(error_message);
    if (null_character_point < 0)
        return FALSE;
    else if (null_character_point == 0)
        return TRUE;

    *value = buffer;

    return TRUE;
}

static gboolean
decode_reply_insert_header (MilterDecoder *decoder, GError **error)
{
    const gchar *name = NULL, *value = NULL;
    gint index = 0;
    const gchar *buffer;
    gint32 command_length;
    gboolean decoded;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    decoded = decode_header_content_with_index(buffer + 1, command_length + 1,
                                               &index, &name, &value, error);
    if (!decoded)
        return FALSE;

    milter_debug("[%u] [reply-decoder][insert-header] [%d]<%s>=<%s>",
                 milter_decoder_get_tag(decoder),
                 index, name, value);

    g_signal_emit_by_name(decoder, "insert-header", index, name, value);

    return TRUE;
}

static gboolean
decode_reply_change_header (MilterDecoder *decoder, GError **error)
{
    const gchar *name = NULL, *value = NULL;
    gint index = 0;
    const gchar *buffer;
    gint32 command_length;
    gboolean decoded;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    decoded = decode_header_content_with_index(buffer + 1, command_length + 1,
                                               &index, &name, &value, error);
    if (!decoded)
        return FALSE;

    if (value) {
        milter_debug("[%u] [reply-decoder][change-header] <%s>[%i]=<%s>",
                     milter_decoder_get_tag(decoder),
                     name, index, value);
        g_signal_emit_by_name(decoder, "change-header", name, index, value);
    } else {
        milter_debug("[%u] [reply-decoder][delete-header] <%s>[%i]",
                     milter_decoder_get_tag(decoder),
                     name, index);
        g_signal_emit_by_name(decoder, "delete-header", name, index);
    }

    return TRUE;
}

static gboolean
decode_two_string (const gchar *buffer, gint length,
                   const gchar **first, const gchar **second,
                   const gchar *first_value_name,
                   const gchar *second_value_name,
                   const gchar *protocol_name_for_error_message,
                   GError **error)
{
    gint null_character_point;
    gchar *error_message;

    error_message = g_strdup_printf("%s isn't terminated by NULL "
                                    "on %s",
                                    first_value_name,
                                    protocol_name_for_error_message);

    null_character_point =
        milter_decoder_decode_null_terminated_value(buffer, length, error,
                                                    error_message);
    g_free(error_message);
    if (null_character_point <= 0)
        return FALSE;
    *first = buffer;

    buffer += (null_character_point + 1);
    length -= (null_character_point + 1);
    if (length <= 0)
        return TRUE;

    error_message = g_strdup_printf("%s isn't terminated by NULL "
                                    "on %s: <%s>",
                                    second_value_name,
                                    protocol_name_for_error_message,
                                    *first);
    null_character_point =
        milter_decoder_decode_null_terminated_value(buffer, length, error,
                                                    error_message);
    g_free(error_message);
    if (null_character_point <= 0)
        return FALSE;
    *second = buffer;

    return TRUE;
}

static gboolean
decode_reply_change_from (MilterDecoder *decoder, GError **error)
{
    const gchar *from = NULL, *parameters = NULL;
    const gchar *buffer;
    gint32 command_length;
    gboolean decoded;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    decoded = decode_two_string(buffer + 1, command_length - 1,
                                &from, &parameters,
                                "From", "Parameter",
                                "CHANGE FROM reply",
                                error);

    if (!decoded)
        return FALSE;

    milter_debug("[%u] [reply-decoder][change-from] <%s>:<%s>",
                 milter_decoder_get_tag(decoder),
                 from,
                 MILTER_LOG_NULL_SAFE_STRING(parameters));

    g_signal_emit_by_name(decoder, "change-from", from, parameters);

    return TRUE;
}

static gboolean
decode_reply_add_recipient (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    milter_decoder_decode_null_terminated_value(
        buffer + 1, command_length - 1, error,
        "Recipient isn't terminated by NULL on ADD RECIPIENT reply");

    milter_debug("[%u] [reply-decoder][add-recipient] <%s>",
                 milter_decoder_get_tag(decoder),
                 buffer + 1);

    g_signal_emit_by_name(decoder, "add-recipient", buffer + 1, NULL);

    return TRUE;
}

static gboolean
decode_reply_add_recipient_with_parameters (MilterDecoder *decoder, GError **error)
{
    const gchar *recipient = NULL, *parameters = NULL;
    const gchar *buffer;
    gint32 command_length;
    gboolean decoded;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    decoded = decode_two_string(buffer + 1, command_length - 1,
                                &recipient, &parameters,
                                "Recipient", "Parameter",
                                "ADD RECIPIENT WITH PARAMETERS reply",
                                error);

    if (!decoded)
        return FALSE;

    milter_debug("[%u] [reply-decoder][add-recipient-with-parameters] <%s>:<%s>",
                 milter_decoder_get_tag(decoder),
                 recipient,
                 MILTER_LOG_NULL_SAFE_STRING(parameters));

    g_signal_emit_by_name(decoder, "add-recipient", recipient, parameters);

    return TRUE;
}

static gboolean
decode_reply_delete_recipient (MilterDecoder *decoder, GError **error)
{
    gint null_character_point;
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    null_character_point =
        milter_decoder_decode_null_terminated_value(
            buffer + 1, command_length - 1, error,
            "Recipient isn't terminated by NULL on DELETE RECIPIENT reply");
    if (null_character_point <= 0)
        return FALSE;

    milter_debug("[%u] [reply-decoder][delete-recipient] <%s>",
                 milter_decoder_get_tag(decoder),
                 buffer + 1);

    g_signal_emit_by_name(decoder, "delete-recipient", buffer + 1);

    return TRUE;
}

static gboolean
decode_reply_replace_body (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    milter_debug("[%u] [reply-decoder][replace-body] <%d>",
                 milter_decoder_get_tag(decoder),
                 command_length);

    g_signal_emit_by_name(decoder, "replace-body",
                  buffer + 1, command_length - 1);

    return TRUE;
}

static gboolean
decode_reply_progress (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "PROGRESS reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][progress]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "progress");

    return TRUE;
}

static gboolean
decode_reply_quarantine (MilterDecoder *decoder, GError **error)
{
    gint null_character_point;
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    null_character_point =
        milter_decoder_decode_null_terminated_value(
            buffer + 1, command_length - 1, error,
            "Reason isn't terminated by NULL on QUARANTINE reply");
    if (null_character_point <= 0)
        return FALSE;

    milter_debug("[%u] [reply-decoder][quarantine] <%s>",
                 milter_decoder_get_tag(decoder),
                 buffer + 1);

    g_signal_emit_by_name(decoder, "quarantine", buffer + 1);

    return TRUE;
}

static gboolean
decode_reply_connection_failure (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "CONNECTION FAILURE reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][connection-failure]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "connection-failure");

    return TRUE;
}

static gboolean
decode_reply_shutdown (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "SHUTDOWN reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][shutdown]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "shutdown");

    return TRUE;
}

static gboolean
decode_reply_skip (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    if (!milter_decoder_check_command_length(
            buffer + 1, command_length - 1, 0,
            MILTER_DECODER_COMPARE_EXACT, error,
            "SKIP reply"))
        return FALSE;

    milter_debug("[%u] [reply-decoder][skip]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "skip");

    return TRUE;
}

static gboolean
decode_macros_requests (const gchar *buffer, gsize length,
                        MilterMacrosRequests **macros_requests,
                        GError **error)
{
    gint null_character_point;
    gint i = 0;
    gboolean error_occurred = FALSE;
    MilterMacrosRequests *requests;

    if (length == 0)
        return TRUE;

    requests = milter_macros_requests_new();
    while (i < length) {
        guint32 stage, normalized_stage;
        MilterCommand command;
        gchar **symbol_names;

        if (!milter_decoder_check_command_length(
                buffer + i, length - i, sizeof(guint32),
                MILTER_DECODER_COMPARE_AT_LEAST, error,
                "Stage index doesn't exist in NEGOTIATE reply")) {
            error_occurred = TRUE;
            break;
        }

        memcpy(&normalized_stage, buffer + i, sizeof(guint32));
        stage = g_ntohl(normalized_stage);
        i += sizeof(guint32);

        null_character_point =
            milter_decoder_decode_null_terminated_value(
                buffer + i, length - i, error,
                "Symbol list isn't terminated by NULL on NEGOTIATE reply");
        if (null_character_point <= 0) {
            error_occurred = TRUE;
            break;
        }

        command = milter_utils_macro_stage_to_command(stage);
        if (command == '\0') {
            g_set_error(error,
                        MILTER_REPLY_DECODER_ERROR,
                        MILTER_REPLY_DECODER_ERROR_UNEXPECTED_MACRO_STAGE,
                        "unexpected macro stage was received: %04x", stage);
            error_occurred = TRUE;
            break;
        }
        symbol_names = g_strsplit(buffer + i, " ", -1);
        milter_macros_requests_set_symbols_string_array(
            requests, command, (const gchar **)symbol_names);
        g_strfreev(symbol_names);
        i += null_character_point + 1;
    }

    if (error_occurred) {
        g_object_unref(requests);
    } else {
        *macros_requests = requests;
    }

    return !error_occurred;
}

static gboolean
decode_reply_negotiate (MilterDecoder *decoder, GError **error)
{
    const gchar *buffer;
    gint32 command_length;
    MilterOption *option;
    gint processed_length;
    MilterMacrosRequests *macros_requests = NULL;

    command_length = milter_decoder_get_command_length(decoder);
    buffer = milter_decoder_get_buffer(decoder);

    option = milter_decoder_decode_negotiate(buffer,
                                             command_length,
                                             &processed_length,
                                             error);
    if (!option)
        return FALSE;

    if (!milter_decoder_check_command_length(buffer + processed_length,
                                             command_length - processed_length,
                                             0,
                                             MILTER_DECODER_COMPARE_AT_LEAST,
                                             error,
                                             "option negotiation command")) {
        g_object_unref(option);
        return FALSE;
    }

    if (!decode_macros_requests(buffer + processed_length,
                                command_length - processed_length,
                                &macros_requests,
                                error)) {
        g_object_unref(option);
        return FALSE;
    }

    milter_debug("[%u] [reply-decoder][negotiate]",
                 milter_decoder_get_tag(decoder));

    g_signal_emit_by_name(decoder, "negotiate-reply", option, macros_requests);
    g_object_unref(option);
    if (macros_requests)
        g_object_unref(macros_requests);

    return TRUE;
}

static gboolean
decode (MilterDecoder *decoder, GError **error)
{
    gboolean success = TRUE;
    gchar command;

    command = milter_decoder_get_buffer(decoder)[0];
    switch (command) {
      case MILTER_COMMAND_NEGOTIATE:
        success = decode_reply_negotiate(decoder, error);
        break;
      case MILTER_REPLY_CONTINUE:
        success = decode_reply_continue(decoder, error);
        break;
      case MILTER_REPLY_REPLY_CODE:
        success = decode_reply_reply_code(decoder, error);
        break;
      case MILTER_REPLY_TEMPORARY_FAILURE:
        success = decode_reply_temporary_failure(decoder, error);
        break;
      case MILTER_REPLY_REJECT:
        success = decode_reply_reject(decoder, error);
        break;
      case MILTER_REPLY_ACCEPT:
        success = decode_reply_accept(decoder, error);
        break;
      case MILTER_REPLY_DISCARD:
        success = decode_reply_discard(decoder, error);
        break;
      case MILTER_REPLY_ADD_HEADER:
        success = decode_reply_add_header(decoder, error);
        break;
      case MILTER_REPLY_INSERT_HEADER:
        success = decode_reply_insert_header(decoder, error);
        break;
      case MILTER_REPLY_CHANGE_HEADER:
        success = decode_reply_change_header(decoder, error);
        break;
      case MILTER_REPLY_CHANGE_FROM:
        success = decode_reply_change_from(decoder, error);
        break;
      case MILTER_REPLY_ADD_RECIPIENT:
        success = decode_reply_add_recipient(decoder, error);
        break;
      case MILTER_REPLY_ADD_RECIPIENT_WITH_PARAMETERS:
        success = decode_reply_add_recipient_with_parameters(decoder, error);
        break;
      case MILTER_REPLY_DELETE_RECIPIENT:
        success = decode_reply_delete_recipient(decoder, error);
        break;
      case MILTER_REPLY_REPLACE_BODY:
        success = decode_reply_replace_body(decoder, error);
        break;
      case MILTER_REPLY_PROGRESS:
        success = decode_reply_progress(decoder, error);
        break;
      case MILTER_REPLY_QUARANTINE:
        success = decode_reply_quarantine(decoder, error);
        break;
      case MILTER_REPLY_CONNECTION_FAILURE:
        success = decode_reply_connection_failure(decoder, error);
        break;
      case MILTER_REPLY_SHUTDOWN:
        success = decode_reply_shutdown(decoder, error);
        break;
      case MILTER_REPLY_SKIP:
        success = decode_reply_skip(decoder, error);
        break;
      default:
        g_set_error(error,
                    MILTER_DECODER_ERROR,
                    MILTER_DECODER_ERROR_UNEXPECTED_COMMAND,
                    "unexpected reply was received: %c", command);
        success = FALSE;
        break;
    }

    return success;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
