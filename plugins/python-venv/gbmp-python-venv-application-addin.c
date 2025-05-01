#include "gbmp-python-venv-application-addin.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "gbmp-python-venv-application-addin"


////////

static void
gbmp_python_venv_application_addin_add_python_venv_data_new (GObject      *source,
                                                             GAsyncResult *result,
                                                             gpointer      user_data);

typedef struct
{
  IdeTask *task;
  gchar   *path;
}
ClosureSetupDone;

static void
gbmp_python_venv_application_addin_make_python_venv_mkdir (GObject      *source,
                                                           GAsyncResult *result,
                                                           gpointer      user_data);

static void
gbmp_python_venv_application_addin_make_python_venv_setup (GbmpPythonVenvApplicationAddin *addin,
                                                          gchar                           *path,
                                                           IdeTask                        *task);

static void
gbmp_python_venv_application_addin_make_python_venv_setup_done (GObject      *source,
                                                                GAsyncResult *result,
                                                                gpointer      user_data);

static void
gbmp_python_venv_application_addin_make_python_venv_data_new (GObject      *source,
                                                              GAsyncResult *result,
                                                              gpointer      user_data);

typedef struct
{
  IdeTask *task;
  GbmpPythonVenvVenvData *data;
}
ClosureReap;

static void
gbmp_python_venv_application_addin_purge_python_venv_reap (GObject      *source,
                                                           GAsyncResult *result,
                                                           gpointer      user_data);
//////// GTypeInstance

struct _GbmpPythonVenvApplicationAddin
{
  GObject parent_instance;

  GSettings *settings;
  gboolean python_venvs_is_setting;

  GHashTable *table_path_data;
};

static GbmpPythonVenvApplicationAddin *instance = NULL;

static void
_ide_application_addin_iface_init (IdeApplicationAddinInterface *iface);

static guint sig_python_venv_added = 0;
static guint sig_python_venv_removed = 0;

//////// GObject

enum
{
  PROP_0,
  PROP_PYTHON_VENV_DATAS,

  N_PROPS
};

GParamSpec *props[N_PROPS] = { NULL };

static void
_g_object_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec);

static void
_g_object_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec);

static void
_g_object_finalize (GObject *object);


//////// IdeApplicationAddin

static void
_ide_application_addin_load (IdeApplicationAddin *addin,
                             IdeApplication      *application);

//////// Privates

static void
_settings_python_venvs_changed (GSettings *settings,
                                gchar     *key,
                                gpointer   user_data);

static void
_setup_python_venvs (GbmpPythonVenvApplicationAddin *addin,
                     const gchar * const            *python_venvs);

static void
_setup_python_venvs_remove_create (gpointer key,
                                   gpointer value,
                                   gpointer user_data);

typedef struct _ClosureVenvsDataNew {
  GbmpPythonVenvApplicationAddin *addin;
  guint                           num_left_running;
} ClosureVenvsDataNew;

static void
_setup_python_venvs_data_new (GObject      *source,
                              GAsyncResult *result,
                              gpointer      user_data);

static void
_setup_python_venvs_data_new_then (ClosureVenvsDataNew *closure);

static void
_update_settings (GbmpPythonVenvApplicationAddin *addin);

//////// Public functions

GbmpPythonVenvApplicationAddin *
gbmp_python_venv_application_addin_get_instance (void)
{
  return instance;
}


GPtrArray *
gbmp_python_venv_application_addin_get_venv_datas (GbmpPythonVenvApplicationAddin *addin)
{
  return g_hash_table_get_values_as_ptr_array (addin->table_path_data);
}


