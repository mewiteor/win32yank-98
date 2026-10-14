# win32yank-98

A clipboard tool for Windows.
This is a pure C rewrite of [win32yank](https://github.com/equalsraf/win32yank) with no CRT dependency (`/nodefaultlib`).
It requires Windows 98 or above.

Get the clipboard.

    win32yank -o

Set the clipboard

    echo "hello brave new world!" | win32yank -i

## Building

Requires **Visual Studio 2017** or later (Visual Studio Build Tools 2017+ is sufficient).
The build script uses `vswhere.exe` to locate the MSVC toolchain.

run `make.bat`
