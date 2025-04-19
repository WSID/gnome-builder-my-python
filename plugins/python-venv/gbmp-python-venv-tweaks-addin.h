#pragma once

#include <libide-tweaks.h>

G_BEGIN_DECLS

#define GBMP_TYPE_PYTHON_VENV_TWEAKS_ADDIN (gbmp_python_venv_tweaks_addin_get_type())

G_DECLARE_FINAL_TYPE (GbmpPythonVenvTweaksAddin,
                      gbmp_python_venv_tweaks_addin,
                      GBMP, PYTHON_VENV_TWEAKS_ADDIN,
                      IdeTweaksAddin)

G_END_DECLS