void
gbmp_python_venv_application_addin_add_python_venv_async (GbmpPythonVenvApplicationAddin *addin,
                                                          GFile                          *directory,
                                                          GCancellable                   *cancel,
                                                          GAsyncReadyCallback             callback,
                                                          gpointer                        user_data)
{
  IdeTask *task = NULL;
  gchar *directory_path = NULL;
  gboolean directory_exists = FALSE;
  GFileType directory_type = G_FILE_TYPE_UNKNOWN;

  task = ide_task_new (addin, cancel, callback, user_data);
  if (ide_task_return_error_if_cancelled (task))
    {
      return;
    }

  // Check directory and get path.
  directory_path = g_file_get_path (directory);
  directory_exists = g_file_query_exists (directory, cancel);
  if (! directory_exists)
    {
      ide_task_return_new_error (task,
                                 G_IO_ERROR,
                                 G_IO_ERROR_NOT_FOUND,
                                 "Directory not found: %s",
                                 directory_path);
      g_free (directory_path);
      return;
    }

  directory_type = g_file_query_file_type (directory,
                                           G_FILE_QUERY_INFO_NONE,
                                           cancel);
  if (directory_type != G_FILE_TYPE_DIRECTORY)
    {
      ide_task_return_new_error (task,
                                 G_IO_ERROR,
                                 G_IO_ERROR_NOT_DIRECTORY,
                                 "Not a directory: %s",
                                 directory_path);
      g_free (directory_path);
      return;
    }

  // Initialize data.
  gbmp_python_venv_venv_data_new_async (directory_path,
                                        cancel,
                                        gbmp_python_venv_application_addin_add_python_venv_data_new,
                                        task);
  g_free (directory_path);
}

static void
gbmp_python_venv_application_addin_add_python_venv_data_new (GObject      *source,
                                                             GAsyncResult *result,
                                                             gpointer      user_data)
{
  IdeTask *task = NULL;
  GbmpPythonVenvApplicationAddin *addin = NULL;
  GbmpPythonVenvVenvData *data = NULL;
  const gchar *data_path = NULL;

  GError *error = NULL;

  task = IDE_TASK (user_data);
  data = gbmp_python_venv_venv_data_new_finish (source, result, &error);
  if (error != NULL)
    {
      ide_task_return_error(task, error);
      return;
    }

  // Add data list

  data_path = gbmp_python_venv_venv_data_get_path (data);
  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (ide_task_get_source_object (task));
  g_hash_table_insert (addin->table_path_data, g_strdup (data_path), data);

  // Gather the result and set it back to settings.
  _update_settings (addin);

  // Emit notify and python-venv-added
  g_signal_emit (addin, sig_python_venv_added, 0, data);
  g_object_notify_by_pspec (G_OBJECT(addin), props[PROP_PYTHON_VENV_DATAS]);


  ide_task_return_boolean (task, TRUE);
}

gboolean
gbmp_python_venv_application_addin_add_python_venv_finish (GbmpPythonVenvApplicationAddin  *addin,
                                                           GAsyncResult                    *result,
                                                           GError                         **error)
{
  IdeTask *task = NULL;

  task = IDE_TASK (result);

  return ide_task_propagate_boolean (task, error);
}

void
gbmp_python_venv_application_addin_make_python_venv_async (GbmpPythonVenvApplicationAddin *addin,
                                                           GFile                          *directory,
                                                           GCancellable                   *cancel,
                                                           GAsyncReadyCallback             callback,
                                                           gpointer                        user_data)
{
  IdeTask *task = NULL;
  gchar *directory_path = NULL;
  gboolean directory_exists = FALSE;
  GFileType directory_type = G_FILE_TYPE_UNKNOWN;

  task = ide_task_new (addin, cancel, callback, user_data);
  if (ide_task_return_error_if_cancelled (task))
    {
      return;
    }

  // Check directory and get path.
  directory_path = g_file_get_path (directory);
  directory_exists = g_file_query_exists (directory, cancel);
  if (! directory_exists)
    {
      // Try to make directory.
      g_file_make_directory_async (directory,
                                   G_PRIORITY_DEFAULT,
                                   cancel,
                                   gbmp_python_venv_application_addin_make_python_venv_mkdir,
                                   task);
      g_free (directory_path);
      return;
    }

  directory_type = g_file_query_file_type (directory,
                                           G_FILE_QUERY_INFO_NONE,
                                           cancel);
  if (directory_type != G_FILE_TYPE_DIRECTORY)
    {
      ide_task_return_new_error (task,
                                 G_IO_ERROR,
                                 G_IO_ERROR_NOT_DIRECTORY,
                                 "Not a directory: %s",
                                 directory_path);
      g_free (directory_path);
      return;
    }

  gbmp_python_venv_application_addin_make_python_venv_setup (addin, directory_path, task);
}


