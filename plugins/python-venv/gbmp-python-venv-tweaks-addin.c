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
  gboolean       is_adding_venv;
};

G_DEFINE_FINAL_TYPE (GbmpPythonVenvTweaksAddin,
                     gbmp_python_venv_tweaks_addin,
                     IDE_TYPE_TWEAKS_ADDIN)

//////// GObject

enum
{
  PROP_0,
  PROP_IS_ADDING_VENV,
  N_PROPS
};

static GParamSpec *pspecs[N_PROPS] = { NULL };

static void
_g_object_finalize (GObject *self);

static void
_g_object_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec);

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

static GtkWidget *
wait_item_create_for_item (IdeTweaksWidget *self,
                           IdeTweaksItem   *item,
                           gpointer         user_data);

static void
add_button_clicked (GtkButton *self, gpointer user_data);

static void
add_button_clicked_folder_file_dialog_select_folder_done (GObject      *source,
                                                          GAsyncResult *result,
                                                          gpointer      user_data);
static void
add_button_clicked_folder_add_done (GObject      *source,
                                    GAsyncResult *result,
                                    gpointer      user_data);

static void
make_button_clicked (GtkButton *self, gpointer user_data);

static void
make_button_clicked_folder_file_dialog_select_folder_done (GObject      *source,
                                                           GAsyncResult *result,
                                                           gpointer      user_data);

static void
make_button_clicked_folder_make_done (GObject      *source,
                                      GAsyncResult *result,
                                      gpointer      user_data);

//// Actions

typedef struct _ClosureChoice
{
  GbmpPythonVenvApplicationAddin *app_addin;
  gchar *path;
} ClosureChoice;

static GSimpleActionGroup *
list_action_group (GbmpPythonVenvApplicationAddin *app_addin);

static void
list_action_remove_activate (GSimpleAction *action,
                             GVariant      *param,
                             gpointer       user_data);

static void
list_action_remove_activate_chosen (GObject      *source,
                                    GAsyncResult *result,
                                    gpointer      user_data);

static void
list_action_purge_activate (GSimpleAction *action,
                            GVariant      *param,
                            gpointer       user_data);

static void
list_action_purge_activate_chosen (GObject      *source,
                                   GAsyncResult *result,
                                   gpointer      user_data);

static void
list_action_purge_done (GObject      *source,
                        GAsyncResult *result,
                        gpointer      user_data);

//////// GTypeInstance

static void
gbmp_python_venv_tweaks_addin_class_init (GbmpPythonVenvTweaksAddinClass *c)
{
  GObjectClass *c_g_object = NULL;

  c_g_object = G_OBJECT_CLASS (c);
  c_g_object->finalize = _g_object_finalize;
  c_g_object->get_property = _g_object_get_property;

  pspecs[PROP_IS_ADDING_VENV] = g_param_spec_boolean ("is-adding-venv", "Is Adding Venv",
                                                      "Whether this is adding virtual environment.",
                                                      FALSE,
                                                      G_PARAM_STATIC_STRINGS |
                                                      G_PARAM_READABLE);

  g_object_class_install_properties (c_g_object, N_PROPS, pspecs);
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

  ide_tweaks_addin_bind_callback (IDE_TWEAKS_ADDIN (self),
                                  wait_item_create_for_item);

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
_g_object_finalize (GObject *self)
{
  GObjectClass *parent_class = NULL;
  GbmpPythonVenvTweaksAddin *addin_self = NULL;

  parent_class = G_OBJECT_CLASS (gbmp_python_venv_tweaks_addin_parent_class);
  addin_self = GBMP_PYTHON_VENV_TWEAKS_ADDIN (self);

  addin_self->is_adding_venv = false;
  g_clear_object (&addin_self->folder_file_dialog);
  g_clear_object (&addin_self->python_venv_store);
  g_clear_object (&addin_self->addin);
  parent_class->dispose (self);
}

static void
_g_object_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GbmpPythonVenvTweaksAddin *addin_self = NULL;

  addin_self = GBMP_PYTHON_VENV_TWEAKS_ADDIN (object);

  switch (prop_id)
    {
    case PROP_IS_ADDING_VENV:
      g_value_set_boolean (value, addin_self->is_adding_venv);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
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
  GSimpleActionGroup        *action_group = NULL;

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
  action_group = list_action_group (addin->addin);

  gtk_widget_insert_action_group (GTK_WIDGET(content),
                                  "item",
                                  G_ACTION_GROUP(action_group));

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

  const gchar *path = NULL;
  GMenu *menu = NULL;
  GMenuItem *menu_item_remove = NULL;
  GMenuItem *menu_item_purge = NULL;

  GtkBuilder *builder = NULL;
  AdwActionRow *row = NULL;

  GError *error = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);
  data = GBMP_PYTHON_VENV_VENV_DATA (item);

  path = gbmp_python_venv_venv_data_get_path (data);

  menu = g_menu_new ();
  menu_item_remove = g_menu_item_new("Remove", NULL);
  g_menu_item_set_action_and_target (menu_item_remove, "item.remove-python-venv", "s", path);
  menu_item_purge = g_menu_item_new("Purge", NULL);
  g_menu_item_set_action_and_target (menu_item_purge, "item.purge-python-venv", "s", path);
  g_menu_append_item (menu, menu_item_remove);
  g_menu_append_item (menu, menu_item_purge);

  builder = gtk_builder_new ();
  gtk_builder_expose_object (builder, "GbmpPythonVenvTweaksAddin", G_OBJECT(addin));
  gtk_builder_expose_object (builder, "GbmpPythonVenvVenvData", G_OBJECT(data));
  gtk_builder_expose_object (builder, "menu", G_OBJECT(menu));
  gtk_builder_add_from_resource (builder, "/plugins/python-venv/tweaks_list_row.ui", &error);

  if (error != NULL)
    {
      g_warning ("Gtk Builder Error for row: %s", error->message);
      g_error_free (error);
    }

  row = ADW_ACTION_ROW (g_object_ref (gtk_builder_get_object (builder, "row")));
  g_object_unref (builder);
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

static GtkWidget *
wait_item_create_for_item (IdeTweaksWidget *self,
                           IdeTweaksItem   *item,
                           gpointer         user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GtkBuilder *content_builder = NULL;
  GtkWidget *content = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  content_builder = gtk_builder_new ();

  gtk_builder_expose_object (content_builder,
                             "GbmpPythonVenvTweaksAddin",
                             G_OBJECT (addin));

  gtk_builder_add_from_resource (content_builder,
                                 "/plugins/python-venv/tweaks_wait_item.ui",
                                 NULL);

  g_set_object (&content,
                GTK_WIDGET (gtk_builder_get_object (content_builder, "content")));

  g_object_unref (content_builder);
  return content;
}

static void
add_button_clicked (GtkButton *self, gpointer user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  addin->is_adding_venv = TRUE;
  g_object_notify_by_pspec (G_OBJECT(addin), pspecs[PROP_IS_ADDING_VENV]);
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
    addin->is_adding_venv = FALSE;
    g_object_notify_by_pspec (G_OBJECT(addin), pspecs[PROP_IS_ADDING_VENV]);
    g_debug ("add_butotn_clicked_folder_file_dialog_select_folder_done: %s", error->message);
    g_error_free (error);
    return;
  }

  gbmp_python_venv_application_addin_add_python_venv_async (addin->addin,
                                                            selected,
                                                            NULL,
                                                            add_button_clicked_folder_add_done,
                                                            addin);
}

