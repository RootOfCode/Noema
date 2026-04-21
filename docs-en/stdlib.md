# Noema Standard Library

The stdlib is loaded automatically via `stdlib/prelude.noe` at startup. All namespaces below are available without any manual `import`.

The `Std` namespace aggregates all modules as `Std.Core`, `Std.IO`, etc. Each namespace is also available directly in the global scope.

Related reading: [language.md](./language.md) · [syntax.md](./syntax.md) · [ffi.md](./ffi.md) · [files.md](./files.md)

---

## Core modules

### Core — `stdlib/core.noe`

Fundamental utilities, type helpers, conversions, and memory helpers for FFI.

**Constants:** `Core.version` (e.g. `"Noema 0.3"`)

**Types and conversion:**

| Function | Description |
|----------|-------------|
| `Core.type(v)` / `type(v)` | type name as string |
| `Core.string(v)` | converts to string |
| `Core.number(v)` | converts to number |
| `Core.int(v)` | converts to integer |
| `Core.bool(v)` | converts to bool |
| `Core.parse_number(s)` | parses string as number |
| `Core.parse_int(s)` | parses string as integer |

**Type predicates:**

`is_null`, `is_bool`, `is_number`, `is_string`, `is_list`, `is_map`, `is_function`, `is_pointer`

**Logic helpers:**

| Function | Description |
|----------|-------------|
| `Core.default(v, fallback)` | returns `fallback` if `v` is null |
| `Core.defined(v)` | true if `v != null` |
| `Core.truthy(v)` | converts to positive bool |
| `Core.falsy(v)` | inverts truthy |
| `Core.maybe(v, fn, fallback)` | calls `fn(v)` if not null, otherwise `fallback` |
| `Core.identity(v)` | returns `v` unchanged |
| `Core.noop(v)` | alias for identity |
| `Core.is_scalar(v)` | true for null, bool, number, string |

**Collection helpers:**

| Function | Description |
|----------|-------------|
| `Core.clone_list(xs)` | shallow copy of a list |
| `Core.repeat_value(v, n)` | list of `n` copies of `v` |
| `Core.times(n, fn)` | calls `fn(i)` for `i` in `0..n-1`, returns results |
| `Core.enumerate(xs)` | list of `{index, value}` maps |
| `Core.range(start, end)` | integer list `[start, end)` |

**Memory / FFI:**

`memory_alloc`, `memory_free`, `memory_fill`, `memory_get_u8`, `memory_get_i32`, `memory_get_u32`, `memory_set_u8`, `memory_set_i32`, `memory_set_u32`

---

### Assert — `stdlib/assert.noe`

Assertions for tests and invariant checks.

| Function | Description |
|----------|-------------|
| `Assert.assert(cond, msg?)` | panics if `cond` is false |
| `Assert.eq(a, b, msg?)` | fails if `a != b` |
| `Assert.ne(a, b, msg?)` | fails if `a == b` |
| `Assert.truthy(v, msg?)` | fails if `v` is falsy |
| `Assert.is_null(v, msg?)` | fails if `v` is not null |

---

## IO, system, and interactive environments

### IO — `stdlib/io.noe`

Console output, prompts, and user input.

| Function | Description |
|----------|-------------|
| `IO.print(args...)` | prints with newline |
| `IO.println(args...)` | alias for print |
| `IO.puts(str)` | prints string with newline |
| `IO.write(args...)` | prints without newline |
| `IO.flush()` | flushes stdout |
| `IO.show(label, value)` | prints `label: value` formatted |
| `IO.inspect(value)` | debug representation of a value |
| `IO.prompt(text)` | reads a line after displaying text |
| `IO.ask(text)` | alias for prompt |
| `IO.confirm(text)` | reads a line and interprets it as bool |
| `IO.read_line(prompt?)` | reads a line from stdin |
| `IO.read_stdin()` | reads all of stdin |
| `IO.panic(msg)` | prints error and exits the process |

---

### FS — `stdlib/fs.noe`

File and directory operations.

