/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string.h>

#include <milter/core/milter-enum-types.h>
#include <milter/core/milter-event-loop.h>
#include <milter/core/milter-glib-event-loop.h>
#include <milter/core/milter-libev-event-loop.h>

#include <gcutter.h>

void test_glib_add_timeout (void);
void test_glib_add_timeout_zero (void);
void test_glib_add_timeout_negative_value (void);
void test_libev_add_timeout (void);
void test_libev_add_timeout_zero (void);
void test_libev_add_timeout_negative_value (void);

static gboolean timeout_waiting;
static guint n_timeouts;

static gboolean
cb_timeout (gpointer data)
{
    gboolean *waiting = data;

    *waiting = FALSE;
    n_timeouts++;
    return FALSE;
}

void
cut_setup (void)
{
    timeout_waiting = TRUE;
    n_timeouts = 0;
}

void
test_glib_add_timeout (void)
{
    MilterEventLoop *loop = milter_glib_event_loop_new(NULL);

    guint id = milter_event_loop_add_timeout(loop, 1, cb_timeout, &timeout_waiting);
    while (timeout_waiting) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, id);
    cut_assert_false(timeout_waiting, cut_message("timeout"));
}

void
test_glib_add_timeout_zero (void)
{
    MilterEventLoop *loop = milter_glib_event_loop_new(NULL);

    guint id = milter_event_loop_add_timeout(loop, 0, cb_timeout, &timeout_waiting);
    while (timeout_waiting) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, id);
    cut_assert_false(timeout_waiting, cut_message("timeout"));
}

void
test_glib_add_timeout_negative_value (void)
{
    MilterEventLoop *loop = milter_glib_event_loop_new(NULL);

    guint id = milter_event_loop_add_timeout(loop, -1, cb_timeout, &timeout_waiting);
    cut_assert_equal_uint(0, id);
}


void
test_libev_add_timeout (void)
{
    MilterEventLoop *loop = milter_libev_event_loop_new();

    guint id = milter_event_loop_add_timeout(loop, 1, cb_timeout, &timeout_waiting);
    while (timeout_waiting) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, id);
    cut_assert_false(timeout_waiting, cut_message("timeout"));
}

void
test_libev_add_timeout_zero (void)
{
    MilterEventLoop *loop = milter_libev_event_loop_new();

    guint id = milter_event_loop_add_timeout(loop, 0, cb_timeout, &timeout_waiting);
    while (timeout_waiting) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, id);
    cut_assert_false(timeout_waiting, cut_message("timeout"));
}

void
test_libev_add_timeout_negative_value (void)
{
    MilterEventLoop *loop = milter_libev_event_loop_new();

    guint id = milter_event_loop_add_timeout(loop, -1, cb_timeout, &timeout_waiting);
    cut_assert_equal_uint(0, id);
}
