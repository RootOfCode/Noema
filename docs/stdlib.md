# Stdlib de Noema

Este arquivo documenta a stdlib por arquivo, modulo e namespace.

Leituras relacionadas:

- [language.md](./language.md)
- [syntax.md](./syntax.md)
- [repl.md](./repl.md)
- [ffi.md](./ffi.md)
- [files.md](./files.md)

Carregamento:

- `stdlib/prelude.noe` e carregada automaticamente no startup
- `Std` agrega os namespaces principais
- os namespaces tambem ficam disponiveis diretamente no ambiente global

## Arquivo de entrada

### [stdlib/prelude.noe](../stdlib/prelude.noe)

Carrega todos os modulos da stdlib e expoe:

- `Std.version`
- `Std.Core`
- `Std.IO`
- `Std.FS`
- `Std.Path`
- `Std.REPL`
- `Std.Shell`
- `Std.Math`
- `Std.Random`
- `Std.Strings`
- `Std.List`
- `Std.Maps`
- `Std.Fn`
- `Std.Assert`
- `Std.Patterns`
- `Std.Dialogue`
- `Std.Rules`

## Modulos basicos

### [stdlib/core.noe](../stdlib/core.noe)

Namespace: `Core`

Funcoes e constantes principais:

- `version`
- `type`, `type_of`
- `string`, `number`, `int`, `bool`
- `parse_number`, `parse_int`
- `is_null`, `is_bool`, `is_number`, `is_string`, `is_list`, `is_map`, `is_function`, `is_pointer`
- `is_scalar`
- `default`, `defined`
- `identity`, `noop`
- `truthy`, `falsy`
- `maybe`
- `clone_list`, `repeat_value`, `times`, `enumerate`

### [stdlib/assert.noe](../stdlib/assert.noe)

Namespace: `Assert`

Funcoes:

- `assert`
- `eq`
- `ne`
- `truthy`
- `is_null`

## IO, sistema e ambiente interativo

### [stdlib/io.noe](../stdlib/io.noe)

Namespace: `IO`

Funcoes:

- `print`, `println`, `puts`, `write`, `flush`
- `show`, `inspect`
- `prompt`, `ask`, `confirm`
- `read_line`, `read_stdin`
- `panic`

### [stdlib/fs.noe](../stdlib/fs.noe)

Namespace: `FS`

Funcoes:

- `cwd`, `chdir`
- `exists`, `is_file`, `is_dir`
- `list`
- `mkdir`, `mkdir_p`
- `read_text`, `write_text`, `append_text`
- `read_lines`

### [stdlib/path.noe](../stdlib/path.noe)

Namespace: `Path`

Funcoes:

- `join`
- `dirname`
- `basename`
- `extname`
- `stem`
- `parts`

### [stdlib/repl.noe](../stdlib/repl.noe)

Namespace: `REPL`

Funcoes:

- `create`
- `eval`
- `help`
- `start`

Campos esperados em `REPL.start(options)`:

- `prompt`
- `exit`
- `path`
- `quiet`
- `banner`
- `on_line`
- `on_help`
- `on_result`
- `on_error`

### [stdlib/shell.noe](../stdlib/shell.noe)

Namespace: `Shell`

Funcoes:

- `create`
- `run`
- `start`

Builtins de shell em Noema:

- `pwd`
- `cd <path>`
- `help`
- `exit`

## Matematica, aleatoriedade e strings

### [stdlib/math.noe](../stdlib/math.noe)

Namespace: `Math`

Constantes:

- `pi`
- `tau`
- `e`

Funcoes:

- trigonometria: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`
- numericas: `sqrt`, `hypot`, `abs`, `floor`, `ceil`, `round`, `trunc`, `pow`, `mod`, `sign`
- limites e analise: `min`, `max`, `log`, `exp`, `is_nan`, `is_finite`
- helpers: `clamp`, `lerp`, `inverse_lerp`, `normalize`, `deg_to_rad`, `rad_to_deg`, `distance2d`, `average`

### [stdlib/random.noe](../stdlib/random.noe)

Namespace: `Random`

Funcoes:

- `value`
- `int`
- `between`
- `int_between`
- `bool`
- `sample`

### [stdlib/strings.noe](../stdlib/strings.noe)

Namespace: `Strings`

Funcoes:

- `string`, `len`
- `lower`, `upper`
- `trim`, `trim_left`, `trim_right`
- `normalize`
- `split`, `join`, `replace`
- `reverse`, `slice`
- `lines`, `words`
- `starts_with`, `ends_with`, `contains`
- `blank`
- `word_swap`
- `repeat`
- `index_of`, `last_index_of`
- `char_at`, `ord`, `chr`
- `pad_left`, `pad_right`
- `title_case`

## Colecoes e programacao funcional

### [stdlib/lists.noe](../stdlib/lists.noe)

Namespace: `List`

Funcoes:

- `len`, `get`, `push`, `pop`
- `map`, `filter`, `reduce`, `find`, `any`, `all`
- `sum`, `reverse`, `slice`
- `first`, `second`, `last`
- `take`, `drop`
- `append`, `prepend`, `concat`
- `flatten`
- `count`
- `contains`
- `unique`
- `zip`
- `chunk`
- `pluck`
- `partition`
- `group_by`
- `sort_numbers`

### [stdlib/maps.noe](../stdlib/maps.noe)

Namespace: `Maps`

Funcoes:

- `keys`, `values`
- `get`, `has`
- `clone`
- `merge`
- `entries`
- `from_entries`
- `pick`
- `omit`
- `invert`

### [stdlib/functional.noe](../stdlib/functional.noe)

Namespace: `Fn`

Funcoes:

- `pipe`
- `compose`
- `tap`
- `once`
- `memoize`
- `constantly`

## Texto, dialogo e regras

### [stdlib/patterns.noe](../stdlib/patterns.noe)

Namespace: `Patterns`

Funcoes:

- `default_reflections`
- `normalize`
- `match`
- `fill`
- `captures`
- `reflect`
- `sort_rules`
- `first_match`

### [stdlib/dialogue.noe](../stdlib/dialogue.noe)

Namespace: `Dialogue`

Funcoes:

- `create`
- `remember`, `recall`
- `push_memory`, `pop_memory`
- `set_topic`
- `record`
- `last_message`
- `rule`
- `context`
- `apply`
- `start`

Uso tipico:

- criar uma sessao
- definir regras com `Dialogue.rule(...)`
- rodar com `Dialogue.start(...)`

### [stdlib/rules.noe](../stdlib/rules.noe)

Namespace: `Rules`

Funcoes:

- `create`
- `set`, `get`, `known`, `clear`
- `certainty`, `combine`, `update`
- `present`, `absent`
- `rule`
- `conclude`
- `reset`
- `infer`
- `trace`
- `rank`
- `parse_certainty`
- `ask_certainty`

## Como ler a stdlib

Ordem recomendada:

1. [stdlib/prelude.noe](../stdlib/prelude.noe)
2. [stdlib/core.noe](../stdlib/core.noe)
3. [stdlib/io.noe](../stdlib/io.noe)
4. [stdlib/lists.noe](../stdlib/lists.noe)
5. [stdlib/maps.noe](../stdlib/maps.noe)
6. [stdlib/patterns.noe](../stdlib/patterns.noe)
7. [stdlib/dialogue.noe](../stdlib/dialogue.noe)
8. [stdlib/rules.noe](../stdlib/rules.noe)
