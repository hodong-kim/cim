#include <limits.h>
#include <stdint.h>
#include "c-candidate-data.h"

const char*
c_candidate_validation_error (const CimCandidate* candidate)
{
  gsize n_items;

  if (!candidate)
    return "candidate payload is NULL";

  if (candidate->n_pages != 0 &&
      candidate->page_index >= candidate->n_pages)
    return "candidate page index is out of range";

  if (candidate->n_rows > (uint32_t) INT_MAX ||
      candidate->n_cols > (uint32_t) INT_MAX)
    return "candidate dimensions exceed toolkit limits";

  if (candidate->n_rows != 0 &&
      (gsize) candidate->n_cols > G_MAXSIZE / candidate->n_rows)
    return "candidate table size overflows addressable memory";

  if (candidate->aux_text &&
      !g_utf8_validate (candidate->aux_text, -1, NULL))
    return "candidate auxiliary text is not valid UTF-8";

  if (candidate->n_pages == 0)
    return NULL;

  if (candidate->aux_text &&
      candidate->aux_cursor_pos > (uint32_t) INT_MAX)
    return "candidate auxiliary cursor exceeds toolkit limits";

  if (!candidate->table)
    return "candidate table is NULL";

  n_items = (gsize) candidate->n_rows * candidate->n_cols;

  for (gsize i = 0; i < n_items; i++)
  {
    const CimItem* item = &candidate->table[i];

    if (item->type != CIM_ITEM_STRING)
      return "candidate item type is invalid";

    if (!item->data)
      return "candidate item payload is NULL";

    if (!g_utf8_validate ((const char*) item->data, -1, NULL))
      return "candidate item text is not valid UTF-8";
  }

  return NULL;
}

const char*
c_candidate_selection_validation_error (const CimSelection* selection,
                                         uint32_t n_rows,
                                         uint32_t n_cols)
{
  if (!selection)
    return "candidate selection is NULL";

  if (n_rows > (uint32_t) INT_MAX || n_cols > (uint32_t) INT_MAX)
    return "candidate selection dimensions exceed toolkit limits";

  if (selection->start_row > selection->end_row ||
      selection->start_col > selection->end_col)
    return "candidate selection range is reversed";

  if (selection->end_row >= n_rows || selection->end_col >= n_cols)
    return "candidate selection is out of range";

  return NULL;
}

gboolean
c_candidate_measure_window (uint32_t n_rows,
                            uint32_t n_cols,
                            gboolean show_aux,
                            int cell_height,
                            int* width,
                            int* height)
{
  uint64_t row_count;
  uint64_t width_cells;
  uint64_t converted_width;
  uint64_t converted_height;

  if (!width || !height || cell_height < 0)
    return FALSE;

  if (n_rows > (uint32_t) INT_MAX || n_cols > (uint32_t) INT_MAX)
    return FALSE;

  row_count = (uint64_t) n_rows + (show_aux ? 1U : 0U);
  width_cells = (uint64_t) n_cols * 4U;

  if (cell_height != 0 &&
      (row_count > (uint64_t) INT_MAX / (uint64_t) cell_height ||
       width_cells > (uint64_t) INT_MAX / (uint64_t) cell_height))
    return FALSE;

  converted_height = (uint64_t) cell_height * row_count;
  converted_width = (uint64_t) cell_height * width_cells;

  if (converted_height > (uint64_t) INT_MAX ||
      converted_width > (uint64_t) INT_MAX)
    return FALSE;

  *height = (int) converted_height;
  *width = (int) converted_width;
  return TRUE;
}