static void
add_button_clicked_folder_add_done (GObject      *source,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  GbmpPythonVenvApplicationAddin *app_addin = NULL;
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GError                    *error = NULL;

  app_addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (source);
  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  gbmp_python_venv_application_addin_add_python_venv_finish (app_addin, result, &error);

  addin->is_adding_venv = FALSE;
  g_object_notify_by_pspec (G_OBJECT(addin), pspecs[PROP_IS_ADDING_VENV]);

  if (error != NULL)
    {
      g_debug ("add_button_clicked_folder_add_done: %s", error->message);
      g_error_free (error);
      return;
  }
}


static void
make_button_clicked (GtkButton *self, gpointer user_data)
{
  GbmpPythonVenvTweaksAddin *addin = NULL;

  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  addin->is_adding_venv = TRUE;
  g_object_notify_by_pspec (G_OBJECT(addin), pspecs[PROP_IS_ADDING_VENV]);
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
    addin->is_adding_venv = FALSE;
    g_object_notify_by_pspec (G_OBJECT(addin), pspecs[PROP_IS_ADDING_VENV]);
    g_debug ("make_butotn_clicked_folder_file_dialog_select_folder_done: %s", error->message);
    g_error_free (error);
    return;
  }

  gbmp_python_venv_application_addin_make_python_venv_async (addin->addin,
                                                            selected,
                                                            NULL,
                                                            make_button_clicked_folder_make_done,
                                                            addin);
}


static void
make_button_clicked_folder_make_done (GObject      *source,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  GbmpPythonVenvApplicationAddin *app_addin = NULL;
  GbmpPythonVenvTweaksAddin *addin = NULL;
  GError                    *error = NULL;

  app_addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (source);
  addin = GBMP_PYTHON_VENV_TWEAKS_ADDIN (user_data);

  gbmp_python_venv_application_addin_make_python_venv_finish (app_addin, result, &error);

  addin->is_adding_venv = FALSE;
  g_object_notify_by_pspec (G_OBJECT(addin), pspecs[PROP_IS_ADDING_VENV]);

  if (error != NULL)
    {
      g_debug ("make_button_clicked_folder_make_done: %s", error->message);
      g_error_free (error);
      return;
  }
}