static void
gbmp_python_venv_application_addin_make_python_venv_mkdir (GObject      *source,
                                                           GAsyncResult *result,
                                                           gpointer      user_data)
{
  GFile *file = NULL;
  IdeTask *task = NULL;
  GbmpPythonVenvApplicationAddin *addin = NULL;
  gchar *directory_path = NULL;

  GError *error = NULL;

  file = G_FILE (source);
  task = IDE_TASK (user_data);

  g_file_make_directory_finish (file, result, &error);
  if (error != NULL)
    {
      ide_task_return_error (task, error);
      return;
    }

  directory_path = g_file_get_path (file);
  gbmp_python_venv_application_addin_make_python_venv_setup (addin, directory_path, task);
}

static void
gbmp_python_venv_application_addin_make_python_venv_setup (GbmpPythonVenvApplicationAddin *addin,
                                                           gchar                          *path,
                                                           IdeTask                        *task)
{
  GCancellable *cancel = NULL;
  IdeSubprocessLauncher *launcher = NULL;
  IdeSubprocess *subprocess = NULL;
  ClosureSetupDone *closure = NULL;
  GError *error = NULL;

  cancel = ide_task_get_cancellable (task);

  launcher = ide_subprocess_launcher_new (G_SUBPROCESS_FLAGS_NONE);
  ide_subprocess_launcher_set_run_on_host (launcher, TRUE);

  // Explicit use python 3 - some system may use python 2 as `python`

  ide_subprocess_launcher_push_argv(launcher, "python3");
  ide_subprocess_launcher_push_argv(launcher, "-m");
  ide_subprocess_launcher_push_argv(launcher, "venv");
  ide_subprocess_launcher_push_argv(launcher, path);
  subprocess = ide_subprocess_launcher_spawn(launcher, cancel, &error);
  g_object_unref (launcher);

  closure = g_new (ClosureSetupDone, 1);
  closure->task = task;
  closure->path = path;
  ide_subprocess_wait_check_async (subprocess,
                                   cancel,
                                   gbmp_python_venv_application_addin_make_python_venv_setup_done,
                                   closure);
  g_object_unref (subprocess);
}

static void
gbmp_python_venv_application_addin_make_python_venv_setup_done (GObject      *source,
                                                                GAsyncResult *result,
                                                                gpointer      user_data)
{
  IdeSubprocess *subprocess = NULL;
  ClosureSetupDone *closure = (ClosureSetupDone *)user_data;
  GError *error = NULL;

  subprocess = IDE_SUBPROCESS (source);

  ide_subprocess_wait_check_finish (subprocess, result, &error);
  if (error != NULL)
    {
      ide_task_return_error (closure->task, error);
      return;
    }

  gbmp_python_venv_venv_data_new_async (closure->path,
                                        ide_task_get_cancellable (closure->task),
                                        gbmp_python_venv_application_addin_make_python_venv_data_new,
                                        closure->task);
  g_free (closure->path);
  g_free (closure);
}


static void
gbmp_python_venv_application_addin_make_python_venv_data_new (GObject      *source,
                                                              GAsyncResult *result,
                                                              gpointer      user_data)
{
  IdeTask *task = NULL;
  GbmpPythonVenvApplicationAddin *addin = NULL;
  GbmpPythonVenvVenvData *data = NULL;
  const gchar *data_path = NULL;
  GError *error = NULL;

  task = IDE_TASK (user_data);

  data = gbmp_python_venv_venv_data_new_finish (source, result, &error);
  if (error != NULL)
    {
      ide_task_return_error (task, error);
      return;
    }

  // Add data list

  data_path = gbmp_python_venv_venv_data_get_path (data);
  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (ide_task_get_source_object (task));
  g_hash_table_insert (addin->table_path_data, g_strdup (data_path), data);

  // Gather the result and set it back to settings.
  _update_settings (addin);

  // Emit notify and python-venv-added
  g_signal_emit (addin, sig_python_venv_added, 0, data);
  g_object_notify_by_pspec (G_OBJECT(addin), props[PROP_PYTHON_VENV_DATAS]);


  ide_task_return_boolean (task, TRUE);
}


gboolean
gbmp_python_venv_application_addin_make_python_venv_finish (GbmpPythonVenvApplicationAddin  *addin,
                                                            GAsyncResult                    *result,
                                                            GError                         **error)
{
  IdeTask *task = NULL;

  task = IDE_TASK (result);

  return ide_task_propagate_boolean (task, error);
}



