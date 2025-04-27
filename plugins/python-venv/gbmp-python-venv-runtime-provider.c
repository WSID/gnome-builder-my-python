#include "gbmp-python-venv-runtime-provider.h"

#include <libdex.h>

#include "gbmp-python-venv-application-addin.h"
#include "gbmp-python-venv-runtime.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "gbmp-python-venv-runtime-provider"

struct _GbmpPythonVenvRuntimeProvider {
  IdeRuntimeProvider parent_instance;
  GbmpPythonVenvApplicationAddin *app_addin;
  GHashTable                     *table_data_runtime;
};

G_DEFINE_FINAL_TYPE (GbmpPythonVenvRuntimeProvider,
                     gbmp_python_venv_runtime_provider,
                     IDE_TYPE_RUNTIME_PROVIDER)

//////// GObject

static void
_g_object_finalize (GObject *self);

//////// IdeRuntimeProvider

static DexFuture *
_ide_runtime_provider_load (IdeRuntimeProvider *self);

//////// Private functions

static void
_addin_on_python_venv_added (GbmpPythonVenvApplicationAddin *addin,
                             GbmpPythonVenvVenvData         *data,
                             gpointer                        user_data);

static void
_addin_on_python_venv_removed (GbmpPythonVenvApplicationAddin *addin,
                               GbmpPythonVenvVenvData         *data,
                               gpointer                        user_data);

//////// GTypeInstance

static void
gbmp_python_venv_runtime_provider_class_init (GbmpPythonVenvRuntimeProviderClass *c)
{
  GObjectClass *c_g_object = NULL;
  IdeRuntimeProviderClass *c_ide_runtime_provider = NULL;

  c_g_object = G_OBJECT_CLASS (c);

  c_g_object->finalize = _g_object_finalize;

  c_ide_runtime_provider = IDE_RUNTIME_PROVIDER_CLASS (c);

  c_ide_runtime_provider->load = _ide_runtime_provider_load;
}

static void
gbmp_python_venv_runtime_provider_init (GbmpPythonVenvRuntimeProvider *self)
{
  self->app_addin = g_object_ref (gbmp_python_venv_application_addin_get_instance ());
  self->table_data_runtime = g_hash_table_new_full (g_direct_hash,
                                                    g_direct_equal,
                                                    g_object_unref,
                                                    g_object_unref);

  g_signal_connect (self->app_addin,
                    "python-venv-added",
                    G_CALLBACK(_addin_on_python_venv_added), self);

  g_signal_connect (self->app_addin,
                    "python-venv-removed",
                    G_CALLBACK(_addin_on_python_venv_removed), self);
}

//////// GObject

static void
_g_object_finalize (GObject *self) {
  GbmpPythonVenvRuntimeProvider *provider = NULL;

  provider = GBMP_PYTHON_VENV_RUNTIME_PROVIDER (self);

  g_clear_pointer (&provider->table_data_runtime, g_hash_table_unref);
  g_clear_object (&provider->app_addin);
}

//////// IdeRuntimeProvider

static DexFuture *
_ide_runtime_provider_load (IdeRuntimeProvider *self)
{
  GbmpPythonVenvRuntimeProvider *provider = NULL;
  GPtrArray *list_datas = NULL;

  provider = GBMP_PYTHON_VENV_RUNTIME_PROVIDER (self);
  list_datas = gbmp_python_venv_application_addin_get_venv_datas (provider->app_addin);

  for (guint i = 0; i < list_datas->len; i++)
    {
      GbmpPythonVenvVenvData *data = GBMP_PYTHON_VENV_VENV_DATA (list_datas->pdata[i]);
      IdeRuntime *runtime = gbmp_python_venv_runtime_new (data);

      g_hash_table_insert (provider->table_data_runtime,
                           g_object_ref(data),
                           runtime);

      ide_runtime_provider_add (self, runtime);
    }

  g_ptr_array_unref (list_datas);

  return dex_future_new_true ();
}


//////// Private functions

static void
_addin_on_python_venv_added (GbmpPythonVenvApplicationAddin *addin,
                             GbmpPythonVenvVenvData         *data,
                             gpointer                        user_data)
{
  GbmpPythonVenvRuntimeProvider *provider = NULL;
  IdeRuntime *runtime = NULL;

  provider = GBMP_PYTHON_VENV_RUNTIME_PROVIDER (user_data);
  runtime = gbmp_python_venv_runtime_new (data);

  g_hash_table_insert (provider->table_data_runtime,
                       g_object_ref(data),
                       runtime);

  ide_runtime_provider_add (IDE_RUNTIME_PROVIDER(provider), runtime);
}

static void
_addin_on_python_venv_removed (GbmpPythonVenvApplicationAddin *addin,
                               GbmpPythonVenvVenvData         *data,
                               gpointer                        user_data)
{
  GbmpPythonVenvRuntimeProvider *provider = NULL;
  IdeRuntime *runtime = NULL;

  provider = GBMP_PYTHON_VENV_RUNTIME_PROVIDER (user_data);
  runtime = g_hash_table_lookup (provider->table_data_runtime, data);

  if (runtime != NULL)
    {
      ide_runtime_provider_remove (IDE_RUNTIME_PROVIDER (provider), runtime);
      g_hash_table_remove (provider->table_data_runtime, data);
    }
}
