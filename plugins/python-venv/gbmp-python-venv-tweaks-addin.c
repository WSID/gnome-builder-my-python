#define G_LOG_DOMAIN "gbmp-python-venv-tweaks-addin"

#include "gbmp-python-venv-tweaks-addin.h"

struct _GbmpPythonVenvTweaksAddin {
  IdeTweaksAddin parent_instance;
};

G_DEFINE_FINAL_TYPE (GbmpPythonVenvTweaksAddin,
                     gbmp_python_venv_tweaks_addin,
                     IDE_TYPE_TWEAKS_ADDIN)

//////// GTypeInstance

static void
gbmp_python_venv_tweaks_addin_class_init (GbmpPythonVenvTweaksAddinClass *c)
{
}

static void
gbmp_python_venv_tweaks_addin_init (GbmpPythonVenvTweaksAddin *self)
{
  ide_tweaks_addin_set_resource_paths (IDE_TWEAKS_ADDIN(self),
                                       IDE_STRV_INIT("/plugins/python-venv/tweaks.ui"));
}
