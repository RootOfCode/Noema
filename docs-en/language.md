# The Noema Language

Noema is an interpreted programming language with a C runtime, designed for:

- metaprogramming without s-expressions
- symbolic AI and expert systems
- rule-based systems
- interactive tools and custom REPLs
- extensibility via FFI

The C runtime is written in Brazilian Portuguese. The language itself, the standard library, examples, and plugins all use English syntax.

## Language model

Noema combines:

- block-structured infix syntax with optional semicolons
- a preprocessor that handles `import` and module resolution
- AST macros via `macro`, `expand`, and `syntax`
- a dynamic runtime with lexical scoping and closures
- an embedded standard library that loads automatically
- an interactive REPL with multiline block support
- user-written plugins that call native C libraries via libffi

## Core features

- named and anonymous functions (`function`, `fn`)
- lexical scope with closures and recursion
- mutable variables (`var`, `let`) and constants (`const`)
- lists, maps, and strings with interpolation (`"#{expr}"`)
- trailing commas in lists and maps
- `if`, `elif`, `else`, `unless`, `when`, `while`, `for`, `each`
- `return`, `break`, `continue`
- decimal and hexadecimal numbers (`0x`)
- `eval(...)` for dynamic code generation
- `try_eval(...)` for protected evaluation that never throws
- macros that operate directly on the AST
- FFI via libffi and dynamic library loading

## Automatic stdlib loading

On startup, `stdlib/prelude.noe` is loaded automatically. The following namespaces are immediately available in the global environment without any manual `import`:

`Core`, `IO`, `FS`, `Path`, `REPL`, `Shell`, `Math`, `Random`, `Strings`, `List`, `Maps`, `Fn`, `Assert`, `Patterns`, `Dialogue`, `Rules`

The aggregate namespace `Std` is also available, exposing everything as `Std.Core`, `Std.IO`, etc.

## Use cases

Noema works well for:

- embedded languages and language prototyping
- rule-based chatbots in the style of ELIZA, PARRY, and ALICE
- inference engines in the style of MYCIN
- symbolic text pipelines
- console tools and custom shells
- REPLs built inside the language itself
- simple behavioral simulations

## Related reading

- [syntax.md](./syntax.md) — full syntax reference
- [repl.md](./repl.md) — CLI and REPL
- [ffi.md](./ffi.md) — plugins and FFI
- [stdlib.md](./stdlib.md) — stdlib reference
- [files.md](./files.md) — repository guide
