# Guia de Arquivos do Projeto

Este arquivo descreve os arquivos mais importantes do repositório e o papel de cada um.

## Raiz do projeto

| Arquivo | Função |
|---------|--------|
| `README.md` | Hub de navegação — links para docs-ptbr e docs-en |
| `Makefile` | Build para Linux/POSIX e Windows com MinGW |
| `build.bat` | Build alternativo para Windows |
| `install.sh` | Instalador POSIX |
| `install.bat` | Instalador Windows |
| `LICENSE` | Licença do projeto |

### Makefile

Gera:
- objetos em `obj/`
- dependências de headers em `obj/*.d`
- binário em `build/noema` (Linux) ou `build/noema.exe` (Windows)

Funciona tanto em Linux quanto em Windows com MinGW. No Windows, pode-se chamar `make` ou `mingw32-make`.

### install.sh

Instala por padrão em:
- `/usr/local/bin/noema`
- `/usr/local/share/noema/stdlib`

Com prefixo customizado:
```bash
./install.sh /opt/noema
PREFIX=/tmp/noema ./install.sh
```

Não instala plugins — plugins são extensões do usuário.

### install.bat

Instala por padrão em:
- `%LOCALAPPDATA%\Programs\Noema\bin\noema.exe`
- `%LOCALAPPDATA%\Programs\Noema\share\noema\stdlib`

Com prefixo customizado:
```bat
install.bat C:\Noema
```

---

## Runtime em C — `src/`

### `src/noema.h`

Cabeçalho central do runtime. Define:

- tokens (`TipoToken`)
- AST (`TipoNo`, `No`, `Programa`)
- valores (`TipoValor`, `Valor`, `Lista`, `Mapa`)
- ambiente (`Ambiente`, `EntradaAmbiente`)
- interpretador (`Interpretador`)
- estruturas de FFI (`LigacaoPlugin`, `TipoFfi`)
- sinal de execução (`SinalExecucao`, `ResultadoExecucao`)
- assinaturas de todas as funções públicas

### `src/main.c`

Ponto de entrada do executável. Responsabilidades:

- parse de argumentos CLI (`--repl`, `-i`, caminhos de arquivo)
- inicialização do interpretador e stdlib
- loop do REPL nativo com suporte a blocos multiline
- comandos `:help`, `:help syntax`, `:help stdlib`, `:help ffi`, `:quit`, `:reset`
- detecção automática de blocos incompletos (prompt `....>`)

### `src/lexico.c`

Lexer/tokenizador. Produz tokens para:

- identificadores e palavras-chave
- números (decimais e hexadecimais `0x`)
- strings (com interpolação `#{}` e strings brutas com backtick)
- operadores e pontuação

### `src/preprocessador.c`

Preprocessador de Noema. Responsabilidades:

- resolução de `import "caminho.noe"`
- prevenção de importações duplicadas
- montagem do código-fonte final antes do parse

### `src/parser.c`

Parser recursivo descendente. Converte o código em AST (`Programa` de `No`). Lida com:

- todas as declarações e expressões da linguagem
- `macro`, `expand` e `syntax`
- placeholders `$name` e `$(expr)`

### `src/runtime.c`

Runtime principal. Responsabilidades:

- execução da AST
- expansão de macros em tempo de execução
- criação e gerenciamento de ambientes (escopos)
- valores e coerções
- descoberta automática do diretório da stdlib
- resolução e carregamento de imports

### `src/builtins.c`

Builtins da linguagem e ponte host-side para a stdlib. Registra:

- conversão e inspeção de tipos
- operações de coleções (`len`, `get`, `push`, `pop`, `map`, `filter`, etc.)
- operações de strings
- math embutido
- arquivos e paths
- shell
- primitivas host-side para `Patterns`, `Dialogue` e `Rules`
- builtins de sintaxe (`syntax_kind`, `syntax_parts`, `syntax_function`, etc.)

### `src/ffi.c`

Camada de plugins/FFI. Responsabilidades:

- carregamento de bibliotecas dinâmicas com fallbacks (`dlopen` / `LoadLibrary`)
- resolução de símbolos
- preparação de chamadas via `libffi`
- integração com os helpers de memória para structs e buffers

---

## Stdlib — `stdlib/`

Todos os arquivos ficam em `stdlib/`.

| Arquivo | Namespace | Conteúdo |
|---------|-----------|----------|
| `prelude.noe` | `Std` | Ponto de entrada — carrega todos os módulos |
| `core.noe` | `Core` | Tipos, conversões, helpers e memória FFI |
| `io.noe` | `IO` | Console, prompts e entrada |
| `fs.noe` | `FS` | Arquivos e diretórios |
| `path.noe` | `Path` | Operações de caminho |
| `repl.noe` | `REPL` | Construção de REPLs |
| `shell.noe` | `Shell` | Construção de shells |
| `math.noe` | `Math` | Constantes e funções matemáticas |
| `random.noe` | `Random` | Aleatoriedade e amostragem |
| `strings.noe` | `Strings` | Transformações de texto |
| `lists.noe` | `List` | Operações com listas |
| `maps.noe` | `Maps` | Operações com mapas |
| `functional.noe` | `Fn` | Helpers funcionais |
| `assert.noe` | `Assert` | Asserções |
| `patterns.noe` | `Patterns` | Matching simbólico e templates |
| `dialogue.noe` | `Dialogue` | Sessões conversacionais |
| `rules.noe` | `Rules` | Fatos, inferência e certeza |

---

## Plugins — `plugins/`

Plugins são extensões do usuário. Não fazem parte da stdlib e não são carregados automaticamente.

| Arquivo | Descrição |
|---------|-----------|
| `plugins/ncurses.noe` | TUI nativa para Linux/Unix |
| `plugins/sdl2.noe` | Janela, renderer e eventos SDL2 |

---

## Exemplos — `exemplos/`

| Arquivo | Descrição |
|---------|-----------|
| `exemplos/friendly.noe` | Demo pequena da sintaxe e stdlib |
| `exemplos/meta.noe` | Macros, expansão de código e `eval` |
| `exemplos/eliza.noe` | Terapeuta por regras no estilo ELIZA |
| `exemplos/parry.noe` | Entrevistado paranoico no estilo PARRY |
| `exemplos/alice.noe` | Bot por categorias no estilo ALICE/AIML |
| `exemplos/mycin.noe` | Sistema especialista com fatores de certeza |
| `exemplos/shrdlu.noe` | Blocks world inspirado em SHRDLU |
| `exemplos/boids.noe` | Flocking simbólico/comportamental |
| `exemplos/ncurses_demo.noe` | Demo do plugin ncurses (Linux/Unix) |
| `exemplos/sdl2_demo.noe` | Janela e render loop com SDL2 |

---

## Documentação — `docs-ptbr/` e `docs-en/`

| Arquivo | Conteúdo |
|---------|----------|
| `README.md` | Hub da documentação neste idioma |
| `language.md` | Visão geral e posicionamento da linguagem |
| `syntax.md` | Sintaxe completa, macros e builtins |
| `repl.md` | CLI, REPL nativo e REPLs em Noema |
| `ffi.md` | Plugins, bibliotecas nativas e FFI |
| `stdlib.md` | Referência completa da stdlib |
| `files.md` | Este guia |

---

## Diretórios gerados pelo build

| Diretório | Conteúdo |
|-----------|----------|
| `obj/` | Arquivos objeto (`.o`, `.d`) |
| `build/` | Binário final (`noema` ou `noema.exe`) |
