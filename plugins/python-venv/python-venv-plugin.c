/**
 * python-venv-plugin.c
 */

#define G_LOG_DOMAIN "python-venv"

#include <gmodule.h>
#include <libpeas.h>

#include "gbmp-python-venv-application-addin.h"
#include "gbmp-python-venv-tweaks-addin.h"

G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module) {
  peas_object_module_register_extension_type (module,
                                              IDE_TYPE_APPLICATION_ADDIN,
                                              GBMP_TYPE_PYTHON_VENV_APPLICATION_ADDIN);

  peas_object_module_register_extension_type (module,
                                              IDE_TYPE_TWEAKS_ADDIN,
                                              GBMP_TYPE_PYTHON_VENV_TWEAKS_ADDIN);

}
