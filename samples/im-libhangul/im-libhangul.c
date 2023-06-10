/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/*
 * im-libhangul.c
 * This file is part of Cim.
 *
 * Copyright (C) 2023 Hodong Kim <hodong@nimfsoft.art>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <hangul.h>
#include "cim.h"
#include <stdatomic.h>
#include <stdlib.h>
#include "c-str.h"
#include "c-mem.h"

#define N_ROWS 10
#define N_COLS  3

typedef enum
{
  CIM_SHIFT_MASK    = 1 << 0,  /* Shift */
  CIM_LOCK_MASK     = 1 << 1,  /* Lock */
  CIM_CONTROL_MASK  = 1 << 2,  /* Control */
  CIM_MOD1_MASK     = 1 << 3,  /* Mod1 */
  CIM_MOD2_MASK     = 1 << 4,  /* Mod2 */
  CIM_MOD3_MASK     = 1 << 5,  /* Mod3 */
  CIM_MOD4_MASK     = 1 << 6,  /* Mod4 */
  CIM_MOD5_MASK     = 1 << 7,  /* Mod5 */
  CIM_BUTTON1_MASK  = 1 << 8,  /* Button1 */
  CIM_BUTTON2_MASK  = 1 << 9,  /* Button2 */
  CIM_BUTTON3_MASK  = 1 << 10, /* Button3 */
  CIM_BUTTON4_MASK  = 1 << 11, /* Button4 */
  CIM_BUTTON5_MASK  = 1 << 12, /* Button5 */

  /* virtual modifiers */
  CIM_SUPER_MASK    = 1 << 26, /* Super */
  CIM_HYPER_MASK    = 1 << 27, /* Hyper */
  CIM_META_MASK     = 1 << 28, /* Meta */

  /* Combination of the above MASKs */
  CIM_MODIFIER_MASK = 0x1c001fff
} CimModifierType;

