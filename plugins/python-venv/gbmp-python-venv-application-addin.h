#pragma once

#include <libide-gui.h>

#include "gbmp-python-venv-data.h"

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

void
gbmp_python_venv_application_addin_add_python_venv_async (GbmpPythonVenvApplicationAddin *addin,
                                                          GFile                          *directory,
                                                          GCancellable                   *cancel,
                                                          GAsyncReadyCallback             callback,
                                                          gpointer                        user_data);

gboolean
gbmp_python_venv_application_addin_add_python_venv_finish (GbmpPythonVenvApplicationAddin  *addin,
                                                           GAsyncResult                    *result,
                                                           GError                         **error);

void
gbmp_python_venv_application_addin_make_python_venv_async (GbmpPythonVenvApplicationAddin *addin,
                                                           GFile                          *directory,
                                                           GCancellable                   *cancel,
                                                           GAsyncReadyCallback             callback,
                                                           gpointer                        user_data);

gboolean
gbmp_python_venv_application_addin_make_python_venv_finish (GbmpPythonVenvApplicationAddin  *addin,
                                                            GAsyncResult                    *result,
                                                            GError                         **error);

void
gbmp_python_venv_application_addin_remove_python_venv (GbmpPythonVenvApplicationAddin *addin,
                                                       GbmpPythonVenvVenvData         *data);

void
gbmp_python_venv_application_addin_purge_python_venv_async (GbmpPythonVenvApplicationAddin *addin,
                                                            GbmpPythonVenvVenvData         *data,
                                                            GCancellable                   *cancellable,
                                                            GAsyncReadyCallback             callback,
                                                            gpointer                        user_data);

gboolean
gbmp_python_venv_application_addin_purge_python_venv_finish (GbmpPythonVenvApplicationAddin  *addin,
                                                             GAsyncResult                    *result,
                                                             GError                         **error);

void
gbmp_python_venv_application_addin_remove_python_venv_path (GbmpPythonVenvApplicationAddin *addin,
                                                            const gchar                    *path);

void
gbmp_python_venv_application_addin_purge_python_venv_path_async (GbmpPythonVenvApplicationAddin *addin,
                                                                 const gchar                    *path,
                                                                 GCancellable                   *cancellable,
                                                                 GAsyncReadyCallback             callback,
                                                                 gpointer                        user_data);

gboolean
gbmp_python_venv_application_addin_purge_python_venv_path_finish (GbmpPythonVenvApplicationAddin  *addin,
                                                                  GAsyncResult                    *result,
                                                                  GError                         **error);

G_END_DECLS

