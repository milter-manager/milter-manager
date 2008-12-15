/* -*- c-file-style: "ruby" -*- */

#ifndef __RB_MILTER_CORE_PRIVATE_H__
#define __RB_MILTER_CORE_PRIVATE_H__

#include "rb-milter-private.h"

void Init_milter_socket_address (void);
void Init_milter_utils (void);
void Init_milter_connection (void);
void Init_milter_socket_address (void);
void Init_milter_protocol (void);
void Init_milter_option (void);
void Init_milter_macros_requests (void);
void Init_milter_encoder (void);
void Init_milter_command_encoder (void);
void Init_milter_reply_encoder (void);
void Init_milter_decoder (void);

#define RVAL2ENCODER(obj)         (MILTER_ENCODER(RVAL2GOBJ(obj)))
#define RVAL2COMMAND_ENCODER(obj) (MILTER_COMMAND_ENCODER(RVAL2GOBJ(obj)))
#define RVAL2REPLY_ENCODER(obj)   (MILTER_REPLY_ENCODER(RVAL2GOBJ(obj)))
#define RVAL2DECODER(obj)         (MILTER_DECODER(RVAL2GOBJ(obj)))
#define RVAL2OPTION(obj)          (MILTER_OPTION(RVAL2GOBJ(obj)))
#define RVAL2MACROS_REQUESTS(obj) (MILTER_MACROS_REQUESTS(RVAL2GOBJ(obj)))

#define RVAL2ACTION_FLAGS(flags) (RVAL2GFLAGS(flags, MILTER_TYPE_ACTION_FLAGS))
#define RVAL2STEP_FLAGS(flags)   (RVAL2GFLAGS(flags, MILTER_TYPE_STEP_FLAGS))
#define RVAL2COMMAND(value)      (RVAL2GENUM(value, MILTER_TYPE_COMMAND))
#define COMMAND2RVAL(value)      (GENUM2RVAL(value, MILTER_TYPE_COMMAND))

#define RVAL2MACROS(hash)   (rb_milter__rval2macros(hash))
#define MACROS2RVAL(macros) (rb_milter__macros2rval(macros))

/* Workaround for past Ruby-Gnome2 */
#ifndef RSTRING_PTR
#  define RSTRING_PTR(s) (RSTRING(s)->ptr)
#  define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#ifndef RARRAY_PTR
#  define RARRAY_PTR(s) (RARRAY(s)->ptr)
#  define RARRAY_LEN(s) (RARRAY(s)->len)
#endif

GHashTable *rb_milter__rval2macros(VALUE rb_macros);
VALUE       rb_milter__macros2rval(GHashTable *macros);

VALUE rb_milter__connect_signal_convert(guint num, const GValue *values);

#endif
