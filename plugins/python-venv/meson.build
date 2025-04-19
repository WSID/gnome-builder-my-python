python_venv_plugin_dir = plugindir / 'python-venv'

python_venv_plugin_srcs = files(
    ['python-venv-plugin.c'])

python_venv_plugin_incs = include_directories('.')

python_venv_plugin = shared_module('python-venv', python_venv_plugin_srcs,
    dependencies:        plugins_deps,
    include_directories: [python_venv_plugin_incs],
    install:             true,
    install_dir:         python_venv_plugin_dir)

install_data('python-venv.plugin', install_dir: python_venv_plugin_dir)