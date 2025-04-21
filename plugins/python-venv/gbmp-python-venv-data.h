#pragma once

#include <glib-2.0/glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GBMP_TYPE_PYTHON_VENV_VENV_DATA (gbmp_python_venv_venv_data_get_type())

G_DECLARE_FINAL_TYPE (GbmpPythonVenvVenvData,
                      gbmp_python_venv_venv_data,
                      GBMP, PYTHON_VENV_VENV_DATA,
                      GObject);


void
gbmp_python_venv_venv_data_new_async (const gchar         *path,
                                      GCancellable        *cancellable,
                                      GAsyncReadyCallback  callback,
                                      gpointer             user_data);

GbmpPythonVenvVenvData *
gbmp_python_venv_venv_data_new_finish (GObject       *source,
                                       GAsyncResult  *res,
                                       GError       **error);


G_END_DECLS
