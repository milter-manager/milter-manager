/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <gcutter.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <milter-test-utils.h>
#undef shutdown

#include <libmilter/libmilter-compatible.h>

void test_private (void);
void test_getsymval (void);
void data_setreply (void);
void test_setreply (gconstpointer _data);
void test_setmlreply (void);
void test_addheader (void);
void test_chgheader (void);
void test_insheader (void);
void data_chgfrom (void);
void test_chgfrom (gconstpointer data);
void test_addrcpt (void);
void test_addrcpt_par (void);
void data_delrcpt (void);
void test_delrcpt (gconstpointer data);
void test_progress (void);
void test_quarantine (void);
void test_replacebody (void);

static MilterEventLoop *loop;

static GError *actual_error;

static SmfiContext *context;
static MilterClientContext *client_context;
static MilterCommandEncoder *command_encoder;
static MilterReplyEncoder *reply_encoder;

static GIOChannel *channel;
static MilterWriter *writer;
static GString *expected_output;

static gboolean send_progress;

static gchar *macro_name;
static gchar *macro_value;

static gboolean add_header;
static gboolean insert_header;
static gboolean change_header;

static gchar *header_name;
static gchar *header_value;
static gint header_index;

static gboolean do_change_from;
static gboolean do_add_recipient;
static gboolean do_add_recipient_with_parameters;
static gboolean do_delete_recipient;

static gchar *change_from;
static gchar *change_from_parameters;
static gchar *add_recipient;
static gchar *add_recipient_parameters;
static gchar *delete_recipient;

static GString *body;
static int replace_body_result;

static gchar *quarantine_reason;

static gchar *reply_return_code;
static gchar *reply_extended_code;
static gchar *reply_message;
static int set_reply_result;
static sfsistat reply_status;

static gboolean set_mlreply;
static int set_mlreply_result;

