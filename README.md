# Dusk

## Documentation

[The documentation for dusk can be found here](https://kian_shepherd.gitlab.io/Dusk/)

## Description

A simple programming language built from scratch.

The build scripts use CMake and Ninja to find dependencies and build the project.

Uses flex as a tokenizer.

Uses bison as a parser generator.

The AST is generated with C++.

LLVM is used as a backend to generate machine specific object files which are then compiled with a c compiler into an executable.

## Dependencies

This project requires `cmake`, `flex`, `bison`, `llvm` and a `c`/`c++` compiler.

The examples for building this project also include `ninja`, but that is an optional dependency.

## Installation

Build LLVM 

```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build
cd build
cmake -G Ninja -DLLVM_ENABLE_ASSERTIONS=On -DCMAKE_BUILD_TYPE=Release ../llvm
ninja
ninja install
```

### Ubuntu:

Ensure you have flex and bison and build tools 

```
sudo apt install cmake ninja-build flex libfl-dev bison libbison-dev
```

### Mac OS

Ensure you have flex and bison and build tools

```
brew install bison flex cmake ninja
```

An old version of bison is the default version so you must relink a newer version to build Dusk

```
brew unlink bison
brew link bison --force
echo 'export PATH="/opt/homebrew/opt/bison/bin:$PATH"'>> ~/.zshrc
```

## Development

To generate a `compile_commands.json` file for use with `ccls` run

```
./ccls_file_gen.sh
```

## Building

To build the project run.

```
cmake -G Ninja -S . -B CMake
ninja -C CMake
```

To install the binary run.

```
ninja -C CMake install
```

## Usage

Once it is installed you can run `dusk x`, where x is the name of the file you wish to compile.

For example

```
dusk test.ds
```

Would compile a file named `test.ds` into an executable.

## Current Functionality

- Base types
    - Atomic types
        - i1: boolean
        - i8: char   (equivalent to C char)
        - i64: int   (equivalent to C long)
        - i128: long (equivalent to C long long)
        - f64: float (equivalent to C double)
    - Array Types
        - boolean array
        - strings
        - int array
        - float array
        - string arrays
- Functions
    - `main` as entry point
        - accepts and handles command line arguments
    - Can be self recursive
        - Optimized so tail call omptimization can be preformed to remove recursive calls where possible
    - Can pass arguments to and from
- stdlib
    - Casting functions for atomic types
    - `copy` functions to allocate on the heap
    - `del` function to delete heap allocated objects
    - `print` function, takes C style format strings
- Static checking
    - Ensure `main` function is present
    - Checks for non mutable vairables being mutated


## Planned Functionality

- Fix block variable scope
- Structs
- Improve command line options for compilation
    - Optional optimizations
    - link C libraries
    - multiple source files
    - help message
    - output filename
    - specify which C compiler is used to generate the executable
    - compile to object file not just executable
- `extern` keyword to load `C` functions from libraries
-  stdlib improvements
    - vectors
    - maps
- Better compile time failure error messages
    - show line that error occured on
- Tests
- Documentation webpage
- Refcounting for heap allocated objects to free them automatically when they go out of scope

## License

This project is licensed under GNU GPLv3, as this project depends on `bison`.

