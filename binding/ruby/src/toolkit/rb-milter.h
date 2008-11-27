/* -*- c-file-style: "ruby" -*- */

#ifndef __RB_MILTER_H__
#define __RB_MILTER_H__

#include <rbgobject.h>
#include <milter/core.h>
#include <milter/client.h>
#include <milter/server.h>

VALUE rb_mMilter;
VALUE rb_mMilterCore;
VALUE rb_mMilterClient;
VALUE rb_mMilterServer;
VALUE rb_eMilterError;
VALUE rb_cMilterSocketAddressIPv4;
VALUE rb_cMilterSocketAddressIPv6;
VALUE rb_cMilterSocketAddressUnix;

#define CSTR2RVAL_SIZE_FREE(string, size)		\
    (rb_milter_cstr2rval_size_free(string, size))

VALUE rb_milter_cstr2rval_size_free(gchar *string, gsize size);


#endif