typedef enum
{
  CIM_KEY_space            = 0x020, /* space */
  CIM_KEY_exclam           = 0x021, /* exclam */
  CIM_KEY_quotedbl         = 0x022, /* quotedbl */
  CIM_KEY_numbersign       = 0x023, /* numbersign */
  CIM_KEY_dollar           = 0x024, /* dollar */
  CIM_KEY_percent          = 0x025, /* percent */
  CIM_KEY_ampersand        = 0x026, /* ampersand */
  CIM_KEY_apostrophe       = 0x027, /* apostrophe */
  CIM_KEY_parenleft        = 0x028, /* parenleft */
  CIM_KEY_parenright       = 0x029, /* parenright */
  CIM_KEY_asterisk         = 0x02a, /* asterisk */
  CIM_KEY_plus             = 0x02b, /* plus */
  CIM_KEY_comma            = 0x02c, /* comma */
  CIM_KEY_minus            = 0x02d, /* minus */
  CIM_KEY_period           = 0x02e, /* period */
  CIM_KEY_slash            = 0x02f, /* slash */
  CIM_KEY_0                = 0x030, /* 0 */
  CIM_KEY_1                = 0x031, /* 1 */
  CIM_KEY_2                = 0x032, /* 2 */
  CIM_KEY_3                = 0x033, /* 3 */
  CIM_KEY_4                = 0x034, /* 4 */
  CIM_KEY_5                = 0x035, /* 5 */
  CIM_KEY_6                = 0x036, /* 6 */
  CIM_KEY_7                = 0x037, /* 7 */
  CIM_KEY_8                = 0x038, /* 8 */
  CIM_KEY_9                = 0x039, /* 9 */
  CIM_KEY_colon            = 0x03a, /* colon */
  CIM_KEY_semicolon        = 0x03b, /* semicolon */
  CIM_KEY_less             = 0x03c, /* less */
  CIM_KEY_equal            = 0x03d, /* equal */
  CIM_KEY_greater          = 0x03e, /* greater */
  CIM_KEY_question         = 0x03f, /* question */
  CIM_KEY_at               = 0x040, /* at */
  CIM_KEY_A                = 0x041, /* A */
  CIM_KEY_B                = 0x042, /* B */
  CIM_KEY_C                = 0x043, /* C */
  CIM_KEY_D                = 0x044, /* D */
  CIM_KEY_E                = 0x045, /* E */
  CIM_KEY_F                = 0x046, /* F */
  CIM_KEY_G                = 0x047, /* G */
  CIM_KEY_H                = 0x048, /* H */
  CIM_KEY_I                = 0x049, /* I */
  CIM_KEY_J                = 0x04a, /* J */
  CIM_KEY_K                = 0x04b, /* K */
  CIM_KEY_L                = 0x04c, /* L */
  CIM_KEY_M                = 0x04d, /* M */
  CIM_KEY_N                = 0x04e, /* N */
  CIM_KEY_O                = 0x04f, /* O */
  CIM_KEY_P                = 0x050, /* P */
  CIM_KEY_Q                = 0x051, /* Q */
  CIM_KEY_R                = 0x052, /* R */
  CIM_KEY_S                = 0x053, /* S */
  CIM_KEY_T                = 0x054, /* T */
  CIM_KEY_U                = 0x055, /* U */
  CIM_KEY_V                = 0x056, /* V */
  CIM_KEY_W                = 0x057, /* W */
  CIM_KEY_X                = 0x058, /* X */
  CIM_KEY_Y                = 0x059, /* Y */
  CIM_KEY_Z                = 0x05a, /* Z */
  CIM_KEY_bracketleft      = 0x05b, /* bracketleft */
  CIM_KEY_backslash        = 0x05c, /* backslash */
  CIM_KEY_bracketright     = 0x05d, /* bracketright */
  CIM_KEY_asciicircum      = 0x05e, /* asciicircum */
  CIM_KEY_underscore       = 0x05f, /* underscore */
  CIM_KEY_grave            = 0x060, /* grave */
  CIM_KEY_a                = 0x061, /* a */
  CIM_KEY_b                = 0x062, /* b */
  CIM_KEY_c                = 0x063, /* c */
  CIM_KEY_d                = 0x064, /* d */
  CIM_KEY_e                = 0x065, /* e */
  CIM_KEY_f                = 0x066, /* f */
  CIM_KEY_g                = 0x067, /* g */
  CIM_KEY_h                = 0x068, /* h */
  CIM_KEY_i                = 0x069, /* i */
  CIM_KEY_j                = 0x06a, /* j */
  CIM_KEY_k                = 0x06b, /* k */
  CIM_KEY_l                = 0x06c, /* l */
  CIM_KEY_m                = 0x06d, /* m */
  CIM_KEY_n                = 0x06e, /* n */
  CIM_KEY_o                = 0x06f, /* o */
  CIM_KEY_p                = 0x070, /* p */
  CIM_KEY_q                = 0x071, /* q */
  CIM_KEY_r                = 0x072, /* r */
  CIM_KEY_s                = 0x073, /* s */
  CIM_KEY_t                = 0x074, /* t */
  CIM_KEY_u                = 0x075, /* u */
  CIM_KEY_v                = 0x076, /* v */
  CIM_KEY_w                = 0x077, /* w */
  CIM_KEY_x                = 0x078, /* x */
  CIM_KEY_y                = 0x079, /* y */
  CIM_KEY_z                = 0x07a, /* z */
  CIM_KEY_braceleft        = 0x07b, /* braceleft */
  CIM_KEY_bar              = 0x07c, /* bar */
  CIM_KEY_braceright       = 0x07d, /* braceright */
  CIM_KEY_asciitilde       = 0x07e, /* asciitilde */

  CIM_KEY_ISO_Level3_Shift = 0xfe03, /* ISO_Level3_Shift */
  CIM_KEY_ISO_Left_Tab     = 0xfe20, /* ISO_Left_Tab */

  CIM_KEY_BackSpace        = 0xff08, /* BackSpace */
  CIM_KEY_Tab              = 0xff09, /* Tab */

  CIM_KEY_Return           = 0xff0d, /* Return */

  CIM_KEY_Pause            = 0xff13, /* Pause */
  CIM_KEY_Scroll_Lock      = 0xff14, /* Scroll_Lock */
  CIM_KEY_Sys_Req          = 0xff15, /* Sys_Req */

  CIM_KEY_Escape           = 0xff1b, /* Escape */

  CIM_KEY_Multi_key        = 0xff20, /* Multi_key */
  CIM_KEY_Kanji            = 0xff21, /* Kanji */

  CIM_KEY_Kana_Shift       = 0xff2e, /* Kana_Shift */

  CIM_KEY_Hangul           = 0xff31, /* Hangul */

  CIM_KEY_Hangul_Hanja     = 0xff34, /* Hangul_Hanja */

  CIM_KEY_Home             = 0xff50, /* Home */
  CIM_KEY_Left             = 0xff51, /* Left */
  CIM_KEY_Up               = 0xff52, /* Up */
  CIM_KEY_Right            = 0xff53, /* Right */
  CIM_KEY_Down             = 0xff54, /* Down */
  CIM_KEY_Page_Up          = 0xff55, /* Page_Up */
  CIM_KEY_Page_Down        = 0xff56, /* Page_Down */
  CIM_KEY_End              = 0xff57, /* End */

  CIM_KEY_Print            = 0xff61, /* Print */
  CIM_KEY_Execute          = 0xff62, /* Execut */
  CIM_KEY_Insert           = 0xff63, /* Insert */

  CIM_KEY_Menu             = 0xff67, /* Menu */

  CIM_KEY_Break            = 0xff6b, /* Break */

  CIM_KEY_KP_Enter         = 0xff8d, /* KP_Enter */

  CIM_KEY_KP_Left          = 0xff96, /* KP_Left */
  CIM_KEY_KP_Up            = 0xff97, /* KP_Up */
  CIM_KEY_KP_Right         = 0xff98, /* KP_Right */
  CIM_KEY_KP_Down          = 0xff99, /* KP_Down */
  CIM_KEY_KP_Page_Up       = 0xff9a, /* KP_Page_Up */
  CIM_KEY_KP_Page_Down     = 0xff9b, /* KP_Page_Down */

  CIM_KEY_KP_Delete        = 0xff9f, /* KP_Delete */

  CIM_KEY_KP_Multiply      = 0xffaa, /* KP_Multiply */
  CIM_KEY_KP_Add           = 0xffab, /* KP_Add */

  CIM_KEY_KP_Subtract      = 0xffad, /* KP_Subtract */
  CIM_KEY_KP_Decimal       = 0xffae, /* KP_Decimal */
  CIM_KEY_KP_Divide        = 0xffaf, /* KP_Divide */
  CIM_KEY_KP_0             = 0xffb0, /* KP_0 */
  CIM_KEY_KP_1             = 0xffb1, /* KP_1 */
  CIM_KEY_KP_2             = 0xffb2, /* KP_2 */
  CIM_KEY_KP_3             = 0xffb3, /* KP_3 */
  CIM_KEY_KP_4             = 0xffb4, /* KP_4 */
  CIM_KEY_KP_5             = 0xffb5, /* KP_5 */
  CIM_KEY_KP_6             = 0xffb6, /* KP_6 */
  CIM_KEY_KP_7             = 0xffb7, /* KP_7 */
  CIM_KEY_KP_8             = 0xffb8, /* KP_8 */
  CIM_KEY_KP_9             = 0xffb9, /* KP_9 */

  CIM_KEY_F1               = 0xffbe, /* F1 */
  CIM_KEY_F2               = 0xffbf, /* F2 */
  CIM_KEY_F3               = 0xffc0, /* F3 */
  CIM_KEY_F4               = 0xffc1, /* F4 */
  CIM_KEY_F5               = 0xffc2, /* F5 */
  CIM_KEY_F6               = 0xffc3, /* F6 */
  CIM_KEY_F7               = 0xffc4, /* F7 */
  CIM_KEY_F8               = 0xffc5, /* F8 */
  CIM_KEY_F9               = 0xffc6, /* F9 */
  CIM_KEY_F10              = 0xffc7, /* F10 */
  CIM_KEY_F11              = 0xffc8, /* F11 */
  CIM_KEY_F12              = 0xffc9, /* F12 */

  CIM_KEY_Shift_L          = 0xffe1, /* Shift_L */
  CIM_KEY_Shift_R          = 0xffe2, /* Shift_R */
  CIM_KEY_Control_L        = 0xffe3, /* Control_L */
  CIM_KEY_Control_R        = 0xffe4, /* Control_R */
  CIM_KEY_Caps_Lock        = 0xffe5, /* Caps_Lock */
  CIM_KEY_Shift_Lock       = 0xffe6, /* Shift_Lock */
  CIM_KEY_Meta_L           = 0xffe7, /* Meta_L */
  CIM_KEY_Meta_R           = 0xffe8, /* Meta_R */
  CIM_KEY_Alt_L            = 0xffe9, /* Alt_L */
  CIM_KEY_Alt_R            = 0xffea, /* Alt_R */
  CIM_KEY_Super_L          = 0xffeb, /* Super_L */
  CIM_KEY_Super_R          = 0xffec, /* Super_R */
  CIM_KEY_Hyper_L          = 0xffed, /* Hyper_L */
  CIM_KEY_Hyper_R          = 0xffee, /* Hyper_R */

  CIM_KEY_Delete           = 0xffff, /* Delete */

  CIM_KEY_VoidSymbol       = 0xffffff,   /* VoidSymbol */

  CIM_KEY_Switch_VT_1      = 0x1008fe01, /* Switch_VT_1 */
  CIM_KEY_Switch_VT_2      = 0x1008fe02, /* Switch_VT_2 */
  CIM_KEY_Switch_VT_3      = 0x1008fe03, /* Switch_VT_3 */
  CIM_KEY_Switch_VT_4      = 0x1008fe04, /* Switch_VT_4 */
  CIM_KEY_Switch_VT_5      = 0x1008fe05, /* Switch_VT_5 */
  CIM_KEY_Switch_VT_6      = 0x1008fe06, /* Switch_VT_6 */
  CIM_KEY_Switch_VT_7      = 0x1008fe07, /* Switch_VT_7 */
  CIM_KEY_Switch_VT_8      = 0x1008fe08, /* Switch_VT_8 */
  CIM_KEY_Switch_VT_9      = 0x1008fe09, /* Switch_VT_9 */
  CIM_KEY_Switch_VT_10     = 0x1008fe0a, /* Switch_VT_10 */
  CIM_KEY_Switch_VT_11     = 0x1008fe0b, /* Switch_VT_11 */
  CIM_KEY_Switch_VT_12     = 0x1008fe0c, /* Switch_VT_12 */

  CIM_KEY_HomePage         = 0x1008ff18, /* HomePage */

  CIM_KEY_WakeUp           = 0x1008ff2b, /* WakeUp */

  CIM_KEY_WebCam           = 0x1008ff8f, /* WebCam */

  CIM_KEY_WLAN             = 0x1008ff95  /* WLAN */
} CimKeySym;

