# Setup

## Windows

- Create a `cmd.exe` shortcut on the desktop.
- Set its target to: `C:\Windows\System32\cmd.exe /k "%PathToProject%\env\env.bat"`
- Launch the custom command prompt.

## Linux

    cd $PROJECT_PATH

# Building

## Windows

    build

## Linux

    env/build.sh

# To-Do

- Properly support `devenv` with build for Windows.
- Verify building still works on Linux.