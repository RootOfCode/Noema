# REPL e CLI de Noema

## Execução

Linux/POSIX:

```bash
./build/noema                              # abre o REPL
./build/noema --repl                       # alias explícito para o REPL
./build/noema --repl exemplos/friendly.noe # carrega script e continua no REPL
./build/noema exemplos/eliza.noe           # executa um programa
```

Windows:

```bat
build\noema.exe
build\noema.exe --repl
build\noema.exe --repl exemplos\friendly.noe
build\noema.exe exemplos\eliza.noe
```

## Modos de execução

| Invocação | Comportamento |
|-----------|---------------|
| Sem argumentos | Abre o REPL nativo |
| `--repl` ou `-i` | Abre o REPL nativo (explícito) |
| `--repl arquivo.noe` | Carrega o script e continua no mesmo ambiente global |
| `arquivo.noe` | Executa o programa e encerra |

## Blocos multiline

O REPL detecta blocos incompletos automaticamente. Quando a entrada não está fechada, o prompt muda de `noema>` para `....>` e a avaliação só acontece quando o bloco termina:

```
noema> function greet(name) {
....>     return "hello #{name}"
....> }
noema> greet("world")
"hello world"
```

## Comandos do REPL nativo

| Comando | Efeito |
|---------|--------|
| `:help` ou `help` | Ajuda geral |
| `:help syntax` | Referência de sintaxe |
| `:help stdlib` | Referência da stdlib |
| `:help ffi` | Referência de plugins/FFI |
| `:quit`, `:exit` ou `quit` | Encerra o REPL |
| `:reset` | Descarta um bloco multiline pendente |

## Variável de ambiente

`NOEMA_STDLIB` sobrescreve o diretório da stdlib:

```bash
NOEMA_STDLIB=/opt/noema/share/noema/stdlib ./build/noema
```

## REPLs escritos em Noema

`stdlib/repl.noe` permite construir REPLs customizados dentro da própria linguagem usando `REPL.start(options)`.

### Campos de `REPL.start(options)`

| Campo | Tipo | Padrão | Descrição |
|-------|------|--------|-----------|
| `prompt` | string | `"repl> "` | Texto do prompt |
| `exit` | string | `":exit"` | Comando de saída |
| `path` | string | `"<repl>"` | Caminho virtual para erros |
| `quiet` | bool | `false` | Suprime mensagem de boas-vindas |
| `banner` | string\|null | `null` | Texto exibido ao iniciar |
| `on_line` | function\|null | `null` | Callback chamado a cada linha |
| `on_help` | function\|null | `null` | Callback para `:help` |
| `on_result` | function\|null | `null` | Callback para exibir resultados |
| `on_error` | function\|null | `null` | Callback para exibir erros |

### Comportamento padrão

Sem callbacks customizados:

- `help` e `:help` mostram ajuda padrão
- `quit`, `:quit` e o valor de `exit` encerram o loop
- Resultados não-nulos são impressos com `print`
- Erros são impressos com `print("error:", msg)`

### Exemplo mínimo

```noema
REPL.start({
    prompt: "meu-repl> ",
    banner: "Bem-vindo ao meu REPL",
    exit: "sair",
    on_line: fn (line, state, _) => {
        return {ok: true, value: "eco: #{line}", error: null}
    }
})
```

### Exemplo com on_line retornando controle total

```noema
REPL.start({
    prompt: "calc> ",
    on_line: fn (line, state, _) {
        var result = try_eval(line)
        return result
    },
    on_result: fn (value, state, _) {
        IO.puts("= #{value}")
    },
    on_error: fn (result, state, _) {
        IO.puts("erro: #{result.error}")
    }
})
```

## Shell customizado

`stdlib/shell.noe` oferece `Shell.start(options)` para construir shells simples que aceitam comandos do sistema operacional e comandos customizados.

Builtins do shell padrão disponíveis nos exemplos:

- `pwd` — imprime o diretório atual
- `cd <path>` — muda de diretório
- `help` — ajuda
- `exit` — encerra

## Exemplos do repositório

Os seguintes exemplos constroem REPLs e ambientes interativos completos:

- `exemplos/eliza.noe` — terapeuta por regras no estilo ELIZA
- `exemplos/parry.noe` — entrevistado paranoico no estilo PARRY
- `exemplos/alice.noe` — bot por categorias no estilo ALICE
- `exemplos/mycin.noe` — sistema especialista com fatores de certeza
- `exemplos/shrdlu.noe` — blocks world inspirado em SHRDLU

## Leituras relacionadas

- [language.md](./language.md)
- [syntax.md](./syntax.md)
- [ffi.md](./ffi.md)
- [stdlib.md](./stdlib.md)