typedef enum
{
  PREEDIT_STATE_START = 1,
  PREEDIT_STATE_END   = 0
} PreeditState;

typedef struct _Key Key;
struct _Key {
  uint32_t state;
  uint32_t keyval;
};

typedef struct _Hangul Hangul;
struct _Hangul
{
  CimIc ic;

  union {
    CimCallbacks cb;
    void* cb_funcs[CIM_CB_N_TYPES];
  };

  void* cb_user_data[CIM_CB_N_TYPES];

  HangulInputContext* hic;
  PreeditState preedit_state;
  CimPreedit   preedit;
  CimTextAttr  attr;
  /* candidate */
  CimCandidate candidate;
  CimSelection selection;
  HanjaList* hanja_list;
  bool visible;

  Key  hanja_key1;
  Key  hanja_key2;
  Key* hanja_keys[3];
};

static HanjaTable* hangul_hanja_table;
static HanjaTable* hangul_symbol_table;
static atomic_int  hangul_hanja_table_ref_count;

static void hangul_call_preedit_start (Hangul* hangul)
{
  if (hangul->cb.preedit_start)
    hangul->cb.preedit_start (&hangul->ic,
                              hangul->cb_user_data[CIM_CB_PREEDIT_START]);
}

static void hangul_call_preedit_end (Hangul* hangul)
{
  if (hangul->cb.preedit_end)
    hangul->cb.preedit_end (&hangul->ic,
                            hangul->cb_user_data[CIM_CB_PREEDIT_END]);
}