void
gbmp_python_venv_application_addin_remove_python_venv (GbmpPythonVenvApplicationAddin *addin,
                                                       GbmpPythonVenvVenvData         *data)
{
  // Remove virtual environment - Just forget about it.
  const gchar *data_path = NULL;
  GbmpPythonVenvVenvData *data_actual = NULL;

  data_path = gbmp_python_venv_venv_data_get_path (data);

  data_actual = g_hash_table_lookup (addin->table_path_data, data_path);

  if (data != data_actual)
    {
      g_warning ("Requested data and actual data is different.");
      return;
    }

  g_hash_table_remove (addin->table_path_data, data_path);
  _update_settings (addin);
  g_object_notify_by_pspec (G_OBJECT(addin), props[PROP_PYTHON_VENV_DATAS]);
}

void
gbmp_python_venv_application_addin_purge_python_venv_async (GbmpPythonVenvApplicationAddin *addin,
                                                            GbmpPythonVenvVenvData         *data,
                                                            GCancellable                   *cancellable,
                                                            GAsyncReadyCallback             callback,
                                                            gpointer                        user_data)
{
  // Purge virutal environment - Delete its directory as well.
  IdeTask *task = NULL;

  const gchar *data_path = NULL;
  GbmpPythonVenvVenvData *data_actual = NULL;
  IdeDirectoryReaper *reaper = NULL;
  GFile *file = NULL;
  ClosureReap *closure = NULL;

  task = ide_task_new (addin, cancellable, callback, user_data);

  data_path = gbmp_python_venv_venv_data_get_path (data);

  data_actual = g_hash_table_lookup (addin->table_path_data, data_path);

  if (data != data_actual)
    {
      ide_task_return_new_error (task,
                                 G_IO_ERROR,
                                 G_IO_ERROR_UNKNOWN,
                                 "Requested data and actual data is different.");
      return;
    }

  reaper = ide_directory_reaper_new ();
  file = g_file_new_for_path (data_path);

  ide_directory_reaper_add_directory (reaper, file, 0);
  ide_directory_reaper_add_file (reaper, file, 0);

  closure = g_new(ClosureReap, 1);
  closure->task = task;
  closure->data = data;

  ide_directory_reaper_execute_async (reaper,
                                      cancellable,
                                      gbmp_python_venv_application_addin_purge_python_venv_reap,
                                      closure);

  g_object_unref (file);
  g_object_unref (reaper);
}

static void
gbmp_python_venv_application_addin_purge_python_venv_reap (GObject      *source,
                                                           GAsyncResult *result,
                                                           gpointer      user_data)
{
  IdeDirectoryReaper *reaper = NULL;
  ClosureReap *closure = NULL;
  GbmpPythonVenvApplicationAddin *addin = NULL;
  const gchar *path = NULL;

  GError *error = NULL;

  reaper = IDE_DIRECTORY_REAPER (source);
  closure = (ClosureReap *)user_data;
  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (ide_task_get_source_object (closure->task));
  path = gbmp_python_venv_venv_data_get_path (closure->data);

  ide_directory_reaper_execute_finish (reaper, result, &error);
  if (error != NULL)
    {
      ide_task_return_error (closure->task, error);
      g_free (closure);
      return;
    }

  g_hash_table_remove (addin->table_path_data, path);
  _update_settings (addin);
  g_object_notify_by_pspec (G_OBJECT(addin), props[PROP_PYTHON_VENV_DATAS]);

  ide_task_return_boolean (closure->task, true);
  g_free (closure);
}


gboolean
gbmp_python_venv_application_addin_purge_python_venv_finish (GbmpPythonVenvApplicationAddin  *addin,
                                                             GAsyncResult                    *result,
                                                             GError                         **error)
{
  IdeTask *task = NULL;

  task = IDE_TASK(result);

  return ide_task_propagate_boolean (task, error);
}


/////// GTypeInstance

G_DEFINE_FINAL_TYPE_WITH_CODE (GbmpPythonVenvApplicationAddin,
                               gbmp_python_venv_application_addin,
                               G_TYPE_OBJECT,
                               {
                                 G_IMPLEMENT_INTERFACE (
                                   IDE_TYPE_APPLICATION_ADDIN,
                                   _ide_application_addin_iface_init
                                 );
                               })

