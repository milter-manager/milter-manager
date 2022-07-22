/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <milter/core/milter-enum-types.h>
#include <milter/core/milter-event-loop.h>
#include <milter/core/milter-glib-event-loop.h>
#include <milter/core/milter-libev-event-loop.h>

#include <gcutter.h>

void data_add_timeout (void);
void test_add_timeout (gconstpointer data);
void data_add_timeout_negative (void);
void test_add_timeout_negative (gconstpointer data);

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

void data_add_timeout (void)
{
#define ADD_DATUM(label, event_loop_type, interval) \
    gcut_add_datum(label, \
                   "event-loop-type", G_TYPE_GTYPE, MILTER_TYPE_ ## event_loop_type ##  _EVENT_LOOP, \
                   "interval", G_TYPE_INT, interval, \
                   NULL)

    ADD_DATUM("glib 0", GLIB, 0);
    ADD_DATUM("glib 1", GLIB, 1);
    ADD_DATUM("libev 0", LIBEV, 0);
    ADD_DATUM("libev 1", LIBEV, 1);


#undef ADD_DATUM
}

void test_add_timeout (gconstpointer data)
{
    MilterEventLoop *loop = NULL;
    GType event_loop_type = gcut_data_get_type(data, "event-loop-type");
    gint interval = gcut_data_get_int(data, "interval");

    if (event_loop_type == MILTER_TYPE_GLIB_EVENT_LOOP) {
        loop = milter_glib_event_loop_new(NULL);
    } else if (event_loop_type == MILTER_TYPE_LIBEV_EVENT_LOOP) {
        loop = milter_libev_event_loop_new();
    }

    guint id = milter_event_loop_add_timeout(loop, interval, cb_timeout, &timeout_waiting);
    while (timeout_waiting) {
        milter_event_loop_iterate(loop, TRUE);
    }
    milter_event_loop_remove(loop, id);
    cut_assert_false(timeout_waiting, cut_message("timeout"));
    milter_event_loop_quit(loop);
}

void
data_add_timeout_negative (void)
{
#define ADD_DATUM(label, event_loop_type, interval) \
    gcut_add_datum(label, \
                   "event-loop-type", G_TYPE_GTYPE, MILTER_TYPE_ ## event_loop_type ##  _EVENT_LOOP, \
                   "interval", G_TYPE_INT, interval, \
                   NULL)

    ADD_DATUM("glib -1", GLIB, -1);
    ADD_DATUM("libev -1", LIBEV, -1);


#undef ADD_DATUM
}

void test_add_timeout_negative (gconstpointer data)
{
    MilterEventLoop *loop = NULL;
    GType event_loop_type = gcut_data_get_type(data, "event-loop-type");
    gint interval = gcut_data_get_int(data, "interval");

    if (event_loop_type == MILTER_TYPE_GLIB_EVENT_LOOP) {
        loop = milter_glib_event_loop_new(NULL);
    } else if (event_loop_type == MILTER_TYPE_LIBEV_EVENT_LOOP) {
        loop = milter_libev_event_loop_new();
    }

    guint id = milter_event_loop_add_timeout(loop, interval, cb_timeout, &timeout_waiting);
    cut_assert_equal_uint(0, id);
    milter_event_loop_quit(loop);
}
