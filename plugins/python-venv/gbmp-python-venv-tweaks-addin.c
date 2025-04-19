#define G_LOG_DOMAIN "gbmp-python-venv-tweaks-addin"

#include "gbmp-python-venv-tweaks-addin.h"

struct _GbmpPythonVenvTweaksAddin
{
  IdeTweaksAddin parent_instance;

  GtkFileDialog *folder_file_dialog;
};

G_DEFINE_FINAL_TYPE (GbmpPythonVenvTweaksAddin,
                     gbmp_python_venv_tweaks_addin,
                     IDE_TYPE_TWEAKS_ADDIN)

//////// GObject
static void g_object_dispose (GObject *self);

//////// Private Functions
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
  ide_tweaks_addin_set_resource_paths (IDE_TWEAKS_ADDIN (self),
                                       IDE_STRV_INIT ("/plugins/python-venv/tweaks.ui"));

  ide_tweaks_addin_bind_callback (IDE_TWEAKS_ADDIN (self),
                                  list_item_create_for_item);

  ide_tweaks_addin_bind_callback (IDE_TWEAKS_ADDIN (self),
                                  add_item_create_for_item);

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
  parent_class->dispose (self);
}

//////// Private functions

//// UI Callbacks

static GtkWidget *
list_item_create_for_item (IdeTweaksWidget *self,
                           IdeTweaksItem   *item,
                           gpointer         user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GtkStringList *venv_list = NULL;
  GtkListBox *content = NULL;

  content = (GtkListBox*) gtk_list_box_new ();
  gtk_list_box_set_selection_mode (content, GTK_SELECTION_NONE);
  gtk_widget_add_css_class (GTK_WIDGET(content), "boxed-list");

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);
  venv_list = gtk_string_list_new(NULL);
  gtk_string_list_append(venv_list, "A");
  gtk_string_list_append(venv_list, "B");

  gtk_list_box_bind_model (content,
                           G_LIST_MODEL(venv_list),
                           list_create_row,
                           addin,
                           NULL);

  g_object_unref (venv_list);
  return GTK_WIDGET(content);
}

static GtkWidget *
list_create_row (gpointer item, gpointer user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GtkStringObject *stro = NULL;
  const gchar *str = NULL;
  AdwPreferencesRow *row = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);
  stro = GTK_STRING_OBJECT (item);
  str = gtk_string_object_get_string (stro);

  row = ADW_PREFERENCES_ROW(adw_action_row_new());
  adw_preferences_row_set_title (row, str);
  adw_preferences_row_set_title_selectable (row, FALSE);

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
                                 NULL /* GAsyncReadyCallback callback*/,
                                 NULL /* userdata of callback */);
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