static void hangul_call_preedit_changed (Hangul* hangul)
{
  if (hangul->cb.preedit_changed)
    hangul->cb.preedit_changed (&hangul->ic, &hangul->preedit,
                                hangul->cb_user_data[CIM_CB_PREEDIT_CHANGED]);
}

static void hangul_call_commit (Hangul* hangul, const char* text)
{
  if (hangul->cb.commit)
    hangul->cb.commit ((CimIc*) hangul, text,
                       hangul->cb_user_data[CIM_CB_COMMIT]);
}

static const CimSurround* hangul_call_get_surround (Hangul* hangul)
{
  if (hangul->cb.get_surround)
    return hangul->cb.get_surround ((CimIc*) hangul,
                                    hangul->cb_user_data[CIM_CB_GET_SURROUND]);
  return NULL;
}

static bool hangul_call_delete_surround (Hangul* hangul, int offset, int n_chars)
{
  if (hangul->cb.delete_surround)
    return hangul->cb.delete_surround ((CimIc*) hangul, offset, n_chars,
                                       hangul->cb_user_data[CIM_CB_DELETE_SURROUND]);
  return NULL;
}

static void hangul_call_candidate_show (Hangul* hangul)
{
  if (hangul->cb.candidate_show)
    hangul->cb.candidate_show ((CimIc*) hangul, N_ROWS, N_COLS, false,
                               hangul->cb_user_data[CIM_CB_CANDIDATE_SHOW]);
  hangul->visible = true;
}

static void hangul_call_candidate_hide (Hangul* hangul)
{
  hanja_list_delete (hangul->hanja_list);
  hangul->hanja_list = NULL;
  hangul->candidate.page_index = 0;
  hangul->candidate.n_pages = 0;
  hangul->candidate.n_rows  = 0;

  if (hangul->cb.candidate_hide)
    hangul->cb.candidate_hide ((CimIc*) hangul,
                               hangul->cb_user_data[CIM_CB_CANDIDATE_HIDE]);
  hangul->visible = false;
}

static void hangul_call_candidate_changed (Hangul* hangul)
{
  if (hangul->cb.candidate_changed)
    hangul->cb.candidate_changed ((CimIc*) hangul, &hangul->candidate,
                                   hangul->cb_user_data[CIM_CB_CANDIDATE_CHANGED]);
}

static void hangul_call_candidate_selected (Hangul* hangul)
{
  if (hangul->cb.candidate_selected)
  {
    hangul->selection.end_row = hangul->selection.start_row;
    hangul->selection.end_col = hangul->candidate.n_cols - 1;

    hangul->cb.candidate_selected ((CimIc*) hangul, &hangul->selection,
                                   hangul->cb_user_data[CIM_CB_CANDIDATE_SELECTED]);
  }
}

static void hangul_update_preedit (Hangul* hangul, char* new_preedit)
{
  /* preedit-start */
  if (hangul->preedit_state == PREEDIT_STATE_END && new_preedit[0] != 0)
  {
    hangul->preedit_state = PREEDIT_STATE_START;
    hangul_call_preedit_start (hangul);
  }
  /* preedit-changed */
  if (hangul->preedit.text[0] != 0 || new_preedit[0] != 0)
  {
    free (hangul->preedit.text);
    hangul->preedit.text = new_preedit;
    hangul->preedit.cursor_pos = c_utf8_strlen (hangul->preedit.text);
    hangul->preedit.attrs[0].n_chars = hangul->preedit.cursor_pos;
    hangul_call_preedit_changed (hangul);
  }
  else
    free (new_preedit);
  /* preedit-end */
  if (hangul->preedit_state == PREEDIT_STATE_START &&
      hangul->preedit.text[0] == 0)
  {
    hangul->preedit_state = PREEDIT_STATE_END;
    hangul_call_preedit_end (hangul);
  }
}

