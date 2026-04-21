# Sintaxe de Noema

## Declarações de variáveis

```noema
let x = 1
var name = "Noema"
const PI = 3.14159
```

`var` e `let` declaram variáveis mutáveis. `const` declara uma constante — reatribuição gera erro em runtime. Semicolons são opcionais quando a instrução já termina na linha.

## Funções

```noema
function add(a, b) {
    return a + b
}
```

Forma curta com arrow:

```noema
fn add(a, b) => a + b
```

Funções anônimas:

```noema
var double = fn (x) => x * 2
var greet  = fn (name) => "hello #{name}"
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
```

`when` é um alias de `if`:

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

`for` e `each` são equivalentes. `range(start, end)` gera uma lista de inteiros de `start` até `end - 1`.

## Listas

```noema
var xs = [1, 2, 3,]
print(xs[0])          // 1
push(xs, 4)
var last = pop(xs)    // 4
```

Trailing commas são permitidas.

## Mapas

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

Interpolação com `#{}`:

```noema
var msg = "hello #{user.name}, score=#{user.score}"
```

Strings brutas não interpolam:

```noema
var raw = `path\to\file`
```

## Números

Decimais e hexadecimais:

```noema
var ratio = 3.14
var flags = 0x20
var quit  = 0x100
```

## Importação de arquivos

```noema
import "path/to/file.noe"
```

O preprocessador resolve `import` antes da análise. Um arquivo é importado apenas uma vez por sessão. Para módulos da stdlib, use `use`:

```noema
use "std/math"
use "std/strings"
```

## Macros

Macros recebem AST não avaliada e retornam novo código:

```noema
macro unless_do(cond, body) {
    return syntax if not $cond { $body }
}

expand unless_do(len(data) == 0, {
    IO.puts("data ready")
})
```

Regras:

- `macro name(args) { ... }` declara uma macro
- `macro name(args) => expr` forma curta
- `expand name(args)` invoca a macro em tempo de análise
- `syntax form` e `syntax { ... }` constroem nós AST
- `$name` insere um argumento recebido pela macro sem avaliar
- `$(expr)` avalia `expr` em tempo de expansão e insere o resultado

### Introspecção de sintaxe

```noema
macro inspect_kind(expr) {
    return syntax_literal(syntax_kind(expr))
}

expand inspect_kind(1 + 2)   // "binary"
expand inspect_kind(foo)      // "identifier"
```

Funções de introspecção disponíveis como builtins:

- `is_syntax(value)` — verifica se o valor é um nó AST
- `syntax_kind(node)` — retorna o tipo do nó: `"binary"`, `"call"`, `"identifier"`, etc.
- `syntax_text(node)` — retorna o texto do nó (para identificadores e literais)
- `syntax_parts(node)` — retorna os filhos do nó como mapa

### Construtores de sintaxe

```noema
macro make_fn(name, param, expr) {
    return syntax_function(syntax_text(name), [syntax_text(param)], syntax {
        return $expr
    })
}

expand make_fn(square, x, x * x)
print(square(5))   // 25
```

Construtores disponíveis:

- `syntax_identifier(name)` — cria um nó identificador
- `syntax_literal(value)` — cria um nó literal (número, string, bool)
- `syntax_block(stmts)` — cria um bloco de instruções
- `syntax_let(name, init)` — cria uma declaração `let`
- `syntax_const(name, init)` — cria uma declaração `const`
- `syntax_function(name, params, body)` — cria uma declaração de função

## Avaliação dinâmica

```noema
eval("fn square(x) => x * x")
print(square(4))   // 16
```

Forma protegida que não lança erro:

```noema
var result = try_eval("1 +")
print(result.ok)     // false
print(result.error)  // mensagem de erro
```

`try_eval` retorna um mapa `{ok, value, error}`.

## Builtins da linguagem

### Tipos e conversão

