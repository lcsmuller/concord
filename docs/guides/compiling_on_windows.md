# Compiling on Windows (Detailed instructions)

Concord supports Windows natively using Cygwin and Mingw64. It is preferred 
that you use Cygwin if possible, even if it requires runtime dependencies 
(a variety of DLLs from /bin, namely `cygwin1.dll`). Mingw64 will produce a 
build that runs without any dependencies. However, you should note that
Mingw64's wrapping of the `poll` and `select` functions is inferior compared
to how Cygwin does it, so, Cygwin will yield much better results for
"production-quality bots."

## Cygwin and Dependencies

Concord does not have very many dependencies. When you run the Cygwin 
installer, you will be provided with a long list of packages you can choose 
to install. The following packages are required at a minimum:
- gcc-core
- make
- libcurl-devel

This will cause all of the associated dependencies (like OpenSSL and a few 
others) to be automatically installed. Simply follow through the dialog and
wait for everything to install. **Remember to keep that installer program
handy, you will need it to install any further packages.**

You might also want to install git and vim. Please note that because Cygwin 
stores its files on your Windows machine's filesystem, you can run 
`explorer.exe .` at any time to open a Windows Explorer window in your current 
directory. You can use any IDE you would like to use, but you will still have
to enter `make` to recompile your changes from the Cygwin terminal. If you'd
like to open Visual Studio Code, enter `code .` into the shell, and, assuming
VS Code is in your path, it'll open at the current directory path from which
it was ran. 

It is also possible to use a clang-based compiler, but there is little reason 
to do so. Simply pass `make CC=clang` to your Cygwin shell and Clang will be 
used in lieu of GNU C.

## Microsoft Visual C/C++

As it stands right now, MSVC is not supported at all. At the current instant, 
Concord's Makefiles are for UNIX systems, and does not produce anything when
ran with `nmake`. This will change in the near future.  However, Concord itself
cannot be compiled with MSVC, due to a lack of POSIX compliance on behalf of 
Windows. 

## Other compilers

The following compilers are **not** supported:
- OpenWatcom
- Turbo C/C++ (or Borland C/C++)
- Pelles C
- Digital Mars
- Intel C++
- AMD Optimizing C++ (Clang-based, untested)

Generally speaking, your compiler must implement C99 features. The compilers 
listed above don't support C99 completely, or lack POSIX compatibility where
it matters (POSIX threads, `poll` and `select`, and headers being nonstandard).
