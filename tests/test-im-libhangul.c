/******************************************************************************
 * test-im-libhangul.c
 * Copyright (C) 2023-2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 ******************************************************************************/
#include "cim.h"
#include "test-common.h"
#include <clair.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
  CIM_KEY_space        = 0x020,
  CIM_KEY_1            = 0x031,
  CIM_KEY_Hangul       = 0xff31,
  CIM_KEY_Hangul_Hanja = 0xff34
};

typedef struct
{
  bool failed;
  bool saw_multibyte_preedit;
  int preedit_start_count;
  int preedit_end_count;
  int preedit_changed_count;
  int commit_count;
  int candidate_show_count;
  int candidate_hide_count;
  int candidate_changed_count;
  int candidate_selected_count;
} TestState;

static void
on_preedit_start (CimIcHandle ic, void *user_data)
{
  TestState *st = (TestState *) user_data;
  (void) ic;
  st->preedit_start_count++;
  printf ("[cb] preedit_start\n");
}

static void
on_preedit_end (CimIcHandle ic, void *user_data)
{
  TestState *st = (TestState *) user_data;
  (void) ic;
  st->preedit_end_count++;
  printf ("[cb] preedit_end\n");
}

static void
on_preedit_changed (CimIcHandle ic, const CimPreedit *preedit, void *user_data)
{
  TestState *st = (TestState *) user_data;
  size_t byte_count;
  size_t scalar_count;

  (void) ic;
  st->preedit_changed_count++;

  if (preedit != NULL && preedit->text != NULL)
  {
    byte_count = strlen (preedit->text);
    printf ("[cb] preedit_changed: text=\"%s\" cursor=%u\n",
            preedit->text,
            preedit->cursor_pos);

    if (clair_utf8_scalar_count
          (preedit->text, byte_count, &scalar_count) != CLAIR_OK)
    {
      fprintf (stderr, "invalid UTF-8 preedit: \"%s\"\n", preedit->text);
      st->failed = true;
      return;
    }

    if (byte_count > scalar_count)
      st->saw_multibyte_preedit = true;

    if (preedit->cursor_pos != scalar_count)
    {
      fprintf (stderr,
               "preedit cursor unit mismatch: text=\"%s\" cursor=%u "
               "scalars=%zu\n",
               preedit->text,
               preedit->cursor_pos,
               scalar_count);
      st->failed = true;
    }

    if (preedit->attrs_len != 0 && preedit->attrs == NULL)
    {
      fprintf (stderr, "preedit attributes are null\n");
      st->failed = true;
      return;
    }

    for (uint32_t index = 0; index < preedit->attrs_len; index++)
    {
      uint64_t end = (uint64_t) preedit->attrs[index].pos +
                     preedit->attrs[index].n_chars;

      if (end > scalar_count)
      {
        fprintf (stderr,
                 "preedit attribute range exceeds scalar count: "
                 "pos=%u length=%u scalars=%zu\n",
                 preedit->attrs[index].pos,
                 preedit->attrs[index].n_chars,
                 scalar_count);
        st->failed = true;
      }
    }
  }
  else
    printf ("[cb] preedit_changed: (null)\n");
}

static void
on_commit (CimIcHandle ic, const char *text, void *user_data)
{
  TestState *st = (TestState *) user_data;
  (void) ic;
  st->commit_count++;

  printf ("[cb] commit: \"%s\"\n", text != NULL ? text : "(null)");
}

static const CimSurround *
on_get_surround (CimIcHandle ic, void *user_data)
{
  static CimSurround surround = {
    .text = "",
    .len = 0,
    .cursor_pos = 0
  };

  (void) ic;
  (void) user_data;
  return &surround;
}

static bool
on_delete_surround (CimIcHandle ic, int32_t offset, uint32_t n_chars, void *user_data)
{
  (void) ic;
  (void) user_data;
  printf ("[cb] delete_surround: offset=%d n_chars=%u\n", offset, n_chars);
  return true;
}

static void
on_candidate_show (CimIcHandle ic,
                   uint32_t n_rows,
                   uint32_t n_cols,
                   bool has_aux,
                   void *user_data)
{
  TestState *st = (TestState *) user_data;
  (void) ic;
  st->candidate_show_count++;

  printf ("[cb] candidate_show: rows=%u cols=%u aux=%s\n",
          n_rows,
          n_cols,
          has_aux ? "true" : "false");
}

static void
on_candidate_hide (CimIcHandle ic, void *user_data)
{
  TestState *st = (TestState *) user_data;
  (void) ic;
  st->candidate_hide_count++;
  printf ("[cb] candidate_hide\n");
}