| Builtin | Descrição |
|---------|-----------|
| `type(v)` | retorna o nome do tipo como string |
| `string(v)` | converte para string |
| `number(v)` | converte para número |
| `int(v)` | converte para inteiro (trunca decimais) |
| `bool(v)` | converte para booleano |
| `parse_number(s)` | parseia string como número |
| `parse_int(s)` | parseia string como inteiro |
| `is_null(v)` | predicado de tipo |
| `is_bool(v)` | predicado de tipo |
| `is_number(v)` | predicado de tipo |
| `is_string(v)` | predicado de tipo |
| `is_list(v)` | predicado de tipo |
| `is_map(v)` | predicado de tipo |
| `is_function(v)` | predicado de tipo |
| `is_pointer(v)` | predicado de tipo |

### Coleções

| Builtin | Descrição |
|---------|-----------|
| `len(v)` | comprimento de lista, mapa ou string |
| `get(v, key, fallback?)` | acesso seguro a mapa ou lista |
| `has(map, key)` | verifica se mapa tem chave |
| `push(list, item)` | adiciona ao final da lista |
| `pop(list)` | remove e retorna o último item |
| `keys(map)` | lista de chaves do mapa |
| `values(map)` | lista de valores do mapa |
| `range(start, end)` | gera lista de inteiros `[start, end)` |

### Listas (funções de ordem superior)

| Builtin | Descrição |
|---------|-----------|
| `map(list, fn)` | transforma cada elemento |
| `filter(list, fn)` | filtra elementos |
| `reduce(list, fn, initial)` | reduz lista a um valor |
| `find(list, fn)` | retorna primeiro elemento que satisfaz predicado |
| `any(list, fn)` | verdadeiro se algum elemento satisfaz |
| `all(list, fn)` | verdadeiro se todos satisfazem |
| `slice(list, start, end)` | sublista |
| `reverse(list)` | inverte a lista |
| `sum(list)` | soma numérica |

Callbacks recebem `(value, index)`.

### Strings

| Builtin | Descrição |
|---------|-----------|
| `join(list, sep)` | une lista em string |
| `split(str, sep)` | divide string |
| `replace(str, from, to)` | substitui ocorrências |
| `trim(str)` | remove espaços nas pontas |

### IO

| Builtin | Descrição |
|---------|-----------|
| `print(args...)` | imprime com newline |
| `write(args...)` | imprime sem newline |
| `flush()` | descarrega stdout |
| `read_line(prompt?)` | lê linha do stdin |
| `read_stdin()` | lê todo o stdin |

### Arquivos e ambiente

| Builtin | Descrição |
|---------|-----------|
| `read_file(path)` | lê arquivo como string |
| `write_file(path, content)` | escreve string em arquivo |
| `append_file(path, content)` | acrescenta ao arquivo |
| `cwd()` | diretório atual |
| `change_dir(path)` | muda diretório |
| `shell_run(cmd)` | executa comando no shell |

### IA simbólica

| Builtin | Descrição |
|---------|-----------|
| `normalize_text(str)` | normaliza texto para matching |
| `match_pattern(text, pattern)` | compara texto com padrão wildcard |
| `render_template(template, vars)` | preenche template com variáveis |

### Memória / FFI

| Builtin | Descrição |
|---------|-----------|
| `memory_alloc(size)` | aloca buffer nativo |
| `memory_free(ptr)` | libera buffer nativo |
| `memory_fill(ptr, byte, size)` | preenche buffer |
| `memory_get_u8(ptr, offset)` | lê byte sem sinal |
| `memory_get_i32(ptr, offset)` | lê int32 com sinal |
| `memory_get_u32(ptr, offset)` | lê uint32 sem sinal |
| `memory_set_u8(ptr, offset, v)` | escreve byte |
| `memory_set_i32(ptr, offset, v)` | escreve int32 |
| `memory_set_u32(ptr, offset, v)` | escreve uint32 |

## Leituras relacionadas

- [language.md](./language.md)
- [repl.md](./repl.md)
- [ffi.md](./ffi.md)
- [stdlib.md](./stdlib.md)
