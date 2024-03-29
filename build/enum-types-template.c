/* -*- c -*- */

/*** BEGIN file-header ***/
#include <config.h>
#include "@enum_types_header@"
/*** END file-header ***/

/*** BEGIN file-production ***/

/* enumerations from "@filename@" */
#include "@filename@"
/*** END file-production ***/

/*** BEGIN value-header ***/
GType
@enum_name@_get_type(void)
{
  static GType etype = 0;
  if (G_UNLIKELY(etype == 0)) {
    static const G@Type@Value values[] = {
/*** END value-header ***/

/*** BEGIN value-production ***/
      {@VALUENAME@, "@VALUENAME@", "@valuenick@"},
/*** END value-production ***/

/*** BEGIN value-tail ***/
      {0, NULL, NULL}
    };
    etype = g_@type@_register_static(g_intern_static_string("@EnumName@"), values);
  }
  return etype;
}
/*** END value-tail ***/

/*** BEGIN file-tail ***/
/*** END file-tail ***/
