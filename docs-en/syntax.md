# Noema Syntax Reference

## Variable declarations

```noema
let x = 1
var name = "Noema"
const PI = 3.14159
```

`var` and `let` declare mutable variables. `const` declares a constant — reassignment raises a runtime error. Semicolons are optional when the statement already ends on the current line.

## Functions

```noema
function add(a, b) {
    return a + b
}
```

Short arrow form:

```noema
fn add(a, b) => a + b
```

Anonymous functions:

```noema
var double = fn (x) => x * 2
var greet  = fn (name) => "hello #{name}"
```

## Control flow

```noema
if x > 0 {
    print("positive")
} elif x == 0 {
    print("zero")
} else {
    print("negative")
}

unless x < 0 {
    print("non-negative")
}
```

`when` is an alias for `if`:

```noema
when score > 100 {
    IO.puts("high score!")
}
```

## Loops

```noema
while x < 10 {
    x = x + 1
}

for i in range(0, 5) {
    print(i)
}

each item in [1, 2, 3,] {
    print(item)
}
```

`for` and `each` are equivalent. `range(start, end)` generates a list of integers from `start` up to (not including) `end`.

## Lists

```noema
var xs = [1, 2, 3,]
print(xs[0])          // 1
push(xs, 4)
var last = pop(xs)    // 4
```

Trailing commas are allowed.

## Maps

```noema
var user = {
    name: "Noema",
    score: 42,
}

print(user.name)
print(user["score"])
user.level = 3
```

## Strings

Interpolation with `#{}`:

```noema
var msg = "hello #{user.name}, score=#{user.score}"
```

Raw strings (no interpolation):

```noema
var raw = `path\to\file`
```

## Numbers

Decimal and hexadecimal:

```noema
var ratio = 3.14
var flags = 0x20
var quit  = 0x100
```

## Importing files

```noema
import "path/to/file.noe"
```

The preprocessor resolves `import` before parsing. A file is only imported once per session. For stdlib modules, use `use`:

```noema
use "std/math"
use "std/strings"
```

## Macros

Macros receive unevaluated AST and return new code:

```noema
macro unless_do(cond, body) {
    return syntax if not $cond { $body }
}

expand unless_do(len(data) == 0, {
    IO.puts("data ready")
})
```

Rules:

- `macro name(args) { ... }` declares a macro
- `macro name(args) => expr` short form
- `expand name(args)` invokes the macro at parse time
- `syntax form` and `syntax { ... }` construct AST nodes
- `$name` inserts a macro argument as-is, without evaluating it
- `$(expr)` evaluates `expr` at expansion time and inserts the result

### Syntax introspection

```noema
macro inspect_kind(expr) {
    return syntax_literal(syntax_kind(expr))
}

expand inspect_kind(1 + 2)   // "binary"
expand inspect_kind(foo)      // "identifier"
```

Available introspection builtins:

- `is_syntax(value)` — checks if a value is an AST node
- `syntax_kind(node)` — returns the node type: `"binary"`, `"call"`, `"identifier"`, etc.
- `syntax_text(node)` — returns the text of the node (for identifiers and literals)
- `syntax_parts(node)` — returns the node's children as a map

### Syntax constructors

```noema
macro make_fn(name, param, expr) {
    return syntax_function(syntax_text(name), [syntax_text(param)], syntax {
        return $expr
    })
}

expand make_fn(square, x, x * x)
print(square(5))   // 25
```

Available constructors:

- `syntax_identifier(name)` — creates an identifier node
- `syntax_literal(value)` — creates a literal node (number, string, bool)
- `syntax_block(stmts)` — creates a block of statements
- `syntax_let(name, init)` — creates a `let` declaration
- `syntax_const(name, init)` — creates a `const` declaration
- `syntax_function(name, params, body)` — creates a function declaration

## Dynamic evaluation

```noema
eval("fn square(x) => x * x")
print(square(4))   // 16
```

Protected form that never throws:

