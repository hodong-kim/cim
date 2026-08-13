#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include "c-candidate-data.h"

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
test_candidate_payloads (void)
{
  static char first_text[] = "A";
  static char second_text[] = "\xF0\x9F\x98\x80";
  static char auxiliary_text[] = "aux";
  static char invalid_utf8[] = "\xFF";
  CimItem items[2] = {
    {
      .data = first_text,
      .type = CIM_ITEM_STRING,
      .padding = 0
    },
    {
      .data = second_text,
      .type = CIM_ITEM_STRING,
      .padding = 0
    }
  };
  CimCandidate candidate = {
    .page_index = 1,
    .n_pages = 2,
    .table = items,
    .n_rows = 1,
    .n_cols = 2,
    .aux_text = auxiliary_text,
    .aux_cursor_pos = 1,
    .padding = 0
  };

  CHECK (c_candidate_validation_error (&candidate) == NULL);

  candidate.page_index = 2;
  CHECK (c_candidate_validation_error (&candidate) != NULL);
  candidate.page_index = 1;

  candidate.table = NULL;
  CHECK (c_candidate_validation_error (&candidate) != NULL);
  candidate.table = items;

  candidate.n_rows = (uint32_t) INT_MAX + 1U;
  CHECK (c_candidate_validation_error (&candidate) != NULL);
  candidate.n_rows = 1;

  candidate.aux_cursor_pos = (uint32_t) INT_MAX + 1U;
  CHECK (c_candidate_validation_error (&candidate) != NULL);
  candidate.aux_cursor_pos = 1;

  candidate.aux_text = invalid_utf8;
  CHECK (c_candidate_validation_error (&candidate) != NULL);
  candidate.aux_text = auxiliary_text;

  items[1].type = (CimItemType) UINT32_MAX;
  CHECK (c_candidate_validation_error (&candidate) != NULL);
  items[1].type = CIM_ITEM_STRING;

  items[1].data = NULL;
  CHECK (c_candidate_validation_error (&candidate) != NULL);
  items[1].data = second_text;

  items[1].data = invalid_utf8;
  CHECK (c_candidate_validation_error (&candidate) != NULL);
  items[1].data = second_text;

  candidate.n_pages = 0;
  candidate.table = NULL;
  CHECK (c_candidate_validation_error (&candidate) == NULL);
  return true;
}

static bool
test_candidate_selection (void)
{
  CimSelection selection = {
    .start_row = 0,
    .start_col = 0,
    .end_row = 1,
    .end_col = 2
  };

  CHECK (c_candidate_selection_validation_error
    (&selection, 2, 3) == NULL);

  selection.start_row = 2;
  CHECK (c_candidate_selection_validation_error
    (&selection, 2, 3) != NULL);
  selection.start_row = 0;

  selection.end_row = 2;
  CHECK (c_candidate_selection_validation_error
    (&selection, 2, 3) != NULL);
  selection.end_row = 1;

  selection.end_col = 3;
  CHECK (c_candidate_selection_validation_error
    (&selection, 2, 3) != NULL);
  return true;
}

static bool
test_candidate_window_bounds (void)
{
  int width;
  int height;

  CHECK (c_candidate_measure_window (2, 3, FALSE, 27, &width, &height));
  CHECK (width == 324);
  CHECK (height == 54);

  CHECK (c_candidate_measure_window (2, 3, TRUE, 27, &width, &height));
  CHECK (width == 324);
  CHECK (height == 81);

  CHECK (!c_candidate_measure_window
    ((uint32_t) INT_MAX, 1, FALSE, 27, &width, &height));
  CHECK (!c_candidate_measure_window
    (1, (uint32_t) INT_MAX, FALSE, 27, &width, &height));
  CHECK (!c_candidate_measure_window (1, 1, FALSE, -1, &width, &height));
  CHECK (!c_candidate_measure_window (1, 1, FALSE, 27, NULL, &height));
  return true;
}

int
main (void)
{
  if (!test_candidate_payloads () ||
      !test_candidate_selection () ||
      !test_candidate_window_bounds ())
    return 1;

  puts ("GTK candidate validation tests passed");
  return 0;
}
