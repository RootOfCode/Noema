# Sintaxe de Noema

## Declaracoes

```noema
let x = 1;
const name = "Noema";

function add(a, b) {
    return a + b;
}
```

Forma curta:

```noema
var x = 1
fn add(a, b) => a + b
```

## Controle de fluxo

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

while x < 10 {
    x = x + 1
}

each item in [1, 2, 3,] {
    print(item)
}
```

## Funcoes anonimas

```noema
var xs = [1, 2, 3]
var doubled = map(xs, fn (value, index) => value * 2 + index)
```

## Listas, mapas e strings

```noema
var user = {
    name: "Noema",
    score: 42
}

var xs = [1, 2, 3,]
print("hello #{user.name}")
print(xs[0])
```

## Numeros hexadecimais

```noema
var video_flag = 0x20
var quit_event = 0x100
```

## Semicolon

Semicolon e opcional quando a linha ja fecha a instrucao:

```noema
var x = 1
print(x)
```

Ainda e aceito quando necessario ou por estilo:

```noema
let x = 1;
```

## Macros

```noema
macro unless_do(cond, body) {
    return syntax if not $cond { $body }
}

macro define_echo(name) {
    return syntax_function(syntax_text(name), ["value"], syntax {
        print(value)
    })
}

expand define_echo(echo_once)
expand unless_do(false, {
    echo_once("hello")
})
```

Regras centrais:

- `macro name(args) { ... }`
- `macro name(args) => { ... }`
- macros recebem `syntax` nao avaliada
- `expand name(...)` aceita expressoes, instrucoes e blocos
- `syntax form` e `syntax { ... }` constroem AST
- `$name` insere um argumento recebido pela macro
- `$(expr)` insere o resultado calculado em tempo de expansao
- introspecao: `is_syntax`, `syntax_kind`, `syntax_text`, `syntax_parts`
- construtores: `syntax_identifier`, `syntax_literal`, `syntax_block`, `syntax_let`, `syntax_const`, `syntax_function`

## Avaliacao dinamica

```noema
eval("fn square(x) => x * x")
print(square(4))
```

Forma protegida:

```noema
var result = try_eval("1 +")
print(result.ok)
print(result.error)
```

## Builtins importantes

- tipos: `type`, `string`, `number`, `int`, `bool`
- sintaxe: `is_syntax`, `syntax_kind`, `syntax_text`, `syntax_parts`
- memoria/FFI: `memory_alloc`, `memory_free`, `memory_get_u32`, `memory_set_i32`
- colecoes: `len`, `get`, `has`, `push`, `pop`, `keys`, `values`
- listas: `map`, `filter`, `reduce`, `find`, `any`, `all`, `slice`, `reverse`, `sum`
- texto: `join`, `split`, `replace`
- IO: `print`, `write`, `flush`, `read_line`, `read_stdin`
- arquivos: `read_file`, `write_file`, `append_file`
- ambiente: `cwd`, `change_dir`, `shell_run`
- IA simbolica: `normalize_text`, `match_pattern`, `render_template`

## Namespaces uteis

- `Core`
- `IO`
- `Math`
- `Strings`
- `List`
- `Maps`
- `Patterns`
- `Dialogue`
- `Rules`

## Leituras relacionadas

- [language.md](./language.md)
- [repl.md](./repl.md)
- [ffi.md](./ffi.md)
- [stdlib.md](./stdlib.md)
