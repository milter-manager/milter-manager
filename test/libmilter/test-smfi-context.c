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

static SmfiContext *context;
static MilterClientContext *client_context;
static MilterEncoder *encoder;

static gchar *packet;
static gsize packet_size;

static gchar *macro_name;
static gchar *macro_value;

static sfsistat
xxfi_connect (SMFICTX *context, char *host_name, _SOCK_ADDR *address)
{
    return SMFIS_CONTINUE;
}

static sfsistat
xxfi_helo (SMFICTX *context, char *fqdn)
{
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
    smfi_context_attach_to_client_context(context, client_context);

    encoder = milter_encoder_new();

    packet = NULL;
    packet_size = 0;

    macro_name = NULL;
    macro_value = NULL;
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

    if (encoder)
        g_object_unref(encoder);

    packet_free();

    if (macro_name)
        g_free(macro_name);
    if (macro_value)
        g_free(macro_value);
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

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
