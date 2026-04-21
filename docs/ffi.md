# Plugins e FFI em Noema

Plugins sao extensoes do usuario. Eles nao fazem parte da stdlib.

## Estrutura basica

```noema
plugin Ncurses {
    library "libncurses.so.6", "libncursesw.so.6", "libncurses.so"
    bind initscr() -> pointer
    bind endwin() -> int
    bind mvaddstr(int, int, string) -> int
    bind wgetch(pointer) -> int as "wgetch"
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

O runtime tenta as bibliotecas na ordem declarada.
Isso permite escrever plugins portaveis com fallbacks:

```noema
plugin HostMath {
    library "libm.so.6", "libm.so", "msvcrt.dll"
    bind cos(number) -> number
}
```

Depois do carregamento, o namespace do plugin expoe `loaded_library` com a biblioteca efetivamente usada.

## Resolucao de simbolos

Quando o nome local e igual ao nome exportado pela biblioteca, `as "..."` e opcional:

```noema
bind cos(number) -> number
```

Use `as` apenas quando precisar de alias:

```noema
bind sleep_ms(int) -> void as "Sleep"
```

## Structs e buffers

Bibliotecas C frequentemente usam structs, buffers mutaveis e parametros de saida.
Para esse caso, Noema expoe helpers host-side de memoria:

- `memory_alloc(size)`
- `memory_free(pointer)`
- `memory_fill(pointer, byte, size)`
- `memory_get_u8(pointer, offset)`
- `memory_get_i32(pointer, offset)`
- `memory_get_u32(pointer, offset)`
- `memory_set_u8(pointer, offset, value)`
- `memory_set_i32(pointer, offset, value)`
- `memory_set_u32(pointer, offset, value)`

Exemplo resumido com `SDL_Event`:

```noema
plugin SDL2 {
    library "libSDL2-2.0.so.0", "libSDL2.so", "SDL2.dll"
    bind poll_event(pointer) -> int as "SDL_PollEvent"
}

SDL2.EVENT_SIZE = 56

function sdl2_make_event() {
    return memory_alloc(SDL2.EVENT_SIZE)
}

function sdl2_event_type(event) {
    return memory_get_u32(event, 0)
}

SDL2.make_event = sdl2_make_event
SDL2.event_type = sdl2_event_type
```

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
- [../plugins/sdl2.noe](../plugins/sdl2.noe)
- [../examples/sdl2_demo.noe](../examples/sdl2_demo.noe)
