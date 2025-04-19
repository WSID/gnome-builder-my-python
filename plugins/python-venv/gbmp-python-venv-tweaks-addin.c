#define G_LOG_DOMAIN "gbmp-python-venv-tweaks-addin"

#include <adwaita.h>

#include "gbmp-python-venv-tweaks-addin.h"

struct _GbmpPythonVenvTweaksAddin
{
  IdeTweaksAddin parent_instance;

  GSettings     *settings;
  GtkStringList *venv_list;
  GtkFileDialog *folder_file_dialog;
};

G_DEFINE_FINAL_TYPE (GbmpPythonVenvTweaksAddin,
                     gbmp_python_venv_tweaks_addin,
                     IDE_TYPE_TWEAKS_ADDIN)

//////// GObject
static void g_object_dispose (GObject *self);

//////// Private Functions
//// Settings Callbacks
static void
settings_python_venvs_changed(GSettings   *settings,
                              const gchar *key,
                              gpointer     user_data);
//// UI Callbacks

static GtkWidget *
list_item_create_for_item (IdeTweaksWidget *self,
                           IdeTweaksItem   *item,
                           gpointer         user_data);

static GtkWidget *
list_create_row (gpointer item, gpointer user_data);

static GtkWidget *
add_item_create_for_item (IdeTweaksWidget *self,
                          IdeTweaksItem   *item,
                          gpointer         user_data);

static void
add_button_clicked (GtkButton *self, gpointer user_data);

static void
add_button_clicked_folder_file_dialog_select_folder_done (GObject      *source,
                                                          GAsyncResult *result,
                                                          gpointer      user_data);

static void
make_button_clicked (GtkButton *self, gpointer user_data);

//////// GTypeInstance

static void
gbmp_python_venv_tweaks_addin_class_init (GbmpPythonVenvTweaksAddinClass *c)
{
  GObjectClass *c_g_object = NULL;

  c_g_object = G_OBJECT_CLASS (c);
  c_g_object->dispose = g_object_dispose;
}

static void
gbmp_python_venv_tweaks_addin_init (GbmpPythonVenvTweaksAddin *self)
{
  gchar **python_venvs = NULL;

  ide_tweaks_addin_set_resource_paths (IDE_TWEAKS_ADDIN (self),
                                       IDE_STRV_INIT ("/plugins/python-venv/tweaks.ui"));

  ide_tweaks_addin_bind_callback (IDE_TWEAKS_ADDIN (self),
                                  list_item_create_for_item);

  ide_tweaks_addin_bind_callback (IDE_TWEAKS_ADDIN (self),
                                  add_item_create_for_item);

  self->settings = g_settings_new("org.gnome.builder.PythonVenv");
  python_venvs = g_settings_get_strv(self->settings, "python-venvs");

  self->venv_list = gtk_string_list_new((const gchar * const *)python_venvs);
  g_signal_connect (self->settings,
                    "changed::python-venvs",
                    G_CALLBACK(settings_python_venvs_changed),
                    self->venv_list);

  self->folder_file_dialog = gtk_file_dialog_new ();

  g_strfreev(python_venvs);
}

//////// GObject

static void
g_object_dispose (GObject *self)
{
  GObjectClass *parent_class = NULL;
  GbmpPythonVenvTweaksAddin *addin_self = NULL;

  parent_class = G_OBJECT_CLASS (gbmp_python_venv_tweaks_addin_parent_class);
  addin_self = GBMP_PYTHON_VENV_TWEAKS_ADDIN (self);

  g_clear_object (&addin_self->folder_file_dialog);
  g_clear_object (&addin_self->venv_list);
  g_clear_object (&addin_self->settings);
  parent_class->dispose (self);
}

//////// Private functions

//// Settings Callbacks
static void
settings_python_venvs_changed(GSettings   *settings,
                              const gchar *key,
                              gpointer     user_data)
{
  GtkStringList *venv_list = NULL;
  guint          venv_len = 0;
  gchar        **strv = NULL;

  venv_list = GTK_STRING_LIST (user_data);
  venv_len = g_list_model_get_n_items(G_LIST_MODEL(venv_list));

  strv = g_settings_get_strv (settings, key);

  gtk_string_list_splice (venv_list, 0, venv_len, (const gchar* const*)strv);

  g_strfreev(strv);
}

//// UI Callbacks

