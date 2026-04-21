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
macro define_echo(name) {
function $name(value) {
    print(value);
}
}

expand define_echo(echo_once)
echo_once("hello")
```

Formas aceitas:

- `macro name(args) { ... }`
- `macro name(args) => { ... }`
- forma antiga com crases, por compatibilidade

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