static void hangul_reset (CimIc* ic)
{
  Hangul* hangul = (Hangul*) ic;

  if (hangul->visible)
    hangul_call_candidate_hide (hangul);

  const ucschar* flush;
  flush = hangul_ic_flush (hangul->hic);

  if (flush[0] != 0)
  {
    char* text = c_char32_to_utf8 (flush, -1);
    hangul_call_commit (hangul, text);
    free (text);
  }

  hangul_update_preedit (hangul, c_strdup (""));
}

static void hangul_focus_in (CimIc* ic)
{
}

static void hangul_focus_out (CimIc* ic)
{
  hangul_reset (ic);
}

static const CimPreedit* hangul_get_preedit (CimIc* ic)
{
  Hangul* hangul = (Hangul*) ic;
  return &hangul->preedit;
}

static const CimCandidate* hangul_get_candidate (CimIc* ic)
{
  Hangul* hangul = (Hangul*) ic;
  return &hangul->candidate;
}

static void hangul_candidate_commit (Hangul* hangul, const char* text)
{
  if (text)
  {
    /* Make the commit text inside hangul_ic disappear. */
    hangul_ic_reset (hangul->hic);

    if (hangul->preedit.text[0] == 0)
      hangul_call_delete_surround (hangul, -1, 1);

    hangul_call_commit (hangul, text);

    if (hangul->preedit.text[0] != 0)
      hangul_update_preedit (hangul, c_strdup (""));
  }

  hangul_call_candidate_hide (hangul);
}

static void hangul_candidate_free (Hangul* hangul)
{
  for (int i = 0; i < N_ROWS; i++)
  {
    for (int j = 0; j < N_COLS; j++)
      free (hangul->candidate.table[i][j].data);

    free (hangul->candidate.table[i]);
  }

  free (hangul->candidate.table);
}

static void hangul_candidate_insert (Hangul* hangul,
                                    int i,
                                    const char* item1,
                                    const char* item2)
{
  CimItem* cols = hangul->candidate.table[i];

  free (cols[0].data);
  free (cols[1].data);
  free (cols[2].data);

  cols[0].type = CIM_ITEM_STRING;
  cols[1].type = CIM_ITEM_STRING;
  cols[2].type = CIM_ITEM_STRING;
  cols[0].data = c_str_sprintf ("%d", (i + 1) % 10);
  cols[1].data = c_strdup (item1);

  if (item2 && item2[0])
  {
    cols[2].data = c_strdup (item2);
  }
  else
  {
    cols[2].data = c_strdup ("");
  }
}

static void hangul_candidate_update (Hangul* hangul)
{
  if (hangul->hanja_list == NULL)
    return;

  hangul->candidate.n_rows = 0;

  int list_len = hanja_list_get_size (hangul->hanja_list);

  for (int i = hangul->candidate.page_index * 10;
       i < C_MIN ((hangul->candidate.page_index + 1) * 10, list_len); i++)
  {
    const Hanja* hanja = hanja_list_get_nth (hangul->hanja_list, i);
    const char* item1  = hanja_get_value    (hanja);
    const char* item2  = hanja_get_comment  (hanja);
    hangul_candidate_insert (hangul, hangul->candidate.n_rows, item1, item2);
    hangul->candidate.n_rows++;
  }
}

static bool hangul_page_up (Hangul* hangul)
{
  if (hangul->hanja_list == NULL)
    return false;

  if (hangul->candidate.page_index <= 0)
  {
    hangul->selection.start_row = 0;
    hangul_call_candidate_selected (hangul);
    return false;
  }

  hangul->candidate.page_index--;
  hangul_candidate_update (hangul);
  hangul->selection.start_row = hangul->candidate.n_rows - 1;
  hangul_call_candidate_changed  (hangul);
  hangul_call_candidate_selected (hangul);

  return true;
}

static bool hangul_page_down (Hangul* hangul)
{
  if (hangul->hanja_list == NULL)
    return false;

  if ((hangul->candidate.page_index + 1) == hangul->candidate.n_pages)
  {
    hangul->selection.start_row = hangul->candidate.n_rows - 1;
    hangul_call_candidate_selected (hangul);
    return false;
  }

  hangul->candidate.page_index++;
  hangul_candidate_update (hangul);
  hangul->selection.start_row = 0;
  hangul_call_candidate_changed  (hangul);
  hangul_call_candidate_selected (hangul);

  return true;
}

static void hangul_page_home (Hangul* hangul)
{
  if (hangul->hanja_list == NULL)
    return;

  if (hangul->candidate.page_index <= 0)
  {
    hangul->selection.start_row = 0;
    hangul_call_candidate_selected (hangul);
    return;
  }

  hangul->candidate.page_index = 0;
  hangul_candidate_update (hangul);
  hangul->selection.start_row = 0;
  hangul_call_candidate_changed (hangul);
  hangul_call_candidate_selected (hangul);
}

