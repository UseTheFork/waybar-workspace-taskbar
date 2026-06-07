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
  'src/utils/cmd.c',
  'src/utils/common.c',
  'src/utils/socket.c',
  'src/services/app_icons.c',
  'src/services/window_manager/data.c',
  'src/services/window_manager/events.c',
  'src/services/window_manager/subscriptions.c',
  'src/services/window_manager/window_manager.c',
  'src/widgets/navigation_btn.c',
  'src/widgets/tab.c',
  'src/widgets/taskbar.c',
  'src/window_managers/niri.c',
  'src/window_managers/hyprland.c',
  'src/window_managers/sway.c',
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
