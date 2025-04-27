#pragma once

#include <libide-foundry.h>

#include "gbmp-python-venv-data.h"

G_BEGIN_DECLS

#define GBMP_TYPE_PYTHON_VENV_RUNTIME (gbmp_python_venv_runtime_get_type())

G_DECLARE_FINAL_TYPE (GbmpPythonVenvRuntime,
                      gbmp_python_venv_runtime,
                      GBMP, PYTHON_VENV_RUNTIME,
                      IdeRuntime)

IdeRuntime *
gbmp_python_venv_runtime_new (GbmpPythonVenvVenvData *data);

G_END_DECLS