| Function | Description |
|----------|-------------|
| `FS.cwd()` | current working directory |
| `FS.chdir(path)` | changes directory |
| `FS.exists(path)` | true if path exists |
| `FS.is_file(path)` | true if path is a file |
| `FS.is_dir(path)` | true if path is a directory |
| `FS.list(path)` | lists directory entries |
| `FS.mkdir(path)` | creates a directory |
| `FS.mkdir_p(path)` | creates a directory and all ancestors |
| `FS.read_text(path)` | reads file as string |
| `FS.write_text(path, content)` | writes string to file |
| `FS.append_text(path, content)` | appends to file |
| `FS.read_lines(path)` | reads file as list of lines |

---

### Path — `stdlib/path.noe`

File path manipulation.

| Function | Description |
|----------|-------------|
| `Path.join(a, b)` | joins two path segments |
| `Path.dirname(path)` | parent directory |
| `Path.basename(path)` | filename with extension |
| `Path.extname(path)` | extension (e.g. `".noe"`) |
| `Path.stem(path)` | filename without extension |
| `Path.parts(path)` | list of path segments |

---

### REPL — `stdlib/repl.noe`

Building custom REPLs inside the language.

| Function | Description |
|----------|-------------|
| `REPL.create(options)` | creates REPL state |
| `REPL.eval(state, line)` | evaluates a line in REPL context |
| `REPL.help(state)` | shows help |
| `REPL.start(options)` | starts the REPL loop |

See [repl.md](./repl.md) for a full reference of `options` fields.

---

### Shell — `stdlib/shell.noe`

Building simple shells in Noema.

| Function | Description |
|----------|-------------|
| `Shell.create(options)` | creates shell state |
| `Shell.run(state, line)` | executes a command line |
| `Shell.start(options)` | starts the shell loop |

---

## Math, randomness, and strings

### Math — `stdlib/math.noe`

**Constants:** `Math.pi`, `Math.tau`, `Math.e`

**Trigonometry:** `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`

**Numeric:** `sqrt`, `hypot`, `abs`, `floor`, `ceil`, `round`, `trunc`, `pow`, `mod`, `sign`

**Limits and analysis:** `min`, `max`, `log`, `exp`, `is_nan`, `is_finite`

**Helpers:**

| Function | Description |
|----------|-------------|
| `Math.clamp(v, lo, hi)` | clamps `v` to `[lo, hi]` |
| `Math.lerp(a, b, t)` | linear interpolation |
| `Math.inverse_lerp(a, b, v)` | relative position of `v` between `a` and `b` |
| `Math.normalize(v, lo, hi)` | normalizes to `[0, 1]` |
| `Math.deg_to_rad(d)` | degrees to radians |
| `Math.rad_to_deg(r)` | radians to degrees |
| `Math.distance2d(x1, y1, x2, y2)` | 2D Euclidean distance |
| `Math.average(list)` | arithmetic mean |

---

### Random — `stdlib/random.noe`

| Function | Description |
|----------|-------------|
| `Random.value()` | float in `[0, 1)` |
| `Random.int(n)` | integer in `[0, n)` |
| `Random.between(a, b)` | float in `[a, b)` |
| `Random.int_between(a, b)` | integer in `[a, b)` |
| `Random.bool()` | random boolean |
| `Random.sample(list)` | random element from list |

---

### Strings — `stdlib/strings.noe`

| Function | Description |
|----------|-------------|
| `Strings.lower(s)` | lowercase |
| `Strings.upper(s)` | uppercase |
| `Strings.trim(s)` | strips leading and trailing whitespace |
| `Strings.trim_left(s)` | strips leading whitespace |
| `Strings.trim_right(s)` | strips trailing whitespace |
| `Strings.normalize(s)` | lowercase + trim |
| `Strings.split(s, sep)` | splits by separator |
| `Strings.join(list, sep)` | joins with separator |
| `Strings.replace(s, from, to)` | replaces occurrences |
| `Strings.reverse(s)` | reverses string |
| `Strings.slice(s, start, end)` | substring `[start, end)` |
| `Strings.lines(s)` | splits by line breaks |
| `Strings.words(s)` | splits by whitespace |
| `Strings.starts_with(s, prefix)` | prefix predicate |
| `Strings.ends_with(s, suffix)` | suffix predicate |
| `Strings.contains(s, sub)` | substring predicate |
| `Strings.blank(s)` | true if empty or only whitespace |
| `Strings.word_swap(s, map)` | replaces words according to a map |
| `Strings.repeat(s, n)` | repeats string `n` times |
| `Strings.index_of(s, sub)` | position of first occurrence |
| `Strings.last_index_of(s, sub)` | position of last occurrence |
| `Strings.char_at(s, i)` | character at index `i` |
| `Strings.ord(s)` | Unicode code point of first character |
| `Strings.chr(n)` | character from Unicode code point |
| `Strings.pad_left(s, n, ch?)` | left-pads to width `n` |
| `Strings.pad_right(s, n, ch?)` | right-pads to width `n` |
| `Strings.title_case(s)` | capitalizes first letter of each word |

