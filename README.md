Please Note!
============
GitAhead is no longer under active development. Low-level maintenance and bug
fix releases will be made as necessary for the foreseeable future, but no new
features or major changes are planned at this time. Please consider continuing
development in a *rebranded* fork for anything other than trivial changes.


GitAhead - Understand Your History
==================================

GitAhead is a graphical Git client designed to help you understand
and manage your source code history. It's available as a [pre-built
binary for Windows, Linux, and macOS](https://gitahead.github.io/gitahead.com/), or can be built from source by
following the directions below.

Build Environment
-----------------

* C++11 compiler
  * Windows - MSVC >= 2017 recommended
  * Linux - GCC >= 6.2 recommended
  * macOS - Xcode >= 10.1 recommended
* CMake >= 3.3.1
* Ninja (optional)

Dependencies
------------

External dependencies can be satisfied by system libraries or installed
separately. Included dependencies are submodules of this repository. Some
submodules are optional or may also be satisfied by system libraries.

**External Dependencies**

* Qt (required >= 5.9)

**Included Dependencies**

* libgit2 (required)
* cmark (required)
* git (only needed for the credential helpers)
* libssh2 (needed by `libgit2` for SSH support)
* openssl (needed by `libssh2` and `libgit2` on some platforms)

Note that building `OpenSSL` on Windows requires `Perl` and `NASM`.

How to Build
------------

**Initialize Submodules**

    git submodule init
    git submodule update

**Build OpenSSL**

    # Start from root of gitahead repo.
    cd dep/openssl/openssl

Win:

    perl Configure VC-WIN64A
    nmake

Mac:

    ./Configure darwin64-x86_64-cc no-shared
    make

Linux:

    ./config -fPIC
    make

**Configure Build**

    # Start from root of gitahead repo.
    mkdir -p build/release
    cd build/release
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ../..

If you have Qt installed in a non-standard location, you may have to
specify the path to Qt by passing `-DCMAKE_PREFIX_PATH=<path-to-qt>`
where `<path-to-qt>` points to the Qt install directory that contains
`bin`, `lib`, etc.

**Build**

    ninja

License
-------

GitAhead is licensed under the MIT license. See LICENSE.md for details.