static void hangul_page_end (Hangul* hangul)
{
  if (hangul->hanja_list == NULL)
    return;

  if ((hangul->candidate.page_index + 1) == hangul->candidate.n_pages)
  {
    hangul->selection.start_row = hangul->candidate.n_rows - 1;
    hangul_call_candidate_selected (hangul);
    return;
  }

  hangul->candidate.page_index = hangul->candidate.n_pages - 1;
  hangul_candidate_update (hangul);
  hangul->selection.start_row = hangul->candidate.n_rows - 1;
  hangul_call_candidate_changed (hangul);
  hangul_call_candidate_selected (hangul);
}

static unsigned cim_event_keycode_to_qwerty_keyval (const CimEvent* event)
{
  unsigned keyval = 0;
  bool is_shift = event->state & CIM_SHIFT_MASK;

  switch (event->keycode)
  {
    /* 20(-) ~ 21(=) */
    case 20: keyval = is_shift ? '_' : '-';  break;
    case 21: keyval = is_shift ? '+' : '=';  break;
    /* 24(q) ~ 35(]) */
    case 24: keyval = is_shift ? 'Q' : 'q';  break;
    case 25: keyval = is_shift ? 'W' : 'w';  break;
    case 26: keyval = is_shift ? 'E' : 'e';  break;
    case 27: keyval = is_shift ? 'R' : 'r';  break;
    case 28: keyval = is_shift ? 'T' : 't';  break;
    case 29: keyval = is_shift ? 'Y' : 'y';  break;
    case 30: keyval = is_shift ? 'U' : 'u';  break;
    case 31: keyval = is_shift ? 'I' : 'i';  break;
    case 32: keyval = is_shift ? 'O' : 'o';  break;
    case 33: keyval = is_shift ? 'P' : 'p';  break;
    case 34: keyval = is_shift ? '{' : '[';  break;
    case 35: keyval = is_shift ? '}' : ']';  break;
    /* 38(a) ~ 48(') */
    case 38: keyval = is_shift ? 'A' : 'a';  break;
    case 39: keyval = is_shift ? 'S' : 's';  break;
    case 40: keyval = is_shift ? 'D' : 'd';  break;
    case 41: keyval = is_shift ? 'F' : 'f';  break;
    case 42: keyval = is_shift ? 'G' : 'g';  break;
    case 43: keyval = is_shift ? 'H' : 'h';  break;
    case 44: keyval = is_shift ? 'J' : 'j';  break;
    case 45: keyval = is_shift ? 'K' : 'k';  break;
    case 46: keyval = is_shift ? 'L' : 'l';  break;
    case 47: keyval = is_shift ? ':' : ';';  break;
    case 48: keyval = is_shift ? '"' : '\''; break;
    /* 52(z) ~ 61(?) */
    case 52: keyval = is_shift ? 'Z' : 'z';  break;
    case 53: keyval = is_shift ? 'X' : 'x';  break;
    case 54: keyval = is_shift ? 'C' : 'c';  break;
    case 55: keyval = is_shift ? 'V' : 'v';  break;
    case 56: keyval = is_shift ? 'B' : 'b';  break;
    case 57: keyval = is_shift ? 'N' : 'n';  break;
    case 58: keyval = is_shift ? 'M' : 'm';  break;
    case 59: keyval = is_shift ? '<' : ',';  break;
    case 60: keyval = is_shift ? '>' : '.';  break;
    case 61: keyval = is_shift ? '?' : '/';  break;
    default: keyval = event->keyval; break;
  }

  return keyval;
}

static bool hangul_event_matches (const CimEvent* event, Key** keys)
{
  /* Ignore CIM_MOD2_MASK (Number),
   *        CIM_LOCK_MASK (CapsLock),
   *        virtual modifiers
   */
  unsigned mods_mask = CIM_SHIFT_MASK   |
                       CIM_CONTROL_MASK |
                       CIM_MOD1_MASK    |
                       CIM_MOD3_MASK    |
                       CIM_MOD4_MASK    |
                       CIM_MOD5_MASK;

  for (int i = 0; keys[i] != 0; i++)
    if ((event->state & mods_mask) == (keys[i]->state & mods_mask) &&
        event->keyval == keys[i]->keyval)
      return true;

  return false;
}

static void hangul_select_prev_item (Hangul* hangul)
{
  if (hangul->selection.start_row > 0)
  {
    hangul->selection.start_row--;
    hangul_call_candidate_selected (hangul);
  }
  else
  {
    if (hangul_page_up (hangul))
    {
      hangul->selection.start_row = hangul->candidate.n_rows - 1;
      hangul_call_candidate_selected (hangul);
    }
  }
}

static void hangul_select_next_item (Hangul* hangul)
{
  if (hangul->selection.start_row < hangul->candidate.n_rows - 1)
  {
    hangul->selection.start_row++;
    hangul_call_candidate_selected (hangul);
  }
  else
  {
    if (hangul_page_down (hangul))
    {
      hangul->selection.start_row = 0;
      hangul_call_candidate_selected (hangul);
    }
  }
}