---

## Collections and functional programming

### List — `stdlib/lists.noe`

| Function | Description |
|----------|-------------|
| `List.len(xs)` | length |
| `List.get(xs, i)` | element at index |
| `List.push(xs, v)` | appends to end |
| `List.pop(xs)` | removes and returns last |
| `List.map(xs, fn)` | transforms elements |
| `List.filter(xs, fn)` | filters elements |
| `List.reduce(xs, fn, init)` | folds to a single value |
| `List.find(xs, fn)` | first element matching predicate |
| `List.any(xs, fn)` | true if any element matches |
| `List.all(xs, fn)` | true if all elements match |
| `List.sum(xs)` | numeric sum |
| `List.reverse(xs)` | reverses |
| `List.slice(xs, a, b)` | sublist `[a, b)` |
| `List.first(xs)` | first element |
| `List.second(xs)` | second element |
| `List.last(xs)` | last element |
| `List.take(xs, n)` | first `n` elements |
| `List.drop(xs, n)` | without the first `n` elements |
| `List.append(a, b)` | concatenates two lists |
| `List.prepend(xs, v)` | adds to the front |
| `List.concat(lists)` | concatenates a list of lists |
| `List.flatten(xs)` | flattens one level |
| `List.count(xs, fn)` | counts elements matching predicate |
| `List.contains(xs, v)` | checks if value is present |
| `List.unique(xs)` | removes duplicates |
| `List.zip(a, b)` | pairs elements from two lists |
| `List.chunk(xs, n)` | splits into groups of size `n` |
| `List.pluck(xs, key)` | extracts a field from a list of maps |
| `List.partition(xs, fn)` | splits into `[true_items, false_items]` |
| `List.group_by(xs, fn)` | groups by key function |
| `List.sort_numbers(xs)` | sorts numerically |

Callbacks receive `(value, index)`.

---

### Maps — `stdlib/maps.noe`

| Function | Description |
|----------|-------------|
| `Maps.keys(m)` | list of keys |
| `Maps.values(m)` | list of values |
| `Maps.get(m, key, fallback?)` | safe access |
| `Maps.has(m, key)` | checks if key exists |
| `Maps.clone(m)` | shallow copy |
| `Maps.merge(a, b)` | merges two maps (`b` overwrites `a`) |
| `Maps.entries(m)` | list of `{key, value}` maps |
| `Maps.from_entries(list)` | map from `{key, value}` list |
| `Maps.pick(m, keys)` | subset with specified keys |
| `Maps.omit(m, keys)` | map without specified keys |
| `Maps.invert(m)` | swaps keys and values |

---

### Fn — `stdlib/functional.noe`

| Function | Description |
|----------|-------------|
| `Fn.pipe(value, fns)` | applies a list of functions in sequence |
| `Fn.compose(fns)` | returns a composed function |
| `Fn.tap(v, fn)` | calls `fn(v)` and returns `v` |
| `Fn.once(fn)` | returns a version that only executes once |
| `Fn.memoize(fn)` | returns a cached version |
| `Fn.constantly(v)` | returns a function that always returns `v` |

---

## Text, dialogue, and rules

### Patterns — `stdlib/patterns.noe`

Symbolic matching, wildcards, and templates — the foundation for chatbots and symbolic AI.

