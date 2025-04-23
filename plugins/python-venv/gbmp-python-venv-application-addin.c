#include "gbmp-python-venv-application-addin.h"

#include "gbmp-python-venv-data.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "gbmp-python-venv-application-addin"

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

//////// Public functions

GbmpPythonVenvApplicationAddin *
gbmp_python_venv_application_addin_get_instance (void)
{
  return instance;
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
  GPtrArray *paths = NULL;
  // Gather the result and set it back to settings.

  paths = g_hash_table_get_keys_as_ptr_array (closure->addin->table_path_data);
  g_ptr_array_sort (paths, (GCompareFunc) strcmp);

  if (! g_ptr_array_is_null_terminated (paths))
    {
      g_ptr_array_add (paths, NULL);
    }

  closure->addin->python_venvs_is_setting = TRUE;

  g_settings_set_strv (closure->addin->settings,
                       "python-venvs",
                       (const gchar * const *) paths->pdata);

  closure->addin->python_venvs_is_setting = FALSE;

  g_ptr_array_unref (paths);
  g_object_unref (closure->addin);
  g_free (closure);
}

