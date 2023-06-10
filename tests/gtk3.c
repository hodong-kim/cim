#include <gtk/gtk.h>

int main ()
{
  GtkWidget *window;
  GtkWidget *editor;

  setenv ("GTK_IM_MODULE", "cim", TRUE);

  gtk_init (NULL, NULL);

  editor = gtk_text_view_new ();

  window = gtk_window_new     (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 300);

  gtk_container_add (GTK_CONTAINER (window), editor);

  g_signal_connect (window, "destroy", gtk_main_quit, NULL);

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