static GtkWidget *
list_item_create_for_item (IdeTweaksWidget *self,
                           IdeTweaksItem   *item,
                           gpointer         user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;

  AdwActionRow              *empty_placeholder = NULL;
  GtkListBox                *content = NULL;

  empty_placeholder = ADW_ACTION_ROW(adw_action_row_new ());

  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (empty_placeholder),
                                 "No known virtual environment.");

  adw_action_row_set_subtitle (empty_placeholder,
                               "Try add existing one, or make one.");

  content = (GtkListBox*) gtk_list_box_new ();
  gtk_widget_add_css_class (GTK_WIDGET(content), "boxed-list");
  gtk_list_box_set_selection_mode (content, GTK_SELECTION_NONE);
  gtk_list_box_set_placeholder (content, GTK_WIDGET(empty_placeholder));

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  gtk_list_box_bind_model (content,
                           G_LIST_MODEL(addin->venv_list),
                           list_create_row,
                           addin,
                           NULL);

  return GTK_WIDGET(content);
}

static GtkWidget *
list_create_row (gpointer item, gpointer user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GtkStringObject *stro = NULL;
  const gchar *str = NULL;
  gchar *base_name = NULL;
  gchar *dir_name = NULL;
  AdwActionRow *row = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);
  stro = GTK_STRING_OBJECT (item);
  str = gtk_string_object_get_string (stro);

  base_name = g_path_get_basename (str);
  dir_name = g_path_get_dirname (str);

  row = ADW_ACTION_ROW (adw_action_row_new());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), base_name);
  adw_preferences_row_set_title_selectable (ADW_PREFERENCES_ROW (row), FALSE);
  adw_action_row_set_subtitle (row, dir_name);

  g_clear_pointer (&base_name, g_free);
  g_clear_pointer (&dir_name, g_free);

  return GTK_WIDGET(row);
}


static GtkWidget *
add_item_create_for_item (IdeTweaksWidget *self,
                          IdeTweaksItem   *item,
                          gpointer         user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GtkBuilder *content_builder = NULL;
  GtkBuilderScope *content_scope = NULL;
  GtkWidget *content = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  content_builder = gtk_builder_new ();
  content_scope = gtk_builder_cscope_new ();

  gtk_builder_expose_object (content_builder,
                             "GbmpPythonVenvTweaksAddin",
                             G_OBJECT (addin));

  gtk_builder_cscope_add_callback (content_scope, add_button_clicked);
  gtk_builder_cscope_add_callback (content_scope, make_button_clicked);
  gtk_builder_set_scope(content_builder,
                        GTK_BUILDER_SCOPE (content_scope));

  gtk_builder_add_from_resource (content_builder,
                                 "/plugins/python-venv/tweaks_add_item.ui",
                                 NULL);

  g_set_object (&content,
                GTK_WIDGET (gtk_builder_get_object (content_builder, "content")));

  g_object_unref (content_scope);
  g_object_unref (content_builder);
  return content;
}

static void
add_button_clicked (GtkButton *self, gpointer user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  gtk_file_dialog_select_folder (addin->folder_file_dialog,
                                 NULL /* GtkWindow parent */,
                                 NULL /* GCancellable cancellable*/,
                                 add_button_clicked_folder_file_dialog_select_folder_done,
                                 addin);
}

static void
add_button_clicked_folder_file_dialog_select_folder_done (GObject      *source,
                                                          GAsyncResult *result,
                                                          gpointer      user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GtkFileDialog             *file_dialog = NULL;
  GFile                     *selected = NULL;
  gchar                     *selected_str = NULL;
  gchar                   **settings_list = NULL;
  GStrvBuilder             *settings_list_builder = NULL;

  GError                   *error = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);
  file_dialog = GTK_FILE_DIALOG (source);
  selected = gtk_file_dialog_select_folder_finish (file_dialog, result, &error);

  if (error != NULL) {
    g_debug ("add_butotn_clicked_folder_file_dialog_select_folder_done: %s", error->message);
    return;
  }
  // TODO: Check selected is python virtual environment.

  selected_str = g_file_get_path(selected);

  settings_list = g_settings_get_strv (addin->settings, "python-venvs");
  settings_list_builder = g_strv_builder_new ();

  g_strv_builder_addv (settings_list_builder, (const gchar **)settings_list);
  g_strfreev (settings_list);

  g_strv_builder_add (settings_list_builder, selected_str);
  g_free (selected_str);

  settings_list = g_strv_builder_unref_to_strv (settings_list_builder);
  g_settings_set_strv (addin->settings, "python-venvs", (const gchar * const *) settings_list);
  g_strfreev(settings_list);
}


static void
make_button_clicked (GtkButton *self, gpointer user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  gtk_file_dialog_select_folder (addin->folder_file_dialog,
                                 NULL /* GtkWindow parent */,
                                 NULL /* GCancellable cancellable*/,
                                 NULL /* GAsyncReadyCallback callback*/,
                                 NULL /* userdata of callback */);
}

