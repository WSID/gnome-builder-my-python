#define G_LOG_DOMAIN "gbmp-python-venv-tweaks-addin"

#include <adwaita.h>

#include "gbmp-python-venv-application-addin.h"
#include "gbmp-python-venv-data.h"
#include "gbmp-python-venv-tweaks-addin.h"

struct _GbmpPythonVenvTweaksAddin
{
  IdeTweaksAddin parent_instance;

  GbmpPythonVenvApplicationAddin *addin;
  GListStore *python_venv_store;
  GtkFileDialog *folder_file_dialog;
};

G_DEFINE_FINAL_TYPE (GbmpPythonVenvTweaksAddin,
                     gbmp_python_venv_tweaks_addin,
                     IDE_TYPE_TWEAKS_ADDIN)

//////// GObject
static void g_object_dispose (GObject *self);

//////// Private Functions
//// Addin Callbacks
static void
addin_python_venvs_notify(GObject    *object,
                          GParamSpec *pspec,
                          gpointer    user_data);
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

static void
make_button_clicked_folder_file_dialog_select_folder_done (GObject      *source,
                                                           GAsyncResult *result,
                                                           gpointer      user_data);

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
  GPtrArray *python_venv_array = NULL;

  ide_tweaks_addin_set_resource_paths (IDE_TWEAKS_ADDIN (self),
                                       IDE_STRV_INIT ("/plugins/python-venv/tweaks.ui"));

  ide_tweaks_addin_bind_callback (IDE_TWEAKS_ADDIN (self),
                                  list_item_create_for_item);

  ide_tweaks_addin_bind_callback (IDE_TWEAKS_ADDIN (self),
                                  add_item_create_for_item);

  self->addin = g_object_ref (gbmp_python_venv_application_addin_get_instance ());

  python_venv_array = gbmp_python_venv_application_addin_get_venv_datas (self->addin);
  self->python_venv_store = g_list_store_new (GBMP_TYPE_PYTHON_VENV_VENV_DATA);
  g_list_store_splice (self->python_venv_store, 0, 0, python_venv_array->pdata, python_venv_array->len);
  g_ptr_array_unref (python_venv_array);

  g_signal_connect (self->addin,
                    "notify::python-venv-datas",
                    G_CALLBACK(addin_python_venvs_notify),
                    self->python_venv_store);

  self->folder_file_dialog = gtk_file_dialog_new ();
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
  g_clear_object (&addin_self->python_venv_store);
  g_clear_object (&addin_self->addin);
  parent_class->dispose (self);
}

//////// Private functions

//// Settings Callbacks
static void
addin_python_venvs_notify(GObject    *object,
                          GParamSpec *pspec,
                          gpointer    user_data)
{
  GbmpPythonVenvApplicationAddin *addin = NULL;
  GListStore *python_venv_store = NULL;
  guint       python_venv_len = 0;

  GPtrArray *python_venv_array = NULL;

  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN(object);

  python_venv_store = G_LIST_STORE(user_data);
  python_venv_len = g_list_model_get_n_items (G_LIST_MODEL (python_venv_store));

  python_venv_array = gbmp_python_venv_application_addin_get_venv_datas (addin);

  g_list_store_splice (python_venv_store, 0, python_venv_len, python_venv_array->pdata, python_venv_array->len);
  g_ptr_array_unref (python_venv_array);
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
                           G_LIST_MODEL(addin->python_venv_store),
                           list_create_row,
                           addin,
                           NULL);

  return GTK_WIDGET(content);
}

static GtkWidget *
list_create_row (gpointer item, gpointer user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GbmpPythonVenvVenvData *data = NULL;

  // For now we treat python venv only.
  const gchar *prompt = NULL;
  const gchar *path = NULL;

  AdwActionRow *row = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);
  data = GBMP_PYTHON_VENV_VENV_DATA (item);

  prompt = gbmp_python_venv_venv_data_get_prompt (data);
  path = gbmp_python_venv_venv_data_get_path (data);

  row = ADW_ACTION_ROW (adw_action_row_new());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), prompt);
  adw_preferences_row_set_title_selectable (ADW_PREFERENCES_ROW (row), FALSE);
  adw_action_row_set_subtitle (row, path);

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
  GError                    *error = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);
  file_dialog = GTK_FILE_DIALOG (source);
  selected = gtk_file_dialog_select_folder_finish (file_dialog, result, &error);

  if (error != NULL) {
    g_debug ("add_butotn_clicked_folder_file_dialog_select_folder_done: %s", error->message);
    return;
  }

  gbmp_python_venv_application_addin_add_python_venv_async (addin->addin,
                                                            selected,
                                                            NULL,
                                                            NULL,
                                                            NULL);
}


static void
make_button_clicked (GtkButton *self, gpointer user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  gtk_file_dialog_select_folder (addin->folder_file_dialog,
                                 NULL /* GtkWindow parent */,
                                 NULL /* GCancellable cancellable*/,
                                 make_button_clicked_folder_file_dialog_select_folder_done,
                                 addin);
}

static void
make_button_clicked_folder_file_dialog_select_folder_done (GObject      *source,
                                                           GAsyncResult *result,
                                                           gpointer      user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GtkFileDialog             *file_dialog = NULL;
  GFile                     *selected = NULL;
  GError                    *error = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);
  file_dialog = GTK_FILE_DIALOG (source);
  selected = gtk_file_dialog_select_folder_finish (file_dialog, result, &error);

  if (error != NULL) {
    g_debug ("make_butotn_clicked_folder_file_dialog_select_folder_done: %s", error->message);
    return;
  }

  gbmp_python_venv_application_addin_make_python_venv_async (addin->addin,
                                                            selected,
                                                            NULL,
                                                            NULL,
                                                            NULL);
}
