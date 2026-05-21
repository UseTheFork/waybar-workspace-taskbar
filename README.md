# Waybar Workspace Taskbar

**Seems to be fairly stable but still considered alpha. Changes are likely.**

Workspace taskbar for window managers that shows only open windows from the current workspace. This differs from included wlr taskbar which shows all open windows on all workspaces. This takes advantage of the waybar cffi module [Waybar CFFI](https://github.com/Alexays/Waybar/wiki/Module:-CFFI).

This was developed mostly due to non traditional window manager layouts like scrolling, fullscreen tabbed, or monocle where you can easily lose track of your open windows. This gives you the ability to easily visualize your open windows. Tabs will try to be sorted so they reflect the left and right cycling positions of your windows. All floating windows will be sorted to the end in the order they are received.

## Suported Window Managers

Window managers need to have the abilty for us to get the required data to display the windows. This would include an ipc or command line interface to listen to workspace and window events and fetch the required data. Then some way to be able to send commands to focus, close and toggle float on windows.

Currently supported window managers.

- Sway
- Hyprland
- Niri

## Building and Installation

You must have some required packages installed. Look to your package manager for the correct installation of each.

- make
- meson
- gtk+-3.0 version >=3.22.0
- json-glib-1.0

Then clone or dowload the repo and you can simply run

```
make
```

I'll leave the installation location up to you. You only need the `.so` file in the build folder.

```
cp build/waybar-workspace-taskbar.so /your/desired/location/
```

More than likely you would move it to ~/.config/waybar/cffi/

## Configuration

You need to setup the `cffi/module-name` in your `config.json` file for waybar. Reference [Waybar CFFI Example](https://github.com/Alexays/Waybar/wiki/Module:-CFFI#examples).

Minimal example:

```json
"modules-left": [
    "cffi/waybar-workspace-taskbar"
],
"cffi/waybar-workspace-taskbar": {
    "module_path": "~/.config/waybar/cffi/waybar-workspace-taskbar.so",
    "window-manager": "sway"
}
```

Note: You may need to use absolute paths depending on your environment.

### Configuration Options

**IMPORTANT: Configuration inside this module does not support comments in your json**

Even though your using jsonc waybar doesn't parse out the comments when passing them to the cffi module. So as of right now keep your comments outside the cffi module config.

Options use `kebab-case` in key names for spaces to follow most of the waybar convention. It should be noted that `module_path` is a waybar specific option that uses `snake_case`.

| Option                    | Required | Default  | Allowed                                  | Description                                                                                                                                           |
| ------------------------- | -------- | -------- | ---------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| module_path               | yes      | NULL     | string                                   | The path to this modules `.so` file.                                                                                                                  |
| window-manager            | yes      | NULL     | "sway", "hyprland", "niri"               | The window manager you are currently using.                                                                                                           |
| output                    | no       | NULL     | true, false                              | The monitor you want to bind to. If no output is set then it defaults to showing the focused workspace.                                               |
| show-icon                 | no       | true     | true, false                              | Whether or not you want to show the application icon.                                                                                                 |
| show-title                | no       | false    | true, false                              | Whether or not you want to show the window title.                                                                                                     |
| show-tooltip              | no       | false    | true, false                              | Whether or not you want to show a tooltip when hovering on the tab.                                                                                   |
| text-align                | no       | "center" | "left", "right", "center"                | Position of the text and icon in the tab.                                                                                                             |
| max-tabs                  | no       | -1       | int                                      | Max amount of tabs to show -1 for unlimited. (See css configuration below to show the overflow indicator)                                             |
| title-max-chars           | no       | -1       | int > 3                                  | Max amount of characters to show in the title, -1 for unlimited. (Note: this includes elipsis so if you set to 10, 3 of those characters will be ...) |
| icon-size                 | no       | 16       | int > 0                                  | The size of the app icon to be displayed. (Note: icon aspect ratio is 1:1 so default is 16x16)                                                        |
| show-navigation-btns      | no       | 0        | 0 = never, 1 = overlfow only, 2 = always | Whether or not to show navigation buttons. Navigation buttons switch focus prev and next.                                                             |
| navigation-btn-pos        | no       | 0        | 0 = staggered, 1 = before, 2 = after     | Position of the navigation buttons. Before tabs, after tabs, or staggered one on each side.                                                           |
| navigation-btn-prev-label | no       | "<"      | string                                   | The label for the navigation prev button.                                                                                                             |
| navigation-btn-next-label | no       | ">"      | string                                   | The label for the navigation next button.                                                                                                             |

### Configuring Styles

A couple of css classes will be applied so you can style things accordingly.

- `.taskbar`
- `.taskbar.overflow-start`
- `.taskbar.overflow-end`
- `.taskbar.empty`
- `.taskbar.single`
- `.tabs`
- `.tab`
- `.tab.focused`
- `.tab.floating`
- `.tab.urgent`
- `.navigation-btn-prev`
- `.navigation-btn-next`

Simple example might be like the following:

```css
.taskbar.overflow-start .tabs {
  border-left: 10px solid red;
}

.taskbar.overflow-end .tabs {
  border-right: 10px solid red;
}

.tab {
  color: white;
  padding: 5px 10px;
}

.tab.focused {
  background: blue;
}

.tab.floating {
  color: green;
}

.tab.urgent {
  color: red;
}
```

The css parent/child structure is as follows:

```jsx
<Taskbar>
  <NavigationBtnPrev />
  <Tabs>
    <Tab />
    <Tab />
    <Tab />
  </Tabs>
  <NavigationBtnNext />
</Taskbar>
```

## Usage

This is fairly simple.

- Left Click: Focus Window
- Right Click: Close Window
- Middle Click: Toggle Float

## Known issues

Hyprland

- Hyprctl doesn't expose urgent status when fetching window information.
- Hyprland doesn't send an event when toggling floating windows. So window status and sorting will not be updated until you change focus.

## License

Waybar Workspace Taskbar is licensed under the MIT license. See [LICENSE](https://github.com/stevekanger/waybar-workspace-taskbar/blob/main/LICENSE) for more information.
