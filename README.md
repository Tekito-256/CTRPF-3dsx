# CTRPluginFramework

**CTRPluginFramework** is a framework to build plugins for Nintendo 3DS games in *3GX* format.

## Building

The latest version of devkitARM and libctru is needed to build the framework. You can run `make dist-bin` in the `Library` folder to generate a *tar* file containing the library and include files.

## Installing

Place the contents of the provided *tar* file inside a folder named `libctrpf` in your devkitPro installation directory. (The resulting folder should contain the `lib` and `include` subfolders.)

## License

Copyright (c) The Pixellizer Group

This software is licensed under the following terms:

```
Permission to use, copy, modify, and/or distribute this software for any purpose is hereby granted as long as the following three conditions are met.

1) Access to any work in binary form that is based or uses part or the totality of this software must not be restricted to individuals that have been charged a fee or any other kind of compensation.

2) You are exempt of the condition 1 of this license as long as the source code of the work that is based or uses part or the totality of this software is provided along with its binary form.

3) Any copy, modification or distribution of the source code of this software must retain the same license text.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
```