#include <gcutter.h>

#define shutdown inet_shutdown
#include <arpa/inet.h>
#undef shutdown

#include <milter-client/milter-client.h>

void test_parse_empty_text (void);
void test_parse_option_negotiation (void);
void data_parse_define_macro (void);
void test_parse_define_macro (gconstpointer data);

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

typedef void (*setup_packet_func) (void);

typedef struct _DefineMacroTestData
{
    setup_packet_func setup_packet;
    McContextType context;
    GHashTable *expected;
} DefineMacroTestData;

static DefineMacroTestData *define_macro_test_data_new(setup_packet_func setup_packet,
                                                       McContextType context,
                                                       const gchar *key,
                                                       ...) G_GNUC_NULL_TERMINATED;
static DefineMacroTestData *
define_macro_test_data_new (setup_packet_func setup_packet,
                            McContextType context,
                            const gchar *key,
                            ...)
{
    DefineMacroTestData *data;
    va_list args;

    data = g_new0(DefineMacroTestData, 1);

    data->setup_packet = setup_packet;
    data->context = context;
    va_start(args, key);
    data->expected = gcut_hash_table_string_string_newv(key, args);
    va_end(args);

    return data;
}

static void
define_macro_test_data_free (DefineMacroTestData *data)
{
    g_hash_table_unref(data->expected);
    g_free(data);
}

static void
setup_define_macro_connect_packet (void)
{
    g_string_append(buffer, "C");
    append_macro_definition("j", "debian.cozmixng.org");
    append_macro_definition("daemon_name", "debian.cozmixng.org");
    append_macro_definition("v", "Postfix 2.5.5");
    append_macro_definition(NULL, NULL);
    append_macro_definition(NULL, NULL);
    append_macro_definition("!Cmx.local.net",
                            cut_take_printf("4%c%c192.168.123.123",
                                            0xc5, 0x0b));

}

static void
setup_define_macro_helo_packet (void)
{
    g_string_append(buffer, "H");
    append_macro_definition(NULL, NULL);
    append_macro_definition(NULL, cut_take_printf("%cDdelian", 0x08));
}

static void
setup_define_macro_mail_packet (void)
{
    g_string_append(buffer, "M");
    append_macro_definition("{mail_addr}", "kou@cozmixng.org");
    append_macro_definition(NULL, NULL);
    append_macro_definition(NULL, NULL);
    append_macro_definition(NULL,
                            cut_take_printf("%cM<kou@cozmixng.org>", 0x14));
}

void
data_parse_define_macro (void)
{
    cut_add_data("connect",
                 define_macro_test_data_new(setup_define_macro_connect_packet,
                                            MC_CONTEXT_TYPE_CONNECT,
                                            "j", "debian.cozmixng.org",
                                            "daemon_name", "debian.cozmixng.org",
                                            "v", "Postfix 2.5.5",
                                            "!Cmx.local.net",
                                            cut_take_printf("4%c%c"
                                                            "192.168.123.123",
                                                            0xc5, 0x0b),
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("HELO",
                 define_macro_test_data_new(setup_define_macro_helo_packet,
                                            MC_CONTEXT_TYPE_HELO,
                                            NULL, NULL),
                 define_macro_test_data_free);

    cut_add_data("MAIL from",
                 define_macro_test_data_new(setup_define_macro_mail_packet,
                                            MC_CONTEXT_TYPE_MAIL,
                                            "mail_addr", "kou@cozmixng.org",
                                            NULL),
                 define_macro_test_data_free);
}

void
test_parse_define_macro (gconstpointer data)
{
    const DefineMacroTestData *test_data = data;

    g_string_append(buffer, "D");
    test_data->setup_packet();

    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_define_macros);
    cut_assert_equal_int(test_data->context, macro_context);
    gcut_assert_equal_hash_table_string_string(test_data->expected,
                                               defined_macros);
}
