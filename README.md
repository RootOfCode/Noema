# Noema

`Noema` e uma linguagem de programacao interpretada, com sintaxe em ingles, runtime em C e foco em metaprogramacao, IA simbolica, sistemas baseados em regras e desenvolvimento interativo.

O codigo C do projeto foi escrito em pt-BR. A linguagem, a stdlib, os exemplos e os plugins usam sintaxe em ingles.

## O que Noema e

Noema nao e apenas uma DSL. Ela e uma linguagem completa:

- funcoes nomeadas e anonimas
- escopo lexico, fechamento e recursao
- variaveis mutaveis e constantes
- `if`, `when`, `unless`, `while`, `for`, `each`, `return`, `break`, `continue`
- listas, mapas, strings com interpolacao e trailing commas
- macros por expansao de codigo
- `eval(...)` e `try_eval(...)`
- stdlib embarcada e carregada automaticamente
- REPL interativo e shells/REPLs construidos na propria linguagem
- plugins escritos em Noema com FFI para bibliotecas C

Na pratica, Noema serve tanto para programacao geral quanto para prototipacao de linguagens, agentes simbolicos, chatbots classicos, motores de regras, expert systems e ambientes interativos.

## Direcao da linguagem

Os pontos centrais de Noema sao:

- metaprogramacao sem s-expressions
- runtime pequeno e hackeavel em C
- stdlib orientada a linguagem, nao a demos ad-hoc
- ergonomia para escrever IA simbolica e ferramentas interativas
- extensibilidade via plugins do usuario

## Exemplo rapido

```noema
macro make_metric(name, expr) {
function $name(values) {
    return $expr;
}
}

expand make_metric(total_metric, sum(values));
expand make_metric(mean_metric, sum(values) / len(values));

var data = [3, 5, 8, 13]

IO.show("total", total_metric(data))
IO.show("mean", mean_metric(data))

var matched = Patterns.match("i need *", "I need a better rule engine", {})
IO.show("captures", matched.captures)
```

## Recursos principais

- sintaxe blocada e infixa
- aliases amigaveis como `fn`, `var`, `use`, `when`, `unless`, `each`, `elif`
- semicolon opcional quando a instrucao ja termina na linha
- macros em bloco: `macro name(args) { ... }`
- forma antiga com crases ainda aceita por compatibilidade
- `eval(...)` para geracao dinamica de codigo
- `try_eval(...)` para avaliacao protegida no runtime
- FFI com `libffi` e carregamento dinamico de bibliotecas
- stdlib com `Core`, `IO`, `FS`, `Path`, `REPL`, `Shell`, `Math`, `Random`, `Strings`, `List`, `Maps`, `Fn`, `Assert`, `Patterns`, `Dialogue` e `Rules`

## Build

No Linux e outros ambientes POSIX:

```bash
make
```

Isso gera:

- `obj/*.o` e `obj/*.d`
- `build/noema`

No Windows:

```bat
make
```

ou, se o executavel do GNU Make ainda estiver com o nome padrao do MinGW:

```bat
mingw32-make
```

Tambem existe o atalho:

```bat
build.bat
```

Isso gera:

- `obj\*.obj` ou `obj\*.o`
- `build\noema.exe`

O `Makefile` agora funciona tanto em Linux quanto em Windows com MinGW.
`build.bat` continua disponivel como atalho e fallback para Windows.

Se o `libffi` nao estiver em um caminho padrao do compilador no Windows:

```bat
set LIBFFI_INCLUDE=C:\caminho\para\libffi\include
set LIBFFI_LIB=C:\caminho\para\libffi\lib
build.bat
```

## Instalacao

Em ambientes POSIX:

```bash
./install.sh
```

Por padrao:

- compilador em `/usr/local/bin/noema`
- stdlib em `/usr/local/share/noema/stdlib`

Com prefixo customizado:

```bash
./install.sh /opt/noema
PREFIX=/tmp/noema ./install.sh
```

O instalador nao cria diretorio global de plugins. Plugins sao extensoes do usuario e continuam fora da instalacao do sistema.
No Windows, tambem existe um instalador:

```bat
install.bat
```

Por padrao, ele instala em:

- `%LOCALAPPDATA%\Programs\Noema\bin\noema.exe`
- `%LOCALAPPDATA%\Programs\Noema\share\noema\stdlib`

Com prefixo customizado:

```bat
install.bat C:\Noema
```

`install.bat` detecta `mingw32-make` ou `make`, compila o projeto e instala apenas o compilador e a stdlib.

## Executando

```bash
./build/noema
./build/noema --repl
./build/noema --repl examples/friendly.noe
./build/noema examples/eliza.noe
./build/noema examples/mycin.noe
./build/noema examples/shrdlu.noe
```

No Windows, o equivalente e:

```bat
build\noema.exe
build\noema.exe --repl
build\noema.exe examples\friendly.noe
```

Sem argumentos, Noema abre o REPL interativo. Com `--repl` ou `-i`, ele pode carregar um script e continuar no mesmo ambiente global.

## REPL

O REPL suporta blocos multiline. Quando a entrada esta incompleta, o prompt muda para `....>`.

