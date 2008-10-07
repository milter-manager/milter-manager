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

#include "milter-manager-ruby-internal.h"

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif /* HAVE_CONFIG_H */

#include "milter-manager-context.h"

#define MILTER_MANAGER_CONTEXT_GET_PRIVATE(obj)                         \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj),                                 \
                                 MILTER_MANAGER_TYPE_CONTEXT,           \
                                 MilterManagerContextPrivate))

typedef struct _MilterManagerContextPrivate	MilterManagerContextPrivate;
struct _MilterManagerContextPrivate
{
    gpointer private_data;
    GDestroyNotify private_data_destroy;
    guint reply_code;
    gchar *extended_reply_code;
    gchar *reply_message;
};

G_DEFINE_TYPE(MilterManagerContext, milter_manager_context, G_TYPE_OBJECT);

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
milter_manager_context_class_init (MilterManagerContextClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    g_type_class_add_private(gobject_class, sizeof(MilterManagerContextPrivate));
}

static void
milter_manager_context_init (MilterManagerContext *context)
{
    MilterManagerContextPrivate *priv;

    priv = MILTER_MANAGER_CONTEXT_GET_PRIVATE(context);
    priv->private_data = NULL;
    priv->private_data_destroy = NULL;
    priv->reply_code = 0;
    priv->extended_reply_code = NULL;
    priv->reply_message = NULL;
}

static void
dispose (GObject *object)
{
    MilterManagerContextPrivate *priv;

    priv = MILTER_MANAGER_CONTEXT_GET_PRIVATE(object);

    if (priv->private_data) {
        if (priv->private_data_destroy)
            priv->private_data_destroy(priv->private_data);
        priv->private_data = NULL;
    }
    priv->private_data_destroy = NULL;

    if (priv->extended_reply_code) {
        g_free(priv->extended_reply_code);
        priv->extended_reply_code = NULL;
    }

    if (priv->reply_message) {
        g_free(priv->reply_message);
        priv->reply_message = NULL;
    }

    G_OBJECT_CLASS(milter_manager_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    MilterManagerContextPrivate *priv;

    priv = MILTER_MANAGER_CONTEXT_GET_PRIVATE(object);
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
    MilterManagerContextPrivate *priv;

    priv = MILTER_MANAGER_CONTEXT_GET_PRIVATE(object);
    switch (prop_id) {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GQuark
milter_manager_context_error_quark (void)
{
    return g_quark_from_static_string("milter-manager-context-error-quark");
}

MilterManagerContext *
milter_manager_context_new (void)
{
    return g_object_new(MILTER_MANAGER_TYPE_CONTEXT,
                        NULL);
}

MilterStatus
milter_manager_context_option_negotiate (MilterManagerContext *context,
                                         MilterOption         *option)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("negotiate"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_connect (MilterManagerContext *context,
                                const gchar          *host_name,
                                struct sockaddr      *address,
                                socklen_t             address_length)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("connect"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_helo (MilterManagerContext *context,
                             const gchar          *fqdn)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("helo"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_envelope_from (MilterManagerContext *context,
                                      const gchar          *from)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("from"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_envelope_receipt (MilterManagerContext *context,
                                         const gchar          *receipt)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("receipt"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_data (MilterManagerContext *context)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("data"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_unknown (MilterManagerContext *context,
                                const gchar          *command)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("unknown"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_header (MilterManagerContext *context,
                               const gchar          *name,
                               const gchar          *value)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("header"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_end_of_header (MilterManagerContext *context)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("end of header"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_body (MilterManagerContext *context,
                             const guchar         *chunk,
                             gsize                 size)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("body"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_end_of_message (MilterManagerContext *context)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("end of message"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_close (MilterManagerContext *context)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("close"));
    return MILTER_STATUS_DEFAULT;
}

MilterStatus
milter_manager_context_abort (MilterManagerContext *context)
{
    rb_funcall(Qnil, rb_intern("p"), 1, rb_str_new2("abort"));
    return MILTER_STATUS_DEFAULT;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
