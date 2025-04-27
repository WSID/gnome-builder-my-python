#include "gbmp-python-venv-runtime.h"

struct _GbmpPythonVenvRuntime {
  IdeRuntime parent_instance;
  GbmpPythonVenvVenvData *data;
};

G_DEFINE_FINAL_TYPE (GbmpPythonVenvRuntime,
                     gbmp_python_venv_runtime,
                     IDE_TYPE_RUNTIME)

//////// GObject

enum {
  PROP_0,
  PROP_DATA,
  PROP_MAX,
};

GParamSpec *pspecs[PROP_MAX] = { NULL };

static void
_g_object_get_property (GObject    *self,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec);
static void
_g_object_set_property (GObject      *self,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec);

static void
_g_object_finalize (GObject *self);

//////// IdeRuntime

static gboolean
_ide_runtime_contains_program_in_path (IdeRuntime   *self,
                                       const gchar  *program,
                                       GCancellable *cancellable);

static void
_ide_runtime_prepare_to_build (IdeRuntime    *self,
                               IdePipeline   *pipeline,
                               IdeRunContext *run_context);
static void
_ide_runtime_prepare_to_run (IdeRuntime    *self,
                             IdePipeline   *pipeline,
                             IdeRunContext *run_context);


//////// Private functions

static void
prepare_run_context (GbmpPythonVenvRuntime *runtime,
                     IdeRunContext         *run_context);


static gboolean
prepare_run_context_push_context_handle_print (IdeRunContext        *context,
                                               const gchar * const  *argv,
                                               const gchar * const  *env,
                                               const gchar          *cwd,
                                               IdeUnixFDMap         *unix_fd_map,
                                               gpointer              user_data,
                                               GError              **error);

static gboolean
prepare_run_context_push_context_handle (IdeRunContext        *context,
                                         const gchar * const  *argv,
                                         const gchar * const  *env,
                                         const gchar          *cwd,
                                         IdeUnixFDMap         *unix_fd_map,
                                         gpointer              user_data,
                                         GError              **error);


////////////////////////////////////////////////////////////////////////////////

//////// Public constructros

IdeRuntime *
gbmp_python_venv_runtime_new (GbmpPythonVenvVenvData *data)
{
  IdeRuntime *result = NULL;
  const gchar *path = NULL;
  const gchar *prompt = NULL;
  gchar *id = NULL;
  gchar *short_id = NULL;
  gchar *display_name = NULL;

  path = gbmp_python_venv_venv_data_get_path (data);
  prompt = gbmp_python_venv_venv_data_get_prompt (data);

  id = g_strdup_printf ("python-venv:%s", path);
  short_id = g_strdup_printf ("python-venv:%s", prompt);
  display_name = g_strdup_printf ("%s (Python Venv)", prompt);


  result = IDE_RUNTIME (g_object_new (GBMP_TYPE_PYTHON_VENV_RUNTIME,
                                      "data", data,
                                      "id", id,
                                      "short-id", short_id,
                                      "name", prompt,
                                      "display-name", display_name,
                                      "category", "python-venv",
                                      NULL));
  g_free (id);
  g_free (short_id);
  g_free (display_name);

  return result;
}

//////// GTypeInstance

