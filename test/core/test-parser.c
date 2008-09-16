#include <gcutter.h>

#define shutdown inet_shutdown
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#undef shutdown

#include <milter-core/milter-parser.h>

void test_parse_empty_text (void);
void test_parse_option_negotiation (void);
void data_parse_define_macro (void);
void test_parse_define_macro (gconstpointer data);
void test_parse_connect (void);
void test_parse_helo (void);
void test_parse_mail (void);
void test_parse_rcpt (void);
void test_parse_header (void);
void test_parse_end_of_header (void);
void test_parse_body (void);
void test_parse_body_end_with_data (void);
void test_parse_body_end_without_data (void);
void test_parse_abort (void);
void test_parse_quit (void);
void test_parse_unknown (void);

static MilterParser *parser;
static GString *buffer;

static gint n_option_negotiations;
static gint n_define_macros;
static gint n_connects;
static gint n_helos;
static gint n_mails;
static gint n_rcpts;
static gint n_headers;
static gint n_end_of_headers;
static gint n_bodies;
static gint n_end_of_messages;
static gint n_aborts;
static gint n_quits;
static gint n_unknowns;

static MilterContextType macro_context;
static GHashTable *defined_macros;

static gchar *connect_host_name;
static struct sockaddr *connect_address;
static socklen_t connect_address_length;

static gchar *helo_fqdn;

static gchar *mail_from;

static gchar *rcpt_to;

static gchar *header_name;
static gchar *header_value;

static gchar *body_chunk;
static gsize body_chunk_length;

static gchar *unknown_command;
static gsize unknown_command_length;

static void
cb_option_negotiation (MilterParser *parser, gpointer user_data)
{
    n_option_negotiations++;
}

static void
cb_define_macro (MilterParser *parser, MilterContextType context, GHashTable *macros,
                 gpointer user_data)
{
    n_define_macros++;

    macro_context = context;
    if (defined_macros)
        g_hash_table_unref(defined_macros);
    defined_macros = macros;
    if (defined_macros)
        g_hash_table_ref(defined_macros);
}

static void
cb_connect (MilterParser *parser, const gchar *host_name,
            const struct sockaddr *address, socklen_t address_length,
            gpointer user_data)
{
    n_connects++;

    connect_host_name = g_strdup(host_name);
    connect_address = malloc(address_length);
    memcpy(connect_address, address, address_length);
    connect_address_length = address_length;
}

static void
cb_helo (MilterParser *parser, const gchar *fqdn, gpointer user_data)
{
    n_helos++;

    helo_fqdn = g_strdup(fqdn);
}

static void
cb_mail (MilterParser *parser, const gchar *from, gpointer user_data)
{
    n_mails++;

    mail_from = g_strdup(from);
}

static void
cb_rcpt (MilterParser *parser, const gchar *to, gpointer user_data)
{
    n_rcpts++;

    rcpt_to = g_strdup(to);
}


static void
cb_header (MilterParser *parser, const gchar *name, const gchar *value,
           gpointer user_data)
{
    n_headers++;

    if (header_name)
        g_free(header_name);
    header_name = g_strdup(name);

    if (header_value)
        g_free(header_value);
    header_value = g_strdup(value);
}

static void
cb_end_of_header (MilterParser *parser, gpointer user_data)
{
    n_end_of_headers++;
}

static void
cb_body (MilterParser *parser, const gchar *chunk, gsize length, gpointer user_data)
{
    n_bodies++;

    if (body_chunk)
        g_free(body_chunk);
    body_chunk = g_strndup(chunk, length);
    body_chunk_length = length;
}

static void
cb_end_of_message (MilterParser *parser, gpointer user_data)
{
    n_end_of_messages++;
}

static void
cb_abort (MilterParser *parser, gpointer user_data)
{
    n_aborts++;
}

static void
cb_quit (MilterParser *parser, gpointer user_data)
{
    n_quits++;
}

static void
cb_unknown (MilterParser *parser, const gchar *command, gpointer user_data)
{
    n_unknowns++;

    if (unknown_command)
        g_free(unknown_command);
    unknown_command = g_strdup(command);
}

