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

#include <gcutter.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#undef shutdown

#include <libmilter/libmilter-compatible.h>

void test_private (void);
void test_getsymval (void);
void test_addheader (void);
void test_chgheader (void);
void test_insheader (void);
void data_cfgfrom (void);
void test_cfgfrom (gconstpointer data);
void test_addrcpt (void);
void test_addrcpt_par (void);
void test_delrcpt (void);
void test_progress (void);
void test_quarantine (void);
void test_replacebody (void);

static SmfiContext *context;
static MilterClientContext *client_context;
static MilterEncoder *encoder;

static MilterWriter *writer;
static GString *output;
static GString *expected_output;

static gchar *packet;
static gsize packet_size;

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
static gboolean do_add_receipt;
static gboolean do_add_receipt_with_parameters;
static gboolean do_delete_receipt;

static gchar *change_from;
static gchar *change_from_parameters;
static gchar *add_receipt;
static gchar *add_receipt_parameters;
static gchar *delete_receipt;

static GString *body;
static int replace_body_result;

static gchar *quarantine_reason;

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

    if (do_add_receipt)
        smfi_addrcpt(context, add_receipt);

    if (do_add_receipt_with_parameters)
        smfi_addrcpt_par(context, add_receipt, add_receipt_parameters);

    if (do_delete_receipt)
        smfi_delrcpt(context, delete_receipt);

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


void
setup (void)
{
    context = smfi_context_new();
    smfi_register(smfilter);

    client_context = milter_client_context_new();
    output = g_string_new(NULL);
    expected_output = NULL;
    writer = milter_writer_string_new(output);
    milter_handler_set_writer(MILTER_HANDLER(client_context), writer);

    smfi_context_attach_to_client_context(context, client_context);

    encoder = milter_encoder_new();

    packet = NULL;
    packet_size = 0;

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
    do_add_receipt = FALSE;
    do_add_receipt_with_parameters = FALSE;
    do_delete_receipt = FALSE;

    change_from = NULL;
    change_from_parameters = NULL;
    add_receipt = NULL;
    add_receipt_parameters = NULL;
    delete_receipt = NULL;

    body = NULL;
    replace_body_result = 0;

    quarantine_reason = NULL;
}

static void
packet_free (void)
{
    if (packet)
        g_free(packet);
    packet = NULL;
    packet_size = 0;
}

void
teardown (void)
{
    if (context && client_context)
        smfi_context_detach_from_client_context(context, client_context);

    if (context)
        g_object_unref(context);
    if (client_context)
        g_object_unref(client_context);

    if (writer)
        g_object_unref(writer);
    if (output)
        g_string_free(output, TRUE);
    if (expected_output)
        g_string_free(expected_output, TRUE);

    if (encoder)
        g_object_unref(encoder);

    packet_free();

    if (macro_name)
        g_free(macro_name);
    if (macro_value)
        g_free(macro_value);

    if (change_from)
        g_free(change_from);
    if (change_from_parameters)
        g_free(change_from_parameters);
    if (add_receipt)
        g_free(add_receipt);
    if (add_receipt_parameters)
        g_free(add_receipt_parameters);
    if (delete_receipt)
        g_free(delete_receipt);

    if (body)
        g_string_free(body, TRUE);

    if (quarantine_reason)
        g_free(quarantine_reason);
}

static GError *
feed (void)
{
    GError *error = NULL;

    milter_client_context_feed(client_context, packet, packet_size, &error);

    return error;
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

    macros =
        gcut_hash_table_string_string_new("{mail_addr}", "kou@cozmixng.org",
                                          NULL);
    milter_encoder_encode_define_macro(encoder,
                                       &packet, &packet_size,
                                       MILTER_COMMAND_MAIL,
                                       macros);
    g_hash_table_unref(macros);
    gcut_assert_error(feed());
    packet_free();

    cut_assert_equal_string(NULL, macro_value);
    macro_name = g_strdup("mail_addr");
    milter_encoder_encode_mail(encoder,
                               &packet, &packet_size, "<kou@cozmixng.org>");
    gcut_assert_error(feed());
    cut_assert_equal_string("kou@cozmixng.org", macro_value);
}

