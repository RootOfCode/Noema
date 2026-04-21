ifeq ($(OS),Windows_NT)
ifeq ($(origin CC), default)
CC = gcc
endif
EXEEXT ?= .exe
LDLIBS ?= -lffi -lkernel32 -lm
MKDIR_P = cmd /C if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
else
ifeq ($(origin CC), default)
CC = cc
endif
EXEEXT ?=
LDLIBS ?= -ldl -lffi -lm
MKDIR_P = mkdir -p $@
endif

CPPFLAGS ?=
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=

DIRETORIO_OBJ = obj
DIRETORIO_BUILD = build
BINARIO = $(DIRETORIO_BUILD)/noema$(EXEEXT)

FONTES = \
	src/main.c \
	src/lexico.c \
	src/preprocessador.c \
	src/parser.c \
	src/runtime.c \
	src/builtins.c \
	src/ffi.c

OBJETOS = $(patsubst src/%.c,$(DIRETORIO_OBJ)/%.o,$(FONTES))
DEPENDENCIAS = $(OBJETOS:.o=.d)

all: $(BINARIO)

noema: $(BINARIO)

$(BINARIO): $(OBJETOS) | $(DIRETORIO_BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(OBJETOS) $(LDFLAGS) $(LDLIBS)

$(DIRETORIO_OBJ)/%.o: src/%.c | $(DIRETORIO_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(DIRETORIO_OBJ) $(DIRETORIO_BUILD):
	$(MKDIR_P)

clean:
ifeq ($(OS),Windows_NT)
	cmd /C if exist "$(subst /,\,$(DIRETORIO_OBJ))" rmdir /S /Q "$(subst /,\,$(DIRETORIO_OBJ))"
	cmd /C if exist "$(subst /,\,$(DIRETORIO_BUILD))" rmdir /S /Q "$(subst /,\,$(DIRETORIO_BUILD))"
	cmd /C if exist "noema.exe" del /Q "noema.exe"
	cmd /C if exist "noema" del /Q "noema"
else
	rm -rf $(DIRETORIO_OBJ) $(DIRETORIO_BUILD) noema noema.exe
endif

-include $(DEPENDENCIAS)

.PHONY: all clean noema
