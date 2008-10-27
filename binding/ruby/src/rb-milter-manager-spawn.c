/* -*- c-file-style: "ruby" -*- */

#include "rb-milter-manager-private.h"

static VALUE
initialize (VALUE self, VALUE name)
{
    G_INITIALIZE(self, milter_manager_spawn_new(RVAL2CSTR(name)));
    return Qnil;
}

void
Init_milter_manager_spawn (void)
{
    VALUE rb_cMilterManagerSpawn;

    rb_cMilterManagerSpawn =
	G_DEF_CLASS(MILTER_TYPE_MANAGER_SPAWN, "Spawn", rb_mMilterManager);

    rb_define_method(rb_cMilterManagerSpawn, "initialize",
                     initialize, 1);
}
