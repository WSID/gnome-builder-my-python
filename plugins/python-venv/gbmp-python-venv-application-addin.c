#include "gbmp-python-venv-application-addin.h"

struct _GbmpPythonVenvApplicationAddin
{
  GObject parent_instance;

  GSettings *settings;
};

static void
_ide_application_addin_iface_init (IdeApplicationAddinInterface *iface);


//////// GObject

static void
_g_object_finalize (GObject *object);


//////// IdeApplicationAddin

static void
_ide_application_addin_load (IdeApplicationAddin *addin,
                             IdeApplication      *application);


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

  g_clear_object (&addin->settings);

  parent_class->finalize(object);
}


//////// IdeApplicationAddin

static void
_ide_application_addin_load (IdeApplicationAddin *self,
                             IdeApplication      *application)
{
  GbmpPythonVenvApplicationAddin *addin = NULL;

  addin = GBMP_PYTHON_VENV_APPLICATION_ADDIN (self);

  addin->settings = g_settings_new ("org.gnome.builder.PythonVenv");
}

