#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "c-preedit.h"

#define CHECK(expr)                                                        \
  do                                                                       \
  {                                                                        \
    if (!(expr))                                                           \
    {                                                                      \
      fprintf (stderr, "%s:%d: check failed: %s\n",                       \
               __FILE__, __LINE__, #expr);                                 \
      return false;                                                        \
    }                                                                      \
  } while (0)

static bool
has_attribute_range (PangoAttrList* attrs,
                     PangoAttrType type,
                     guint start_index,
                     guint end_index)
{
  PangoAttrIterator* iterator = pango_attr_list_get_iterator (attrs);
  bool found = false;

  do
  {
    PangoAttribute* attr = pango_attr_iterator_get (iterator, type);

    if (attr &&
        attr->start_index == start_index &&
        attr->end_index == end_index)
    {
      found = true;
      break;
    }
  } while (pango_attr_iterator_next (iterator));

  pango_attr_iterator_destroy (iterator);
  return found;
}

static bool
test_supplementary_scalar_range (void)
{
  static char preedit_text[] = "A\xF0\x9F\x98\x80" "B";
  CimTextAttr attr = {
    .type = CIM_TEXT_ATTR_UNDERLINE,
    .pos = 1,
    .n_chars = 1
  };
  CimPreedit preedit = {
    .text = preedit_text,
    .attrs = &attr,
    .attrs_len = 1,
    .cursor_pos = 2
  };
  char* text = NULL;
  PangoAttrList* attrs = NULL;
  int cursor_pos = -1;

  CHECK (c_preedit_to_gtk (&preedit, &text, &attrs, &cursor_pos));
  CHECK (text != NULL);
  CHECK (strcmp (text, preedit_text) == 0);
  CHECK (cursor_pos == 2);
  CHECK (attrs != NULL);

  CHECK (has_attribute_range
    (attrs, PANGO_ATTR_UNDERLINE, 1, 5));
  pango_attr_list_unref (attrs);
  g_free (text);
  return true;
}

static bool
test_highlight_range (void)
{
  static char preedit_text[] = "A\xF0\x9F\x98\x80" "B";
  CimTextAttr attr = {
    .type = CIM_TEXT_ATTR_HIGHLIGHT,
    .pos = 1,
    .n_chars = 1
  };
  CimPreedit preedit = {
    .text = preedit_text,
    .attrs = &attr,
    .attrs_len = 1,
    .cursor_pos = 1
  };
  PangoAttrList* attrs = NULL;
  CHECK (c_preedit_to_gtk (&preedit, NULL, &attrs, NULL));
  CHECK (attrs != NULL);
  CHECK (has_attribute_range
    (attrs, PANGO_ATTR_BACKGROUND, 1, 5));
  CHECK (has_attribute_range
    (attrs, PANGO_ATTR_FOREGROUND, 1, 5));
  pango_attr_list_unref (attrs);
  return true;
}

static bool
test_invalid_payloads (void)
{
  static char valid_text[] = "abc";
  static char invalid_utf8[] = "\xFF";
  CimTextAttr attr = {
    .type = CIM_TEXT_ATTR_UNDERLINE,
    .pos = 0,
    .n_chars = 1
  };
  CimPreedit preedit = {
    .text = valid_text,
    .attrs = &attr,
    .attrs_len = 1,
    .cursor_pos = 0
  };

  preedit.text = invalid_utf8;
  CHECK (!c_preedit_to_gtk (&preedit, NULL, NULL, NULL));

  preedit.text = valid_text;
  preedit.cursor_pos = 4;
  CHECK (!c_preedit_to_gtk (&preedit, NULL, NULL, NULL));

  preedit.cursor_pos = 0;
  attr.pos = 2;
  attr.n_chars = 2;
  CHECK (!c_preedit_to_gtk (&preedit, NULL, NULL, NULL));

  attr.pos = UINT32_MAX;
  attr.n_chars = 1;
  CHECK (!c_preedit_to_gtk (&preedit, NULL, NULL, NULL));

  attr.pos = 0;
  attr.n_chars = 1;
  attr.type = (CimTextAttrType) UINT32_MAX;
  CHECK (!c_preedit_to_gtk (&preedit, NULL, NULL, NULL));

  attr.type = CIM_TEXT_ATTR_UNDERLINE;
  preedit.attrs = NULL;
  CHECK (!c_preedit_to_gtk (&preedit, NULL, NULL, NULL));
  return true;
}

int
main (void)
{
  if (!test_supplementary_scalar_range () ||
      !test_highlight_range () ||
      !test_invalid_payloads ())
    return 1;

  puts ("GTK preedit conversion tests passed");
  return 0;
}