static sfsistat
xxfi_connect (SMFICTX *context, char *host_name, _SOCK_ADDR *address)
{
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_helo (SMFICTX *context, char *fqdn)
{
    if (send_progress)
        smfi_progress(context);

    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_envfrom (SMFICTX *context, char **addresses)
{
    macro_value = g_strdup(smfi_getsymval(context, macro_name));
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_envrcpt (SMFICTX *context, char **addresses)
{
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_header (SMFICTX *context, char *name, char *value)
{
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_eoh (SMFICTX *context)
{
    if (set_mlreply) {
        set_mlreply_result =
            smfi_setmlreply(context,
                            "450", "4.2.0",
                            "Rejected by Greylist",
                            "See http://example.com/ for more info",
                            NULL);
        return SMFIS_TEMPFAIL;
    }

    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_body (SMFICTX *context, unsigned char *data, size_t data_size)
{
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_eom (SMFICTX *context)
{
    if (add_header)
        smfi_addheader(context, header_name, header_value);

    if (insert_header)
        smfi_insheader(context, header_index, header_name, header_value);

    if (change_header)
        smfi_chgheader(context, header_name, header_index, header_value);

    if (do_change_from)
        smfi_chgfrom(context, change_from, change_from_parameters);

    if (do_add_recipient)
        smfi_addrcpt(context, add_recipient);

    if (do_add_recipient_with_parameters)
        smfi_addrcpt_par(context, add_recipient, add_recipient_parameters);

    if (do_delete_recipient)
        smfi_delrcpt(context, delete_recipient);

    if (body)
        replace_body_result = smfi_replacebody(context,
                                               (unsigned char *)body->str,
                                               body->len);
    if (quarantine_reason)
        smfi_quarantine(context, quarantine_reason);

    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_abort (SMFICTX *context)
{
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_close (SMFICTX *context)
{
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_unknown (SMFICTX *context, const char *command)
{
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_data (SMFICTX *context)
{
    if (reply_return_code) {
        set_reply_result = smfi_setreply(context,
                                         reply_return_code,
                                         reply_extended_code,
                                         reply_message);
        return reply_status;
    }

    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_negotiate (SMFICTX *context,
                unsigned long flag0, unsigned long flag1,
                unsigned long flag2, unsigned long flag3,
                unsigned long *flag0_output, unsigned long *flag1_output,
                unsigned long *flag2_output, unsigned long *flag3_output)
{
    return SMFIS_ALL_OPTS;
}

static struct smfiDesc smfilter = {
    "test milter",
    SMFI_VERSION,
    SMFIF_ADDHDRS | SMFIF_CHGHDRS,
    xxfi_connect,
    xxfi_helo,
    xxfi_envfrom,
    xxfi_envrcpt,
    xxfi_header,
    xxfi_eoh,
    xxfi_body,
    xxfi_eom,
    xxfi_abort,
    xxfi_close,
    xxfi_unknown,
    xxfi_data,
    xxfi_negotiate,
};

static void
cb_error (MilterErrorEmittable *emittable, GError *error)
{
    actual_error = g_error_copy(error);
}

static void
setup_signals (MilterClientContext *context)
{
    g_signal_connect(context, "error", G_CALLBACK(cb_error), NULL);
}

void
cut_setup (void)
{
    GError *error = NULL;

    smfi_register(smfilter);

    loop = milter_test_event_loop_new();

    actual_error = NULL;

    client_context = milter_client_context_new(NULL);
    setup_signals(client_context);
    milter_agent_set_event_loop(MILTER_AGENT(client_context), loop);
    expected_output = NULL;

    context = smfi_context_new(client_context);

    channel = gcut_string_io_channel_new(NULL);
    g_io_channel_set_encoding(channel, NULL, NULL);
    writer = milter_writer_io_channel_new(channel);
    milter_agent_set_writer(MILTER_AGENT(client_context), writer);
    milter_agent_start(MILTER_AGENT(client_context), &error);
    gcut_assert_error(error);

    command_encoder = MILTER_COMMAND_ENCODER(milter_command_encoder_new());
    reply_encoder = MILTER_REPLY_ENCODER(milter_reply_encoder_new());

    send_progress = FALSE;

    add_header = FALSE;
    insert_header = FALSE;
    change_header = FALSE;

    header_name = NULL;
    header_value = NULL;
    header_index = 0;

    macro_name = NULL;
    macro_value = NULL;

    do_change_from = FALSE;
    do_add_recipient = FALSE;
    do_add_recipient_with_parameters = FALSE;
    do_delete_recipient = FALSE;

    change_from = NULL;
    change_from_parameters = NULL;
    add_recipient = NULL;
    add_recipient_parameters = NULL;
    delete_recipient = NULL;

    body = NULL;
    replace_body_result = 0;

    quarantine_reason = NULL;

    reply_return_code = NULL;
    reply_extended_code = NULL;
    reply_message = NULL;
    set_reply_result = MI_FAILURE;
    reply_status = SMFIS_CONTINUE;

    set_mlreply = FALSE;
    set_mlreply_result = MI_FAILURE;
}

void
cut_teardown (void)
{
    if (context)
        g_object_unref(context);
    if (client_context)
        g_object_unref(client_context);

    if (channel)
        g_io_channel_unref(channel);
    if (writer)
        g_object_unref(writer);
    if (loop)
        g_object_unref(loop);
    if (actual_error)
        g_error_free(actual_error);
    if (expected_output)
        g_string_free(expected_output, TRUE);

    if (command_encoder)
        g_object_unref(command_encoder);
    if (reply_encoder)
        g_object_unref(reply_encoder);

    if (macro_name)
        g_free(macro_name);
    if (macro_value)
        g_free(macro_value);

    if (change_from)
        g_free(change_from);
    if (change_from_parameters)
        g_free(change_from_parameters);
    if (add_recipient)
        g_free(add_recipient);
    if (add_recipient_parameters)
        g_free(add_recipient_parameters);
    if (delete_recipient)
        g_free(delete_recipient);

    if (body)
        g_string_free(body, TRUE);

    if (quarantine_reason)
        g_free(quarantine_reason);

    if (reply_return_code)
        g_free(reply_return_code);
    if (reply_extended_code)
        g_free(reply_extended_code);
    if (reply_message)
        g_free(reply_message);
}

static void
pump_all_events (void)
{
    GError *error;

    milter_test_pump_all_events(loop);
    error = actual_error;
    actual_error = NULL;
    gcut_assert_error(error);
}

static GError *
feed (const gchar *packet, gsize packet_size)
{
    GError *error = NULL;

    milter_client_context_feed(client_context, packet, packet_size, &error);
    if (!error) {
        if (MILTER_IS_LIBEV_EVENT_LOOP(loop))
            cut_omit("MilterLibevEventLoop doesn't support GCutStringIOChannel.");
        pump_all_events();
    }

    return error;
}

static void
feed_negotiate (void)
{
    MilterOption *option;
    MilterActionFlags action;
    MilterStepFlags step;
    const gchar *packet;
    gsize packet_size;
    GError *error = NULL;

    action = gcut_flags_get_all(MILTER_TYPE_ACTION_FLAGS, &error);
    gcut_assert_error(error);
    step = gcut_flags_get_all(MILTER_TYPE_STEP_FLAGS, &error);
    gcut_assert_error(error);
    option = milter_option_new(2, action, step);
    milter_command_encoder_encode_negotiate(command_encoder,
                                            &packet, &packet_size,
                                            option);
    g_object_unref(option);
    gcut_assert_error(feed(packet, packet_size));
}

static void
feed_connect (void)
{
    struct sockaddr *address;
    socklen_t address_size;
    const gchar *packet;
    gsize packet_size;
    GError *error = NULL;

    milter_connection_parse_spec("inet:2929@192.168.1.5",
                                 NULL, &address, &address_size, &error);
    gcut_assert_error(error);
    milter_command_encoder_encode_connect(command_encoder,
                                          &packet, &packet_size,
                                          "mx.example.com",
                                          address, address_size);
    g_free(address);
    gcut_assert_error(feed(packet, packet_size));
}

static void
feed_helo (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_helo(command_encoder,
                                       &packet, &packet_size,
                                       "mx.example.com");
    gcut_assert_error(feed(packet, packet_size));
}

static void
feed_envelope_from (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_envelope_from(command_encoder,
                                                &packet, &packet_size,
                                                "sender@example.com");
    gcut_assert_error(feed(packet, packet_size));
}

static void
feed_envelope_recipient (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_envelope_recipient(command_encoder,
                                                     &packet, &packet_size,
                                                     "receiver@example.com");
    gcut_assert_error(feed(packet, packet_size));
}

static void
feed_data (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_data(command_encoder,
                                       &packet, &packet_size);
    gcut_assert_error(feed(packet, packet_size));
}

static void
feed_header (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_header(command_encoder,
                                         &packet, &packet_size,
                                         "From", "sender@example.com");
    gcut_assert_error(feed(packet, packet_size));
}

static void
feed_end_of_header (void)
{
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_end_of_header(command_encoder,
                                                &packet, &packet_size);
    gcut_assert_error(feed(packet, packet_size));
}

static void
feed_body (void)
{
    const gchar body[] = "Hello\n\nBye\n";
    const gchar *packet;
    gsize packet_size;

    milter_command_encoder_encode_body(command_encoder,
                                       &packet, &packet_size,
                                       body, strlen(body), NULL);
    gcut_assert_error(feed(packet, packet_size));
}

static void
move_to_body_state (void)
{
    feed_negotiate();
    feed_connect();
    feed_helo();
    feed_envelope_from();
    feed_envelope_recipient();
    feed_data();
    feed_header();
    feed_end_of_header();
    feed_body();

    gcut_string_io_channel_clear(channel);
}

void
test_private (void)
{
    gchar data[] = "private data";

    cut_assert_null(smfi_getpriv(context));
    smfi_setpriv(context, data);
    cut_assert_equal_string(data, smfi_getpriv(context));
}

void
test_getsymval (void)
{
    GHashTable *macros;
    const gchar *packet;
    gsize packet_size;

    macros = gcut_hash_table_string_string_new("{mail_addr}", "kou@example.com",
                                               NULL);
    milter_command_encoder_encode_define_macro(command_encoder,
                                               &packet, &packet_size,
                                               MILTER_COMMAND_ENVELOPE_FROM,
                                               macros);
    g_hash_table_unref(macros);
    gcut_assert_error(feed(packet, packet_size));

    cut_assert_equal_string(NULL, macro_value);
    macro_name = g_strdup("mail_addr");
    milter_command_encoder_encode_envelope_from(command_encoder,
                                                &packet, &packet_size,
                                                "<kou@example.com>");
    gcut_assert_error(feed(packet, packet_size));
    cut_assert_equal_string("kou@example.com", macro_value);
}

void
data_setreply (void)
{
#define ADD(label, expected_return_status,                              \
            return_code, extended_code, message, status)                \
    gcut_add_datum(label,                                               \
                   "expected-return-status",                            \
                   G_TYPE_INT, expected_return_status,                  \
                   "return-code", G_TYPE_STRING, return_code,           \
                   "extended-code", G_TYPE_STRING, extended_code,       \
                   "message", G_TYPE_STRING, message,                   \
                   "status", G_TYPE_INT, status,                        \
                   NULL)

    ADD("4xx - no extended",
        MI_SUCCESS, "450", NULL, "rejected", SMFIS_TEMPFAIL);
    ADD("4xx - extended",
        MI_SUCCESS, "450", "4.2.0", "Rejected by Greylist", SMFIS_TEMPFAIL);
    ADD("5xx",
        MI_SUCCESS, "551", "5.7.1", "Forwarding to remote host disabled",
        SMFIS_REJECT);
    ADD("invalid - 3xx",
        MI_FAILURE, "399", NULL, "message", SMFIS_CONTINUE);
    ADD("invalid - empty extended code",
        MI_FAILURE, "450", "", "Rejected", SMFIS_TEMPFAIL);

#undef ADD
}

void
test_setreply (gconstpointer data)
{
    const gchar *packet;
    gsize packet_size;

    reply_return_code = g_strdup(gcut_data_get_string(data, "return-code"));
    reply_extended_code = g_strdup(gcut_data_get_string(data, "extended-code"));
    reply_message = g_strdup(gcut_data_get_string(data, "message"));
    reply_status = gcut_data_get_int(data, "status");

    milter_command_encoder_encode_data(command_encoder,
                                       &packet, &packet_size);
    gcut_assert_error(feed(packet, packet_size));
    cut_assert_equal_int(gcut_data_get_int(data, "expected-return-status"),
                         set_reply_result);
}

void
test_setmlreply (void)
{
    const gchar *packet;
    gsize packet_size;

    set_mlreply = TRUE;
    milter_command_encoder_encode_end_of_header(command_encoder,
                                                &packet, &packet_size);
    gcut_assert_error(feed(packet, packet_size));
    cut_assert_equal_int(MI_SUCCESS, set_mlreply_result);
    cut_assert_equal_string_with_free(
        "450 4.2.0 Rejected by Greylist\r\n"
        "450 4.2.0 See http://example.com/ for more info",
        milter_client_context_format_reply(client_context));
}

void
test_addheader (void)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;

    move_to_body_state();

    add_header = TRUE;

    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));


    expected_output = g_string_new(NULL);

    milter_reply_encoder_encode_add_header(reply_encoder,
                                           &packet, &packet_size,
                                           header_name, header_value);
    g_string_append_len(expected_output, packet, packet_size);

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
test_insheader (void)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;

    move_to_body_state();

    insert_header = TRUE;

    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    header_index = 2;
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));


    expected_output = g_string_new(NULL);

    milter_reply_encoder_encode_insert_header(reply_encoder,
                                              &packet, &packet_size,
                                              header_index,
                                              header_name, header_value);
    g_string_append_len(expected_output, packet, packet_size);

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
test_chgheader (void)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;

    move_to_body_state();

    change_header = TRUE;

    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    header_index = 2;
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));


    expected_output = g_string_new(NULL);

    milter_reply_encoder_encode_change_header(reply_encoder,
                                              &packet, &packet_size,
                                              header_name,
                                              header_index,
                                              header_value);
    g_string_append_len(expected_output, packet, packet_size);

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
data_chgfrom (void)
{
    cut_add_data("from",
                 g_strsplit("kou@localhost", " ", 2),
                 g_strfreev,
                 "from - parameters",
                 g_strsplit("kou@localhost XXX", " ", 2),
                 g_strfreev,
                 "NULL",
                 NULL,
                 NULL,
                 "empty from - parameters",
                 g_strsplit("", " ", 2),
                 g_strfreev,
                 "empty from - parameters",
                 g_strsplit(" XXX", " ", 2),
                 g_strfreev,
                 "null from - parameters",
                 g_strsplit("(null) XXX", " ", 2),
                 g_strfreev);
}

void
test_chgfrom (gconstpointer data)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;
    gchar * const *test_data = data;

    move_to_body_state();

    do_change_from = TRUE;
    if (test_data && test_data[0]) {
        change_from = g_strdup(test_data[0]);
        change_from_parameters = g_strdup(test_data[1]);
    }
    if (change_from && g_str_equal(change_from, "(null)")) {
        g_free(change_from);
        change_from = NULL;
    }

    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));

    expected_output = g_string_new(NULL);

    if (change_from && change_from[0] != '\0') {
        milter_reply_encoder_encode_change_from(reply_encoder,
                                                &packet, &packet_size,
                                                change_from,
                                                change_from_parameters);
        g_string_append_len(expected_output, packet, packet_size);
    }

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
test_addrcpt (void)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;

    move_to_body_state();

    do_add_recipient = TRUE;

    add_recipient = g_strdup("kou@localhost");
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));


    expected_output = g_string_new(NULL);

    milter_reply_encoder_encode_add_recipient(reply_encoder,
                                              &packet, &packet_size,
                                              add_recipient, NULL);
    g_string_append_len(expected_output, packet, packet_size);

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
test_addrcpt_par (void)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;

    move_to_body_state();

    do_add_recipient_with_parameters = TRUE;

    add_recipient = g_strdup("kou@localhost");
    add_recipient_parameters = g_strdup("XXX");
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));


    expected_output = g_string_new(NULL);

    milter_reply_encoder_encode_add_recipient(reply_encoder,
                                              &packet, &packet_size,
                                              add_recipient,
                                              add_recipient_parameters);
    g_string_append_len(expected_output, packet, packet_size);

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
data_delrcpt (void)
{
    cut_add_data("recipient",
                 g_strdup("kou@localhost"),
                 g_free,
                 "NULL",
                 NULL,
                 NULL,
                 "empty",
                 g_strdup(""),
                 g_free);
}

