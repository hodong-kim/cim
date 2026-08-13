/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-candidate.c
 * This file is part of Nimf.
 *
 * Copyright (C) 2015-2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 */
#include <gtk/gtk.h>
#include <limits.h>
#include <stddef.h>
#include "c-candidate.h"
#include "c-candidate-data.h"

#define C_CANDIDATE_CELL_HEIGHT 27

G_DEFINE_TYPE (TableItem, table_item, GTK_TYPE_BIN)

static void table_item_init (TableItem* item)
{
  GtkStyleContext* style_context;
  style_context = gtk_widget_get_style_context (GTK_WIDGET (item));
  gtk_style_context_add_class (style_context, "view");
}

GtkWidget* table_item_new ()
{
  return g_object_new (table_item_get_type (), NULL);
}

static void table_item_realize (GtkWidget* widget)
{
  TableItem* item = (TableItem*) widget;
  GdkWindow* window;
  GdkWindowAttr attributes;
  gint attributes_mask;

  gtk_widget_set_realized (widget, TRUE);

  window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, window);
  g_object_ref (window);

  GtkAllocation allocation;
  gtk_widget_get_allocation (widget, &allocation);

  attributes.x           = allocation.x;
  attributes.y           = allocation.y;
  attributes.width       = allocation.width;
  attributes.height      = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass      = GDK_INPUT_ONLY;
  attributes.event_mask  = gtk_widget_get_events (widget) |
                           GDK_BUTTON_PRESS_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  item->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                       &attributes, attributes_mask);
  gtk_widget_register_window (widget, item->event_window);
}

static void table_item_unrealize (GtkWidget* widget)
{
  TableItem* item = (TableItem*) widget;
  gtk_widget_unregister_window (widget, item->event_window);
  gdk_window_destroy (item->event_window);
  item->event_window = NULL;

  GTK_WIDGET_CLASS (table_item_parent_class)->unrealize (widget);
}

static void table_item_dispose (GObject* object)
{
  GtkStyleContext* style_context;
  style_context = gtk_widget_get_style_context (GTK_WIDGET (object));
  gtk_style_context_remove_class (style_context, "view");

  G_OBJECT_CLASS (table_item_parent_class)->dispose (object);
}

static gboolean table_item_draw (GtkWidget* widget, cairo_t* cr)
{
  GtkStateFlags state = gtk_widget_get_state_flags (widget);

  int width  = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  if (state & GTK_STATE_FLAG_SELECTED)
  {
    GtkStyleContext* context = gtk_widget_get_style_context (widget);
    const GdkRGBA* color;
    GValue gvalue = G_VALUE_INIT;
    gtk_style_context_get_property (context,
                                    "background-color",
                                    GTK_STATE_FLAG_SELECTED,
                                    &gvalue);
    color = g_value_get_boxed (&gvalue);

    cairo_save (cr);
    cairo_set_source_rgba (cr, color->red, color->green, color->blue, color->alpha);
    cairo_rectangle (cr, 0, 0, width, height);
    cairo_fill (cr);
    cairo_restore (cr);

    g_value_unset (&gvalue);
  }

  GTK_WIDGET_CLASS (table_item_parent_class)->draw (widget, cr);

  return FALSE;
}

void table_item_select (TableItem* item)
{
  GtkStateFlags state = gtk_widget_get_state_flags (GTK_WIDGET (item));

  if (state & GTK_STATE_FLAG_SELECTED)
    return;

  GtkWidget* child = gtk_bin_get_child ((GtkBin*) item);
  g_return_if_fail (child != NULL);

  gtk_widget_set_state_flags (GTK_WIDGET (item), GTK_STATE_FLAG_SELECTED, FALSE);
  gtk_widget_set_state_flags (GTK_WIDGET (child), GTK_STATE_FLAG_SELECTED, FALSE);

  GtkStyleContext* style_context = gtk_widget_get_style_context (GTK_WIDGET (item));

  state = state | GTK_STATE_FLAG_SELECTED;
  gtk_style_context_set_state (style_context, state);

  gtk_widget_queue_draw (GTK_WIDGET (item));
}

void table_item_deselect (TableItem* item)
{
  GtkStateFlags state = gtk_widget_get_state_flags (GTK_WIDGET (item));

  if (!(state & GTK_STATE_FLAG_SELECTED))
    return;

  GtkWidget* child = gtk_bin_get_child ((GtkBin*) item);
  g_return_if_fail (child != NULL);

  gtk_widget_unset_state_flags (GTK_WIDGET (item), GTK_STATE_FLAG_SELECTED);
  gtk_widget_unset_state_flags (GTK_WIDGET (child), GTK_STATE_FLAG_SELECTED);

  gtk_widget_queue_draw (GTK_WIDGET (item));
}

