/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif /* HAVE_CONFIG_H */

#include "libmilter-compatible.h"

G_DEFINE_TYPE(SmfiContext, smfi_context, G_TYPE_OBJECT);

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
smfi_context_class_init (SmfiContextClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;
}

static void
smfi_context_init (SmfiContext *context)
{
}

static void
dispose (GObject *object)
{
    SmfiContext *context;

    context = SMFI_CONTEXT(object);

    G_OBJECT_CLASS(smfi_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
smfi_context_error_quark (void)
{
    return g_quark_from_static_string("smfi-context-error-quark");
}

SmfiContext *
smfi_context_new (void)
{
    return g_object_new(SMFI_TYPE_CONTEXT,
                        NULL);
}

int
smfi_opensocket (bool remove_socket)
{
    return MI_SUCCESS;
}

int
smfi_register (struct smfiDesc description)
{
    return MI_SUCCESS;
}

int
smfi_main (void)
{
    MilterClient *client;

    g_type_init();

    client = milter_client_new();
    milter_client_main(client);
    g_object_unref(client);

    return MI_SUCCESS;
}

int
smfi_setbacklog (int backlog)
{
    return MI_SUCCESS;
}

int
smfi_setdbg (int level)
{
    return MI_SUCCESS;
}

int
smfi_settimeout (int timeout)
{
    return MI_SUCCESS;
}

int
smfi_setconn (char *connection_spec)
{
    return MI_SUCCESS;
}

int
smfi_stop (void)
{
    return MI_SUCCESS;
}

int
smfi_version (unsigned int *major, unsigned int *minor,
              unsigned int *patch_level)
{
    return MI_SUCCESS;
}

char *
smfi_getsymval (SMFICTX *context, char *name)
{
    return NULL;
}

int
smfi_setreply (SMFICTX *context,
               char *return_code, char *extended_code, char *message)
{
    return MI_SUCCESS;
}

int
smfi_setmlreply (SMFICTX *context,
                 const char *return_code, const char *extended_code, ...)
{
    return MI_SUCCESS;
}

int
smfi_addheader (SMFICTX *context, char *name, char *value)
{
    return MI_SUCCESS;
}

int
smfi_chgheader (SMFICTX *context, char *name, int index, char *value)
{
    return MI_SUCCESS;
}

int
smfi_insheader (SMFICTX *context, int index, char *name, char *value)
{
    return MI_SUCCESS;
}

int
smfi_chgfrom (SMFICTX *context, char *mail, char *args)
{
    return MI_SUCCESS;
}

int
smfi_addrcpt (SMFICTX *context, char *receipt)
{
    return MI_SUCCESS;
}

int
smfi_addrcpt_par (SMFICTX *context, char *receipt, char *args)
{
    return MI_SUCCESS;
}

int
smfi_delrcpt (SMFICTX *context, char *receipt)
{
    return MI_SUCCESS;
}

int
smfi_progress (SMFICTX *context)
{
    return MI_SUCCESS;
}

int
smfi_replacebody (SMFICTX *context, unsigned char *new_body, int new_body_size)
{
    return MI_SUCCESS;
}

int
smfi_quarantine (SMFICTX *context, char *reason)
{
    return MI_SUCCESS;
}

int
smfi_setpriv (SMFICTX *context, void *data)
{
    return MI_SUCCESS;
}

void *
smfi_getpriv (SMFICTX *context)
{
    return NULL;
}

int
smfi_setsymlist (SMFICTX *context, int where, char *macros)
{
    return MI_SUCCESS;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