static void
setup_signals (MilterParser *parser)
{
#define CONNECT(name)                                                   \
    g_signal_connect(parser, #name, G_CALLBACK(cb_ ## name), NULL)

    CONNECT(option_negotiation);
    CONNECT(define_macro);
    CONNECT(connect);
    CONNECT(helo);
    CONNECT(mail);
    CONNECT(rcpt);
    CONNECT(header);
    CONNECT(end_of_header);
    CONNECT(body);
    CONNECT(end_of_message);
    CONNECT(abort);
    CONNECT(quit);
    CONNECT(unknown);

#undef CONNECT
}

void
setup (void)
{
    parser = milter_parser_new();
    setup_signals(parser);

    n_option_negotiations = 0;
    n_define_macros = 0;
    n_connects = 0;
    n_helos = 0;
    n_mails = 0;
    n_rcpts = 0;
    n_headers = 0;
    n_end_of_headers = 0;
    n_bodies = 0;
    n_end_of_messages = 0;
    n_aborts = 0;
    n_quits = 0;
    n_unknowns = 0;

    buffer = g_string_new(NULL);

    defined_macros = NULL;

    connect_host_name = NULL;
    connect_address = NULL;
    connect_address_length = 0;

    helo_fqdn = NULL;

    mail_from = NULL;

    rcpt_to = NULL;

    header_name = NULL;
    header_value = NULL;

    body_chunk = NULL;
    body_chunk_length = 0;

    unknown_command = NULL;
    unknown_command_length = 0;
}

void
teardown (void)
{
    if (parser) {
        milter_parser_end_parse(parser, NULL);
        g_object_unref(parser);
    }

    if (buffer)
        g_string_free(buffer, TRUE);

    if (defined_macros)
        g_hash_table_unref(defined_macros);

    if (connect_host_name)
        g_free(connect_host_name);
    if (connect_address)
        g_free(connect_address);

    if (helo_fqdn)
        g_free(helo_fqdn);

    if (mail_from)
        g_free(mail_from);

    if (rcpt_to)
        g_free(rcpt_to);

    if (header_name)
        g_free(header_name);
    if (header_value)
        g_free(header_value);

    if (body_chunk)
        g_free(body_chunk);

    if (unknown_command)
        g_free(unknown_command);
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

    milter_parser_parse(parser, packet, packet_size, &error);
    g_free(packet);

    g_string_free(buffer, TRUE);
    buffer = g_string_new(NULL);

    return error;
}

void
test_parse_empty_text (void)
{
    GError *expected = NULL;
    GError *actual = NULL;

    return; /* do nothing for now */
    cut_assert_false(milter_parser_parse(parser, "", 0, &actual));

    expected = g_error_new(MILTER_PARSER_ERROR,
                           MILTER_PARSER_ERROR_SHORT_COMMAND_LENGTH,
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
append_name_and_value(const gchar *name, const gchar *value)
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
    MilterContextType context;
    GHashTable *expected;
} DefineMacroTestData;

static DefineMacroTestData *define_macro_test_data_new(setup_packet_func setup_packet,
                                                       MilterContextType context,
                                                       const gchar *key,
                                                       ...) G_GNUC_NULL_TERMINATED;
static DefineMacroTestData *
define_macro_test_data_new (setup_packet_func setup_packet,
                            MilterContextType context,
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
    append_name_and_value("j", "debian.cozmixng.org");
    append_name_and_value("daemon_name", "debian.cozmixng.org");
    append_name_and_value("v", "Postfix 2.5.5");
}

static void
setup_define_macro_helo_packet (void)
{
    g_string_append(buffer, "H");
}

static void
setup_define_macro_mail_packet (void)
{
    g_string_append(buffer, "M");
    append_name_and_value("{mail_addr}", "kou@cozmixng.org");
}

static void
setup_define_macro_rcpt_packet (void)
{
    g_string_append(buffer, "R");
    append_name_and_value("{rcpt_addr}", "kou@cozmixng.org");
}

static void
setup_define_macro_header_packet (void)
{
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
}

void
data_parse_define_macro (void)
{
    cut_add_data("connect",
                 define_macro_test_data_new(setup_define_macro_connect_packet,
                                            MILTER_CONTEXT_TYPE_CONNECT,
                                            "j", "debian.cozmixng.org",
                                            "daemon_name", "debian.cozmixng.org",
                                            "v", "Postfix 2.5.5",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("HELO",
                 define_macro_test_data_new(setup_define_macro_helo_packet,
                                            MILTER_CONTEXT_TYPE_HELO,
                                            NULL, NULL),
                 define_macro_test_data_free);

    cut_add_data("MAIL FROM",
                 define_macro_test_data_new(setup_define_macro_mail_packet,
                                            MILTER_CONTEXT_TYPE_MAIL,
                                            "mail_addr", "kou@cozmixng.org",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("RCPT TO",
                 define_macro_test_data_new(setup_define_macro_rcpt_packet,
                                            MILTER_CONTEXT_TYPE_RCPT,
                                            "rcpt_addr", "kou@cozmixng.org",
                                            NULL),
                 define_macro_test_data_free);

    cut_add_data("header",
                 define_macro_test_data_new(setup_define_macro_header_packet,
                                            MILTER_CONTEXT_TYPE_HEADER,
                                            "i", "69FDD42DF4A",
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

void
test_parse_connect (void)
{
    struct sockaddr_in *address;
    const gchar host_name[] = "mx.local.net";
    const gchar ip_address[] = "192.168.123.123";
    gchar port_string[sizeof(uint16_t)];
    uint16_t port;

    port = htons(50443);
    memcpy(port_string, &port, sizeof(port));

    g_string_append(buffer, "C");
    g_string_append(buffer, host_name);
    g_string_append_c(buffer, '\0');
    g_string_append(buffer, "4");
    g_string_append_len(buffer, port_string, sizeof(port_string));
    g_string_append(buffer, ip_address);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_connects);
    cut_assert_equal_string(host_name, connect_host_name);
    cut_assert_equal_int(sizeof(struct sockaddr_in), connect_address_length);

    address = (struct sockaddr_in *)connect_address;
    cut_assert_equal_int(AF_INET, address->sin_family);
    cut_assert_equal_uint(port, address->sin_port);
    cut_assert_equal_string(ip_address, inet_ntoa(address->sin_addr));
}

void
test_parse_helo (void)
{
    const gchar fqdn[] = "delian";

    g_string_append(buffer, "H");
    g_string_append(buffer, fqdn);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_helos);
    cut_assert_equal_string(fqdn, helo_fqdn);
}

void
test_parse_mail (void)
{
    const gchar from[] = "<kou@cozmixng.org>";

    g_string_append(buffer, "M");
    g_string_append(buffer, from);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_mails);
    cut_assert_equal_string(from, mail_from);
}

void
test_parse_rcpt (void)
{
    const gchar to[] = "<kou@cozmixng.org>";

    g_string_append(buffer, "R");
    g_string_append(buffer, to);
    g_string_append_c(buffer, '\0');

    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_rcpts);
    cut_assert_equal_string(to, rcpt_to);
}

void
test_parse_header (void)
{
    const gchar from[] = "<kou@cozmixng.org>";
    const gchar to[] = "<kou@cozmixng.org>";
    const gchar date[] = "Fri,  5 Sep 2008 09:19:56 +0900 (JST)";
    const gchar message_id[] = "<1ed5.0003.0000@delian>";

    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(parse());

    g_string_append(buffer, "L");
    append_name_and_value("From", from);
    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_headers);
    cut_assert_equal_string("From", header_name);
    cut_assert_equal_string(from, header_value);


    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(parse());

    g_string_append(buffer, "L");
    append_name_and_value("To", to);
    gcut_assert_error(parse());
    cut_assert_equal_int(2, n_headers);
    cut_assert_equal_string("To", header_name);
    cut_assert_equal_string(to, header_value);


    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(parse());

    g_string_append(buffer, "L");
    append_name_and_value("Date", date);
    gcut_assert_error(parse());
    cut_assert_equal_int(3, n_headers);
    cut_assert_equal_string("Date", header_name);
    cut_assert_equal_string(date, header_value);


    g_string_append(buffer, "D");
    g_string_append(buffer, "L");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(parse());

    g_string_append(buffer, "L");
    append_name_and_value("Message-Id", message_id);
    gcut_assert_error(parse());
    cut_assert_equal_int(4, n_headers);
    cut_assert_equal_string("Message-Id", header_name);
    cut_assert_equal_string(message_id, header_value);
}

void
test_parse_end_of_header (void)
{
    g_string_append(buffer, "D");
    g_string_append(buffer, "N");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(parse());

    g_string_append(buffer, "N");
    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_end_of_headers);
}

void
test_parse_body (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";

    g_string_append(buffer, "D");
    g_string_append(buffer, "B");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(parse());

    g_string_append(buffer, "B");
    g_string_append(buffer, body);
    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_bodies);
    cut_assert_equal_string(body, body_chunk);
    cut_assert_equal_uint(strlen(body), body_chunk_length);
}

void
test_parse_body_end_with_data (void)
{
    const gchar body[] =
        "La de da de da 1.\n"
        "La de da de da 2.\n"
        "La de da de da 3.\n"
        "La de da de da 4.";

    g_string_append(buffer, "D");
    g_string_append(buffer, "E");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(parse());

    g_string_append(buffer, "E");
    g_string_append(buffer, body);
    gcut_assert_error(parse());
    cut_assert_equal_int(1, n_bodies);
    cut_assert_equal_string(body, body_chunk);
    cut_assert_equal_int(strlen(body), body_chunk_length);
    cut_assert_equal_int(1, n_end_of_messages);
}

void
test_parse_body_end_without_data (void)
{
    g_string_append(buffer, "D");
    g_string_append(buffer, "E");
    append_name_and_value("i", "69FDD42DF4A");
    gcut_assert_error(parse());

    g_string_append(buffer, "E");
    gcut_assert_error(parse());
    cut_assert_equal_int(0, n_bodies);
    cut_assert_equal_int(1, n_end_of_messages);
}

void
test_parse_abort (void)
{
    g_string_append(buffer, "A");
    gcut_assert_error(parse());

    cut_assert_equal_int(1, n_aborts);
}

void
test_parse_quit (void)
{
    g_string_append(buffer, "Q");
    gcut_assert_error(parse());

    cut_assert_equal_int(1, n_quits);
}

void
test_parse_unknown (void)
{
    const gchar command[] = "UNKNOWN COMMAND WITH ARGUMENT";

    g_string_append(buffer, "U");
    g_string_append(buffer, command);
    g_string_append_c(buffer, '\0');
    gcut_assert_error(parse());

    cut_assert_equal_int(1, n_unknowns);
    cut_assert_equal_string(command, unknown_command);
}
