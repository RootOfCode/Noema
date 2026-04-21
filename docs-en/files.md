# Repository File Guide

This file describes the most important files in the repository and the role each one plays.

## Project root

| File | Purpose |
|------|---------|
| `README.md` | Navigation hub — links to docs-ptbr and docs-en |
| `Makefile` | Primary build for Linux/POSIX and Windows with MinGW |
| `build.bat` | Alternative build script for Windows |
| `install.sh` | POSIX installer |
| `install.bat` | Windows installer |
| `LICENSE` | Project license |

### Makefile

Produces:
- object files in `obj/`
- header dependency files in `obj/*.d`
- final binary at `build/noema` (Linux) or `build/noema.exe` (Windows)

Works on both Linux and Windows with MinGW. On Windows, call either `make` or `mingw32-make`.

### install.sh

Default install locations:
- `/usr/local/bin/noema`
- `/usr/local/share/noema/stdlib`

Custom prefix:
```bash
./install.sh /opt/noema
PREFIX=/tmp/noema ./install.sh
```

Does not install plugins — plugins are user extensions.

### install.bat

Default install locations:
- `%LOCALAPPDATA%\Programs\Noema\bin\noema.exe`
- `%LOCALAPPDATA%\Programs\Noema\share\noema\stdlib`

Custom prefix:
```bat
install.bat C:\Noema
```

---

## C runtime — `src/`

### `src/noema.h`

Central runtime header. Defines:

- tokens (`TipoToken`)
- AST node types (`TipoNo`, `No`, `Programa`)
- value types (`TipoValor`, `Valor`, `Lista`, `Mapa`)
- scope and environment (`Ambiente`, `EntradaAmbiente`)
- interpreter (`Interpretador`)
- FFI structures (`LigacaoPlugin`, `TipoFfi`)
- execution signal (`SinalExecucao`, `ResultadoExecucao`)
- signatures of all public functions

### `src/main.c`

Executable entry point. Responsibilities:

- CLI argument parsing (`--repl`, `-i`, file paths)
- interpreter and stdlib initialization
- native REPL loop with multiline block support
- commands `:help`, `:help syntax`, `:help stdlib`, `:help ffi`, `:quit`, `:reset`
- automatic detection of incomplete blocks (changes prompt to `....>`)

### `src/lexico.c`

Lexer and tokenizer. Produces tokens for:

- identifiers and keywords
- numbers (decimal and hexadecimal `0x`)
- strings (with `#{}` interpolation and raw backtick strings)
- operators and punctuation

### `src/preprocessador.c`

Noema preprocessor. Responsibilities:

- resolves `import "path.noe"` directives
- prevents duplicate imports within the same session
- assembles the final source text before parsing

### `src/parser.c`

Recursive descent parser. Converts processed source into an AST (`Programa` of `No` nodes). Handles:

- all language declarations and expressions
- `macro`, `expand`, and `syntax`
- placeholders `$name` and `$(expr)`

### `src/runtime.c`

Main runtime. Responsibilities:

- AST execution
- macro expansion
- environment (scope) creation and management
- values and coercions
- automatic stdlib directory discovery
- import resolution and loading

### `src/builtins.c`

Language builtins and host-side bridge for the stdlib. Registers:

- type conversion and inspection
- collection operations (`len`, `get`, `push`, `pop`, `map`, `filter`, etc.)
- string operations
- built-in math
- file and path operations
- shell
- host-side primitives for `Patterns`, `Dialogue`, and `Rules`
- syntax builtins (`syntax_kind`, `syntax_parts`, `syntax_function`, etc.)

### `src/ffi.c`

Plugin and FFI layer. Responsibilities:

- loading dynamic libraries with fallbacks (`dlopen` / `LoadLibrary`)
- symbol resolution
- call setup via libffi
- integration with memory helpers for structs and buffers

---

## Standard library — `stdlib/`

All files live in `stdlib/`.

| File | Namespace | Contents |
|------|-----------|----------|
| `prelude.noe` | `Std` | Entry point — loads all modules |
| `core.noe` | `Core` | Types, conversions, helpers, and FFI memory |
| `io.noe` | `IO` | Console, prompts, and input |
| `fs.noe` | `FS` | Files and directories |
| `path.noe` | `Path` | Path operations |
| `repl.noe` | `REPL` | Building REPLs |
| `shell.noe` | `Shell` | Building shells |
| `math.noe` | `Math` | Math constants and functions |
| `random.noe` | `Random` | Randomness and sampling |
| `strings.noe` | `Strings` | String transformations |
| `lists.noe` | `List` | List operations |
| `maps.noe` | `Maps` | Map operations |
| `functional.noe` | `Fn` | Functional helpers |
| `assert.noe` | `Assert` | Assertions |
| `patterns.noe` | `Patterns` | Symbolic matching and templates |
| `dialogue.noe` | `Dialogue` | Conversational sessions |
| `rules.noe` | `Rules` | Facts, inference, and certainty |

---

## Plugins — `plugins/`

Plugins are user extensions. They are not part of the stdlib and are not loaded automatically.

| File | Description |
|------|-------------|
| `plugins/ncurses.noe` | ncurses TUI (Linux/Unix only) |
| `plugins/sdl2.noe` | Window, renderer, and event polling via SDL2 |

---

## Examples — `examples/`

> The directory is named `exemplos/` in the repository (Portuguese). Adjust paths accordingly.

| File | Description |
|------|-------------|
| `exemplos/friendly.noe` | Small demo of syntax and general stdlib |
| `exemplos/meta.noe` | Macros, code expansion, and `eval` |
| `exemplos/eliza.noe` | Rule-based therapist in the ELIZA style |
| `exemplos/parry.noe` | Paranoid interviewee in the PARRY style |
| `exemplos/alice.noe` | Category-based bot in the ALICE/AIML style |
| `exemplos/mycin.noe` | Expert system with certainty factors |
| `exemplos/shrdlu.noe` | Blocks world inspired by SHRDLU |
| `exemplos/boids.noe` | Symbolic/behavioral flocking simulation |
| `exemplos/ncurses_demo.noe` | ncurses plugin demo (Linux/Unix) |
| `exemplos/sdl2_demo.noe` | Window and render loop via SDL2 |

---

## Documentation — `docs-ptbr/` and `docs-en/`

| File | Contents |
|------|----------|
| `README.md` | Documentation hub for this language |
| `language.md` | Language overview and design goals |
| `syntax.md` | Full syntax, macros, and builtins |
| `repl.md` | CLI, native REPL, and REPLs in Noema |
| `ffi.md` | Plugins, native libraries, and FFI |
| `stdlib.md` | Complete stdlib reference |
| `files.md` | This guide |

---

## Build output directories

| Directory | Contents |
|-----------|----------|
| `obj/` | Object files (`.o`, `.d`) |
| `build/` | Final binary (`noema` or `noema.exe`) |
