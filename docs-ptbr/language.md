# Linguagem Noema

Noema é uma linguagem de programação interpretada, com runtime em C, voltada para:

- metaprogramação sem s-expressions
- IA simbólica e sistemas especialistas
- sistemas baseados em regras
- ferramentas interativas e REPLs customizados
- extensibilidade via FFI

O código C do runtime usa pt-BR. A linguagem, a stdlib, os exemplos e os plugins usam sintaxe em inglês.

## Modelo da linguagem

Noema combina:

- sintaxe blocada e infixa com semicolons opcionais
- preprocessador para `import` e resolução de módulos
- macros em AST via `macro`, `expand` e `syntax`
- runtime dinâmico com escopo léxico e fechamentos
- stdlib embarcada e carregada automaticamente
- REPL interativo com suporte a blocos multiline
- plugins escritos em Noema para chamar bibliotecas C nativas via libffi

## Características centrais

A linguagem oferece:

- funções nomeadas e anônimas (`function`, `fn`)
- escopo léxico com fechamento e recursão
- variáveis mutáveis (`var`, `let`) e constantes (`const`)
- listas, mapas e strings com interpolação (`"#{expr}"`)
- trailing commas em listas e mapas
- `if`, `elif`, `else`, `unless`, `when`, `while`, `for`, `each`
- `return`, `break`, `continue`
- números decimais e hexadecimais (`0x`)
- `eval(...)` para geração dinâmica de código
- `try_eval(...)` para avaliação protegida que não lança erro
- macros que operam diretamente na AST
- FFI com `libffi` e carregamento dinâmico de bibliotecas

## Carregamento automático da stdlib

No startup, `stdlib/prelude.noe` é carregada automaticamente. Isso disponibiliza os seguintes namespaces no ambiente global sem nenhum `import` manual:

`Core`, `IO`, `FS`, `Path`, `REPL`, `Shell`, `Math`, `Random`, `Strings`, `List`, `Maps`, `Fn`, `Assert`, `Patterns`, `Dialogue`, `Rules`

O namespace agregado `Std` também fica disponível com todos os módulos sob `Std.Core`, `Std.IO`, etc.

## Áreas de uso

Noema funciona bem para:

- linguagens embutidas e prototipação de linguagens
- chatbots por regras no estilo ELIZA, PARRY, ALICE
- motores de fatos e inferência no estilo MYCIN
- pipelines simbólicos de texto
- ferramentas de console e shells customizados
- REPLs construídos na própria linguagem
- simulações comportamentais simples

## Leituras relacionadas

- [syntax.md](./syntax.md) — sintaxe completa
- [repl.md](./repl.md) — CLI e REPL
- [ffi.md](./ffi.md) — plugins e FFI
- [stdlib.md](./stdlib.md) — referência da stdlib
- [files.md](./files.md) — guia do repositório
