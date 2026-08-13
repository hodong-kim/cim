#ifndef C_PREEDIT_H
#define C_PREEDIT_H

#include <glib.h>
#include <pango/pango.h>
#include "cim.h"

G_BEGIN_DECLS

gboolean c_preedit_to_gtk (const CimPreedit* preedit,
                            char** text,
                            PangoAttrList** attrs,
                            int* cursor_pos);

G_END_DECLS

#endif