void
test_delrcpt (gconstpointer data)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;
    const gchar *test_data = data;

    move_to_body_state();

    do_delete_recipient = TRUE;
    delete_recipient = g_strdup(test_data);
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));

    expected_output = g_string_new(NULL);

    if (delete_recipient && delete_recipient[0] != '\0') {
        milter_reply_encoder_encode_delete_recipient(reply_encoder,
                                                     &packet, &packet_size,
                                                     delete_recipient);
        g_string_append_len(expected_output, packet, packet_size);
    }

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
test_progress (void)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;
    const gchar fqdn[] = "delian";

    send_progress = TRUE;
    milter_command_encoder_encode_helo(command_encoder,
                                       &packet, &packet_size, fqdn);
    gcut_assert_error(feed(packet, packet_size));


    expected_output = g_string_new(NULL);

    milter_reply_encoder_encode_progress(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
test_quarantine (void)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;

    move_to_body_state();

    quarantine_reason = g_strdup("virus mail!");
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));


    expected_output = g_string_new(NULL);

    milter_reply_encoder_encode_quarantine(reply_encoder, &packet, &packet_size,
                                           quarantine_reason);
    g_string_append_len(expected_output, packet, packet_size);

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

void
test_replacebody (void)
{
    const gchar *packet;
    gsize packet_size;
    GString *actual_data;
    gsize packed_size;

    move_to_body_state();

    body = g_string_new("XXX");
    replace_body_result = MI_FAILURE;
    milter_command_encoder_encode_end_of_message(command_encoder,
                                                 &packet, &packet_size,
                                                 NULL, 0);
    gcut_assert_error(feed(packet, packet_size));
    cut_assert_equal_int(MI_SUCCESS, replace_body_result);


    expected_output = g_string_new(NULL);

    milter_reply_encoder_encode_replace_body(reply_encoder,
                                             &packet, &packet_size,
                                             body->str, body->len, &packed_size);
    g_string_append_len(expected_output, packet, packet_size);

    milter_reply_encoder_encode_continue(reply_encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    actual_data = gcut_string_io_channel_get_string(channel);
    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            actual_data->str, actual_data->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