```noema
var result = try_eval("1 +")
print(result.ok)     // false
print(result.error)  // error message
```

`try_eval` returns a map `{ok, value, error}`.

## Language builtins

### Types and conversion

| Builtin | Description |
|---------|-------------|
| `type(v)` | returns the type name as a string |
| `string(v)` | converts to string |
| `number(v)` | converts to number |
| `int(v)` | converts to integer (truncates decimals) |
| `bool(v)` | converts to boolean |
| `parse_number(s)` | parses a string as a number |
| `parse_int(s)` | parses a string as an integer |
| `is_null(v)` | type predicate |
| `is_bool(v)` | type predicate |
| `is_number(v)` | type predicate |
| `is_string(v)` | type predicate |
| `is_list(v)` | type predicate |
| `is_map(v)` | type predicate |
| `is_function(v)` | type predicate |
| `is_pointer(v)` | type predicate |

### Collections

| Builtin | Description |
|---------|-------------|
| `len(v)` | length of list, map, or string |
| `get(v, key, fallback?)` | safe access to map or list |
| `has(map, key)` | checks if map has key |
| `push(list, item)` | appends to list |
| `pop(list)` | removes and returns last item |
| `keys(map)` | list of map keys |
| `values(map)` | list of map values |
| `range(start, end)` | generates integer list `[start, end)` |

### Higher-order list functions

| Builtin | Description |
|---------|-------------|
| `map(list, fn)` | transforms each element |
| `filter(list, fn)` | filters elements |
| `reduce(list, fn, initial)` | folds list to a single value |
| `find(list, fn)` | returns first element matching predicate |
| `any(list, fn)` | true if any element satisfies predicate |
| `all(list, fn)` | true if all elements satisfy predicate |
| `slice(list, start, end)` | sublist |
| `reverse(list)` | reverses the list |
| `sum(list)` | numeric sum |

Callbacks receive `(value, index)`.

### Strings

| Builtin | Description |
|---------|-------------|
| `join(list, sep)` | joins list into string |
| `split(str, sep)` | splits string |
| `replace(str, from, to)` | replaces occurrences |
| `trim(str)` | strips leading/trailing whitespace |

### IO

| Builtin | Description |
|---------|-------------|
| `print(args...)` | prints with newline |
| `write(args...)` | prints without newline |
| `flush()` | flushes stdout |
| `read_line(prompt?)` | reads a line from stdin |
| `read_stdin()` | reads all of stdin |

### Files and environment

| Builtin | Description |
|---------|-------------|
| `read_file(path)` | reads file as string |
| `write_file(path, content)` | writes string to file |
| `append_file(path, content)` | appends to file |
| `cwd()` | current directory |
| `change_dir(path)` | changes directory |
| `shell_run(cmd)` | executes shell command |

### Symbolic AI

| Builtin | Description |
|---------|-------------|
| `normalize_text(str)` | normalizes text for matching |
| `match_pattern(text, pattern)` | matches text against wildcard pattern |
| `render_template(template, vars)` | fills a template with variables |

### Memory / FFI

| Builtin | Description |
|---------|-------------|
| `memory_alloc(size)` | allocates a native buffer |
| `memory_free(ptr)` | frees a native buffer |
| `memory_fill(ptr, byte, size)` | fills buffer bytes |
| `memory_get_u8(ptr, offset)` | reads unsigned byte at offset |
| `memory_get_i32(ptr, offset)` | reads signed int32 at offset |
| `memory_get_u32(ptr, offset)` | reads unsigned int32 at offset |
| `memory_set_u8(ptr, offset, v)` | writes byte at offset |
| `memory_set_i32(ptr, offset, v)` | writes int32 at offset |
| `memory_set_u32(ptr, offset, v)` | writes uint32 at offset |

## Related reading

- [language.md](./language.md)
- [repl.md](./repl.md)
- [ffi.md](./ffi.md)
- [stdlib.md](./stdlib.md)
