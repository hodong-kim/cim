#ifndef __C_CANDIDATE_DATA_H__
#define __C_CANDIDATE_DATA_H__

#include <glib.h>
#include <cim.h>

G_BEGIN_DECLS

const char* c_candidate_validation_error
  (const CimCandidate* candidate);
const char* c_candidate_selection_validation_error
  (const CimSelection* selection,
   uint32_t n_rows,
   uint32_t n_cols);
gboolean c_candidate_measure_window
  (uint32_t n_rows,
   uint32_t n_cols,
   gboolean show_aux,
   int cell_height,
   int* width,
   int* height);

G_END_DECLS

#endif /* __C_CANDIDATE_DATA_H__ */
