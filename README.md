# Setup

## Windows

- Create a `cmd.exe` shortcut on the desktop.
- Set its target to: `C:\Windows\System32\cmd.exe /k "%PathToProject%\env\shell.bat"`
- Launch the custom command prompt.

## Linux

- Install KDevelop4
- In KDevelop:

  - [Project] -> [Open/Import Project]
    - Browse to path for software-renderer.
    - Specify 'software-renderer' as the project name.

  - [Run] -> [Configure Launches]
    - Add New.
    - Select the executable from the /build directory.
    - Set the working directory appropriately.

Then:

    cd $PROJECT_PATH
    source env/shell

# Building

## Windows

    build

## Linux

    build

# Running

## Windows

    run

## Linux

    run

# Debugging

## Windows

    debugger

- Explicitly open the source file where the breakpoint will go.
- Use F5, F10 and F11 to manipulate the program counter to step around the sourcecode.
*Note*: Without SDL, just using a legitimate WinMain, it is possible to simply hit F11 to reach the first instruction in the program.
In this case we're using SDL, so this won't work.

## Linux

    debugger

In setup we've already added the source directory to the project, so setting breakpoints and all should be pretty typical of using IDEs.
