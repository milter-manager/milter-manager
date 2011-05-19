/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-client-single-thread-runner.h"

#define MILTER_CLIENT_SINGLE_THREAD_RUNNER_GET_PRIVATE(obj)             \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER, \
                                 MilterClientSingleThreadRunnerPrivate))

typedef struct _MilterClientSingleThreadRunnerPrivate MilterClientSingleThreadRunnerPrivate;
struct _MilterClientSingleThreadRunnerPrivate
{
    gboolean use_accept_thread;
};

G_DEFINE_TYPE(MilterClientSingleThreadRunner,
              milter_client_single_thread_runner,
              MILTER_TYPE_CLIENT_RUNNER);

static void     dispose        (GObject             *object);
static gboolean run            (MilterClientRunner  *runner,
                                GError             **error);
static void     quit           (MilterClientRunner  *runner);

static void
milter_client_single_thread_runner_class_init (MilterClientSingleThreadRunnerClass *klass)
{
    GObjectClass *gobject_class;
    MilterClientRunnerClass *runner_class;

    gobject_class = G_OBJECT_CLASS(klass);
    runner_class = MILTER_CLIENT_RUNNER_CLASS(klass);

    gobject_class->dispose      = dispose;

    runner_class->run = run;
    runner_class->quit = quit;

    g_type_class_add_private(gobject_class,
                             sizeof(MilterClientSingleThreadRunnerPrivate));
}

static void
milter_client_single_thread_runner_init (MilterClientSingleThreadRunner *runner)
{
    MilterClientSingleThreadRunnerPrivate *priv;

    priv = MILTER_CLIENT_SINGLE_THREAD_RUNNER_GET_PRIVATE(runner);

    priv->use_accept_thread = TRUE;
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(milter_client_single_thread_runner_parent_class)->dispose(object);
}

MilterClientRunner *
milter_client_single_thread_runner_new (MilterClient *client)
{
    return g_object_new(MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER,
                        "client", client,
                        NULL);
}

static gboolean
run (MilterClientRunner *runner, GError **error)
{
    return TRUE;
}

static void
quit (MilterClientRunner *runner)
{
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