static void
gbmp_python_venv_application_addin_class_init (GbmpPythonVenvApplicationAddinClass *c)
{
  GObjectClass *c_g_object = G_OBJECT_CLASS (c);

  // Signals

  sig_python_venv_added = g_signal_new ("python-venv-added",
                                        GBMP_TYPE_PYTHON_VENV_APPLICATION_ADDIN,
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL, NULL,
                                        NULL,
                                        G_TYPE_NONE,
                                        1,
                                        GBMP_TYPE_PYTHON_VENV_VENV_DATA);

  sig_python_venv_removed = g_signal_new ("python-venv-removed",
                                          GBMP_TYPE_PYTHON_VENV_APPLICATION_ADDIN,
                                          G_SIGNAL_RUN_LAST,
                                          0,
                                          NULL, NULL,
                                          NULL,
                                          G_TYPE_NONE,
                                          1,
                                          GBMP_TYPE_PYTHON_VENV_VENV_DATA);

  // GObject

  c_g_object->finalize = _g_object_finalize;
  c_g_object->get_property = _g_object_get_property;
  c_g_object->set_property = _g_object_set_property;

  props[PROP_PYTHON_VENV_DATAS] = g_param_spec_boxed ("python-venv-datas",
                                                      "Python Venv Data List",
                                                      "Python Venv Data List",
                                                      G_TYPE_PTR_ARRAY,
                                                      G_PARAM_STATIC_STRINGS |
                                                      G_PARAM_READABLE);
}

static void
gbmp_python_venv_application_addin_init (GbmpPythonVenvApplicationAddin *inst)
{
}

static void
_ide_application_addin_iface_init (IdeApplicationAddinInterface *iface)
{
  iface->load = _ide_application_addin_load;
}


//////// GObject

static void
_g_object_finalize (GObject *object)
{
  GObjectClass *parent_class = NULL;
  GbmpPythonVenvApplicationAddin *addin = NULL;

  parent_class = G_OBJECT_CLASS (gbmp_python_venv_application_addin_parent_class);
  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (object);

  if (instance == addin)
    {
      instance = NULL;
    }
  else
    {
      g_warning ("Stored instance is not equal to this!");
    }

  g_clear_object (&addin->settings);
  g_clear_pointer (&addin->table_path_data, g_hash_table_unref);

  parent_class->finalize(object);
}

static void
_g_object_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GbmpPythonVenvApplicationAddin *addin = NULL;

  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (addin);

  switch (prop_id)
    {
    case PROP_PYTHON_VENV_DATAS:
      g_value_take_boxed (value,
                          g_hash_table_get_values_as_ptr_array (addin->table_path_data));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
    }
}

static void
_g_object_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}


//////// IdeApplicationAddin

static void
_ide_application_addin_load (IdeApplicationAddin *self,
                             IdeApplication      *application)
{
  GbmpPythonVenvApplicationAddin *addin = NULL;

  gchar **list_venvs = NULL;

  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (self);

  addin->settings = g_settings_new ("org.gnome.builder.PythonVenv");
  addin->table_path_data = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  g_object_unref);

  g_signal_connect (addin->settings,
                    "changed::python-venvs",
                    (GCallback) _settings_python_venvs_changed,
                    addin);

  list_venvs = g_settings_get_strv (addin->settings, "python-venvs");
  _setup_python_venvs (addin,
                       (const gchar * const *)list_venvs);

  g_strfreev (list_venvs);

  if (instance != NULL)
    {
      g_critical ("Instance is not null! CRITICAL");
    }
  instance = g_object_ref(addin);
}



//////// Privates



static void
_settings_python_venvs_changed (GSettings *settings,
                                gchar     *key,
                                gpointer   user_data)
{
  GbmpPythonVenvApplicationAddin *addin = NULL;

  gchar **python_venvs = NULL;

  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (user_data);

  if (! addin->python_venvs_is_setting)
    {
      python_venvs = g_settings_get_strv (addin->settings, "python-venvs");
      _setup_python_venvs (addin,
                           (const gchar * const *)python_venvs);

    }
}