static void hangul_activate_candidate_item (CimIc* ic, int row, int col)
{
  Hangul* hangul = (Hangul*) ic;

  if (row < hangul->candidate.n_rows)
  {
    const char* text = hangul->candidate.table[row][1].data;

    if (text && text[1])
      hangul_candidate_commit (hangul, text);
  }
}

static bool hangul_filter_event (CimIc* ic, const CimEvent* event)
{
  uint keyval;
  bool retval = false;

  Hangul* hangul = (Hangul*) ic;

  if (event->type   == CIM_EVENT_KEY_RELEASE ||
      event->keyval == CIM_KEY_Shift_L ||
      event->keyval == CIM_KEY_Shift_R)
    return false;

  if (event->state & (CIM_CONTROL_MASK | CIM_MOD1_MASK))
  {
    hangul_reset (ic);
    return false;
  }

  if (hangul_event_matches (event, hangul->hanja_keys))
  {
    if (!hangul->visible)
    {
      char item[5];
      const char* key = hangul->preedit.text;

      if (hangul->preedit.text[0] == 0)
      {
        const CimSurround* surround;

        if ((surround = hangul_call_get_surround (hangul)))
        {
          if  (surround->len && surround->cursor_pos > 0)
          {
            char* p = c_utf8_offset_to_pointer (surround->text,
                                                surround->cursor_pos - 1);
            c_utf8_strncpy (item, p, 1);
            key = item;
          }
        }
      }

      hanja_list_delete (hangul->hanja_list);
      hangul->hanja_list = hanja_table_match_exact (hangul_hanja_table, key);

      if (hangul->hanja_list == NULL)
        hangul->hanja_list = hanja_table_match_exact (hangul_symbol_table, key);

      hangul->candidate.n_pages = (hanja_list_get_size (hangul->hanja_list) + 9) / 10;
      hangul->candidate.page_index = 0;
      hangul->selection.start_row = 0;
      hangul_call_candidate_show (hangul);
      hangul_candidate_update (hangul);
      hangul_call_candidate_changed (hangul);
      hangul_call_candidate_selected (hangul);
    }
    else
    {
      hangul_call_candidate_hide (hangul);
    }

    return true;
  }

  if (hangul->visible)
  {
    switch (event->keyval)
    {
      case CIM_KEY_Return:
      case CIM_KEY_KP_Enter:
        hangul_activate_candidate_item (ic, hangul->selection.start_row, 0);
        hangul_call_candidate_hide (hangul);
        break;
      case CIM_KEY_Up:
      case CIM_KEY_KP_Up:
        hangul_select_prev_item (hangul);
        break;
      case CIM_KEY_Down:
      case CIM_KEY_KP_Down:
        hangul_select_next_item (hangul);
        break;
      case CIM_KEY_Page_Up:
      case CIM_KEY_KP_Page_Up:
        hangul_page_up (hangul);
        break;
      case CIM_KEY_Page_Down:
      case CIM_KEY_KP_Page_Down:
        hangul_page_down (hangul);
        break;
      case CIM_KEY_Home:
        hangul_page_home (hangul);
        break;
      case CIM_KEY_End:
        hangul_page_end (hangul);
        break;
      case CIM_KEY_Escape:
        hangul_call_candidate_hide (hangul);
        break;
      case CIM_KEY_0:
      case CIM_KEY_1:
      case CIM_KEY_2:
      case CIM_KEY_3:
      case CIM_KEY_4:
      case CIM_KEY_5:
      case CIM_KEY_6:
      case CIM_KEY_7:
      case CIM_KEY_8:
      case CIM_KEY_9:
      case CIM_KEY_KP_0:
      case CIM_KEY_KP_1:
      case CIM_KEY_KP_2:
      case CIM_KEY_KP_3:
      case CIM_KEY_KP_4:
      case CIM_KEY_KP_5:
      case CIM_KEY_KP_6:
      case CIM_KEY_KP_7:
      case CIM_KEY_KP_8:
      case CIM_KEY_KP_9:
        {
          if (!hangul->hanja_list || hangul->candidate.page_index < 0)
            break;

          int i, n;
          int list_len = hanja_list_get_size (hangul->hanja_list);

          if (event->keyval >= CIM_KEY_0 && event->keyval <= CIM_KEY_9)
            n = (event->keyval - CIM_KEY_0 + 9) % 10;
          else if (event->keyval >= CIM_KEY_KP_0 &&
                   event->keyval <= CIM_KEY_KP_9)
            n = (event->keyval - CIM_KEY_KP_0 + 9) % 10;
          else
            break;

          i = hangul->candidate.page_index * 10 + n;

          if (i < C_MIN ((hangul->candidate.page_index + 1) * 10, list_len))
          {
            const Hanja* hanja = hanja_list_get_nth (hangul->hanja_list, i);
            const char * text = hanja_get_value (hanja);
            hangul_candidate_commit (hangul, text);
          }
        }
        break;
      default:
        break;
    }

    return true;
  }

  const ucschar* ucs_commit;
  const ucschar* ucs_preedit;

  if (event->keyval == CIM_KEY_BackSpace)
  {
    retval = hangul_ic_backspace (hangul->hic);

    if (retval)
    {
      ucs_preedit = hangul_ic_get_preedit_string (hangul->hic);
      char* new_preedit = c_char32_to_utf8 (ucs_preedit, -1);
      hangul_update_preedit (hangul, new_preedit);
    }

    return retval;
  }

  keyval = cim_event_keycode_to_qwerty_keyval (event);
  retval = hangul_ic_process (hangul->hic, keyval);

  ucs_commit  = hangul_ic_get_commit_string  (hangul->hic);
  ucs_preedit = hangul_ic_get_preedit_string (hangul->hic);

  if (ucs_commit[0] != 0)
  {
    char* new_commit = c_char32_to_utf8 (ucs_commit, -1);
    hangul_call_commit (hangul, new_commit);
    free (new_commit);
  }

  char* new_preedit = c_char32_to_utf8 (ucs_preedit, -1);
  hangul_update_preedit (hangul, new_preedit);

  if (!retval)
  {
    switch (keyval)
    {
      case '_':
      case '-':
      case '+':
      case '=':
      case '{':
      case '[':
      case '}':
      case ']':
      case ':':
      case ';':
      case '\"':
      case '\'':
      case '<':
      case ',':
      case '>':
      case '.':
      case '?':
      case '/':
        {
          char text[2] = { (char) keyval, '\0' };
          hangul_call_commit (hangul, text);
          retval = true;
        }
      default:
        break;
    }
  }

  return retval;
}