void
test_addheader (void)
{
    add_header = TRUE;
    cut_assert_equal_memory("", 0, output->str, output->len);
    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());


    expected_output = g_string_new(NULL);

    packet_free();
    milter_encoder_encode_reply_add_header(encoder, &packet, &packet_size,
                                           header_name, header_value);
    g_string_append_len(expected_output, packet, packet_size);

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

void
test_insheader (void)
{
    insert_header = TRUE;
    cut_assert_equal_memory("", 0, output->str, output->len);
    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    header_index = 2;
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());


    expected_output = g_string_new(NULL);

    packet_free();
    milter_encoder_encode_reply_insert_header(encoder, &packet, &packet_size,
                                              header_index,
                                              header_name, header_value);
    g_string_append_len(expected_output, packet, packet_size);

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

void
test_chgheader (void)
{
    change_header = TRUE;
    cut_assert_equal_memory("", 0, output->str, output->len);
    header_name = g_strdup("X-Test-Header");
    header_value = g_strdup("Test Value");
    header_index = 2;
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());


    expected_output = g_string_new(NULL);

    packet_free();
    milter_encoder_encode_reply_change_header(encoder, &packet, &packet_size,
                                              header_name,
                                              header_index,
                                              header_value);
    g_string_append_len(expected_output, packet, packet_size);

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

void
data_cfgfrom (void)
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
test_cfgfrom (gconstpointer data)
{
    gchar * const *test_data = data;

    do_change_from = TRUE;
    if (test_data && test_data[0]) {
        change_from = g_strdup(test_data[0]);
        change_from_parameters = g_strdup(test_data[1]);
    }
    if (change_from && g_str_equal(change_from, "(null)")) {
        g_free(change_from);
        change_from = NULL;
    }

    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());

    expected_output = g_string_new(NULL);

    if (change_from && change_from[0] != '\0') {
        packet_free();
        milter_encoder_encode_reply_change_from(encoder, &packet, &packet_size,
                                                change_from,
                                                change_from_parameters);
        g_string_append_len(expected_output, packet, packet_size);
    }

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

void
test_addrcpt (void)
{
    do_add_receipt = TRUE;

    add_receipt = g_strdup("kou@localhost");
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());


    expected_output = g_string_new(NULL);

    packet_free();
    milter_encoder_encode_reply_add_receipt(encoder, &packet, &packet_size,
                                            add_receipt, NULL);
    g_string_append_len(expected_output, packet, packet_size);

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

void
test_addrcpt_par (void)
{
    do_add_receipt_with_parameters = TRUE;

    add_receipt = g_strdup("kou@localhost");
    add_receipt_parameters = g_strdup("XXX");
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());


    expected_output = g_string_new(NULL);

    packet_free();
    milter_encoder_encode_reply_add_receipt(encoder, &packet, &packet_size,
                                            add_receipt, add_receipt_parameters);
    g_string_append_len(expected_output, packet, packet_size);

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

void
test_progress (void)
{
    const gchar fqdn[] = "delian";

    send_progress = TRUE;
    milter_encoder_encode_helo(encoder, &packet, &packet_size, fqdn);
    gcut_assert_error(feed());


    expected_output = g_string_new(NULL);

    packet_free();
    milter_encoder_encode_reply_progress(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

void
test_quarantine (void)
{
    quarantine_reason = g_strdup("virus mail!");
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());


    expected_output = g_string_new(NULL);

    packet_free();
    milter_encoder_encode_reply_quarantine(encoder, &packet, &packet_size,
                                           quarantine_reason);
    g_string_append_len(expected_output, packet, packet_size);

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

void
test_replacebody (void)
{
    gsize packed_size;

    body = g_string_new("XXX");
    replace_body_result = MI_FAILURE;
    milter_encoder_encode_end_of_message(encoder, &packet, &packet_size,
                                         NULL, 0);
    gcut_assert_error(feed());
    cut_assert_equal_int(MI_SUCCESS, replace_body_result);


    expected_output = g_string_new(NULL);

    packet_free();
    milter_encoder_encode_reply_replace_body(encoder, &packet, &packet_size,
                                             body->str, body->len, &packed_size);
    g_string_append_len(expected_output, packet, packet_size);

    packet_free();
    milter_encoder_encode_reply_continue(encoder, &packet, &packet_size);
    g_string_append_len(expected_output, packet, packet_size);

    cut_assert_equal_memory(expected_output->str, expected_output->len,
                            output->str, output->len);
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
