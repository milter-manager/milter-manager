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

#ifndef __MILTER_CLIENT_SINGLE_THREAD_RUNNER_H__
#define __MILTER_CLIENT_SINGLE_THREAD_RUNNER_H__

#include <milter/client/milter-client-runner.h>

G_BEGIN_DECLS

#define MILTER_CLIENT_SINGLE_THREAD_RUNNER_ERROR           (milter_client_single_thread_runner_error_quark())

#define MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER            (milter_client_single_thread_runner_get_type())
#define MILTER_CLIENT_SINGLE_THREAD_RUNNER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER, MilterClientSingleThreadRunner))
#define MILTER_CLIENT_SINGLE_THREAD_RUNNER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER, MilterClientSingleThreadRunnerClass))
#define MILTER_IS_CLIENT_SINGLE_THREAD_RUNNER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER))
#define MILTER_IS_CLIENT_SINGLE_THREAD_RUNNER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER))
#define MILTER_CLIENT_SINGLE_THREAD_RUNNER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER, MilterClientSingleThreadRunnerClass))

typedef enum
{
    MILTER_CLIENT_SINGLE_THREAD_RUNNER_ERROR_TODO
} MilterClientSingleThreadRunnerError;

typedef struct _MilterClientSingleThreadRunner         MilterClientSingleThreadRunner;
typedef struct _MilterClientSingleThreadRunnerClass    MilterClientSingleThreadRunnerClass;

struct _MilterClientSingleThreadRunner
{
    MilterClientRunner object;
};

struct _MilterClientSingleThreadRunnerClass
{
    MilterClientRunnerClass parent_class;
};

GQuark               milter_client_single_thread_runner_error_quark       (void);

GType                milter_client_single_thread_runner_get_type          (void) G_GNUC_CONST;

MilterClientRunner  *milter_client_single_thread_runner_new               (MilterClient *client);

G_END_DECLS

#endif /* __MILTER_CLIENT_SINGLE_THREAD_RUNNER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
