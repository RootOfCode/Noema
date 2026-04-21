# Noema

`Noema` is an interpreted programming language with English syntax, a C runtime, and a focus on metaprogramming, symbolic AI, rule-based systems, and interactive development.

The C source code is written in Brazilian Portuguese. The language, stdlib, examples, and plugins all use English syntax.

## What Noema is

Noema is not just a DSL. It is a complete language:

- named and anonymous functions
- lexical scope, closures, and recursion
- mutable variables and constants
- `if`, `when`, `unless`, `while`, `for`, `each`, `return`, `break`, `continue`
- lists, maps, strings with interpolation, and trailing commas
- macros via code expansion
- `eval(...)` and `try_eval(...)`
- embedded stdlib loaded automatically
- interactive REPL and shells/REPLs built inside the language itself
- plugins written in Noema with FFI for C libraries

In practice, Noema works for general programming as much as for language prototyping, symbolic agents, classic chatbots, rule engines, expert systems, and interactive environments.

## Language direction

The core priorities of Noema are:

- metaprogramming without s-expressions
- small, hackable C runtime
- a stdlib oriented toward the language, not ad-hoc demos
- ergonomics for writing symbolic AI and interactive tools
- extensibility via user plugins

## Quick example

```noema
macro unless_do(cond, body) {
    return syntax if not $cond { $body }
}

macro make_metric(name, expr) {
    return syntax_function(syntax_text(name), ["values"], syntax {
        return $expr
    })
}

var data = [3, 5, 8, 13]

expand make_metric(total_metric, sum(values))
expand make_metric(mean_metric, sum(values) / len(values))
expand unless_do(len(data) == 0, {
    IO.puts("dataset ready")
})

IO.show("total", total_metric(data))
IO.show("mean", mean_metric(data))
```

## Main features

- block-structured infix syntax
- friendly aliases: `fn`, `var`, `use`, `when`, `unless`, `each`, `elif`
- optional semicolons when the statement already ends on the line
- numbers accept decimal and hexadecimal with `0x` prefix
- AST macros: `macro name(args) { return syntax ... }`
- `expand name(...)` accepts full forms including blocks
- `syntax ...`, `$name`, and `$(expr)` for assembling code
- syntax introspection and construction with `syntax_kind`, `syntax_parts`, `syntax_function`, and related
- `eval(...)` for dynamic code generation
- `try_eval(...)` for protected runtime evaluation
- FFI with `libffi` and dynamic library loading
- stdlib with `Core`, `IO`, `FS`, `Path`, `REPL`, `Shell`, `Math`, `Random`, `Strings`, `List`, `Maps`, `Fn`, `Assert`, `Patterns`, `Dialogue`, and `Rules`

## Build

On Linux and other POSIX environments:

```bash
make
```

This produces:

- `obj/*.o` and `obj/*.d`
- `build/noema`

On Windows:

```bat
make
```

or, if the GNU Make executable still has the default MinGW name:

```bat
mingw32-make
```

There is also the shortcut:

```bat
build.bat
```

This produces:

- `obj\*.obj` or `obj\*.o`
- `build\noema.exe`

The `Makefile` works on both Linux and Windows with MinGW.
`build.bat` remains available as a shortcut and fallback for Windows.

If `libffi` is not in a default compiler path on Windows:

```bat
set LIBFFI_INCLUDE=C:\path\to\libffi\include
set LIBFFI_LIB=C:\path\to\libffi\lib
build.bat
```

## Installation

On POSIX environments:

```bash
./install.sh
```

Defaults:

- compiler at `/usr/local/bin/noema`
- stdlib at `/usr/local/share/noema/stdlib`

With a custom prefix:

```bash
./install.sh /opt/noema
PREFIX=/tmp/noema ./install.sh
```

The installer does not create a global plugin directory. Plugins are user extensions and stay outside the system installation.

On Windows, there is also an installer:

```bat
install.bat
```

Defaults:

- `%LOCALAPPDATA%\Programs\Noema\bin\noema.exe`
- `%LOCALAPPDATA%\Programs\Noema\share\noema\stdlib`

With a custom prefix:

```bat
install.bat C:\Noema
```

`install.bat` detects `mingw32-make` or `make`, compiles the project, and installs only the compiler and stdlib.

## Running

```bash
./build/noema
./build/noema --repl
./build/noema --repl examples/friendly.noe
./build/noema examples/eliza.noe
./build/noema examples/mycin.noe
./build/noema examples/shrdlu.noe
```

On Windows:

```bat
build\noema.exe
build\noema.exe --repl
build\noema.exe examples\friendly.noe
```

Without arguments, Noema opens the interactive REPL. With `--repl` or `-i`, it can load a script and continue in the same global environment.

## REPL

The REPL supports multiline blocks. When input is incomplete, the prompt changes to `....>`.

Main commands:

- `:help` or `help`
- `:help syntax`
- `:help stdlib`
- `:help ffi`
- `:exit`, `:quit`, or `quit`
- `:reset` to discard a pending multiline block