static void
on_candidate_changed (CimIcHandle ic, const CimCandidate *candidate, void *user_data)
{
  TestState *st = (TestState *) user_data;
  (void) ic;
  st->candidate_changed_count++;

  printf ("[cb] candidate_changed: page=%u/%u rows=%u cols=%u\n",
          candidate != NULL ? candidate->page_index : 0,
          candidate != NULL ? candidate->n_pages : 0,
          candidate != NULL ? candidate->n_rows : 0,
          candidate != NULL ? candidate->n_cols : 0);

  if (candidate != NULL && candidate->table != NULL)
  {
    for (uint32_t r = 0; r < candidate->n_rows; r++)
    {
      uint32_t base = r * candidate->n_cols;
      const char *label = candidate->table[base + 0].data;
      const char *text  = candidate->table[base + 1].data;
      const char *desc  = candidate->table[base + 2].data;

      printf ("  [%u] %s | %s | %s\n",
              r,
              label != NULL ? label : "",
              text  != NULL ? text  : "",
              desc  != NULL ? desc  : "");
    }
  }
}

static void
on_candidate_selected (CimIcHandle ic, const CimSelection *sel, void *user_data)
{
  TestState *st = (TestState *) user_data;
  (void) ic;
  st->candidate_selected_count++;

  if (sel != NULL)
  {
    printf ("[cb] candidate_selected: row=%u col=%u\n",
            sel->start_row,
            sel->start_col);
  }
  else
  {
    printf ("[cb] candidate_selected: (null)\n");
  }
}

static void
send_key (CimIcHandle ic, uint32_t keyval, uint32_t keycode, uint32_t state)
{
  CimEvent ev;

  memset (&ev, 0, sizeof (ev));
  ev.type = CIM_EVENT_KEY_PRESS;
  ev.keyval = keyval;
  ev.keycode = keycode;
  ev.state = state;

  printf ("[send] keyval=0x%x keycode=%u state=0x%x\n",
          keyval, keycode, state);

  printf ("[send] handled=%s\n",
          cim_ic_filter_event (ic, &ev) ? "true" : "false");
}

int
main (void)
{
  CimIcHandle ic;
  CimCallbacks cb;
  TestState st;

  if (test_set_plugin_env ("lib/im-libhangul.so") != 0)
    return 1;

  memset (&st, 0, sizeof (st));
  memset (&cb, 0, sizeof (cb));

  cb.preedit_start = on_preedit_start;
  cb.preedit_end = on_preedit_end;
  cb.preedit_changed = on_preedit_changed;
  cb.commit = on_commit;
  cb.get_surround = on_get_surround;
  cb.delete_surround = on_delete_surround;
  cb.candidate_show = on_candidate_show;
  cb.candidate_hide = on_candidate_hide;
  cb.candidate_changed = on_candidate_changed;
  cb.candidate_selected = on_candidate_selected;

  ic = cim_ic_create ();
  if (ic == NULL)
  {
    fprintf (stderr, "cim_ic_create failed: %s\n",
             cim_strerror (cim_get_last_error ()));
    return 1;
  }

  cim_ic_set_callbacks (ic, &cb, &st);

  send_key (ic, CIM_KEY_Hangul, 0, 0);

  /*
   * 2-set Korean keycodes in your plugin mapping:
   * r=27, k=45, f=41, etc.
   *
   * "gks" 같은 식으로 직접 넣어도 되고,
   * 우선 간단히 자모 조합이 되는지만 본다.
   */
  send_key (ic, 'r', 27, 0);
  send_key (ic, 'k', 45, 0);
  send_key (ic, 's', 39, 0);

  /*
   * 조합 종료 테스트
   */
  send_key (ic, CIM_KEY_space, 65, 0);

  /*
   * Hanja candidate test
   * preedit 또는 주변 글자 기준으로 후보창이 뜨는지 확인
   */
  send_key (ic, 'g', 43, 0);
  send_key (ic, 'k', 45, 0);
  send_key (ic, 's', 39, 0);

  /*
   * Hangul_Hanja
   */
  send_key (ic, CIM_KEY_Hangul_Hanja, 0, 0);

  /*
   * 후보 선택 테스트
   */
  send_key (ic, CIM_KEY_1, 0, 0);

  cim_ic_destroy (ic);
  test_unset_plugin_env ();

  printf ("\n=== summary ===\n");
  printf ("preedit_start      = %d\n", st.preedit_start_count);
  printf ("preedit_end        = %d\n", st.preedit_end_count);
  printf ("preedit_changed    = %d\n", st.preedit_changed_count);
  printf ("commit             = %d\n", st.commit_count);
  printf ("candidate_show     = %d\n", st.candidate_show_count);
  printf ("candidate_hide     = %d\n", st.candidate_hide_count);
  printf ("candidate_changed  = %d\n", st.candidate_changed_count);
  printf ("candidate_selected = %d\n", st.candidate_selected_count);

  if (!st.saw_multibyte_preedit)
  {
    fprintf (stderr, "no multibyte preedit was observed\n");
    st.failed = true;
  }

  return st.failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