static void hangul_set_vcallbacks (CimIc* ic, va_list ap)
{
  Hangul* hangul = (Hangul*) ic;
  CimCbType type;

  while ((type = va_arg (ap, CimCbType)) != -1)
  {
    void*  callback  = va_arg (ap, void*);
    void*  user_data = va_arg (ap, void*);
    void** cb = (void**) &hangul->cb_funcs;
    cb[type] = callback;
    hangul->cb_user_data[type] = user_data;
  }
}

static void hangul_change_candidate_page (CimIc* ic, int page_index)
{
  Hangul* hangul = (Hangul*) ic;

  if (page_index < 0 ||
      page_index == hangul->candidate.page_index ||
      page_index >= hangul->candidate.n_pages)
    return;

  hangul->candidate.page_index = page_index;
  hangul_candidate_update (hangul);
  hangul->selection.start_row = 0;
  hangul_call_candidate_changed  (hangul);
  hangul_call_candidate_selected (hangul);
}

static CimIc* hangul_new ()
{
  hangul_hanja_table_ref_count++;

  Hangul* hangul = c_calloc (1, sizeof (Hangul));
  CimIc* ic = (CimIc*) hangul;

  hangul->hic = hangul_ic_new ("2");
  hangul->preedit.text  = c_strdup ("");
  hangul->attr.type     = CIM_TEXT_ATTR_UNDERLINE;
  hangul->preedit.attrs = &hangul->attr;
  hangul->preedit.attrs_len = 1;

  hangul->candidate.table = c_calloc (N_ROWS, sizeof (CimItem*));
  for (int i = 0; i < N_ROWS; i++)
    hangul->candidate.table[i] = c_calloc (N_COLS, sizeof (CimItem));

  hangul->candidate.page_index = 0;
  hangul->candidate.n_cols = N_COLS;

  hangul->hanja_key1.keyval = CIM_KEY_Hangul_Hanja;
  hangul->hanja_key2.keyval = CIM_KEY_Control_R;
  hangul->hanja_keys[0] = &hangul->hanja_key1;
  hangul->hanja_keys[1] = &hangul->hanja_key2;
  hangul->hanja_keys[2] = NULL;

  if (hangul_hanja_table_ref_count == 1)
    hangul_hanja_table = hanja_table_load (NULL);

  ic->filter_event            = hangul_filter_event;
  ic->reset                   = hangul_reset;
  ic->focus_in                = hangul_focus_in;
  ic->focus_out               = hangul_focus_out;
  ic->get_preedit             = hangul_get_preedit;
  ic->get_candidate           = hangul_get_candidate;
  ic->set_vcallbacks          = hangul_set_vcallbacks;
  ic->activate_candidate_item = hangul_activate_candidate_item;
  ic->change_candidate_page   = hangul_change_candidate_page;

  return ic;
}

static void hangul_free (Hangul* hangul)
{
  if (--hangul_hanja_table_ref_count == 0)
  {
    hanja_table_delete (hangul_hanja_table);
    hanja_table_delete (hangul_symbol_table);
  }

  hanja_list_delete (hangul->hanja_list);
  hangul_ic_delete (hangul->hic);
  free (hangul->preedit.text);
  hangul_candidate_free (hangul);
  free (hangul);
}

CimIc* cim_plugin_new_ic ()
{
  return (CimIc*) hangul_new ();
}

void cim_plugin_free_ic (CimIc* ic)
{
  hangul_free ((Hangul*) ic);
}

void cim_plugin_get_version (int* major, int* minor, int* micro)
{
  if (major)
    *major = CIM_MAJOR_VERSION;

  if (minor)
    *minor = CIM_MINOR_VERSION;

  if (micro)
    *micro = CIM_MICRO_VERSION;
}
