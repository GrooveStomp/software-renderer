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

# Debugging

## Windows

    devenv build\main.exe

- Explicitly open the source file where the breakpoint will go.
- Use F5, F10 and F11 to manipulate the program counter to step around the sourcecode.
*Note*: Without SDL, just using a legitimate WinMain, it is possible to simply hit F11 to reach the first instruction in the program.
In this case we're using SDL, so this won't work.

## Linux

# To-Do

- Verify building still works on Linux.