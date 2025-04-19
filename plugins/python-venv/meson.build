python_venv_plugin_dir = plugindir / 'python-venv'

python_venv_plugin_srcs = files(
    ['python-venv-plugin.c',
     'gbmp-python-venv-tweaks-addin.c'])

python_venv_plugin_incs = include_directories('.')

python_venv_res_srcs = gnome.compile_resources('python-venv-resources', 'python-venv.gresources.xml', c_name: 'gbmp_python_venv')

python_venv_plugin = shared_module('python-venv', python_venv_plugin_srcs, python_venv_res_srcs,
    dependencies:        plugins_deps,
    include_directories: [python_venv_plugin_incs],
    install:             true,
    install_dir:         python_venv_plugin_dir)

gnome.compile_schemas(
    build_by_default: true,
    depend_files: 'org.gnome.builder.PythonVenv.gschema.xml')

install_data('python-venv.plugin', install_dir: python_venv_plugin_dir)
install_data(['org.gnome.builder.PythonVenv.gschema.xml'], install_dir: schemadir)