Comandos principais:

- `:help` ou `help`
- `:help syntax`
- `:help stdlib`
- `:help ffi`
- `:exit`, `:quit` ou `quit`
- `:reset` para descartar um bloco multiline pendente

## Biblioteca padrao

`stdlib/prelude.noe` e carregada automaticamente. Isso deixa os namespaces abaixo disponiveis desde o inicio:

- `Core`
- `IO`
- `FS`
- `Path`
- `REPL`
- `Shell`
- `Math`
- `Random`
- `Strings`
- `List`
- `Maps`
- `Fn`
- `Assert`
- `Patterns`
- `Dialogue`
- `Rules`

Exemplo:

```noema
var xs = [1, 2, 3, 4]
var doubled = List.map(xs, fn (value, index) => value * 2 + index)

IO.show("title", Strings.title_case("noema standard library"))
IO.show("sum", sum(doubled))
IO.show("clamped", Math.clamp(sum(doubled), 0, 99))
```

Para referencia por arquivo e modulo:

- [docs/stdlib.md](docs/stdlib.md)

## IA simbolica e sistemas de regras

Noema tem suporte host-side e de stdlib para esse tipo de programa.

- `Patterns` trata normalizacao de texto, wildcards e templates
- `Dialogue` trata sessoes, memoria, topico e historico conversacional
- `Rules` trata fatos, inferencia, ranking e fatores de certeza

Isso permite escrever programas no estilo:

- ELIZA
- PARRY
- ALICE/AIML-style bots
- MYCIN-like expert systems
- SHRDLU-like blocks worlds

Esses exemplos estao no repositorio como programas, nao como parte da stdlib.

## Plugins e FFI

Plugins sao escritos em Noema, mas separados da stdlib:

```noema
plugin Ncurses {
    library "libncurses.so.6"
    bind initscr() -> pointer as "initscr"
    bind endwin() -> int as "endwin"
    bind mvaddstr(int, int, string) -> int as "mvaddstr"
}
```

Depois de importar o arquivo:

```noema
import "../plugins/ncurses.noe"
Ncurses.initscr()
Ncurses.endwin()
```

Tipos FFI suportados:

- `number`
- `int`
- `bool`
- `string`
- `pointer`
- `void`

Em geral:

- Linux usa `.so`
- Windows usa `.dll`

## Documentacao

Documentacao principal:

- [docs/language.md](docs/language.md): visao geral da linguagem
- [docs/syntax.md](docs/syntax.md): sintaxe, declaracoes, macros e builtins importantes
- [docs/repl.md](docs/repl.md): CLI, REPL nativo e REPLs escritos em Noema
- [docs/ffi.md](docs/ffi.md): plugins, bibliotecas nativas e FFI
- [docs/stdlib.md](docs/stdlib.md): referencia da stdlib por arquivo e namespace
- [docs/files.md](docs/files.md): guia do repositorio e funcao de cada arquivo importante

## Estrutura do projeto

- `src/`: lexer, preprocessador, parser, runtime, builtins e FFI
- `stdlib/`: biblioteca padrao distribuida com a linguagem
- `examples/`: programas de exemplo, incluindo AIs classicas
- `plugins/`: plugins de usuario e demos de FFI
- `docs/`: documentacao
- `obj/`: objetos gerados no build
- `build/`: binarios gerados no build

Para uma descricao por arquivo, veja [docs/files.md](docs/files.md).

## Exemplos incluidos

- [exemplos/meta.noe](exemplos/meta.noe): macros e `eval(...)`
- [exemplos/eliza.noe](exemplos/eliza.noe): terapeuta por regras estilo ELIZA
- [exemplos/parry.noe](exemplos/parry.noe): entrevistado paranoico estilo PARRY
- [exemplos/alice.noe](exemplos/alice.noe): bot por categorias estilo ALICE
- [exemplos/mycin.noe](exemplos/mycin.noe): expert system com fatores de certeza
- [exemplos/shrdlu.noe](exemplos/shrdlu.noe): blocks world inspirado em SHRDLU
- [exemplos/boids.noe](exemplos/boids.noe): flocking simbolico/comportamental
- [exemplos/ncurses_demo.noe](exemplos/ncurses_demo.noe): demo de plugin `ncurses`

`ncurses_demo.noe` e especifico de Linux/Unix.

## Arquivos centrais do runtime

- `src/main.c`: CLI e REPL
- `src/lexico.c`: tokenizacao
- `src/preprocessador.c`: `import`, `macro`, `expand`
- `src/parser.c`: parser recursivo
- `src/runtime.c`: valores, escopo, execucao e resolucao da stdlib
- `src/builtins.c`: builtins da linguagem e base host-side da stdlib
- `src/ffi.c`: plugins e chamadas FFI
- `src/noema.h`: tipos e interfaces centrais

## Observacoes

- A stdlib e carregada automaticamente.
- Plugins nao sao carregados automaticamente.
- `NOEMA_STDLIB` pode ser usado para sobrescrever o diretorio da stdlib.
- O runtime e compativel com Linux e Windows.
