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

if plugindir == ''
  plugindir = join_paths(get_option('prefix'), get_option('datadir'), 'gnome-builder', 'plugins')
endif

message('Plugins will be installed at : ', plugindir)
