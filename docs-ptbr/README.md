# Noema

`Noema` é uma linguagem de programação interpretada, com sintaxe em inglês, runtime em C e foco em metaprogramação, IA simbólica, sistemas baseados em regras e desenvolvimento interativo.

O código C do projeto foi escrito em pt-BR. A linguagem, a stdlib, os exemplos e os plugins usam sintaxe em inglês.

## O que Noema é

Noema não é apenas uma DSL. Ela é uma linguagem completa:

- funções nomeadas e anônimas
- escopo léxico, fechamento e recursão
- variáveis mutáveis e constantes
- `if`, `when`, `unless`, `while`, `for`, `each`, `return`, `break`, `continue`
- listas, mapas, strings com interpolação e trailing commas
- macros por expansão de código
- `eval(...)` e `try_eval(...)`
- stdlib embarcada e carregada automaticamente
- REPL interativo e shells/REPLs construídos na própria linguagem
- plugins escritos em Noema com FFI para bibliotecas C

Na prática, Noema serve tanto para programação geral quanto para prototipação de linguagens, agentes simbólicos, chatbots clássicos, motores de regras, expert systems e ambientes interativos.

## Direção da linguagem

Os pontos centrais de Noema são:

- metaprogramação sem s-expressions
- runtime pequeno e hackeável em C
- stdlib orientada à linguagem, não a demos ad-hoc
- ergonomia para escrever IA simbólica e ferramentas interativas
- extensibilidade via plugins do usuário

## Exemplo rápido

```noema
macro unless_do(cond, body) {
    return syntax if not $cond { $body }
}

macro make_metric(name, expr) {
    return syntax_function(syntax_text(name), ["values"], syntax {
        return $expr
    })
}

var data = [3, 5, 8, 13]

expand make_metric(total_metric, sum(values))
expand make_metric(mean_metric, sum(values) / len(values))
expand unless_do(len(data) == 0, {
    IO.puts("dataset ready")
})

IO.show("total", total_metric(data))
IO.show("mean", mean_metric(data))
```

## Recursos principais

- sintaxe blocada e infixa
- aliases amigáveis como `fn`, `var`, `use`, `when`, `unless`, `each`, `elif`
- semicolon opcional quando a instrução já termina na linha
- números aceitam decimal e hexadecimal com prefixo `0x`
- macros em AST: `macro name(args) { return syntax ... }`
- `expand name(...)` aceita formas completas, incluindo blocos
- `syntax ...`, `$name` e `$(expr)` para montar código
- introspecção e construção de sintaxe com `syntax_kind`, `syntax_parts`, `syntax_function` e afins
- `eval(...)` para geração dinâmica de código
- `try_eval(...)` para avaliação protegida no runtime
- FFI com `libffi` e carregamento dinâmico de bibliotecas
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

ou, se o executável do GNU Make ainda estiver com o nome padrão do MinGW:

```bat
mingw32-make
```

Também existe o atalho:

```bat
build.bat
```

Isso gera:

- `obj\*.obj` ou `obj\*.o`
- `build\noema.exe`

O `Makefile` funciona tanto em Linux quanto em Windows com MinGW.
`build.bat` continua disponível como atalho e fallback para Windows.

Se o `libffi` não estiver em um caminho padrão do compilador no Windows:

```bat
set LIBFFI_INCLUDE=C:\caminho\para\libffi\include
set LIBFFI_LIB=C:\caminho\para\libffi\lib
build.bat
```

## Instalação

Em ambientes POSIX:

```bash
./install.sh
```

Por padrão:

- compilador em `/usr/local/bin/noema`
- stdlib em `/usr/local/share/noema/stdlib`

Com prefixo customizado:

```bash
./install.sh /opt/noema
PREFIX=/tmp/noema ./install.sh
```

O instalador não cria diretório global de plugins. Plugins são extensões do usuário e continuam fora da instalação do sistema.

No Windows, também existe um instalador:

```bat
install.bat
```

