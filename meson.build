project(
    'waybar-workspace-taskbar', 'c',
    version: '0.1.0',
    license: 'MIT',
)

inc = include_directories('src')

src_files = files(
  'src/main.c',
  'src/core/app.c',
  'src/core/config.c',
  'src/core/services.c',
  'src/core/utils.c',
  'src/core/window_manager_data.c',
  'src/services/app_icons.c',
  'src/services/window_manager_events.c',
  'src/services/window_manager_spec.c',
  'src/widgets/navigation_btn.c',
  'src/widgets/tab.c',
  'src/widgets/taskbar.c',
  'src/window_managers/niri.c',
  'src/window_managers/hyprland.c',
  'src/window_managers/sway.c',
  'src/window_managers/common.c'
) 

shared_library('waybar-workspace-taskbar',
    src_files,
    include_directories: inc,
    dependencies: [
        dependency('gtk+-3.0', version : ['>=3.22.0']),
        dependency('json-glib-1.0')
    ],
    name_prefix: ''
)