static void
gbmp_python_venv_runtime_class_init(GbmpPythonVenvRuntimeClass *c)
{
  GObjectClass *c_g_object = G_OBJECT_CLASS (c);
  IdeRuntimeClass *c_ide_runtime = IDE_RUNTIME_CLASS (c);

  c_g_object->get_property = _g_object_get_property;
  c_g_object->set_property = _g_object_set_property;
  c_g_object->finalize = _g_object_finalize;

  pspecs[PROP_DATA] = g_param_spec_object ("data",
                                           "data",
                                           "Data of python virtual environment.",
                                           GBMP_TYPE_PYTHON_VENV_VENV_DATA,
                                           G_PARAM_READWRITE |
                                           G_PARAM_CONSTRUCT_ONLY |
                                           G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (c_g_object, PROP_MAX, pspecs);

  c_ide_runtime->contains_program_in_path = _ide_runtime_contains_program_in_path;
  c_ide_runtime->prepare_to_build = _ide_runtime_prepare_to_build;
  c_ide_runtime->prepare_to_run = _ide_runtime_prepare_to_run;
}

static void
gbmp_python_venv_runtime_init (GbmpPythonVenvRuntime *self)
{
}

//////// GObject

static void
_g_object_get_property (GObject    *self,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GbmpPythonVenvRuntime *runtime = GBMP_PYTHON_VENV_RUNTIME (self);

  switch (property_id)
    {
    case PROP_DATA:
      g_value_set_object (value, runtime->data);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
    }
}

static void
_g_object_set_property (GObject      *self,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GbmpPythonVenvRuntime *runtime = GBMP_PYTHON_VENV_RUNTIME (self);

  switch (property_id)
    {
    case PROP_DATA:
      g_set_object (&runtime->data, g_value_get_object(value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
    }
}

static void
_g_object_finalize (GObject *self)
{
  GbmpPythonVenvRuntime *runtime = GBMP_PYTHON_VENV_RUNTIME (self);
  GObjectClass *parent_class = G_OBJECT_CLASS (gbmp_python_venv_runtime_parent_class);

  g_clear_object (&runtime->data);

  parent_class->finalize(self);
}


//////// IdeRuntime

static gboolean
_ide_runtime_contains_program_in_path (IdeRuntime   *self,
                                       const gchar  *program,
                                       GCancellable *cancellable)
{
  GbmpPythonVenvRuntime *runtime = NULL;
  const gchar *venv_path = NULL;
  gchar *venv_program_path = NULL;
  GFile *venv_program_path_file = NULL;

  gboolean exists = FALSE;
  gboolean executable = FALSE;

  gchar *then_program = NULL;
  gboolean has_then_program = FALSE;

  GError *error = NULL;

  runtime = GBMP_PYTHON_VENV_RUNTIME (self);

  // Check venv first. (There might be program installed by pip)
  venv_path = gbmp_python_venv_venv_data_get_path (runtime->data);
  venv_program_path = g_build_path (G_DIR_SEPARATOR_S, venv_path, "bin", program, NULL);
  venv_program_path_file = g_file_new_for_path (venv_program_path);

  exists = g_file_query_exists (venv_program_path_file, cancellable);

  if (exists)
    {
      GFileInfo *info = NULL;
      info = g_file_query_info (venv_program_path_file,
                                      G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE,
                                      G_FILE_QUERY_INFO_NONE,
                                      cancellable,
                                      &error);

      executable = g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE);

      g_object_unref (info);
    }

  if (executable)
    {
      return TRUE;
    }

  g_free (venv_program_path);
  g_object_unref (venv_program_path_file);

  // TODO: Use more appropriate way.
  then_program = g_find_program_in_path (program);
  has_then_program = then_program != NULL;
  g_free (then_program);

  return has_then_program;
}

static void
_ide_runtime_prepare_to_build (IdeRuntime    *self,
                               IdePipeline   *pipeline,
                               IdeRunContext *run_context)
{
  GbmpPythonVenvRuntime *runtime = GBMP_PYTHON_VENV_RUNTIME (self);
  prepare_run_context (runtime, run_context);
}

static void
_ide_runtime_prepare_to_run (IdeRuntime    *self,
                             IdePipeline   *pipeline,
                             IdeRunContext *run_context)
{
  GbmpPythonVenvRuntime *runtime = GBMP_PYTHON_VENV_RUNTIME (self);
  prepare_run_context (runtime, run_context);
}

//////// Private functions

static void
prepare_run_context (GbmpPythonVenvRuntime *runtime,
                     IdeRunContext         *run_context)
{
  const gchar *path = NULL;

  path = gbmp_python_venv_venv_data_get_path (runtime->data);

  ide_run_context_push_host(run_context);
  ide_run_context_add_minimal_environment (run_context);
  ide_run_context_push(run_context,
                       prepare_run_context_push_context_handle,
                       (gpointer)path,
                       NULL);

  // Just Print the results.
  ide_run_context_push(run_context,
                       prepare_run_context_push_context_handle_print,
                       (gpointer)"POST",
                       NULL);

}

static gboolean
prepare_run_context_push_context_handle_print (IdeRunContext        *context,
                                               const gchar * const  *argv,
                                               const gchar * const  *env,
                                               const gchar          *cwd,
                                               IdeUnixFDMap         *unix_fd_map,
                                               gpointer              user_data,
                                               GError              **error)
{
  // Prints just message.

  const gchar *title = (const gchar *) user_data;
  const gchar * const *iter_argv = argv;
  const gchar * const *iter_env = env;

  GError *in_error = NULL;

  g_print ("%s\n", title);

  ide_run_context_merge_unix_fd_map (context, unix_fd_map, &in_error);

  if (in_error != NULL)
    {
      g_propagate_prefixed_error (error, in_error, "Push Context Handle:");
      return false;
    }

  while (*iter_argv != NULL)
    {
      g_print ("  argv : %s\n", *iter_argv);
      iter_argv ++;
    }

  ide_run_context_set_argv (context, argv);

  while (*iter_env != NULL)
    {
      g_print ("  env : %s\n", *iter_env);
      iter_env ++;
    }

  ide_run_context_set_environ (context, env);
  ide_run_context_set_cwd (context, cwd);
  return true;
}

static gboolean
prepare_run_context_push_context_handle (IdeRunContext        *context,
                                         const gchar * const  *argv,
                                         const gchar * const  *env,
                                         const gchar          *cwd,
                                         IdeUnixFDMap         *unix_fd_map,
                                         gpointer              user_data,
                                         GError              **error)
{
  const gchar *path = (const gchar *) user_data;
  const gchar *user_shell = NULL;
  gchar *joined_args = NULL;

  GError *in_error = NULL;

  ide_run_context_merge_unix_fd_map (context, unix_fd_map, &in_error);
  if (in_error != NULL)
    {
      g_propagate_error (error, in_error);
      return false;
    }

  user_shell = ide_get_user_shell ();
  joined_args = g_strjoinv (" ", (gchar **) argv);

  ide_run_context_append_argv (context, user_shell);
  ide_run_context_append_argv (context, "-l");
  ide_run_context_append_argv (context, "-c");
  ide_run_context_append_formatted (context,
                                    "source %s/bin/activate ; %s",
                                    path, joined_args);

  ide_run_context_set_environ (context, env);
  ide_run_context_set_cwd (context, cwd);

  g_free (joined_args);

  return true;
}