Por padrão, ele instala em:

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
./build/noema --repl exemplos/friendly.noe
./build/noema exemplos/eliza.noe
./build/noema exemplos/mycin.noe
./build/noema exemplos/shrdlu.noe
```

No Windows, o equivalente é:

```bat
build\noema.exe
build\noema.exe --repl
build\noema.exe exemplos\friendly.noe
```

Sem argumentos, Noema abre o REPL interativo. Com `--repl` ou `-i`, ele pode carregar um script e continuar no mesmo ambiente global.

## REPL

O REPL suporta blocos multiline. Quando a entrada está incompleta, o prompt muda para `....>`.

Comandos principais:

- `:help` ou `help`
- `:help syntax`
- `:help stdlib`
- `:help ffi`
- `:exit`, `:quit` ou `quit`
- `:reset` para descartar um bloco multiline pendente

## Biblioteca padrão

`stdlib/prelude.noe` é carregada automaticamente. Isso deixa os namespaces abaixo disponíveis desde o início:

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

Para referência por arquivo e módulo:

- [stdlib.md](stdlib.md)

## IA simbólica e sistemas de regras

Noema tem suporte host-side e de stdlib para esse tipo de programa.

- `Patterns` trata normalização de texto, wildcards e templates
- `Dialogue` trata sessões, memória, tópico e histórico conversacional
- `Rules` trata fatos, inferência, ranking e fatores de certeza

Isso permite escrever programas no estilo:

- ELIZA
- PARRY
- ALICE/AIML-style bots
- MYCIN-like expert systems
- SHRDLU-like blocks worlds

Esses exemplos estão no repositório como programas, não como parte da stdlib.

## Plugins e FFI

Plugins são escritos em Noema, mas separados da stdlib:

```noema
plugin Ncurses {
    library "libncurses.so.6", "libncursesw.so.6", "libncurses.so"
    bind initscr() -> pointer
    bind endwin() -> int
    bind mvaddstr(int, int, string) -> int
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

O runtime tenta as bibliotecas na ordem declarada em `library`.
Se o nome local da função já for igual ao símbolo nativo, `as "..."` é opcional.
Use `as` apenas quando precisar expor um alias diferente.
Para structs, buffers e parâmetros de saída, use os helpers de memória:
`memory_alloc`, `memory_free`, `memory_fill`, `memory_get_u8`, `memory_get_i32`,
`memory_get_u32`, `memory_set_u8`, `memory_set_i32` e `memory_set_u32`.

## Documentação

- [language.md](language.md): visão geral da linguagem
- [syntax.md](syntax.md): sintaxe, declarações, macros e builtins importantes
- [repl.md](repl.md): CLI, REPL nativo e REPLs escritos em Noema
- [ffi.md](ffi.md): plugins, bibliotecas nativas e FFI
- [stdlib.md](stdlib.md): referência da stdlib por arquivo e namespace
- [files.md](files.md): guia do repositório e função de cada arquivo importante

## Estrutura do projeto

- `src/`: lexer, preprocessador, parser, runtime, builtins e FFI
- `stdlib/`: biblioteca padrão distribuída com a linguagem
- `exemplos/`: programas de exemplo, incluindo AIs clássicas
- `plugins/`: plugins de usuário e demos de FFI
- `docs-ptbr/`: esta documentação em português
- `docs-en/`: documentação em inglês
- `obj/`: objetos gerados no build
- `build/`: binários gerados no build

Para uma descrição por arquivo, veja [files.md](files.md).

## Exemplos incluídos

- [../exemplos/meta.noe](../exemplos/meta.noe): macros AST, `syntax` e introspecção
- [../exemplos/eliza.noe](../exemplos/eliza.noe): terapeuta por regras estilo ELIZA
- [../exemplos/parry.noe](../exemplos/parry.noe): entrevistado paranoico estilo PARRY
- [../exemplos/alice.noe](../exemplos/alice.noe): bot por categorias estilo ALICE
- [../exemplos/mycin.noe](../exemplos/mycin.noe): expert system com fatores de certeza
- [../exemplos/shrdlu.noe](../exemplos/shrdlu.noe): blocks world inspirado em SHRDLU
- [../exemplos/boids.noe](../exemplos/boids.noe): flocking simbólico/comportamental
- [../exemplos/ncurses_demo.noe](../exemplos/ncurses_demo.noe): demo de plugin `ncurses`
- [../exemplos/sdl2_demo.noe](../exemplos/sdl2_demo.noe): janela e render loop com plugin `SDL2`

`ncurses_demo.noe` é específico de Linux/Unix.

## Arquivos centrais do runtime

- `src/main.c`: CLI e REPL
- `src/lexico.c`: tokenização
- `src/preprocessador.c`: `import` e montagem do código fonte
- `src/parser.c`: parser recursivo, `macro`, `expand` e `syntax`
- `src/runtime.c`: valores, escopo, execução, expansão de macros e resolução da stdlib
- `src/builtins.c`: builtins da linguagem e base host-side da stdlib
- `src/ffi.c`: plugins e chamadas FFI
- `src/noema.h`: tipos e interfaces centrais

## Observações

- A stdlib é carregada automaticamente.
- Plugins não são carregados automaticamente.
- `NOEMA_STDLIB` pode ser usado para sobrescrever o diretório da stdlib.
- O runtime é compatível com Linux e Windows.
