/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

#include <milter/server.h>

#include <milter-test-utils.h>

#include <gcutter.h>

void data_need_reply_without_no_reply_step (void);
void test_need_reply_without_no_reply_step (gconstpointer data);
void data_need_reply_with_no_reply_step (void);
void test_need_reply_with_no_reply_step (gconstpointer data);

static MilterServerContext *context;
static MilterOption *option;

void
setup (void)
{
    context = milter_server_context_new();
    option = milter_option_new(6,
                               MILTER_ACTION_ADD_HEADERS |
                               MILTER_ACTION_CHANGE_BODY |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_DELETE_ENVELOPE_RECIPIENT |
                               MILTER_ACTION_CHANGE_HEADERS |
                               MILTER_ACTION_QUARANTINE |
                               MILTER_ACTION_CHANGE_ENVELOPE_FROM |
                               MILTER_ACTION_ADD_ENVELOPE_RECIPIENT_WITH_PARAMETERS |
                               MILTER_ACTION_SET_SYMBOL_LIST,
                               MILTER_STEP_SKIP);
    milter_server_context_set_option(context, option);
}

void
teardown (void)
{
    if (option)
        g_object_unref(option);

    if (context)
        g_object_unref(context);
}

void
data_need_reply_without_no_reply_step (void)
{
#define ADD(label, state)                                       \
    gcut_add_datum(label,                                       \
                   "state",                                     \
                   MILTER_TYPE_SERVER_CONTEXT_STATE,            \
                   MILTER_SERVER_CONTEXT_STATE_ ## state,       \
                   NULL)

    ADD("negotiate", NEGOTIATE);
    ADD("connect", CONNECT);
    ADD("helo", HELO);
    ADD("envelope-from", ENVELOPE_FROM);
    ADD("envelope-recipient", ENVELOPE_RECIPIENT);
    ADD("data", DATA);
    ADD("header", HEADER);
    ADD("end-of-header", END_OF_HEADER);
    ADD("body", BODY);
    ADD("end-of-message", END_OF_MESSAGE);
    ADD("unknown", UNKNOWN);

#undef ADD
}

void
test_need_reply_without_no_reply_step (gconstpointer data)
{
    MilterServerContextState state;

    state = gcut_data_get_enum(data, "state");
    cut_assert_true(milter_server_context_need_reply(context, state));
}

void
data_need_reply_with_no_reply_step (void)
{
#define ADD(label, state)                                       \
    gcut_add_datum(label,                                       \
                   "no-reply-step",                             \
                   MILTER_TYPE_STEP_FLAGS,                      \
                   MILTER_STEP_NO_REPLY_ ## state,              \
                   "state",                                     \
                   MILTER_TYPE_SERVER_CONTEXT_STATE,            \
                   MILTER_SERVER_CONTEXT_STATE_ ## state,       \
                   NULL)

    ADD("connect", CONNECT);
    ADD("helo", HELO);
    ADD("envelope-from", ENVELOPE_FROM);
    ADD("envelope-recipient", ENVELOPE_RECIPIENT);
    ADD("data", DATA);
    ADD("header", HEADER);
    ADD("end-of-header", END_OF_HEADER);
    ADD("body", BODY);
    ADD("unknown", UNKNOWN);

#undef ADD
}

void
test_need_reply_with_no_reply_step (gconstpointer data)
{
    MilterServerContextState state;
    MilterStepFlags step;

    state = gcut_data_get_enum(data, "state");
    step = gcut_data_get_flags(data, "no-reply-step");

#define NEED_REPLY() milter_server_context_need_reply(context, state)
    cut_assert_true(NEED_REPLY());
    milter_option_add_step(option, step);
    milter_server_context_set_option(context, option);
    cut_assert_false(NEED_REPLY());
#undef NEED_REPLY
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
