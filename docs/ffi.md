# Plugins e FFI em Noema

Plugins sao extensoes do usuario. Eles nao fazem parte da stdlib.

## Estrutura basica

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

## Tipos suportados

- `number`
- `int`
- `bool`
- `string`
- `pointer`
- `void`

## Resolucao de bibliotecas

Em geral:

- Linux usa `.so`
- Windows usa `.dll`

## Quando usar plugin

Use plugin quando a funcionalidade:

- depende de biblioteca nativa
- nao pertence ao nucleo da linguagem
- nao faz sentido como stdlib embarcada

Nao use plugin para coisas que ja pertencem a linguagem, como:

- math
- io basico
- strings
- listas e mapas
- paths e filesystem

## Exemplo do repositorio

- [../plugins/ncurses.noe](../plugins/ncurses.noe)
- [../examples/ncurses_demo.noe](../examples/ncurses_demo.noe)
