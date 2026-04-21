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
- definicao de `macro`
- `expand`
- expansao de placeholders com `$`

### [../src/parser.c](../src/parser.c)

Parser recursivo que converte o codigo processado em AST.

### [../src/runtime.c](../src/runtime.c)

Runtime principal.

Responsavel por:

- execucao da AST
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

- carregar bibliotecas dinamicas
- resolver simbolos
- preparar chamadas `libffi`

## Stdlib

Todos os arquivos abaixo ficam em [../stdlib/](../stdlib).

### [../stdlib/prelude.noe](../stdlib/prelude.noe)

Ponto de entrada da stdlib. Carrega os modulos e expoe `Std`.

### [../stdlib/core.noe](../stdlib/core.noe)

Helpers basicos, tipos e conversoes.

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

Observacao:

- e um plugin de usuario
- nao faz parte da stdlib
- e especifico de Linux/Unix

## Exemplos

Todos os arquivos abaixo ficam em [../examples/](../examples).

### [../examples/friendly.noe](../examples/friendly.noe)

Exemplo pequeno da sintaxe atual e da stdlib geral.

### [../examples/meta.noe](../examples/meta.noe)

Macros, expansao de codigo e `eval`.

### [../examples/eliza.noe](../examples/eliza.noe)

REPL conversacional inspirado em ELIZA.

### [../examples/parry.noe](../examples/parry.noe)

REPL conversacional inspirado em PARRY.

### [../examples/alice.noe](../examples/alice.noe)

Bot por categorias no estilo ALICE/AIML.

### [../examples/mycin.noe](../examples/mycin.noe)

Expert system com fatores de certeza no estilo MYCIN.

### [../examples/shrdlu.noe](../examples/shrdlu.noe)

Blocks world inspirado em SHRDLU.

### [../examples/boids.noe](../examples/boids.noe)

Demo de flocking e simulacao simples.

### [../examples/ncurses_demo.noe](../examples/ncurses_demo.noe)

Demo isolada do plugin `ncurses`.

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
