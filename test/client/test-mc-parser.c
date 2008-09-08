#include <gcutter.h>

#define shutdown inet_shutdown
#include <arpa/inet.h>
#undef shutdown

#include <milter-client/milter-client.h>

void test_short_parse_text (void);
void test_parse_option_negotiation (void);

static MCParser *parser;
static GString *buffer;
static gint n_option_negotiations;

static void
cb_option_negotiation (MCParser *parser, gpointer user_data)
{
    n_option_negotiations++;
}

static void
setup_signals (MCParser *parser)
{
#define CONNECT(name)                                                   \
    g_signal_connect(parser, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(option_negotiation);

#undef CONNECT
}

void
setup (void)
{
    parser = mc_parser_new();
    setup_signals(parser);

    n_option_negotiations = 0;

    buffer = g_string_new(NULL);
}

void
teardown (void)
{
    if (parser) {
        mc_parser_end_parse(parser, NULL);
        g_object_unref(parser);
    }

    if (buffer)
        g_string_free(buffer, TRUE);
}

static GError *
parse (void)
{
    gchar *packet;
    gssize packet_size;
    guint32 content_size;
    GError *error = NULL;

    packet_size = sizeof(content_size) + buffer->len;
    packet = g_new0(gchar, packet_size);

    content_size = htonl(buffer->len);
    memcpy(packet, &content_size, sizeof(content_size));
    memcpy(packet + sizeof(content_size),
           buffer->str, buffer->len - sizeof(content_size));

    mc_parser_parse(parser, packet, packet_size, &error);
    g_free(packet);

    return error;
}

void
test_short_parse_text (void)
{
    GError *expected = NULL;
    GError *actual = NULL;

    cut_assert_false(mc_parser_parse(parser, "", 0, &actual));

    expected = g_error_new(MC_PARSER_ERROR,
                           MC_PARSER_ERROR_SHORT_COMMAND_LENGTH,
                           "too short command length");
    gcut_assert_equal_error(gcut_take_error(expected),
                            gcut_take_error(actual));
}

void
test_parse_option_negotiation (void)
{
    g_string_append_c(buffer, 'O');
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, 0x02);
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, 0x01);
    g_string_append_c(buffer, 0x3f);
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, '\0');
    g_string_append_c(buffer, 0x7f);

    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_option_negotiations);
}
