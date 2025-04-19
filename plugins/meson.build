plugins_deps = [
  dependency('libadwaita-1'),
  dependency('libpanel-1'),
  dependency('libpeas-2'),
  dependency('libdex-1'),
  dependency('gnome-builder-48.0'),
  dependency('libsoup-3.0'),
  dependency('json-glib-1.0'),
]

plugindir = get_option('plugindir')
schemadir = get_option('schemadir')

if plugindir == ''
  plugindir = join_paths(get_option('prefix'), get_option('datadir'), 'gnome-builder', 'plugins')
endif

if schemadir == ''
  schemadir = join_paths(get_option('prefix'), get_option('datadir'), 'glib-2.0', 'schemas')
endif

message('Plugins will be installed at : ', plugindir)
message('GSettings schemas will be installed at : ', schemadir)

subdir('python-venv')

gnome.post_install(glib_compile_schemas:true)