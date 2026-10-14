# win32yank-98

Windows 剪贴板工具。
本项目是 [win32yank](https://github.com/equalsraf/win32yank) 的纯 C 语言重写版本，不依赖 CRT，支持 Windows 98 及以上系统。

获取剪贴板内容：

    win32yank -o

设置剪贴板内容：

    echo "hello brave new world!" | win32yank -i

## 编译

需要 **Visual Studio 2017** 或更高版本（Visual Studio Build Tools 2017 就够）。
构建脚本使用 `vswhere.exe` 来定位 MSVC 工具链。

直接运行 `make.bat` 即可
