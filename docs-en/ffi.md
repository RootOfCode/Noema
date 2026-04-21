# Plugins and FFI

Plugins are user-written extensions in Noema that call native libraries via `libffi`. They are not part of the stdlib and are not loaded automatically.

## Basic plugin structure

```noema
plugin Ncurses {
    library "libncurses.so.6", "libncursesw.so.6", "libncurses.so"
    bind initscr() -> pointer
    bind endwin() -> int
    bind mvaddstr(int, int, string) -> int
    bind wgetch(pointer) -> int
}
```

After importing the plugin file:

```noema
import "../plugins/ncurses.noe"
Ncurses.initscr()
Ncurses.mvaddstr(0, 0, "hello from Noema")
Ncurses.endwin()
```

## Supported FFI types

| FFI type | C equivalent |
|----------|-------------|
| `number` | double |
| `int` | platform integer |
| `bool` | integer 0/1 |
| `string` | const char* (read-only) |
| `pointer` | opaque void* |
| `void` | no return value |

## Library resolution

The runtime tries each name in the `library` list in order and uses the first one that opens successfully. This lets you write portable plugins with platform fallbacks:

```noema
plugin HostMath {
    library "libm.so.6", "libm.so", "msvcrt.dll"
    bind cos(number) -> number
    bind sin(number) -> number
    bind sqrt(number) -> number
}
```

Linux generally uses `.so`, Windows uses `.dll`.

After loading, the plugin namespace exposes `loaded_library` with the name of the library that was actually used:

```noema
import "../plugins/hostmath.noe"
IO.puts(HostMath.loaded_library)
```

## Symbol resolution

When the local function name matches the exported symbol exactly, `as "..."` is optional:

```noema
bind cos(number) -> number          // implicit: as "cos"
```

Use `as` only when you need a different alias:

```noema
bind sleep_ms(int) -> void as "Sleep"
bind poll_event(pointer) -> int as "SDL_PollEvent"
```

## Structs and buffers

C libraries frequently use structs by reference, mutable buffers, and output parameters. Noema exposes raw memory helpers for these cases:

| Builtin | Description |
|---------|-------------|
| `memory_alloc(size)` | allocates `size` bytes, returns a pointer |
| `memory_free(ptr)` | frees the pointer |
| `memory_fill(ptr, byte, size)` | fills `size` bytes with `byte` |
| `memory_get_u8(ptr, offset)` | reads unsigned byte at offset |
| `memory_get_i32(ptr, offset)` | reads signed int32 at offset |
| `memory_get_u32(ptr, offset)` | reads unsigned int32 at offset |
| `memory_set_u8(ptr, offset, v)` | writes byte at offset |
| `memory_set_i32(ptr, offset, v)` | writes int32 at offset |
| `memory_set_u32(ptr, offset, v)` | writes uint32 at offset |

### Example: SDL_Event and SDL_Rect

```noema
plugin SDL2 {
    library "libSDL2-2.0.so.0", "libSDL2.so", "SDL2.dll"
    bind init(int) -> int as "SDL_Init"
    bind quit() -> void as "SDL_Quit"
    bind create_window(string, int, int, int, int, int) -> pointer as "SDL_CreateWindow"
    bind create_renderer(pointer, int, int) -> pointer as "SDL_CreateRenderer"
    bind poll_event(pointer) -> int as "SDL_PollEvent"
    bind render_fill_rect(pointer, pointer) -> int as "SDL_RenderFillRect"
    bind render_present(pointer) -> void as "SDL_RenderPresent"
    bind delay(int) -> void as "SDL_Delay"
}

SDL2.QUIT = 0x100
SDL2.EVENT_SIZE = 56
SDL2.RECT_SIZE  = 16

function sdl2_make_event() {
    return memory_alloc(SDL2.EVENT_SIZE)
}

function sdl2_event_type(event) {
    return memory_get_u32(event, 0)
}

function sdl2_make_rect(x, y, w, h) {
    var rect = memory_alloc(SDL2.RECT_SIZE)
    memory_set_i32(rect, 0, x)
    memory_set_i32(rect, 4, y)
    memory_set_i32(rect, 8, w)
    memory_set_i32(rect, 12, h)
    return rect
}

SDL2.make_event = sdl2_make_event
SDL2.event_type = sdl2_event_type
SDL2.make_rect  = sdl2_make_rect
SDL2.free       = memory_free
```

## Adding constants and helpers to the namespace

After declaring a plugin, you can add fields to its namespace just like any map:

```noema
SDL2.INIT_VIDEO           = 0x00000020
SDL2.WINDOW_SHOWN         = 0x00000004
SDL2.RENDERER_ACCELERATED = 0x00000002
```

## When to use a plugin

Use a plugin when functionality:

- depends on a native library
- does not belong in the language core
- does not make sense as an embedded stdlib module

Do not use a plugin for things already in the stdlib, such as math, basic IO, strings, lists, maps, paths, and filesystem operations.

## Repository plugins

- `plugins/ncurses.noe` — ncurses TUI (Linux/Unix only)
- `plugins/sdl2.noe` — window, renderer, and event polling via SDL2

To run the corresponding examples:

```bash
./build/noema examples/ncurses_demo.noe   # Linux/Unix
./build/noema examples/sdl2_demo.noe
```

## Related reading

- [language.md](./language.md)
- [syntax.md](./syntax.md)
- [stdlib.md](./stdlib.md)