## Standard library

`stdlib/prelude.noe` is loaded automatically. This makes the following namespaces available from the start:

- `Core`
- `IO`
- `FS`
- `Path`
- `REPL`
- `Shell`
- `Math`
- `Random`
- `Strings`
- `List`
- `Maps`
- `Fn`
- `Assert`
- `Patterns`
- `Dialogue`
- `Rules`

Example:

```noema
var xs = [1, 2, 3, 4]
var doubled = List.map(xs, fn (value, index) => value * 2 + index)

IO.show("title", Strings.title_case("noema standard library"))
IO.show("sum", sum(doubled))
IO.show("clamped", Math.clamp(sum(doubled), 0, 99))
```

Full reference by file and module:

- [stdlib.md](stdlib.md)

## Symbolic AI and rule systems

Noema has host-side and stdlib support for this kind of program.

- `Patterns` handles text normalization, wildcards, and templates
- `Dialogue` handles sessions, memory, topic, and conversational history
- `Rules` handles facts, inference, ranking, and certainty factors

This allows writing programs in the style of:

- ELIZA
- PARRY
- ALICE/AIML-style bots
- MYCIN-like expert systems
- SHRDLU-like blocks worlds

These examples are included in the repository as programs, not as part of the stdlib.

## Plugins and FFI

Plugins are written in Noema but kept separate from the stdlib:

```noema
plugin Ncurses {
    library "libncurses.so.6", "libncursesw.so.6", "libncurses.so"
    bind initscr() -> pointer
    bind endwin() -> int
    bind mvaddstr(int, int, string) -> int
}
```

After importing the file:

```noema
import "../plugins/ncurses.noe"
Ncurses.initscr()
Ncurses.endwin()
```

Supported FFI types:

- `number`
- `int`
- `bool`
- `string`
- `pointer`
- `void`

In general:

- Linux uses `.so`
- Windows uses `.dll`

The runtime tries libraries in the order declared in `library`.
If the local function name already matches the native symbol, `as "..."` is optional.
Use `as` only when you need to expose a different alias.
For structs, buffers, and output parameters, use the memory helpers:
`memory_alloc`, `memory_free`, `memory_fill`, `memory_get_u8`, `memory_get_i32`,
`memory_get_u32`, `memory_set_u8`, `memory_set_i32`, and `memory_set_u32`.

## Documentation

- [language.md](language.md): overview of the language
- [syntax.md](syntax.md): syntax, declarations, macros, and key builtins
- [repl.md](repl.md): CLI, native REPL, and REPLs written in Noema
- [ffi.md](ffi.md): plugins, native libraries, and FFI
- [stdlib.md](stdlib.md): stdlib reference by file and namespace
- [files.md](files.md): repository guide and description of each important file

## Project structure

- `src/`: lexer, preprocessor, parser, runtime, builtins, and FFI
- `stdlib/`: standard library distributed with the language
- `exemplos/`: example programs, including classic AIs
- `plugins/`: user plugins and FFI demos
- `docs-ptbr/`: documentation in Portuguese
- `docs-en/`: this documentation in English
- `obj/`: objects produced by the build
- `build/`: binaries produced by the build

For a per-file description, see [files.md](files.md).

## Included examples

- [../exemplos/meta.noe](../exemplos/meta.noe): AST macros, `syntax`, and introspection
- [../exemplos/eliza.noe](../exemplos/eliza.noe): rule-based therapist in the ELIZA style
- [../exemplos/parry.noe](../exemplos/parry.noe): paranoid interviewee in the PARRY style
- [../exemplos/alice.noe](../exemplos/alice.noe): category-based bot in the ALICE style
- [../exemplos/mycin.noe](../exemplos/mycin.noe): expert system with certainty factors
- [../exemplos/shrdlu.noe](../exemplos/shrdlu.noe): blocks world inspired by SHRDLU
- [../exemplos/boids.noe](../exemplos/boids.noe): symbolic/behavioral flocking
- [../exemplos/ncurses_demo.noe](../exemplos/ncurses_demo.noe): `ncurses` plugin demo
- [../exemplos/sdl2_demo.noe](../exemplos/sdl2_demo.noe): window and render loop with `SDL2` plugin

`ncurses_demo.noe` is Linux/Unix only.

## Core runtime files

- `src/main.c`: CLI and REPL
- `src/lexico.c`: tokenization
- `src/preprocessador.c`: `import` and source assembly
- `src/parser.c`: recursive parser, `macro`, `expand`, and `syntax`
- `src/runtime.c`: values, scope, execution, macro expansion, and stdlib resolution
- `src/builtins.c`: language builtins and host-side stdlib foundation
- `src/ffi.c`: plugins and FFI calls
- `src/noema.h`: central types and interfaces

## Notes

- The stdlib is loaded automatically.
- Plugins are not loaded automatically.
- `NOEMA_STDLIB` can be used to override the stdlib directory.
- The runtime is compatible with Linux and Windows.