| Function | Description |
|----------|-------------|
| `Patterns.default_reflections()` | default pronoun reflection map (I→you, etc.) |
| `Patterns.normalize(text)` | normalizes text for matching |
| `Patterns.match(text, pattern)` | returns capture map or null |
| `Patterns.fill(template, vars)` | substitutes `$var` in template |
| `Patterns.captures(text, pattern)` | list of captured groups |
| `Patterns.reflect(text, map)` | applies pronoun reflection |
| `Patterns.sort_rules(rules)` | sorts rules by specificity |
| `Patterns.first_match(text, rules)` | first matching rule |

Patterns use `*` as a wildcard. Captures are accessed by wildcard position.

**Example:**

```noema
var m = Patterns.match("my name is Alice", "my name is *")
IO.puts(m["0"])   // "Alice"
```

---

### Dialogue — `stdlib/dialogue.noe`

Conversational sessions with memory, topic tracking, and history.

| Function | Description |
|----------|-------------|
| `Dialogue.create(options?)` | creates a conversational session |
| `Dialogue.remember(session, key, value)` | stores a value in memory |
| `Dialogue.recall(session, key, fallback?)` | retrieves from memory |
| `Dialogue.push_memory(session, key, value)` | pushes value onto a memory stack |
| `Dialogue.pop_memory(session, key)` | pops value from a memory stack |
| `Dialogue.set_topic(session, topic)` | sets the current topic |
| `Dialogue.record(session, speaker, text)` | records an utterance in history |
| `Dialogue.last_message(session, speaker?)` | last message (from speaker, if given) |
| `Dialogue.rule(session, pattern, response)` | adds a response rule |
| `Dialogue.context(session)` | returns the current context |
| `Dialogue.apply(session, input)` | applies rules to input and returns response |
| `Dialogue.start(session, options?)` | starts the conversational loop |

**`Dialogue.create(options)` fields:**

| Field | Default | Description |
|-------|---------|-------------|
| `name` | `"agent"` | agent name |
| `topic` | `null` | initial topic |
| `memory` | `{}` | initial memory map |
| `fallback` | `null` | default response when no rule matches |

---

### Rules — `stdlib/rules.noe`

Fact base, inference engine, and certainty factors — foundation for expert systems.

| Function | Description |
|----------|-------------|
| `Rules.create(name)` | creates a knowledge base |
| `Rules.set(kb, name, value)` | sets a fact |
| `Rules.get(kb, name, fallback?)` | retrieves a fact |
| `Rules.known(kb, name)` | true if fact is defined |
| `Rules.clear(kb, names)` | removes facts |
| `Rules.certainty(value)` | normalizes to `[-1, 1]` |
| `Rules.combine(a, b)` | combines two certainty factors |
| `Rules.update(kb, name, value)` | updates by combining with existing certainty |
| `Rules.present(kb, name, threshold?)` | fact above positive threshold |
| `Rules.absent(kb, name, threshold?)` | fact below negative threshold |
| `Rules.rule(kb, name, cond, action)` | adds an inference rule |
| `Rules.conclude(name, cf)` | standard conclusion action |
| `Rules.reset(kb)` | clears all facts |
| `Rules.infer(kb)` | runs an inference cycle |
| `Rules.trace(kb)` | runs inference with logging |
| `Rules.rank(kb, names)` | sorts facts by certainty |
| `Rules.parse_certainty(text)` | parses a verbal certainty description |
| `Rules.ask_certainty(prompt)` | asks the user for a certainty value |

**Certainty factors** are values in `[-1, 1]`:

- `1.0` — absolute certainty that the fact is true
- `0.0` — unknown
- `-1.0` — absolute certainty that the fact is false

**Minimal example:**

```noema
var kb = Rules.create("diagnosis")

Rules.set(kb, "fever", 0.8)
Rules.set(kb, "cough", 0.6)

Rules.rule(kb, "flu",
    fn (kb, _) => Rules.present(kb, "fever") and Rules.present(kb, "cough"),
    Rules.conclude("flu", 0.7)
)

Rules.infer(kb)
IO.show("flu", Rules.get(kb, "flu"))
```

---

## Recommended reading order

1. `stdlib/prelude.noe`
2. `stdlib/core.noe`
3. `stdlib/io.noe`
4. `stdlib/lists.noe`
5. `stdlib/maps.noe`
6. `stdlib/patterns.noe`
7. `stdlib/dialogue.noe`
8. `stdlib/rules.noe`
