#include <gcutter.h>

#define shutdown inet_shutdown
#include <arpa/inet.h>
#undef shutdown

#include <milter-client/milter-client.h>

void test_parse_empty_text (void);
void test_parse_option_negotiation (void);
void test_parse_define_macro (void);

static MCParser *parser;
static GString *buffer;

static gint n_option_negotiations;
static gint n_define_macros;

static McContextType macro_context;
static GHashTable *defined_macros;

static void
cb_option_negotiation (MCParser *parser, gpointer user_data)
{
    n_option_negotiations++;
}

static void
cb_define_macro (MCParser *parser, McContextType context, GHashTable *macros,
                 gpointer user_data)
{
    n_define_macros++;

    macro_context = context;
    defined_macros = macros;
    if (defined_macros)
        g_hash_table_ref(defined_macros);
}

static void
setup_signals (MCParser *parser)
{
#define CONNECT(name)                                                   \
    g_signal_connect(parser, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(option_negotiation);
    CONNECT(define_macro);

#undef CONNECT
}

void
setup (void)
{
    parser = mc_parser_new();
    setup_signals(parser);

    n_option_negotiations = 0;
    n_define_macros = 0;

    buffer = g_string_new(NULL);

    defined_macros = NULL;
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

    if (defined_macros)
        g_hash_table_unref(defined_macros);
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
    memcpy(packet + sizeof(content_size), buffer->str, buffer->len);

    mc_parser_parse(parser, packet, packet_size, &error);
    g_free(packet);

    return error;
}

void
test_parse_empty_text (void)
{
    GError *expected = NULL;
    GError *actual = NULL;

    return; /* do nothing for now */
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

static void
append_macro_definition(const gchar *name, const gchar *value)
{
    if (name)
        g_string_append(buffer, name);
    g_string_append_c(buffer, '\0');
    if (value)
        g_string_append(buffer, value);
    g_string_append_c(buffer, '\0');
}

void
test_parse_define_macro (void)
{
    GHashTable *macros;

    g_string_append(buffer, "D");
    g_string_append(buffer, "C");
    append_macro_definition("j", "debian.cozmixng.org");
    append_macro_definition("daemon_name", "debian.cozmixng.org");
    append_macro_definition("v", "Postfix 2.5.5");
    append_macro_definition(NULL, NULL);
    append_macro_definition(NULL, NULL);
    append_macro_definition("!Cmx.local.net",
                            cut_take_printf("4%c%c192.168.123.123",
                                            0xc5, 0x0b));


    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_define_macros);
    cut_assert_equal_int(MC_CONTEXT_TYPE_CONNECT, macro_context);

    macros =
        gcut_hash_table_string_string_new("j", "debian.cozmixng.org",
                                          "daemon_name", "debian.cozmixng.org",
                                          "v", "Postfix 2.5.5",
                                          "!Cmx.local.net",
                                          cut_take_printf("4%c%c192.168.123.123",
                                                          0xc5, 0x0b),
                                          NULL);
    macros = gcut_take_hash_table(macros);
    gcut_assert_equal_hash_table_string_string(macros, defined_macros);
}
