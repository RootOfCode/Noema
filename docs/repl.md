# REPL e CLI de Noema

## Execucao

Linux/POSIX:

```bash
./build/noema
./build/noema --repl
./build/noema --repl examples/friendly.noe
./build/noema examples/eliza.noe
```

Windows:

```bat
build\noema.exe
build\noema.exe --repl
build\noema.exe examples\friendly.noe
```

## Modos

- sem argumentos: abre o REPL nativo
- `--repl` ou `-i`: opcionalmente carrega um script e continua no REPL
- `arquivo.noe`: executa um programa

## Multiline

O REPL detecta blocos incompletos automaticamente.

Quando isso acontece:

- o prompt muda de `noema>` para `....>`
- a avaliacao so acontece quando o bloco fecha

## Comandos do REPL nativo

- `:help`
- `help`
- `:help syntax`
- `:help stdlib`
- `:help ffi`
- `:quit`
- `:exit`
- `quit`
- `:reset`

## REPLs escritos em Noema

`stdlib/repl.noe` permite construir REPLs dentro da propria linguagem.

Campos de `REPL.start(options)`:

- `prompt`
- `exit`
- `path`
- `quiet`
- `banner`
- `on_line`
- `on_help`
- `on_result`
- `on_error`

Comportamento padrao:

- `help` e `:help` mostram ajuda
- `quit`, `:quit` e o valor de `exit` encerram

## Exemplos

- [../examples/eliza.noe](../examples/eliza.noe)
- [../examples/parry.noe](../examples/parry.noe)
- [../examples/alice.noe](../examples/alice.noe)
- [../examples/mycin.noe](../examples/mycin.noe)
- [../examples/shrdlu.noe](../examples/shrdlu.noe)
