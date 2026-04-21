# REPL and CLI

## Running Noema

Linux/POSIX:

```bash
./build/noema                              # opens the REPL
./build/noema --repl                       # explicit REPL flag
./build/noema --repl examples/friendly.noe # loads script, then stays in REPL
./build/noema examples/eliza.noe           # runs a program
```

Windows:

```bat
build\noema.exe
build\noema.exe --repl
build\noema.exe --repl examples\friendly.noe
build\noema.exe examples\eliza.noe
```

## Execution modes

| Invocation | Behavior |
|------------|----------|
| No arguments | Opens the native REPL |
| `--repl` or `-i` | Opens the native REPL (explicit) |
| `--repl file.noe` | Loads the script and stays in the same global environment |
| `file.noe` | Runs the program and exits |

## Multiline blocks

The REPL automatically detects incomplete blocks. When input is not yet closed, the prompt changes from `noema>` to `....>` and evaluation only happens once the block is complete:

```
noema> function greet(name) {
....>     return "hello #{name}"
....> }
noema> greet("world")
"hello world"
```

## Native REPL commands

| Command | Effect |
|---------|--------|
| `:help` or `help` | General help |
| `:help syntax` | Syntax reference |
| `:help stdlib` | Stdlib reference |
| `:help ffi` | Plugin/FFI reference |
| `:quit`, `:exit`, or `quit` | Exits the REPL |
| `:reset` | Discards a pending multiline block |

## Environment variable

`NOEMA_STDLIB` overrides the stdlib directory:

```bash
NOEMA_STDLIB=/opt/noema/share/noema/stdlib ./build/noema
```

## REPLs written in Noema

`stdlib/repl.noe` allows building custom REPLs inside the language itself using `REPL.start(options)`.

### Fields of `REPL.start(options)`

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `prompt` | string | `"repl> "` | Prompt text |
| `exit` | string | `":exit"` | Exit command |
| `path` | string | `"<repl>"` | Virtual path for error messages |
| `quiet` | bool | `false` | Suppresses the welcome message |
| `banner` | string\|null | `null` | Text shown on startup |
| `on_line` | function\|null | `null` | Callback for each line |
| `on_help` | function\|null | `null` | Callback for `:help` |
| `on_result` | function\|null | `null` | Callback to display results |
| `on_error` | function\|null | `null` | Callback to display errors |

### Default behavior

Without custom callbacks:

- `help` and `:help` show the default help message
- `quit`, `:quit`, and the `exit` value terminate the loop
- Non-null results are printed with `print`
- Errors are printed as `print("error:", msg)`

### Minimal example

```noema
REPL.start({
    prompt: "my-repl> ",
    banner: "Welcome to my REPL",
    exit: "bye",
    on_line: fn (line, state, _) => {
        return {ok: true, value: "echo: #{line}", error: null}
    }
})
```

### Example with full result control

```noema
REPL.start({
    prompt: "calc> ",
    on_line: fn (line, state, _) {
        return try_eval(line)
    },
    on_result: fn (value, state, _) {
        IO.puts("= #{value}")
    },
    on_error: fn (result, state, _) {
        IO.puts("error: #{result.error}")
    }
})
```

## Custom shells

`stdlib/shell.noe` provides `Shell.start(options)` for building simple shells that accept OS commands and custom commands.

Default shell builtins available in examples:

- `pwd` — prints the current directory
- `cd <path>` — changes directory
- `help` — shows help
- `exit` — exits the shell

## Repository examples

The following examples build complete interactive environments:

- `examples/eliza.noe` — rule-based therapist in the ELIZA style
- `examples/parry.noe` — paranoid interviewee in the PARRY style
- `examples/alice.noe` — category-based bot in the ALICE/AIML style
- `examples/mycin.noe` — expert system with certainty factors
- `examples/shrdlu.noe` — blocks world inspired by SHRDLU

## Related reading

- [language.md](./language.md)
- [syntax.md](./syntax.md)
- [ffi.md](./ffi.md)
- [stdlib.md](./stdlib.md)
