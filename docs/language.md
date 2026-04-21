# Linguagem Noema

`Noema` e uma linguagem de programacao interpretada com foco em:

- metaprogramacao
- IA simbolica
- expert systems
- ambientes interativos
- extensibilidade via FFI

O runtime e escrito em C. A implementacao em C usa pt-BR; a linguagem usa sintaxe em ingles.

## Modelo da linguagem

Noema combina:

- sintaxe blocada e infixa
- preprocessador para `import`
- macros em AST via `macro`, `expand` e `syntax`
- runtime dinamico
- stdlib embarcada
- REPL interativo
- plugins escritos em Noema para chamar bibliotecas nativas

## Caracteristicas centrais

- funcoes nomeadas e anonimas
- escopo lexico e fechamento
- variaveis mutaveis e constantes
- listas, mapas e strings interpoladas
- loops e controle de fluxo
- `eval(...)` e `try_eval(...)`
- macros sem s-expressions
- stdlib automatica

## Carregamento automatico da stdlib

No startup, `stdlib/prelude.noe` carrega:

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

## Areas de uso

Noema funciona bem para:

- linguagens embutidas e metaprogramacao
- chatbots por regras
- motores de fatos e inferencia
- pipelines simbolicos de texto
- ferramentas de console
- shells e REPLs customizados

## Leituras relacionadas

- [syntax.md](./syntax.md)
- [repl.md](./repl.md)
- [ffi.md](./ffi.md)
- [stdlib.md](./stdlib.md)