static void table_item_map (GtkWidget *widget)
{
  TableItem* item = (TableItem*) widget;

  GTK_WIDGET_CLASS (table_item_parent_class)->map (widget);

  gdk_window_show (item->event_window);
}

static void table_item_unmap (GtkWidget *widget)
{
  TableItem* item = (TableItem*) widget;

  gdk_window_hide (item->event_window);

  GTK_WIDGET_CLASS (table_item_parent_class)->unmap (widget);
}

static void table_item_size_allocate (GtkWidget     *widget,
                                      GtkAllocation *allocation)
{
  TableItem* item = (TableItem*) widget;

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (item->event_window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  GTK_WIDGET_CLASS (table_item_parent_class)->size_allocate (widget, allocation);
}

static void table_item_class_init (TableItemClass* class)
{
  GObjectClass *gobject_class  = G_OBJECT_CLASS (class);
  GtkWidgetClass* widget_class = (GtkWidgetClass*) class;

  gobject_class->dispose      = table_item_dispose;

  widget_class->draw          = table_item_draw;
  widget_class->realize       = table_item_realize;
  widget_class->unrealize     = table_item_unrealize;
  widget_class->map           = table_item_map;
  widget_class->unmap         = table_item_unmap;
  widget_class->size_allocate = table_item_size_allocate;
}

/*****************************************************************************/

#include "c-candidate.h"

static gboolean cb_range_change_value (GtkRange*     range,
                                       GtkScrollType scroll,
                                       gdouble       value,
                                       CCandidate*   ccandidate)
{
  GtkAdjustment* adjustment;
  gdouble lower;
  gdouble upper;
  uint32_t page_index;

  (void) scroll;

  adjustment = gtk_range_get_adjustment (range);
  lower = gtk_adjustment_get_lower (adjustment);
  upper = gtk_adjustment_get_upper (adjustment);

  g_return_val_if_fail (ccandidate != NULL, FALSE);
  g_return_val_if_fail (lower >= 0.0, FALSE);
  g_return_val_if_fail (upper >= lower, FALSE);
  g_return_val_if_fail (upper >= 1.0, FALSE);

  if (value < lower)
    value = lower;

  if (value > upper - 1.0)
    value = upper - 1.0;

  g_return_val_if_fail (value >= 0.0, FALSE);
  g_return_val_if_fail (value <= (gdouble) UINT32_MAX, FALSE);

  page_index = (uint32_t) value;

  if (page_index < ccandidate->n_pages &&
      page_index != ccandidate->page_index)
  {
    if (ccandidate->cb.change_page)
      ccandidate->cb.change_page
        (ccandidate,
         page_index,
         ccandidate->cb_user_data[C_CANDIDATE_CB_CHANGE_PAGE]);
  }

  return FALSE;
}

static void cb_remove (GtkWidget* widget, GtkContainer* container)
{
  gtk_container_remove (container, widget);
}

static void c_candidate_clear (CCandidate* ccandidate)
{
  gtk_container_foreach ((GtkContainer*) ccandidate->table,
                         (GtkCallback) cb_remove, ccandidate->table);
}

void c_candidate_move (CCandidate* ccandidate, int x, int y)
{
  gtk_window_move ((GtkWindow*) ccandidate->window, x, y);
}

void c_candidate_show (CCandidate* ccandidate,
                       uint32_t    n_rows,
                       uint32_t    n_cols,
                       bool        show_aux)
{
  int new_height;
  int new_width;

  if (!ccandidate)
    g_error ("Cim GTK candidate window is NULL");

  if (!c_candidate_measure_window
        (n_rows, n_cols, show_aux, C_CANDIDATE_CELL_HEIGHT,
         &new_width, &new_height))
    g_error ("Cim GTK candidate window size exceeds toolkit limits");

  if (ccandidate->n_rows != n_rows ||
      ccandidate->n_cols != n_cols)
  {
    c_candidate_clear (ccandidate);

    if (ccandidate->n_rows > n_rows)
    {
      for (uint32_t row = ccandidate->n_rows; row > n_rows; row--)
      {
        gtk_grid_remove_row ((GtkGrid*) ccandidate->table,
                             (int) (row - 1));
      }
    }
    else if (ccandidate->n_rows < n_rows)
    {
      for (uint32_t row = ccandidate->n_rows; row < n_rows; row++)
      {
        gtk_grid_insert_row ((GtkGrid*) ccandidate->table,
                             (int) row);
      }
    }

    if (ccandidate->n_cols > n_cols)
    {
      for (uint32_t col = ccandidate->n_cols; col > n_cols; col--)
      {
        gtk_grid_remove_column ((GtkGrid*) ccandidate->table,
                                (int) (col - 1));
      }
    }
    else if (ccandidate->n_cols < n_cols)
    {
      for (uint32_t col = ccandidate->n_cols; col < n_cols; col++)
      {
        gtk_grid_insert_column ((GtkGrid*) ccandidate->table,
                                (int) col);
      }
    }

    ccandidate->n_rows = n_rows;
    ccandidate->n_cols = n_cols;
  }

  if (show_aux)
    gtk_widget_show (ccandidate->entry);
  else
    gtk_widget_hide (ccandidate->entry);

  gtk_widget_show_all (ccandidate->window);

  /* FIXME: Fix how to get widget size. */
  ccandidate->cell_height = C_CANDIDATE_CELL_HEIGHT;

  gtk_widget_set_size_request (ccandidate->window, new_width, new_height);
  gtk_window_resize ((GtkWindow*) ccandidate->window, new_width, new_height);
}

void c_candidate_hide (CCandidate* ccandidate)
{
  gtk_widget_hide (ccandidate->window);
}

static void c_candidate_set_vcallbacks (CCandidate* ccandidate, va_list ap)
{
  CCandidateCbType type;

  while ((type = va_arg (ap, CCandidateCbType)) != -1)
  {
    void*  callback  = va_arg (ap, void*);
    void*  user_data = va_arg (ap, void*);
    void** cb = (void**) &ccandidate->cb;
    cb[type] = callback;
    ccandidate->cb_user_data[type] = user_data;
  }
}

void c_candidate_set_callbacks (CCandidate* ccandidate, ...)
{
  va_list ap;
  va_start (ap, ccandidate);
  c_candidate_set_vcallbacks (ccandidate, ap);
  va_end (ap);
}

void c_candidate_free (CCandidate* ccandidate)
{
  gtk_widget_destroy (ccandidate->window);
  free (ccandidate);
}

static void cb_button_pressed (GObject*    object,
                               GdkEvent*   event,
                               CCandidate* ccandidate)
{
  (void) event;

  int row = GPOINTER_TO_INT (g_object_get_data (object, "row"));
  int col = GPOINTER_TO_INT (g_object_get_data (object, "col"));

  if (!ccandidate || row < 0 || col < 0 ||
      (uint32_t) row >= ccandidate->n_rows ||
      (uint32_t) col >= ccandidate->n_cols)
    g_error ("Cim GTK candidate activation index is invalid");

  if (ccandidate->cb.activate_item)
    ccandidate->cb.activate_item (ccandidate,
                                  (uint32_t) row,
                                  (uint32_t) col,
                                  ccandidate->cb_user_data[C_CANDIDATE_CB_ACTIVATE_ITEM]);
}

/**
 * @brief Return the item at one candidate-table position.
 * @return Item pointer on success, or NULL for an invalid position.
 */
static inline CimItem* c_candidate_get_item (const CimCandidate* candidate,
                                             uint32_t row,
                                             uint32_t col)
{
  gsize index;

  if (!candidate || !candidate->table ||
      row >= candidate->n_rows || col >= candidate->n_cols)
    return NULL;

  if ((gsize) row > (G_MAXSIZE - (gsize) col) / candidate->n_cols)
    g_error ("Cim GTK candidate table index overflow");

  index = (gsize) row * candidate->n_cols + col;
  return &candidate->table[index];
}

void c_candidate_change (CCandidate* ccandidate,
                         const CimCandidate* candidate)
{
  GtkRange* range;
  gdouble value;
  gdouble max;
  uint32_t visible_rows;
  uint32_t visible_cols;

  if (!ccandidate || !candidate)
    g_error ("Cim GTK candidate state is NULL");

  if ((candidate->n_pages != 0 &&
       candidate->page_index >= candidate->n_pages) ||
      (candidate->n_pages != 0 && !candidate->table) ||
      candidate->n_rows > (uint32_t) INT_MAX ||
      candidate->n_cols > (uint32_t) INT_MAX ||
      ccandidate->n_rows > (uint32_t) INT_MAX ||
      ccandidate->n_cols > (uint32_t) INT_MAX)
    g_error ("Cim GTK candidate state is invalid");

  c_candidate_clear (ccandidate);

  range = GTK_RANGE (ccandidate->scrollbar);
  max = (gdouble) MAX (candidate->n_pages, 1);
  gtk_range_set_range (range, 0.0, max);

  value = gtk_range_get_value (range);

  if (!(value >= 0.0 && value <= (gdouble) UINT32_MAX))
    g_error ("Cim GTK candidate page value is invalid");

  if (candidate->page_index != (uint32_t) value)
    gtk_range_set_value (range, (gdouble) candidate->page_index);

  ccandidate->n_pages    = candidate->n_pages;
  ccandidate->page_index = candidate->page_index;

  if (!candidate->n_pages)
    return;

  visible_rows = MIN (candidate->n_rows, ccandidate->n_rows);
  visible_cols = MIN (candidate->n_cols, ccandidate->n_cols);

  for (uint32_t row = 0; row < visible_rows; row++)
  {
    for (uint32_t col = 0; col < visible_cols; col++)
    {
      CimItem* item = c_candidate_get_item (candidate, row, col);

      if (!item)
        g_error ("Cim GTK candidate table index is invalid");

      switch (item->type)
      {
        case CIM_ITEM_STRING:
          {
            const char* str = (const char*) item->data;
            GtkWidget* label;
            GtkCssProvider* provider;
            GtkStyleContext* style_context;
            GtkWidget* table_item;

            if (!str)
              g_error ("Cim GTK candidate item payload is NULL");

            label = gtk_label_new (str);

            provider = gtk_css_provider_new ();
            gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (provider),
                                             "* { font-size: 18px; }",
                                             -1,
                                             NULL);

            style_context = gtk_widget_get_style_context (label);
            gtk_style_context_add_provider
              (style_context,
               GTK_STYLE_PROVIDER (provider),
               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            g_object_unref (provider);

            gtk_widget_set_halign (label, GTK_ALIGN_START);
            gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
            gtk_widget_set_margin_start (label, 5);

            if (col == candidate->n_cols - 1)
            {
              gtk_widget_set_hexpand (label, TRUE);
              gtk_widget_set_margin_end (label, 5);
            }

            table_item = table_item_new ();
            g_object_set_data ((GObject*) table_item,
                               "row",
                               GINT_TO_POINTER ((int) row));
            g_object_set_data ((GObject*) table_item,
                               "col",
                               GINT_TO_POINTER ((int) col));

            g_signal_connect (table_item,
                              "button-press-event",
                              (GCallback) cb_button_pressed,
                              ccandidate);

            gtk_container_add ((GtkContainer*) table_item, label);
            gtk_grid_attach ((GtkGrid*) ccandidate->table,
                             table_item,
                             (int) col,
                             (int) row,
                             1,
                             1);
          }
          break;

        default:
          g_error ("Cim GTK candidate item type is invalid");
      }
    }
  }

  if (candidate->aux_text)
  {
    gtk_entry_set_text ((GtkEntry*) ccandidate->entry, candidate->aux_text);
    gtk_editable_set_position ((GtkEditable*) ccandidate->entry,
                               (int) candidate->aux_cursor_pos);
  }

  gtk_widget_show_all (ccandidate->table);
}

void c_candidate_select (CCandidate* ccandidate, const CimSelection* selection)
{
  const char* validation_error;

  if (!ccandidate)
    g_error ("Cim GTK candidate window is NULL");

  validation_error = c_candidate_selection_validation_error
    (selection, ccandidate->n_rows, ccandidate->n_cols);
  if (validation_error)
    g_error ("Cim GTK candidate selection is invalid: %s",
             validation_error);

  for (uint32_t row = 0; row < ccandidate->n_rows; row++)
    for (uint32_t col = 0; col < ccandidate->n_cols; col++)
    {
      TableItem* item =
        (TableItem*) gtk_grid_get_child_at ((GtkGrid*) ccandidate->table,
                                            (int) col,
                                            (int) row);
      if (item)
        table_item_deselect (item);
    }

  for (uint32_t row = selection->start_row;
       row <= selection->end_row;
       row++)
    for (uint32_t col = selection->start_col;
         col <= selection->end_col;
         col++)
    {
      TableItem* item =
        (TableItem*) gtk_grid_get_child_at ((GtkGrid*) ccandidate->table,
                                            (int) col,
                                            (int) row);
      if (item)
        table_item_select (item);
    }
}

static gboolean cb_entry_draw (GtkWidget *widget,
                               cairo_t   *cr,
                               gpointer   user_data)
{
  GtkEntry* entry = (GtkEntry*) widget;
  GtkStyleContext *style_context;
  PangoContext    *pango_context;
  PangoLayout     *layout;
  const char      *text;
  ptrdiff_t        index_in_bytes_tmp;
  int              index_in_bytes;
  int              x, y;
  int              pos;

  (void) user_data;

  style_context = gtk_widget_get_style_context (widget);
  pango_context = gtk_widget_get_pango_context (widget);
  layout = gtk_entry_get_layout (entry);
  text = pango_layout_get_text (layout);
  gtk_entry_get_layout_offsets (entry, &x, &y);

  pos = gtk_editable_get_position (GTK_EDITABLE (widget));
  index_in_bytes_tmp = g_utf8_offset_to_pointer (text, pos) - text;

  g_return_val_if_fail (index_in_bytes_tmp >= 0, FALSE);
  g_return_val_if_fail (index_in_bytes_tmp <= INT_MAX, FALSE);

  index_in_bytes = (int) index_in_bytes_tmp;

  gtk_render_insertion_cursor (style_context,
                               cr,
                               x,
                               y,
                               layout,
                               index_in_bytes,
                               pango_context_get_base_dir (pango_context));

  return FALSE;
}

static gboolean cb_event_stop (GtkWidget* widget,
                               GdkEvent*  event,
                               gpointer   user_data)
{
  (void) widget;
  (void) event;
  (void) user_data;
  return TRUE; /*  stopping the propagation of an event handler */
}

CCandidate* c_candidate_new ()
{
  if (!gtk_init_check (NULL, NULL))
    return NULL;

  CCandidate* ccandidate = g_malloc0 (sizeof (CCandidate));

  /* create entry */
  ccandidate->entry = gtk_entry_new ();
  gtk_editable_set_editable (GTK_EDITABLE (ccandidate->entry), FALSE);
  gtk_widget_set_no_show_all (ccandidate->entry, TRUE);

  g_signal_connect_after (ccandidate->entry, "draw",
                          G_CALLBACK (cb_entry_draw), NULL);
  g_signal_connect (ccandidate->entry, "button-press-event",
                    G_CALLBACK (cb_event_stop), NULL);
  g_signal_connect (ccandidate->entry, "button-release-event",
                    G_CALLBACK (cb_event_stop), NULL);
  g_signal_connect (ccandidate->entry, "motion-notify-event",
                    G_CALLBACK (cb_event_stop), NULL);

  /* create table */
  ccandidate->table = gtk_grid_new ();

  /* create adjustment */
  GtkAdjustment* adjustment;
  adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 1.0, 1.0, 1.0, 1.0);

  ccandidate->scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adjustment);

  gtk_range_set_slider_size_fixed (GTK_RANGE (ccandidate->scrollbar), FALSE);
  g_signal_connect (ccandidate->scrollbar, "change-value",
                    G_CALLBACK (cb_range_change_value), ccandidate);

  GtkCssProvider*  provider;
  GtkStyleContext* style_context;
  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (provider),
                       "scrollbar {"
                       "  -GtkScrollbar-has-backward-stepper: true;"
                       "  -GtkScrollbar-has-forward-stepper:  true;"
                       "  -GtkScrollbar-has-secondary-forward-stepper:  true;"
                       "}" , -1, NULL);
  style_context = gtk_widget_get_style_context (ccandidate->scrollbar);
  gtk_style_context_add_provider (style_context,
                                  GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);


  gtk_widget_set_hexpand (ccandidate->entry,     TRUE);
  gtk_widget_set_hexpand (ccandidate->table,     TRUE);
  gtk_widget_set_vexpand (ccandidate->table,     TRUE);
  gtk_widget_set_vexpand (ccandidate->scrollbar, TRUE);
  GtkWidget* grid = gtk_grid_new ();
  gtk_grid_attach ((GtkGrid*) grid, ccandidate->table,     0, 1, 1, 1);
  gtk_grid_attach ((GtkGrid*) grid, ccandidate->scrollbar, 1, 1, 1, 1);
  gtk_grid_attach ((GtkGrid*) grid, ccandidate->entry, 0, 0, 2, 1);

  ccandidate->window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (ccandidate->window),
                            GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_focus_on_map (GTK_WINDOW (ccandidate->window), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (ccandidate->window), 1);
  gtk_container_add ((GtkContainer*) ccandidate->window, grid);

  return ccandidate;
}