static void
_setup_python_venvs (GbmpPythonVenvApplicationAddin *addin,
                     const gchar * const            *python_venvs)
{
  GHashTable *table_remove_create = NULL;
  GList *prev_paths = NULL;
  GList *prev_paths_iter = NULL;

  const gchar * const *python_venvs_iter = NULL;
  ClosureVenvsDataNew *closure = NULL;

  // Take what to remove and what to create.
  table_remove_create = g_hash_table_new (g_str_hash, g_str_equal);

  prev_paths = g_hash_table_get_keys (addin->table_path_data);
  prev_paths_iter = prev_paths;

  while (prev_paths_iter != NULL)
    {
      g_hash_table_insert (table_remove_create,
                           prev_paths_iter->data,
                           (gpointer) -1);

      prev_paths_iter = prev_paths_iter->next;
    }
  g_list_free (prev_paths);

  if (python_venvs != NULL)
    {
      python_venvs_iter = python_venvs;
      while (*python_venvs_iter != NULL)
        {
          gintptr retain_or_create = 0;
          retain_or_create = (gintptr) g_hash_table_lookup (table_remove_create,
                                                            (gpointer) *python_venvs_iter);
          retain_or_create ++;
          g_hash_table_insert (table_remove_create,
                               (gpointer) *python_venvs_iter,
                               (gpointer) retain_or_create);

          python_venvs_iter++;
        }
    }

  // Perform remove and create.
  closure = g_new(ClosureVenvsDataNew, 1);
  closure->addin = g_object_ref (addin);
  closure->num_left_running = 0;

  g_hash_table_foreach (table_remove_create,
                        _setup_python_venvs_remove_create,
                        closure);

  if (closure->num_left_running == 0)
    {
      _setup_python_venvs_data_new_then (closure);
    }

  g_hash_table_unref (table_remove_create);
}

static void
_setup_python_venvs_remove_create (gpointer key,
                                   gpointer value,
                                   gpointer user_data)
{
  ClosureVenvsDataNew *closure = NULL;
  const gchar *path = NULL;
  gintptr remove_or_create = 0;

  GbmpPythonVenvVenvData *data = NULL;

  closure = (ClosureVenvsDataNew *) user_data;
  path = (const gchar *) key;
  remove_or_create = (gintptr) value;

  if (remove_or_create == -1)
    {
      data = GBMP_PYTHON_VENV_VENV_DATA (g_hash_table_lookup (closure->addin->table_path_data, path));
      g_signal_emit (closure->addin, sig_python_venv_removed, 0, data);
      g_hash_table_remove (closure->addin->table_path_data, path);
    }
  else if (remove_or_create == 1)
    {
      closure->num_left_running ++;
      gbmp_python_venv_venv_data_new_async (path,
                                            NULL, // Cancellable
                                            _setup_python_venvs_data_new,
                                            closure);
    }
}


static void
_setup_python_venvs_data_new (GObject      *source,
                              GAsyncResult *result,
                              gpointer      user_data)
{
  ClosureVenvsDataNew *closure = NULL;
  GbmpPythonVenvVenvData *data = NULL;

  gchar *path = NULL;

  GError *error = NULL;

  closure = (ClosureVenvsDataNew *)user_data;
  data = gbmp_python_venv_venv_data_new_finish (source, result, &error);
  if (error != NULL)
    {
      g_warning ("Virtual Env Init failed: %s", error->message);
    }
  else
    {
      path = g_strdup (gbmp_python_venv_venv_data_get_path (data));
      g_hash_table_insert (closure->addin->table_path_data, path, data);
      g_signal_emit (closure->addin, sig_python_venv_added, 0, data);
    }

  closure->num_left_running --;

  if (closure->num_left_running == 0)
    {
      _setup_python_venvs_data_new_then (closure);
    }
}

static void
_setup_python_venvs_data_new_then (ClosureVenvsDataNew *closure)
{
  _update_settings (closure->addin);
  g_object_notify_by_pspec (G_OBJECT(closure->addin),
                            G_PARAM_SPEC (props[PROP_PYTHON_VENV_DATAS]));

  g_object_unref (closure->addin);
  g_free (closure);
}

static void
_update_settings (GbmpPythonVenvApplicationAddin *addin)
{
  GPtrArray *paths = NULL;
  // Gather the result and set it back to settings.

  paths = g_hash_table_get_keys_as_ptr_array (addin->table_path_data);
  g_ptr_array_sort (paths, (GCompareFunc) strcmp);

  if (! g_ptr_array_is_null_terminated (paths))
    {
      g_ptr_array_add (paths, NULL);
    }

  addin->python_venvs_is_setting = TRUE;

  g_settings_set_strv (addin->settings,
                       "python-venvs",
                       (const gchar * const *) paths->pdata);

  addin->python_venvs_is_setting = FALSE;
  g_ptr_array_unref (paths);
}








