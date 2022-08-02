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

#define MILTER_CLIENT_SINGLE_THREAD_RUNNER_ERROR (milter_client_single_thread_runner_error_quark())

GQuark milter_client_single_thread_runner_error_quark (void);

typedef enum
{
    MILTER_CLIENT_SINGLE_THREAD_RUNNER_ERROR_TODO
} MilterClientSingleThreadRunnerError;

#define MILTER_TYPE_CLIENT_SINGLE_THREAD_RUNNER (milter_client_single_thread_runner_get_type())
G_DECLARE_DERIVABLE_TYPE(MilterClientSingleThreadRunner,
                         milter_client_single_thread_runner,
                         MILTER,
                         CLIENT_SINGLE_THREAD_RUNNER,
                         MilterClientRunner)
struct _MilterClientSingleThreadRunnerClass
{
    MilterClientRunnerClass parent_class;
};

MilterClientRunner  *milter_client_single_thread_runner_new               (MilterClient *client);

G_END_DECLS

#endif /* __MILTER_CLIENT_SINGLE_THREAD_RUNNER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
