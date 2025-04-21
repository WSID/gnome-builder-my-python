#include "gbmp-python-venv-data.h"

#include <string.h>
#include <libide-io.h>
#include <libide-foundry.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "gbmp-python-venv-data"

struct _GbmpPythonVenvVenvData
{
  GObject parent_instance;

  gchar *path;
  gchar *prompt;
};

//////// GObject

static void
_g_object_finalize (GObject *self);

static void
_g_object_get_property (GObject    *self,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec);
static void
_g_object_set_property (GObject      *self,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec);

enum
{
  PROP_0,
  PROP_PATH,
  PROP_PROMPT,
  N_PROPS
};

static GParamSpec *pspecs[N_PROPS] = {NULL};


//////// GAsyncInitable

static void
_g_async_initable_iface_init (GAsyncInitableIface *iface);

static void
_g_async_initable_init_async (GAsyncInitable      *initable,
                              gint                 io_priority,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data);

static void
_g_async_initable_init_communicate (GObject      *source,
                                    GAsyncResult *result,
                                    gpointer      user_data);

static gboolean
_g_async_initable_init_finish (GAsyncInitable  *initable,
                               GAsyncResult    *result,
                               GError         **error);


//////// Public Functions

void
gbmp_python_venv_venv_data_new_async (const gchar         *path,
                                      GCancellable        *cancellable,
                                      GAsyncReadyCallback  callback,
                                      gpointer             user_data)
{
  g_async_initable_new_async(GBMP_TYPE_PYTHON_VENV_VENV_DATA,
                             G_PRIORITY_DEFAULT,
                             cancellable,
                             callback,
                             user_data,
                             "path", path,
                             NULL);
}

GbmpPythonVenvVenvData *
gbmp_python_venv_venv_data_new_finish (GObject       *source,
                                       GAsyncResult  *res,
                                       GError       **error)
{
  return GBMP_PYTHON_VENV_VENV_DATA (
    g_async_initable_new_finish (G_ASYNC_INITABLE (source), res, error)
  );
}


//////// GTypeInstance

G_DEFINE_FINAL_TYPE_WITH_CODE (
    GbmpPythonVenvVenvData,
    gbmp_python_venv_venv_data,
    G_TYPE_OBJECT,
    {
        G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                               _g_async_initable_iface_init)
    })