static GSimpleActionGroup *
list_action_group (GbmpPythonVenvApplicationAddin *app_addin)
{
  GSimpleActionGroup *action_group = NULL;
  GSimpleAction *action_remove = NULL;
  GSimpleAction *action_purge = NULL;

  action_group = g_simple_action_group_new ();

  action_remove = g_simple_action_new ("remove-python-venv",
                                       g_variant_type_new ("s"));

  g_signal_connect (action_remove,
                    "activate",
                    G_CALLBACK (list_action_remove_activate),
                    app_addin);

  action_purge = g_simple_action_new ("purge-python-venv",
                                       g_variant_type_new ("s"));

  g_signal_connect (action_purge,
                    "activate",
                    G_CALLBACK (list_action_purge_activate),
                    app_addin);

  g_action_map_add_action (G_ACTION_MAP(action_group),
                           G_ACTION(action_remove));
  g_action_map_add_action (G_ACTION_MAP(action_group),
                           G_ACTION(action_purge));

  return action_group;
}

static void
list_action_remove_activate (GSimpleAction *action,
                             GVariant      *param,
                             gpointer       user_data)
{
  GbmpPythonVenvApplicationAddin *app_addin = NULL;
  const gchar *param_str = NULL;

  GtkAlertDialog *dialog = NULL;
  gchar *detailed_message = NULL;

  ClosureChoice *closure = NULL;

  app_addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (user_data);
  param_str = g_variant_get_string (param, NULL);

  dialog = gtk_alert_dialog_new ("Remove the virtual environment?");

  detailed_message = g_strdup_printf(
      "Removing virtual environment.\n"
      "%s\n"
      "Removed environment can be added back later.",
      param_str);

  gtk_alert_dialog_set_detail (dialog, detailed_message);
  gtk_alert_dialog_set_modal (dialog, TRUE);
  gtk_alert_dialog_set_buttons (dialog, IDE_STRV_INIT("Confirm", "Cancel"));
  gtk_alert_dialog_set_default_button (dialog, 0);
  gtk_alert_dialog_set_cancel_button (dialog, 1);

  g_free (detailed_message);

  closure = g_new (ClosureChoice, 1);
  closure->app_addin = app_addin;
  closure->path = g_strdup (param_str);

  gtk_alert_dialog_choose (dialog,
                           NULL,
                           NULL,
                           list_action_remove_activate_chosen,
                           closure);
  g_object_unref (dialog);
}


static void
list_action_remove_activate_chosen (GObject      *source,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  GtkAlertDialog *dialog = NULL;
  ClosureChoice *closure = NULL;
  GError *error = NULL;
  gint choice = 0;

  dialog = GTK_ALERT_DIALOG(source);
  closure = (ClosureChoice *)user_data;

  choice = gtk_alert_dialog_choose_finish (dialog, result, &error);

  if (choice == 0)
    {
      gbmp_python_venv_application_addin_remove_python_venv_path (closure->app_addin,
                                                                  closure->path);
    }

  g_free (closure->path);
  g_free (closure);
}

static void
list_action_purge_activate (GSimpleAction *action,
                             GVariant      *param,
                             gpointer       user_data)
{
  GbmpPythonVenvApplicationAddin *app_addin = NULL;
  const gchar *param_str = NULL;

  GtkAlertDialog *dialog = NULL;
  gchar *detailed_message = NULL;

  ClosureChoice *closure = NULL;

  app_addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (user_data);
  param_str = g_variant_get_string (param, NULL);

  dialog = gtk_alert_dialog_new ("Purge the virtual environment?");

  detailed_message = g_strdup_printf(
      "Purging virtual environment.\n"
      "%s\n"
      "This will DELETE all content and the directory.",
      param_str);

  gtk_alert_dialog_set_detail (dialog, detailed_message);
  gtk_alert_dialog_set_modal (dialog, TRUE);
  gtk_alert_dialog_set_buttons (dialog, IDE_STRV_INIT("Confirm", "Cancel"));
  gtk_alert_dialog_set_default_button (dialog, 0);
  gtk_alert_dialog_set_cancel_button (dialog, 1);

  g_free (detailed_message);

  closure = g_new (ClosureChoice, 1);
  closure->app_addin = app_addin;
  closure->path = g_strdup (param_str);

  gtk_alert_dialog_choose (dialog,
                           NULL,
                           NULL,
                           list_action_purge_activate_chosen,
                           closure);
  g_object_unref (dialog);
}

static void
list_action_purge_activate_chosen (GObject      *source,
                                   GAsyncResult *result,
                                   gpointer      user_data)
{
  GtkAlertDialog *dialog = NULL;
  ClosureChoice *closure = NULL;
  GError *error = NULL;
  gint choice = 0;

  dialog = GTK_ALERT_DIALOG(source);
  closure = (ClosureChoice *)user_data;

  choice = gtk_alert_dialog_choose_finish (dialog, result, &error);

  if (choice == 0)
    {
      gbmp_python_venv_application_addin_purge_python_venv_path_async (closure->app_addin,
                                                                       closure->path,
                                                                       NULL,
                                                                       list_action_purge_done,
                                                                       NULL);
    }

  g_free (closure->path);
  g_free (closure);
}

static void
list_action_purge_done (GObject      *source,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  GbmpPythonVenvApplicationAddin *app_addin = NULL;
  GError *error = NULL;

  app_addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (source);

  gbmp_python_venv_application_addin_purge_python_venv_finish (app_addin, result, &error);

  if (error != NULL)
    {
      g_warning ("list_row_purge_done: %s", error->message);
      g_error_free (error);
      return;
    }
}

