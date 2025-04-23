#pragma once

#include <libide-gui.h>

G_BEGIN_DECLS

#define GBMP_TYPE_PYTHON_VENV_APPLICATION_ADDIN (gbmp_python_venv_application_addin_get_type())

G_DECLARE_FINAL_TYPE (GbmpPythonVenvApplicationAddin,
                      gbmp_python_venv_application_addin,
                      GBMP, PYTHON_VENV_APPLICATION_ADDIN,
                      GObject)

GbmpPythonVenvApplicationAddin *
gbmp_python_venv_application_addin_get_instance (void);

GPtrArray *
gbmp_python_venv_application_addin_get_venv_datas (GbmpPythonVenvApplicationAddin *addin);

G_END_DECLS
