#pragma once

#include <libide-foundry.h>

G_BEGIN_DECLS

#define GBMP_TYPE_PYTHON_VENV_RUNTIME_PROVIDER (gbmp_python_venv_runtime_provider_get_type())

G_DECLARE_FINAL_TYPE (GbmpPythonVenvRuntimeProvider,
                      gbmp_python_venv_runtime_provider,
                      GBMP, PYTHON_VENV_RUNTIME_PROVIDER,
                      IdeRuntimeProvider)

G_END_DECLS
