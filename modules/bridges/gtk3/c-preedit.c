#include <limits.h>
#include <stdint.h>
#include <string.h>
#include "c-preedit.h"

static gboolean
c_preedit_validate (const CimPreedit* preedit)
{
  const char* p;
  guint64 scalar_count = 0;
  gsize byte_length;

  if (!preedit || !preedit->text ||
      (preedit->attrs_len != 0 && !preedit->attrs))
    return FALSE;

  if (!g_utf8_validate (preedit->text, -1, NULL))
    return FALSE;

  byte_length = strlen (preedit->text);
  if (byte_length >= G_MAXUINT)
    return FALSE;

  for (p = preedit->text; *p; p = g_utf8_next_char (p))
    scalar_count++;

  if ((guint64) preedit->cursor_pos > scalar_count ||
      preedit->cursor_pos > (uint32_t) INT_MAX)
    return FALSE;

  for (uint32_t i = 0; i < preedit->attrs_len; i++)
  {
    const CimTextAttr* source = &preedit->attrs[i];
    guint64 end_scalar = (guint64) source->pos + source->n_chars;

    if (source->type != CIM_TEXT_ATTR_UNDERLINE &&
        source->type != CIM_TEXT_ATTR_HIGHLIGHT)
      return FALSE;

    if ((guint64) source->pos > scalar_count ||
        end_scalar > scalar_count ||
        (guint64) source->pos > (guint64) G_MAXLONG ||
        end_scalar > (guint64) G_MAXLONG)
      return FALSE;
  }

  return TRUE;
}

static void
c_preedit_set_range (PangoAttribute* attr,
                     const char* text,
                     const CimTextAttr* source)
{
  const char* start;
  const char* end;

  start = g_utf8_offset_to_pointer (text, (glong) source->pos);
  end = g_utf8_offset_to_pointer
    (text, (glong) ((guint64) source->pos + source->n_chars));

  attr->start_index = (guint) (start - text);
  attr->end_index = (guint) (end - text);
}

gboolean
c_preedit_to_gtk (const CimPreedit* preedit,
                  char** text,
                  PangoAttrList** attrs,
                  int* cursor_pos)
{
  PangoAttrList* converted_attrs = NULL;

  if (!c_preedit_validate (preedit))
    return FALSE;

  if (attrs)
  {
    converted_attrs = pango_attr_list_new ();

    for (uint32_t i = 0; i < preedit->attrs_len; i++)
    {
      const CimTextAttr* source = &preedit->attrs[i];
      PangoAttribute* attr;

      if (source->type == CIM_TEXT_ATTR_UNDERLINE)
      {
        attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
        c_preedit_set_range (attr, preedit->text, source);
        pango_attr_list_insert (converted_attrs, attr);
        continue;
      }

      attr = pango_attr_background_new (0, 0xffff, 0);
      c_preedit_set_range (attr, preedit->text, source);
      pango_attr_list_insert (converted_attrs, attr);

      attr = pango_attr_foreground_new (0, 0, 0);
      c_preedit_set_range (attr, preedit->text, source);
      pango_attr_list_insert (converted_attrs, attr);
    }
  }

  if (text)
    *text = g_strdup (preedit->text);

  if (attrs)
    *attrs = converted_attrs;

  if (cursor_pos)
    *cursor_pos = (int) preedit->cursor_pos;

  return TRUE;
}