static void
gbmp_python_venv_venv_data_class_init (GbmpPythonVenvVenvDataClass *c)
{
  GObjectClass *c_g_object = G_OBJECT_CLASS (c);

  c_g_object->finalize = _g_object_finalize;
  c_g_object->get_property = _g_object_get_property;
  c_g_object->set_property = _g_object_set_property;

  pspecs[PROP_PATH] = g_param_spec_string ("path", "Virtual Env Path",
                                           "Virtual Environment Path",
                                           "",
                                           G_PARAM_READWRITE |
                                           G_PARAM_CONSTRUCT_ONLY |
                                           G_PARAM_STATIC_STRINGS);

  pspecs[PROP_PROMPT] = g_param_spec_string ("prompt", "Virtual Env Prompt",
                                             "Virtual Environment Prompt that shown on shell.",
                                             "",
                                             G_PARAM_READABLE |
                                             G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (c_g_object, N_PROPS, pspecs);
}

static void
gbmp_python_venv_venv_data_init (GbmpPythonVenvVenvData *inst)
{
}

//////// GObject

static void
_g_object_finalize (GObject *object)
{
  GbmpPythonVenvVenvData *self = NULL;
  GObjectClass *parent_class = NULL;

  self = GBMP_PYTHON_VENV_VENV_DATA (object);
  parent_class = G_OBJECT_CLASS(gbmp_python_venv_venv_data_parent_class);

  g_clear_pointer (&self->path, g_free);
  g_clear_pointer (&self->prompt, g_free);

  parent_class->finalize (object);
}


static void
_g_object_get_property (GObject    *self,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GbmpPythonVenvVenvData *data = NULL;
  data = GBMP_PYTHON_VENV_VENV_DATA (self);

  switch (prop_id)
    {
    case PROP_PATH:
      g_value_set_string (value, data->path);
      break;

    case PROP_PROMPT:
      g_value_set_string (value, data->prompt);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
    }
}

static void
_g_object_set_property (GObject      *self,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GbmpPythonVenvVenvData *data = NULL;
  data = GBMP_PYTHON_VENV_VENV_DATA (self);

  switch (prop_id)
    {
    case PROP_PATH:
      g_set_str (&data->path, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
    }
}


//////// GAsyncInitable

static void
_g_async_initable_iface_init (GAsyncInitableIface *iface)
{
  iface->init_async = _g_async_initable_init_async;
  iface->init_finish = _g_async_initable_init_finish;
}

static void
_g_async_initable_init_async (GAsyncInitable      *initable,
                              gint                 io_priority,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  GbmpPythonVenvVenvData *data = NULL;

  const gchar *user_shell = NULL;
  gchar       *user_shell_basename = NULL;
  const gchar *activate_basename = NULL;

  IdeSubprocessLauncher *launcher = NULL;
  IdeSubprocess *subprocess = NULL;

  IdeTask       *task = NULL;
  GError        *error = NULL;

  data = GBMP_PYTHON_VENV_VENV_DATA (initable);
  task = ide_task_new (data, cancellable, callback, user_data);

  if (ide_task_return_error_if_cancelled (task))
    {
      return;
    }

  // 1. Identify user shell.
  g_print ("Identify user shell\n");
  user_shell = ide_get_user_shell ();
  user_shell_basename = g_path_get_basename (user_shell);

  if (strcmp (user_shell_basename, "csh") == 0)
    {
      activate_basename = "activate.csh";
    }
  else if (strcmp (user_shell_basename, "fish") == 0)
    {
      activate_basename = "activate.fish";
    }
  else
    {
      // fallback to activate script, written for sh.
      activate_basename = "activate";
    }

  g_clear_pointer (&user_shell_basename, g_free);

  // 2. Execute command
  g_print ("Execute command\n");

  launcher = ide_subprocess_launcher_new (G_SUBPROCESS_FLAGS_STDOUT_PIPE |
                                          G_SUBPROCESS_FLAGS_STDERR_PIPE);
  ide_subprocess_launcher_set_run_on_host (launcher, TRUE);
  ide_subprocess_launcher_push_argv (launcher, "env");
  ide_subprocess_launcher_push_argv (launcher, "-i");
  ide_subprocess_launcher_push_argv (launcher, user_shell);
  ide_subprocess_launcher_push_argv (launcher, "-c");
  ide_subprocess_launcher_push_argv_format (launcher,
                                            "export PS1=TEST && source %s/bin/%s && env",
                                            data->path,
                                            activate_basename);

  subprocess = ide_subprocess_launcher_spawn (launcher, cancellable, &error);
  g_object_unref (launcher);
  if (error != NULL)
    {
      ide_task_return_error (task, error);
      return;
    }

  ide_subprocess_communicate_utf8_async (subprocess,
                                         NULL,
                                         cancellable,
                                         _g_async_initable_init_communicate,
                                         task);
}


static void
_g_async_initable_init_communicate (GObject      *source,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  IdeSubprocess *subprocess = NULL;
  IdeTask *task = NULL;
  GbmpPythonVenvVenvData *data = NULL;

  gchar *out = NULL;
  gchar *err = NULL;
  gchar **lines = NULL;
  gchar **lines_iter = NULL;

  gboolean bail_out = FALSE;
  GError *error = NULL;

  subprocess = IDE_SUBPROCESS (source);
  task = IDE_TASK(user_data);
  data = GBMP_PYTHON_VENV_VENV_DATA (ide_task_get_source_object (task));

  ide_subprocess_communicate_utf8_finish (subprocess, result, &out, &err, &error);
  if (error != NULL)
    {
      ide_task_return_error (task, error);
      bail_out = TRUE;
    }
  else if (ide_subprocess_get_exit_status (subprocess) != 0)
    {
      ide_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed: %s", err);
      bail_out = TRUE;
    }
  else if (out == NULL)
    {
      ide_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED, "No output: %s", err);
      bail_out = TRUE;
    }

  g_free (err);
  if (bail_out)
    {
      return;
    }

  // 4. Extract results.
  lines = g_strsplit_set (out, "\n\r", 0);
  g_free (out);

  for (lines_iter = lines; *lines_iter != NULL; lines_iter++)
    {
      g_print ("  > %s", *lines_iter);
      if (g_str_has_prefix(*lines_iter, "VIRTUAL_ENV_PROMPT="))
        {
          data->prompt = g_strdup (*lines_iter + 19);
        }
    }

  g_strfreev (lines);

  ide_task_return_boolean (task, TRUE);
}



static gboolean
_g_async_initable_init_finish (GAsyncInitable  *initable,
                               GAsyncResult    *result,
                               GError         **error)
{
  IdeTask *task = NULL;

  task = IDE_TASK (result);

  return ide_task_propagate_boolean (task, error);
}

