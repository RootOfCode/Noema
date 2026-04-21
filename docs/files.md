# Guia de Arquivos do Projeto

Este arquivo descreve os arquivos mais importantes do repositorio e o papel de cada um.

## Raiz do projeto

### [../README.md](../README.md)

Visao geral da linguagem, build, instalacao, execucao, stdlib, plugins e exemplos.

### [../Makefile](../Makefile)

Build principal para Linux/POSIX e Windows com MinGW.

Gera:

- objetos em `obj/`
- dependencias de headers em `obj/*.d`
- binario em `build/noema` ou `build/noema.exe`

### [../build.bat](../build.bat)

Build para Windows.

Detecta:

- `cl`
- `gcc`
- `clang`

Gera o binario em `build\noema.exe`.

### [../install.sh](../install.sh)

Instalador POSIX.

Instala:

- compilador
- stdlib

Nao instala:

- plugins

### [../install.bat](../install.bat)

Instalador para Windows.

Instala:

- `noema.exe`
- stdlib

Nao instala:

- plugins

## Runtime em C

### [../src/noema.h](../src/noema.h)

Cabecalho central do runtime. Define:

- tokens
- AST
- valores
- ambiente
- interpretador
- estruturas de FFI

### [../src/main.c](../src/main.c)

Entrada do executavel.

Responsabilidades:

- parse de argumentos CLI
- inicializacao do interpretador
- REPL nativo
- comandos `:help`, `:quit`, `:reset`

### [../src/lexico.c](../src/lexico.c)

Lexer/tokenizador da linguagem.

Responsavel por:

- identificadores
- numeros
- strings
- operadores
- palavras-chave

### [../src/preprocessador.c](../src/preprocessador.c)

Preprocessador de Noema.

Responsavel por:

- `import`
- montagem do codigo apos resolver imports

### [../src/parser.c](../src/parser.c)

Parser recursivo que converte o codigo processado em AST.

### [../src/runtime.c](../src/runtime.c)

Runtime principal.

Responsavel por:

- execucao da AST
- expansao de macros em AST
- escopo e ambientes
- valores e coercoes
- descoberta da stdlib
- resolucao de imports

### [../src/builtins.c](../src/builtins.c)

Builtins da linguagem e ponte host-side para a stdlib.

Responsavel por:

- tipos e conversoes
- colecoes
- strings
- math
- arquivos e paths
- shell
- primitivas para `Patterns`, `Dialogue` e `Rules`

### [../src/ffi.c](../src/ffi.c)

Camada de plugins/FFI.

Responsavel por:

- carregar bibliotecas dinamicas com fallbacks
- resolver simbolos
- preparar chamadas `libffi`
- viabilizar plugins com buffers e structs simples em conjunto com os helpers de memoria

## Stdlib

Todos os arquivos abaixo ficam em [../stdlib/](../stdlib).

### [../stdlib/prelude.noe](../stdlib/prelude.noe)

Ponto de entrada da stdlib. Carrega os modulos e expoe `Std`.

### [../stdlib/core.noe](../stdlib/core.noe)

Helpers basicos, tipos, conversoes e utilitarios de memoria para FFI.

### [../stdlib/io.noe](../stdlib/io.noe)

Console, prompts e entrada do usuario.

### [../stdlib/fs.noe](../stdlib/fs.noe)

Arquivos e diretorios.

### [../stdlib/path.noe](../stdlib/path.noe)

Operacoes de caminho.

### [../stdlib/repl.noe](../stdlib/repl.noe)

Construcao de REPLs na propria linguagem.

### [../stdlib/shell.noe](../stdlib/shell.noe)

Construcao de shells simples em Noema.

### [../stdlib/math.noe](../stdlib/math.noe)

Constantes e funcoes matematicas.

### [../stdlib/random.noe](../stdlib/random.noe)

Aleatoriedade, amostragem e inteiros aleatorios.

### [../stdlib/strings.noe](../stdlib/strings.noe)

Transformacoes e utilitarios de texto.

### [../stdlib/lists.noe](../stdlib/lists.noe)

Operacoes com listas.

### [../stdlib/maps.noe](../stdlib/maps.noe)

Operacoes com mapas.

### [../stdlib/functional.noe](../stdlib/functional.noe)

Helpers funcionais.

### [../stdlib/assert.noe](../stdlib/assert.noe)

Assercoes e falhas controladas.

### [../stdlib/patterns.noe](../stdlib/patterns.noe)

Matching simbolico, templates e reflexao de texto.

### [../stdlib/dialogue.noe](../stdlib/dialogue.noe)

Sessoes, memoria e bots por regras.

### [../stdlib/rules.noe](../stdlib/rules.noe)

Motores de fatos, inferencia e certeza.

## Plugins

### [../plugins/ncurses.noe](../plugins/ncurses.noe)

Plugin de exemplo para `ncurses`.

Mostra:

- bibliotecas fallback em `library`
- `bind` com resolucao implicita do simbolo quando o nome local ja coincide

Plugin de exemplo para `ncurses`.

Observacao:

- e um plugin de usuario
- nao faz parte da stdlib
- e especifico de Linux/Unix

### [../plugins/sdl2.noe](../plugins/sdl2.noe)

Plugin de exemplo para `SDL2`.

Mostra:

- bibliotecas fallback multiplataforma
- funcoes de janela, renderer e polling de eventos
- constantes e helpers adicionados ao namespace do plugin
- empacotamento de `SDL_Event` e `SDL_Rect` com os helpers de memoria

## Exemplos

Todos os arquivos abaixo ficam em [../exemplos/](../exemplos).

### [../exemplos/friendly.noe](../exemplos/friendly.noe)

Exemplo pequeno da sintaxe atual e da stdlib geral.

### [../exemplos/meta.noe](../exemplos/meta.noe)

Macros, expansao de codigo e `eval`.

### [../exemplos/eliza.noe](../exemplos/eliza.noe)

REPL conversacional inspirado em ELIZA.

### [../exemplos/parry.noe](../examplos/parry.noe)

REPL conversacional inspirado em PARRY.

### [../exemplos/alice.noe](../exemplos/alice.noe)

Bot por categorias no estilo ALICE/AIML.

### [../exemplos/mycin.noe](../exemplos/mycin.noe)

Expert system com fatores de certeza no estilo MYCIN.

### [../exemplos/shrdlu.noe](../exemplos/shrdlu.noe)

Blocks world inspirado em SHRDLU.

### [../exemplos/boids.noe](../exemplos/boids.noe)

Demo de flocking e simulacao simples.

### [../exemplos/ncurses_demo.noe](../exemplos/ncurses_demo.noe)

Demo isolada do plugin `ncurses`.

### [../exemplos/sdl2_demo.noe](../exemplos/sdl2_demo.noe)

Demo de janela/render loop com `SDL2`.

Mostra:

- init e shutdown de SDL2
- polling de eventos com buffer nativo
- renderizacao 2D com `SDL_RenderFillRect`

## Documentacao

### [./language.md](./language.md)

Visao geral da linguagem e do seu posicionamento.

### [./syntax.md](./syntax.md)

Sintaxe, declaracoes, macros e builtins principais.

### [./repl.md](./repl.md)

CLI, REPL nativo e REPLs implementados em Noema.

### [./ffi.md](./ffi.md)

Plugins, bibliotecas nativas e FFI.

### [./stdlib.md](./stdlib.md)

Referencia da stdlib por modulo e arquivo.

### [./files.md](./files.md)

Este guia de estrutura do projeto.
