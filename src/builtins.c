#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "noema.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
typedef struct _stat NoemaStat;
#define noema_stat _stat
#else
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
typedef struct stat NoemaStat;
#define noema_stat stat
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#endif

static Valor builtin_print(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_write(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_flush(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_read_line(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_read_stdin(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_panic(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_type(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_null(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_bool(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_number(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_string(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_list(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_map(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_function(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_pointer(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_syntax(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_kind(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_text(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_parts(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_identifier(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_literal(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_block(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_let(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_const(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_syntax_function(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_alloc(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_free(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_fill(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_get_u8(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_get_i32(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_get_u32(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_set_u8(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_set_i32(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_memory_set_u32(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_string(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_number(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_int(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_bool(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_parse_number(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_parse_int(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_len(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_push(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_pop(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_keys(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_values(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_get(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_has(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_range(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_map(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_filter(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_reduce(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_find(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_any(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_all(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_join(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_split(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_slice(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_reverse(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_sum(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_starts_with(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_ends_with(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_contains(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_index_of(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_last_index_of(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_repeat(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_char_at(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_ord(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_chr(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_lower(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_upper(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_trim(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_trim_left(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_trim_right(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_replace(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_normalize_text(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_match_pattern(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_render_template(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_rand(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_rand_int(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_choice(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_eval(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_try_eval(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_sin(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_cos(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_tan(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_asin(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_acos(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_atan(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_atan2(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_sqrt(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_hypot(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_abs(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_floor(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_ceil(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_round(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_trunc(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_pow(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_mod(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_sign(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_min(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_max(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_log(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_exp(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_nan(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_finite(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_read_file(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_write_file(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_append_file(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_file_exists(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_file(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_is_dir(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_list_dir(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_make_dir(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_cwd(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_change_dir(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_shell_run(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_path_join(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_path_dirname(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_path_basename(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_path_extname(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_path_stem(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);

static Valor builtin_make_agent(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_add_pattern(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_set_fallback(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_respond(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_remember(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_recall(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);

static Valor builtin_expert(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_set_fact(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_get_fact(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_add_rule(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_run_rules(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_reset_rules(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_trace(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);

static Valor builtin_world(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_place(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_where(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_objects_on(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);

static Valor builtin_flock(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_add_boid(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
static Valor builtin_step_flock(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);

static void garantir_argumentos(const char *nome, int argc, int minimo, int maximo) {
    if (argc < minimo || argc > maximo) {
        noema_falhar("builtin '%s' esperava entre %d e %d argumentos e recebeu %d", nome, minimo, maximo, argc);
    }
}

static Lista *esperar_lista(const char *nome, Valor valor) {
    if (valor.tipo != VALOR_LISTA) {
        noema_falhar("builtin '%s' exige list", nome);
    }
    return valor.como.lista;
}

static Mapa *esperar_mapa(const char *nome, Valor valor) {
    if (valor.tipo != VALOR_MAPA) {
        noema_falhar("builtin '%s' exige map", nome);
    }
    return valor.como.mapa;
}

static const char *esperar_texto(const char *nome, Valor valor) {
    if (valor.tipo != VALOR_TEXTO) {
        noema_falhar("builtin '%s' exige string", nome);
    }
    return valor.como.texto;
}

static void *esperar_ponteiro(const char *nome, Valor valor) {
    if (valor.tipo == VALOR_NULO) {
        return NULL;
    }
    if (valor.tipo != VALOR_PONTEIRO) {
        noema_falhar("builtin '%s' exige pointer", nome);
    }
    return valor.como.ponteiro;
}

static No *esperar_sintaxe(const char *nome, Valor valor) {
    if (valor.tipo != VALOR_SINTAXE) {
        noema_falhar("builtin '%s' exige syntax", nome);
    }
    return valor.como.sintaxe;
}

static No *builtin_novo_no(TipoNo tipo, int linha) {
    No *no = calloc(1, sizeof(No));
    if (no == NULL) {
        noema_falhar("memoria insuficiente");
    }
    no->tipo = tipo;
    no->linha = linha;
    return no;
}

static void builtin_bloco_adicionar_item(No *bloco, No *item) {
    if (bloco->como.bloco.quantidade >= bloco->como.bloco.capacidade) {
        bloco->como.bloco.capacidade = bloco->como.bloco.capacidade == 0 ? 8 : bloco->como.bloco.capacidade * 2;
        bloco->como.bloco.itens = realloc(bloco->como.bloco.itens,
                                          (size_t)bloco->como.bloco.capacidade * sizeof(No *));
        if (bloco->como.bloco.itens == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    bloco->como.bloco.itens[bloco->como.bloco.quantidade++] = item;
}

static void builtin_lista_adicionar_item(No *lista, No *item) {
    if (lista->como.lista.quantidade >= lista->como.lista.capacidade) {
        lista->como.lista.capacidade = lista->como.lista.capacidade == 0 ? 8 : lista->como.lista.capacidade * 2;
        lista->como.lista.itens = realloc(lista->como.lista.itens,
                                          (size_t)lista->como.lista.capacidade * sizeof(No *));
        if (lista->como.lista.itens == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    lista->como.lista.itens[lista->como.lista.quantidade++] = item;
}

static void builtin_mapa_adicionar_item(No *mapa, const char *chave, No *valor) {
    if (mapa->como.mapa.quantidade >= mapa->como.mapa.capacidade) {
        mapa->como.mapa.capacidade = mapa->como.mapa.capacidade == 0 ? 8 : mapa->como.mapa.capacidade * 2;
        mapa->como.mapa.chaves = realloc(mapa->como.mapa.chaves,
                                         (size_t)mapa->como.mapa.capacidade * sizeof(char *));
        mapa->como.mapa.valores = realloc(mapa->como.mapa.valores,
                                          (size_t)mapa->como.mapa.capacidade * sizeof(No *));
        if (mapa->como.mapa.chaves == NULL || mapa->como.mapa.valores == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    mapa->como.mapa.chaves[mapa->como.mapa.quantidade] = noema_duplicar(chave);
    mapa->como.mapa.valores[mapa->como.mapa.quantidade] = valor;
    mapa->como.mapa.quantidade++;
}

static bool builtin_no_eh_expressao(TipoNo tipo) {
    switch (tipo) {
        case NO_LITERAL:
        case NO_VARIAVEL:
        case NO_LISTA_LITERAL:
        case NO_MAPA_LITERAL:
        case NO_BINARIO:
        case NO_UNARIO:
        case NO_CHAMADA:
        case NO_MEMBRO:
        case NO_INDICE:
        case NO_ATRIBUICAO:
        case NO_FUNCAO_ANONIMA:
        case NO_EXPAND:
        case NO_SYNTAX_TEMPLATE:
        case NO_SYNTAX_PLACEHOLDER:
            return true;
        default:
            return false;
    }
}

static const char *builtin_operador_texto(TipoToken operador) {
    switch (operador) {
        case TOKEN_MAIS: return "+";
        case TOKEN_MENOS: return "-";
        case TOKEN_ASTERISCO: return "*";
        case TOKEN_BARRA: return "/";
        case TOKEN_PERCENTUAL: return "%";
        case TOKEN_IGUAL: return "==";
        case TOKEN_DIFERENTE: return "!=";
        case TOKEN_MENOR: return "<";
        case TOKEN_MENOR_IGUAL: return "<=";
        case TOKEN_MAIOR: return ">";
        case TOKEN_MAIOR_IGUAL: return ">=";
        case TOKEN_AND: return "and";
        case TOKEN_OR: return "or";
        case TOKEN_NOT: return "not";
        default: return "?";
    }
}

static No *builtin_valor_para_sintaxe(Valor valor, int linha) {
    switch (valor.tipo) {
        case VALOR_SINTAXE:
            return valor.como.sintaxe;
        case VALOR_NULO:
        case VALOR_BOOL:
        case VALOR_NUMERO:
        case VALOR_TEXTO: {
            No *literal = builtin_novo_no(NO_LITERAL, linha);
            literal->como.literal.valor = valor;
            return literal;
        }
        case VALOR_LISTA: {
            No *lista = builtin_novo_no(NO_LISTA_LITERAL, linha);
            for (int i = 0; i < valor.como.lista->quantidade; i++) {
                builtin_lista_adicionar_item(lista, builtin_valor_para_sintaxe(valor.como.lista->itens[i], linha));
            }
            return lista;
        }
        case VALOR_MAPA: {
            No *mapa = builtin_novo_no(NO_MAPA_LITERAL, linha);
            for (int i = 0; i < valor.como.mapa->quantidade; i++) {
                builtin_mapa_adicionar_item(mapa,
                                            valor.como.mapa->entradas[i].chave,
                                            builtin_valor_para_sintaxe(valor.como.mapa->entradas[i].valor, linha));
            }
            return mapa;
        }
        default:
            noema_falhar("valor do tipo %s nao pode ser convertido para syntax", valor_tipo_nome(valor));
    }
    return NULL;
}

static No *builtin_normalizar_para_instrucao(No *no, int linha) {
    if (no == NULL) {
        return NULL;
    }
    if (builtin_no_eh_expressao(no->tipo)) {
        No *stmt = builtin_novo_no(NO_EXPR_STMT, linha);
        stmt->como.expressao.expressao = no;
        return stmt;
    }
    return no;
}

static const char *builtin_syntax_kind_texto(TipoNo tipo) {
    switch (tipo) {
        case NO_BLOCO: return "block";
        case NO_LITERAL: return "literal";
        case NO_VARIAVEL: return "identifier";
        case NO_LISTA_LITERAL: return "list";
        case NO_MAPA_LITERAL: return "map";
        case NO_BINARIO: return "binary";
        case NO_UNARIO: return "unary";
        case NO_CHAMADA: return "call";
        case NO_MEMBRO: return "member";
        case NO_INDICE: return "index";
        case NO_ATRIBUICAO: return "assign";
        case NO_DECL_LET: return "let";
        case NO_DECL_CONST: return "const";
        case NO_DECL_FUNCAO: return "function";
        case NO_DECL_MACRO: return "macro";
        case NO_FUNCAO_ANONIMA: return "lambda";
        case NO_EXPR_STMT: return "expr_stmt";
        case NO_EXPAND: return "expand";
        case NO_SYNTAX_TEMPLATE: return "syntax_template";
        case NO_SYNTAX_PLACEHOLDER: return "syntax_placeholder";
        case NO_IF: return "if";
        case NO_WHILE: return "while";
        case NO_FOR: return "for";
        case NO_RETURN: return "return";
        case NO_BREAK: return "break";
        case NO_CONTINUE: return "continue";
        case NO_DECL_PLUGIN: return "plugin";
    }
    return "unknown";
}

static Valor builtin_lista_textos_para_valor(char **textos, int quantidade) {
    Lista *lista = lista_criar();
    for (int i = 0; i < quantidade; i++) {
        lista_adicionar(lista, valor_texto(textos[i]));
    }
    return valor_lista(lista);
}

static char **builtin_parametros_texto(const char *nome, Valor valor, int *quantidade) {
    Lista *lista = esperar_lista(nome, valor);
    char **parametros = calloc((size_t)lista->quantidade, sizeof(char *));
    if (parametros == NULL && lista->quantidade > 0) {
        noema_falhar("memoria insuficiente");
    }
    for (int i = 0; i < lista->quantidade; i++) {
        parametros[i] = noema_duplicar(esperar_texto(nome, lista->itens[i]));
    }
    *quantidade = lista->quantidade;
    return parametros;
}

static Valor builtin_syntax_partes(No *no) {
    Mapa *partes = mapa_criar();
    char *texto = valor_para_texto(valor_sintaxe(no));
    mapa_definir(partes, "kind", valor_texto(builtin_syntax_kind_texto(no->tipo)));
    mapa_definir(partes, "expression", valor_bool(builtin_no_eh_expressao(no->tipo)));
    mapa_definir(partes, "text", valor_texto(texto));

    switch (no->tipo) {
        case NO_BLOCO: {
            Lista *itens = lista_criar();
            for (int i = 0; i < no->como.bloco.quantidade; i++) {
                lista_adicionar(itens, valor_sintaxe(no->como.bloco.itens[i]));
            }
            mapa_definir(partes, "items", valor_lista(itens));
            break;
        }
        case NO_LITERAL:
            mapa_definir(partes, "value", no->como.literal.valor);
            break;
        case NO_VARIAVEL:
            mapa_definir(partes, "name", valor_texto(no->como.variavel.nome));
            break;
        case NO_LISTA_LITERAL: {
            Lista *itens = lista_criar();
            for (int i = 0; i < no->como.lista.quantidade; i++) {
                lista_adicionar(itens, valor_sintaxe(no->como.lista.itens[i]));
            }
            mapa_definir(partes, "items", valor_lista(itens));
            break;
        }
        case NO_MAPA_LITERAL: {
            Lista *entradas = lista_criar();
            for (int i = 0; i < no->como.mapa.quantidade; i++) {
                Mapa *entrada = mapa_criar();
                mapa_definir(entrada, "key", valor_texto(no->como.mapa.chaves[i]));
                mapa_definir(entrada, "value", valor_sintaxe(no->como.mapa.valores[i]));
                lista_adicionar(entradas, valor_mapa(entrada));
            }
            mapa_definir(partes, "entries", valor_lista(entradas));
            break;
        }
        case NO_BINARIO:
            mapa_definir(partes, "left", valor_sintaxe(no->como.binario.esquerda));
            mapa_definir(partes, "operator", valor_texto(builtin_operador_texto(no->como.binario.operador)));
            mapa_definir(partes, "right", valor_sintaxe(no->como.binario.direita));
            break;
        case NO_UNARIO:
            mapa_definir(partes, "operator", valor_texto(builtin_operador_texto(no->como.unario.operador)));
            mapa_definir(partes, "value", valor_sintaxe(no->como.unario.direita));
            break;
        case NO_CHAMADA: {
            Lista *args = lista_criar();
            for (int i = 0; i < no->como.chamada.quantidade_argumentos; i++) {
                lista_adicionar(args, valor_sintaxe(no->como.chamada.argumentos[i]));
            }
            mapa_definir(partes, "target", valor_sintaxe(no->como.chamada.alvo));
            mapa_definir(partes, "args", valor_lista(args));
            break;
        }
        case NO_MEMBRO:
            mapa_definir(partes, "object", valor_sintaxe(no->como.membro.objeto));
            mapa_definir(partes, "name", valor_texto(no->como.membro.nome));
            break;
        case NO_INDICE:
            mapa_definir(partes, "object", valor_sintaxe(no->como.indice.objeto));
            mapa_definir(partes, "index", valor_sintaxe(no->como.indice.indice));
            break;
        case NO_ATRIBUICAO:
            mapa_definir(partes, "target", valor_sintaxe(no->como.atribuicao.alvo));
            mapa_definir(partes, "value", valor_sintaxe(no->como.atribuicao.valor));
            break;
        case NO_DECL_LET:
        case NO_DECL_CONST:
            mapa_definir(partes, "name", valor_texto(no->como.declaracao_variavel.nome));
            mapa_definir(partes,
                         "value",
                         no->como.declaracao_variavel.inicializador != NULL
                             ? valor_sintaxe(no->como.declaracao_variavel.inicializador)
                             : valor_nulo());
            break;
        case NO_DECL_FUNCAO:
        case NO_FUNCAO_ANONIMA:
            mapa_definir(partes,
                         "name",
                         no->como.declaracao_funcao.nome != NULL
                             ? valor_texto(no->como.declaracao_funcao.nome)
                             : valor_nulo());
            mapa_definir(partes,
                         "params",
                         builtin_lista_textos_para_valor(no->como.declaracao_funcao.parametros,
                                                         no->como.declaracao_funcao.quantidade_parametros));
            mapa_definir(partes, "body", valor_sintaxe(no->como.declaracao_funcao.corpo));
            break;
        case NO_DECL_MACRO:
            mapa_definir(partes,
                         "name",
                         no->como.declaracao_macro.nome != NULL
                             ? valor_texto(no->como.declaracao_macro.nome)
                             : valor_nulo());
            mapa_definir(partes,
                         "params",
                         builtin_lista_textos_para_valor(no->como.declaracao_macro.parametros,
                                                         no->como.declaracao_macro.quantidade_parametros));
            mapa_definir(partes, "body", valor_sintaxe(no->como.declaracao_macro.corpo));
            break;
        case NO_EXPR_STMT:
            mapa_definir(partes, "value", valor_sintaxe(no->como.expressao.expressao));
            break;
        case NO_EXPAND: {
            Lista *args = lista_criar();
            for (int i = 0; i < no->como.expansao_macro.quantidade_argumentos; i++) {
                lista_adicionar(args, valor_sintaxe(no->como.expansao_macro.argumentos[i]));
            }
            mapa_definir(partes, "name", valor_texto(no->como.expansao_macro.nome));
            mapa_definir(partes, "args", valor_lista(args));
            break;
        }
        case NO_SYNTAX_TEMPLATE:
            mapa_definir(partes, "body", valor_sintaxe(no->como.template_sintaxe.corpo));
            break;
        case NO_SYNTAX_PLACEHOLDER:
            mapa_definir(partes,
                         "name",
                         no->como.placeholder_sintaxe.nome != NULL
                             ? valor_texto(no->como.placeholder_sintaxe.nome)
                             : valor_nulo());
            mapa_definir(partes,
                         "value",
                         no->como.placeholder_sintaxe.expressao != NULL
                             ? valor_sintaxe(no->como.placeholder_sintaxe.expressao)
                             : valor_nulo());
            break;
        case NO_IF:
            mapa_definir(partes, "condition", valor_sintaxe(no->como.condicional.condicao));
            mapa_definir(partes, "then", valor_sintaxe(no->como.condicional.ramo_entao));
            mapa_definir(partes,
                         "else",
                         no->como.condicional.ramo_senao != NULL
                             ? valor_sintaxe(no->como.condicional.ramo_senao)
                             : valor_nulo());
            break;
        case NO_WHILE:
            mapa_definir(partes, "condition", valor_sintaxe(no->como.enquanto.condicao));
            mapa_definir(partes, "body", valor_sintaxe(no->como.enquanto.corpo));
            break;
        case NO_FOR:
            mapa_definir(partes, "name", valor_texto(no->como.para.variavel));
            mapa_definir(partes, "iterable", valor_sintaxe(no->como.para.iteravel));
            mapa_definir(partes, "body", valor_sintaxe(no->como.para.corpo));
            break;
        case NO_RETURN:
            mapa_definir(partes,
                         "value",
                         no->como.retorno.valor != NULL ? valor_sintaxe(no->como.retorno.valor) : valor_nulo());
            break;
        case NO_BREAK:
        case NO_CONTINUE:
            break;
        case NO_DECL_PLUGIN:
            mapa_definir(partes, "name", valor_texto(no->como.plugin.nome));
            break;
    }

    return valor_mapa(partes);
}

static size_t esperar_tamanho_nao_negativo(const char *nome, Valor valor, const char *campo) {
    double numero = trunc(valor_para_numero(valor));
    if (!isfinite(numero) || numero < 0.0) {
        noema_falhar("builtin '%s' exige %s >= 0", nome, campo);
    }
    return (size_t)numero;
}

static uint8_t esperar_u8(const char *nome, Valor valor, const char *campo) {
    double numero = trunc(valor_para_numero(valor));
    if (!isfinite(numero) || numero < 0.0 || numero > 255.0) {
        noema_falhar("builtin '%s' exige %s entre 0 e 255", nome, campo);
    }
    return (uint8_t)numero;
}

static int32_t esperar_i32(const char *nome, Valor valor, const char *campo) {
    double numero = trunc(valor_para_numero(valor));
    if (!isfinite(numero) || numero < (double)INT32_MIN || numero > (double)INT32_MAX) {
        noema_falhar("builtin '%s' exige %s no intervalo de int32", nome, campo);
    }
    return (int32_t)numero;
}

static uint32_t esperar_u32(const char *nome, Valor valor, const char *campo) {
    double numero = trunc(valor_para_numero(valor));
    if (!isfinite(numero) || numero < 0.0 || numero > (double)UINT32_MAX) {
        noema_falhar("builtin '%s' exige %s no intervalo de uint32", nome, campo);
    }
    return (uint32_t)numero;
}

static bool valor_eh_funcao(Valor valor) {
    return valor.tipo == VALOR_FUNCAO || valor.tipo == VALOR_NATIVA;
}

static bool noema_eh_separador_caminho(char caractere) {
    return caractere == '/' || caractere == '\\';
}

static char noema_obter_separador_caminho(void) {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

static size_t noema_comprimento_raiz_caminho(const char *caminho) {
#ifdef _WIN32
    if (isalpha((unsigned char)caminho[0]) && caminho[1] == ':') {
        if (noema_eh_separador_caminho(caminho[2])) {
            return 3;
        }
        return 2;
    }
    if (noema_eh_separador_caminho(caminho[0]) && noema_eh_separador_caminho(caminho[1])) {
        size_t cursor = 2;
        int componentes = 0;
        while (caminho[cursor] != '\0') {
            while (noema_eh_separador_caminho(caminho[cursor])) {
                cursor++;
            }
            if (caminho[cursor] == '\0') {
                break;
            }
            while (caminho[cursor] != '\0' && !noema_eh_separador_caminho(caminho[cursor])) {
                cursor++;
            }
            componentes++;
            if (componentes == 2) {
                while (noema_eh_separador_caminho(caminho[cursor])) {
                    cursor++;
                }
                return cursor;
            }
        }
        return 2;
    }
#endif
    if (noema_eh_separador_caminho(caminho[0])) {
        return 1;
    }
    return 0;
}

static int noema_criar_diretorio_sistema(const char *caminho) {
#ifdef _WIN32
    return _mkdir(caminho);
#else
    return mkdir(caminho, 0777);
#endif
}

static char *noema_obter_cwd_sistema(char *buffer, size_t tamanho) {
#ifdef _WIN32
    return _getcwd(buffer, (int)tamanho);
#else
    return getcwd(buffer, tamanho);
#endif
}

static int noema_mudar_diretorio_sistema(const char *caminho) {
#ifdef _WIN32
    return _chdir(caminho);
#else
    return chdir(caminho);
#endif
}

static FILE *noema_abrir_processo(const char *comando) {
#ifdef _WIN32
    return _popen(comando, "r");
#else
    return popen(comando, "r");
#endif
}

static int noema_fechar_processo(FILE *processo) {
#ifdef _WIN32
    return _pclose(processo);
#else
    return pclose(processo);
#endif
}

static char *basename_de_caminho(const char *caminho) {
    const char *fim = caminho + strlen(caminho);
    size_t raiz = noema_comprimento_raiz_caminho(caminho);
    while ((size_t)(fim - caminho) > raiz && noema_eh_separador_caminho(fim[-1])) {
        fim--;
    }
    const char *inicio = fim;
    while ((size_t)(inicio - caminho) > raiz && !noema_eh_separador_caminho(inicio[-1])) {
        inicio--;
    }
    return noema_formatar("%.*s", (int)(fim - inicio), inicio);
}

static char *extensao_de_caminho(const char *caminho) {
    char *base = basename_de_caminho(caminho);
    char *ponto = strrchr(base, '.');
    if (ponto == NULL || ponto == base) {
        return noema_duplicar("");
    }
    return noema_duplicar(ponto);
}

static char *dirname_de_caminho(const char *caminho) {
    char *copia = noema_duplicar(caminho);
    size_t raiz = noema_comprimento_raiz_caminho(copia);
    size_t tamanho = strlen(copia);
    while (tamanho > raiz && noema_eh_separador_caminho(copia[tamanho - 1])) {
        copia[tamanho - 1] = '\0';
        tamanho--;
    }

    char *barra = copia + tamanho;
    while (barra > copia + raiz && !noema_eh_separador_caminho(barra[-1])) {
        barra--;
    }

    if (barra <= copia + raiz) {
        if (raiz > 0) {
            copia[raiz] = '\0';
            return copia;
        }
        free(copia);
        return noema_duplicar(".");
    }

    while (barra > copia + raiz && noema_eh_separador_caminho(barra[-1])) {
        barra--;
    }

    if (barra <= copia + raiz) {
        copia[raiz] = '\0';
        return copia;
    }

    *barra = '\0';
    return copia;
}

static char *stem_de_caminho(const char *caminho) {
    char *base = basename_de_caminho(caminho);
    char *ponto = strrchr(base, '.');
    if (ponto == NULL || ponto == base) {
        return base;
    }
    *ponto = '\0';
    return base;
}

static char *recortar_espacos(const char *texto, bool esquerda, bool direita) {
    const char *inicio = texto;
    const char *fim = texto + strlen(texto);
    if (esquerda) {
        while (*inicio != '\0' && isspace((unsigned char)*inicio)) {
            inicio++;
        }
    }
    if (direita) {
        while (fim > inicio && isspace((unsigned char)fim[-1])) {
            fim--;
        }
    }
    return noema_formatar("%.*s", (int)(fim - inicio), inicio);
}

static double parsear_numero_texto(const char *nome, const char *texto, bool inteiro, bool tem_padrao, Valor padrao) {
    char *fim = NULL;
    double numero = inteiro ? (double)strtoll(texto, &fim, 10) : strtod(texto, &fim);
    if (fim == texto || *fim != '\0') {
        if (tem_padrao) {
            return valor_para_numero(padrao);
        }
        noema_falhar("builtin '%s' nao conseguiu converter '%s'", nome, texto);
    }
    return numero;
}

static Valor escrever_arquivo(const char *nome, const char *caminho, const char *conteudo, const char *modo) {
    FILE *arquivo = fopen(caminho, modo);
    if (arquivo == NULL) {
        noema_falhar("builtin '%s' nao conseguiu abrir '%s': %s", nome, caminho, strerror(errno));
    }
    size_t tamanho = strlen(conteudo);
    size_t escritos = fwrite(conteudo, 1, tamanho, arquivo);
    fclose(arquivo);
    if (escritos != tamanho) {
        noema_falhar("builtin '%s' nao conseguiu escrever '%s' por completo", nome, caminho);
    }
    return valor_numero((double)escritos);
}

static char *ler_fluxo_texto(FILE *fluxo) {
    size_t capacidade = 256;
    size_t comprimento = 0;
    char *saida = malloc(capacidade);
    if (saida == NULL) {
        noema_falhar("memoria insuficiente");
    }
    saida[0] = '\0';

    for (;;) {
        char buffer[4096];
        size_t lidos = fread(buffer, 1, sizeof(buffer), fluxo);
        if (lidos == 0) {
            break;
        }
        while (comprimento + lidos + 1 > capacidade) {
            capacidade *= 2;
            saida = realloc(saida, capacidade);
            if (saida == NULL) {
                noema_falhar("memoria insuficiente");
            }
        }
        memcpy(saida + comprimento, buffer, lidos);
        comprimento += lidos;
        saida[comprimento] = '\0';
    }
    return saida;
}

static bool caminho_existe(const char *caminho) {
    NoemaStat info;
    return noema_stat(caminho, &info) == 0;
}

static bool caminho_eh_arquivo(const char *caminho) {
    NoemaStat info;
    return noema_stat(caminho, &info) == 0 && S_ISREG(info.st_mode);
}

static bool caminho_eh_diretorio(const char *caminho) {
    NoemaStat info;
    return noema_stat(caminho, &info) == 0 && S_ISDIR(info.st_mode);
}

static void criar_diretorios(const char *nome, const char *caminho, bool recursivo) {
    if (!recursivo) {
        if (noema_criar_diretorio_sistema(caminho) != 0 && errno != EEXIST) {
            noema_falhar("builtin '%s' nao conseguiu criar diretorio '%s': %s", nome, caminho, strerror(errno));
        }
        if (!caminho_eh_diretorio(caminho)) {
            noema_falhar("builtin '%s' encontrou caminho existente que nao e diretorio: '%s'", nome, caminho);
        }
        return;
    }

    char *copia = noema_duplicar(caminho);
    size_t raiz = noema_comprimento_raiz_caminho(copia);
    for (char *cursor = copia + raiz; *cursor != '\0'; cursor++) {
        if (!noema_eh_separador_caminho(*cursor)) {
            continue;
        }
        *cursor = '\0';
        if (copia[0] != '\0' && !caminho_eh_diretorio(copia)) {
            if (noema_criar_diretorio_sistema(copia) != 0 && errno != EEXIST) {
                noema_falhar("builtin '%s' nao conseguiu criar diretorio '%s': %s", nome, copia, strerror(errno));
            }
        }
        *cursor = noema_obter_separador_caminho();
    }
    if (!caminho_eh_diretorio(copia)) {
        if (noema_criar_diretorio_sistema(copia) != 0 && errno != EEXIST) {
            noema_falhar("builtin '%s' nao conseguiu criar diretorio '%s': %s", nome, copia, strerror(errno));
        }
    }
    if (!caminho_eh_diretorio(copia)) {
        noema_falhar("builtin '%s' encontrou caminho existente que nao e diretorio: '%s'", nome, copia);
    }
    free(copia);
}

static Mapa *mapa_em(Mapa *mapa, const char *chave) {
    Valor valor;
    if (!mapa_obter(mapa, chave, &valor) || valor.tipo != VALOR_MAPA) {
        noema_falhar("objeto interno invalido: campo '%s' nao e map", chave);
    }
    return valor.como.mapa;
}

static Lista *lista_em(Mapa *mapa, const char *chave) {
    Valor valor;
    if (!mapa_obter(mapa, chave, &valor) || valor.tipo != VALOR_LISTA) {
        noema_falhar("objeto interno invalido: campo '%s' nao e list", chave);
    }
    return valor.como.lista;
}

static int normalizar_indice(int indice, int tamanho) {
    if (indice < 0) {
        indice += tamanho;
    }
    if (indice < 0) {
        indice = 0;
    }
    if (indice > tamanho) {
        indice = tamanho;
    }
    return indice;
}

static Valor chamar_callback_com_item(Interpretador *interpretador,
                                      Ambiente *ambiente,
                                      Valor callback,
                                      Valor item,
                                      int indice) {
    Valor argumentos[2];
    argumentos[0] = item;
    argumentos[1] = valor_numero((double)indice);
    return chamar_valor(interpretador, ambiente, callback, 2, argumentos);
}

static unsigned int proximo_aleatorio(Interpretador *interpretador) {
    interpretador->semente_aleatoria = interpretador->semente_aleatoria * 1103515245u + 12345u;
    return interpretador->semente_aleatoria;
}

static void registrar_nativa(Interpretador *interpretador,
                             const char *nome,
                             FuncaoNativaBruta funcao,
                             void *dados) {
    FuncaoNativa *nativa = calloc(1, sizeof(FuncaoNativa));
    if (nativa == NULL) {
        noema_falhar("memoria insuficiente");
    }
    nativa->nome = noema_duplicar(nome);
    nativa->funcao = funcao;
    nativa->dados = dados;
    ambiente_definir(interpretador->global, nome, valor_nativa(nativa), true);
}

static void buffer_anexar_texto(char **saida,
                                size_t *capacidade,
                                size_t *comprimento,
                                const char *texto,
                                size_t tamanho) {
    while (*comprimento + tamanho + 1 > *capacidade) {
        *capacidade = *capacidade == 1 ? 64 : *capacidade * 2;
        *saida = realloc(*saida, *capacidade);
        if (*saida == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    memcpy(*saida + *comprimento, texto, tamanho);
    *comprimento += tamanho;
    (*saida)[*comprimento] = '\0';
}

static char *resolver_modelo_valor(Valor dados, const char *chave) {
    if (dados.tipo == VALOR_LISTA) {
        char *fim = NULL;
        long indice = strtol(chave, &fim, 10);
        if (fim != chave && *fim == '\0' && indice > 0 && indice <= dados.como.lista->quantidade) {
            return valor_para_texto(dados.como.lista->itens[indice - 1]);
        }
    }

    if (dados.tipo == VALOR_MAPA) {
        Valor valor;
        if (mapa_obter(dados.como.mapa, chave, &valor)) {
            return valor_para_texto(valor);
        }
    }

    return noema_duplicar("");
}

static char *renderizar_modelo(const char *modelo, Valor dados) {
    char *saida = noema_duplicar("");
    size_t capacidade = 1;
    size_t comprimento = 0;

    for (size_t i = 0; modelo[i] != '\0'; i++) {
        if (modelo[i] == '$') {
            if (modelo[i + 1] == '$') {
                buffer_anexar_texto(&saida, &capacidade, &comprimento, "$", 1);
                i++;
                continue;
            }

            size_t inicio = i + 1;
            size_t fim = inicio;
            if (isdigit((unsigned char)modelo[inicio])) {
                while (isdigit((unsigned char)modelo[fim])) {
                    fim++;
                }
            } else if (isalpha((unsigned char)modelo[inicio]) || modelo[inicio] == '_') {
                while (isalnum((unsigned char)modelo[fim]) || modelo[fim] == '_') {
                    fim++;
                }
            }

            if (fim > inicio) {
                char *chave = noema_formatar("%.*s", (int)(fim - inicio), modelo + inicio);
                char *insercao = resolver_modelo_valor(dados, chave);
                buffer_anexar_texto(&saida, &capacidade, &comprimento, insercao, strlen(insercao));
                free(chave);
                i = fim - 1;
                continue;
            }
        }

        buffer_anexar_texto(&saida, &capacidade, &comprimento, modelo + i, 1);
    }

    return saida;
}

static char *substituir_capturas(const char *modelo, Lista *capturas) {
    return renderizar_modelo(modelo, valor_lista(capturas));
}

static char *normalizar_texto_para_padrao(const char *texto) {
    size_t tamanho = strlen(texto);
    char *saida = malloc(tamanho + 1);
    if (saida == NULL) {
        noema_falhar("memoria insuficiente");
    }

    size_t j = 0;
    bool ultimo_foi_espaco = true;
    for (size_t i = 0; i < tamanho; i++) {
        unsigned char c = (unsigned char)texto[i];
        if (isalnum(c)) {
            saida[j++] = (char)tolower(c);
            ultimo_foi_espaco = false;
            continue;
        }

        if (c == '*') {
            saida[j++] = '*';
            ultimo_foi_espaco = false;
            continue;
        }

        if (c == '\'' && ((i > 0 && isalnum((unsigned char)texto[i - 1])) ||
                          (i + 1 < tamanho && isalnum((unsigned char)texto[i + 1])))) {
            continue;
        }

        if (!ultimo_foi_espaco && j > 0) {
            saida[j++] = ' ';
            ultimo_foi_espaco = true;
        }
    }

    while (j > 0 && saida[j - 1] == ' ') {
        j--;
    }
    saida[j] = '\0';
    return saida;
}

static bool casar_padrao(const char *padrao, const char *texto, Lista *capturas) {
    int estrelas = 0;
    for (const char *c = padrao; *c != '\0'; c++) {
        if (*c == '*') {
            estrelas++;
        }
    }

    if (estrelas == 0) {
        return strcmp(padrao, texto) == 0;
    }

    char **segmentos = calloc((size_t)estrelas + 1, sizeof(char *));
    if (segmentos == NULL) {
        noema_falhar("memoria insuficiente");
    }

    int quantidade_segmentos = 0;
    const char *inicio = padrao;
    for (const char *c = padrao;; c++) {
        if (*c == '*' || *c == '\0') {
            size_t tamanho = (size_t)(c - inicio);
            segmentos[quantidade_segmentos] = malloc(tamanho + 1);
            if (segmentos[quantidade_segmentos] == NULL) {
                noema_falhar("memoria insuficiente");
            }
            memcpy(segmentos[quantidade_segmentos], inicio, tamanho);
            segmentos[quantidade_segmentos][tamanho] = '\0';
            quantidade_segmentos++;
            if (*c == '\0') {
                break;
            }
            inicio = c + 1;
        }
    }

    bool comeca_livre = padrao[0] == '*';
    bool termina_livre = padrao[strlen(padrao) - 1] == '*';
    const char *cursor = texto;

    if (!comeca_livre && segmentos[0][0] != '\0') {
        size_t tamanho = strlen(segmentos[0]);
        if (strncmp(cursor, segmentos[0], tamanho) != 0) {
            return false;
        }
        cursor += tamanho;
    }

    for (int i = 0; i < estrelas; i++) {
        const char *proximo_segmento = segmentos[i + 1];
        if (proximo_segmento[0] == '\0') {
            lista_adicionar(capturas, valor_texto(cursor));
            cursor += strlen(cursor);
            continue;
        }

        const char *achado = strstr(cursor, proximo_segmento);
        if (achado == NULL) {
            return false;
        }

        size_t tamanho_captura = (size_t)(achado - cursor);
        char *captura = malloc(tamanho_captura + 1);
        if (captura == NULL) {
            noema_falhar("memoria insuficiente");
        }
        memcpy(captura, cursor, tamanho_captura);
        captura[tamanho_captura] = '\0';
        lista_adicionar(capturas, valor_texto(captura));
        cursor = achado + strlen(proximo_segmento);
    }

    if (!termina_livre && segmentos[quantidade_segmentos - 1][0] != '\0') {
        size_t tamanho_final = strlen(segmentos[quantidade_segmentos - 1]);
        size_t tamanho_texto = strlen(texto);
        if (tamanho_texto < tamanho_final) {
            return false;
        }
        if (strcmp(texto + tamanho_texto - tamanho_final, segmentos[quantidade_segmentos - 1]) != 0) {
            return false;
        }
    }

    return true;
}

static Valor responder_com_valor(Interpretador *interpretador, Ambiente *ambiente, Valor resposta, Lista *capturas) {
    if (resposta.tipo == VALOR_TEXTO) {
        char *texto = substituir_capturas(resposta.como.texto, capturas);
        return valor_texto(texto);
    }

    if (resposta.tipo == VALOR_LISTA) {
        if (resposta.como.lista->quantidade == 0) {
            return valor_nulo();
        }
        unsigned int sorteio = proximo_aleatorio(interpretador);
        Valor escolhido = resposta.como.lista->itens[sorteio % (unsigned int)resposta.como.lista->quantidade];
        return responder_com_valor(interpretador, ambiente, escolhido, capturas);
    }

    if (resposta.tipo == VALOR_FUNCAO || resposta.tipo == VALOR_NATIVA) {
        int argc = capturas->quantidade;
        Valor *argv = capturas->itens;
        return chamar_valor(interpretador, ambiente, resposta, argc, argv);
    }

    return resposta;
}

static Valor builtin_print(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    for (int i = 0; i < argc; i++) {
        char *texto = valor_para_texto(argv[i]);
        if (i > 0) {
            printf(" ");
        }
        printf("%s", texto);
    }
    printf("\n");
    return valor_nulo();
}

static Valor builtin_write(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    for (int i = 0; i < argc; i++) {
        char *texto = valor_para_texto(argv[i]);
        if (i > 0) {
            printf(" ");
        }
        printf("%s", texto);
    }
    fflush(stdout);
    return valor_nulo();
}

static Valor builtin_flush(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)argv;
    (void)dados;
    garantir_argumentos("flush", argc, 0, 0);
    fflush(stdout);
    return valor_nulo();
}

static Valor builtin_read_line(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("read_line", argc, 0, 1);
    if (argc == 1) {
        char *texto = valor_para_texto(argv[0]);
        printf("%s", texto);
        fflush(stdout);
    }

    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return valor_nulo();
    }
    size_t tamanho = strlen(buffer);
    while (tamanho > 0 && (buffer[tamanho - 1] == '\n' || buffer[tamanho - 1] == '\r')) {
        buffer[--tamanho] = '\0';
    }
    return valor_texto(buffer);
}

static Valor builtin_read_stdin(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)argv;
    (void)dados;
    garantir_argumentos("read_stdin", argc, 0, 0);
    return valor_texto(ler_fluxo_texto(stdin));
}

static Valor builtin_panic(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("panic", argc, 1, 1);
    char *texto = valor_para_texto(argv[0]);
    noema_falhar("%s", texto);
}

static Valor builtin_type(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("type", argc, 1, 1);
    return valor_texto(valor_tipo_nome(argv[0]));
}

static Valor builtin_is_null(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_null", argc, 1, 1);
    return valor_bool(argv[0].tipo == VALOR_NULO);
}

static Valor builtin_is_bool(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_bool", argc, 1, 1);
    return valor_bool(argv[0].tipo == VALOR_BOOL);
}

static Valor builtin_is_number(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_number", argc, 1, 1);
    return valor_bool(argv[0].tipo == VALOR_NUMERO);
}

static Valor builtin_is_string(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_string", argc, 1, 1);
    return valor_bool(argv[0].tipo == VALOR_TEXTO);
}

static Valor builtin_is_list(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_list", argc, 1, 1);
    return valor_bool(argv[0].tipo == VALOR_LISTA);
}

static Valor builtin_is_map(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_map", argc, 1, 1);
    return valor_bool(argv[0].tipo == VALOR_MAPA);
}

static Valor builtin_is_function(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_function", argc, 1, 1);
    return valor_bool(valor_eh_funcao(argv[0]));
}

static Valor builtin_is_pointer(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_pointer", argc, 1, 1);
    return valor_bool(argv[0].tipo == VALOR_PONTEIRO);
}

static Valor builtin_is_syntax(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_syntax", argc, 1, 1);
    return valor_bool(argv[0].tipo == VALOR_SINTAXE);
}

static Valor builtin_syntax_kind(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_kind", argc, 1, 1);
    No *sintaxe = esperar_sintaxe("syntax_kind", argv[0]);
    return valor_texto(builtin_syntax_kind_texto(sintaxe->tipo));
}

static Valor builtin_syntax_text(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_text", argc, 1, 1);
    No *sintaxe = esperar_sintaxe("syntax_text", argv[0]);
    char *texto = valor_para_texto(valor_sintaxe(sintaxe));
    return valor_texto(texto);
}

static Valor builtin_syntax_parts(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_parts", argc, 1, 1);
    return builtin_syntax_partes(esperar_sintaxe("syntax_parts", argv[0]));
}

static Valor builtin_syntax_identifier(Interpretador *interpretador,
                                       Ambiente *ambiente,
                                       int argc,
                                       Valor *argv,
                                       void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_identifier", argc, 1, 1);
    const char *nome = esperar_texto("syntax_identifier", argv[0]);
    No *identificador = builtin_novo_no(NO_VARIAVEL, 0);
    identificador->como.variavel.nome = noema_duplicar(nome);
    return valor_sintaxe(identificador);
}

static Valor builtin_syntax_literal(Interpretador *interpretador,
                                    Ambiente *ambiente,
                                    int argc,
                                    Valor *argv,
                                    void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_literal", argc, 1, 1);
    return valor_sintaxe(builtin_valor_para_sintaxe(argv[0], 0));
}

static Valor builtin_syntax_block(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_block", argc, 1, 1);
    Lista *itens = esperar_lista("syntax_block", argv[0]);
    No *bloco = builtin_novo_no(NO_BLOCO, 0);
    for (int i = 0; i < itens->quantidade; i++) {
        builtin_bloco_adicionar_item(bloco,
                                     builtin_normalizar_para_instrucao(
                                         builtin_valor_para_sintaxe(itens->itens[i], 0),
                                         0));
    }
    return valor_sintaxe(bloco);
}

static Valor builtin_syntax_let(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_let", argc, 1, 2);
    No *no = builtin_novo_no(NO_DECL_LET, 0);
    no->como.declaracao_variavel.nome = noema_duplicar(esperar_texto("syntax_let", argv[0]));
    no->como.declaracao_variavel.inicializador =
        argc == 2 ? builtin_valor_para_sintaxe(argv[1], 0) : NULL;
    if (no->como.declaracao_variavel.inicializador != NULL &&
        !builtin_no_eh_expressao(no->como.declaracao_variavel.inicializador->tipo)) {
        noema_falhar("builtin 'syntax_let' exige inicializador em forma de expressao");
    }
    return valor_sintaxe(no);
}

static Valor builtin_syntax_const(Interpretador *interpretador,
                                  Ambiente *ambiente,
                                  int argc,
                                  Valor *argv,
                                  void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_const", argc, 1, 2);
    No *no = builtin_novo_no(NO_DECL_CONST, 0);
    no->como.declaracao_variavel.nome = noema_duplicar(esperar_texto("syntax_const", argv[0]));
    no->como.declaracao_variavel.inicializador =
        argc == 2 ? builtin_valor_para_sintaxe(argv[1], 0) : NULL;
    if (no->como.declaracao_variavel.inicializador != NULL &&
        !builtin_no_eh_expressao(no->como.declaracao_variavel.inicializador->tipo)) {
        noema_falhar("builtin 'syntax_const' exige inicializador em forma de expressao");
    }
    return valor_sintaxe(no);
}

static Valor builtin_syntax_function(Interpretador *interpretador,
                                     Ambiente *ambiente,
                                     int argc,
                                     Valor *argv,
                                     void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("syntax_function", argc, 3, 3);

    No *funcao = builtin_novo_no(NO_DECL_FUNCAO, 0);
    funcao->como.declaracao_funcao.nome = noema_duplicar(esperar_texto("syntax_function", argv[0]));
    funcao->como.declaracao_funcao.parametros = builtin_parametros_texto("syntax_function", argv[1],
                                                                         &funcao->como.declaracao_funcao.quantidade_parametros);

    No *corpo = builtin_valor_para_sintaxe(argv[2], 0);
    if (corpo->tipo != NO_BLOCO) {
        No *bloco = builtin_novo_no(NO_BLOCO, 0);
        if (builtin_no_eh_expressao(corpo->tipo)) {
            No *retorno = builtin_novo_no(NO_RETURN, 0);
            retorno->como.retorno.valor = corpo;
            builtin_bloco_adicionar_item(bloco, retorno);
        } else {
            builtin_bloco_adicionar_item(bloco, builtin_normalizar_para_instrucao(corpo, 0));
        }
        corpo = bloco;
    }

    funcao->como.declaracao_funcao.corpo = corpo;
    return valor_sintaxe(funcao);
}

static Valor builtin_memory_alloc(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_alloc", argc, 1, 1);
    size_t tamanho = esperar_tamanho_nao_negativo("memory_alloc", argv[0], "size");
    void *bloco = calloc(tamanho > 0 ? tamanho : 1, 1);
    if (bloco == NULL) {
        noema_falhar("memoria insuficiente");
    }
    return valor_ponteiro(bloco);
}

static Valor builtin_memory_free(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_free", argc, 1, 1);
    free(esperar_ponteiro("memory_free", argv[0]));
    return valor_nulo();
}

static Valor builtin_memory_fill(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_fill", argc, 3, 3);
    void *ponteiro = esperar_ponteiro("memory_fill", argv[0]);
    if (ponteiro == NULL) {
        noema_falhar("builtin 'memory_fill' exige pointer nao nulo");
    }
    uint8_t valor = esperar_u8("memory_fill", argv[1], "value");
    size_t tamanho = esperar_tamanho_nao_negativo("memory_fill", argv[2], "size");
    memset(ponteiro, valor, tamanho);
    return argv[0];
}

static Valor builtin_memory_get_u8(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_get_u8", argc, 2, 2);
    unsigned char *base = esperar_ponteiro("memory_get_u8", argv[0]);
    if (base == NULL) {
        noema_falhar("builtin 'memory_get_u8' exige pointer nao nulo");
    }
    size_t deslocamento = esperar_tamanho_nao_negativo("memory_get_u8", argv[1], "offset");
    return valor_numero((double)base[deslocamento]);
}

static Valor builtin_memory_get_i32(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_get_i32", argc, 2, 2);
    unsigned char *base = esperar_ponteiro("memory_get_i32", argv[0]);
    if (base == NULL) {
        noema_falhar("builtin 'memory_get_i32' exige pointer nao nulo");
    }
    size_t deslocamento = esperar_tamanho_nao_negativo("memory_get_i32", argv[1], "offset");
    int32_t valor = 0;
    memcpy(&valor, base + deslocamento, sizeof(valor));
    return valor_numero((double)valor);
}

static Valor builtin_memory_get_u32(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_get_u32", argc, 2, 2);
    unsigned char *base = esperar_ponteiro("memory_get_u32", argv[0]);
    if (base == NULL) {
        noema_falhar("builtin 'memory_get_u32' exige pointer nao nulo");
    }
    size_t deslocamento = esperar_tamanho_nao_negativo("memory_get_u32", argv[1], "offset");
    uint32_t valor = 0;
    memcpy(&valor, base + deslocamento, sizeof(valor));
    return valor_numero((double)valor);
}

static Valor builtin_memory_set_u8(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_set_u8", argc, 3, 3);
    unsigned char *base = esperar_ponteiro("memory_set_u8", argv[0]);
    if (base == NULL) {
        noema_falhar("builtin 'memory_set_u8' exige pointer nao nulo");
    }
    size_t deslocamento = esperar_tamanho_nao_negativo("memory_set_u8", argv[1], "offset");
    base[deslocamento] = esperar_u8("memory_set_u8", argv[2], "value");
    return argv[0];
}

static Valor builtin_memory_set_i32(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_set_i32", argc, 3, 3);
    unsigned char *base = esperar_ponteiro("memory_set_i32", argv[0]);
    if (base == NULL) {
        noema_falhar("builtin 'memory_set_i32' exige pointer nao nulo");
    }
    size_t deslocamento = esperar_tamanho_nao_negativo("memory_set_i32", argv[1], "offset");
    int32_t valor = esperar_i32("memory_set_i32", argv[2], "value");
    memcpy(base + deslocamento, &valor, sizeof(valor));
    return argv[0];
}

static Valor builtin_memory_set_u32(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("memory_set_u32", argc, 3, 3);
    unsigned char *base = esperar_ponteiro("memory_set_u32", argv[0]);
    if (base == NULL) {
        noema_falhar("builtin 'memory_set_u32' exige pointer nao nulo");
    }
    size_t deslocamento = esperar_tamanho_nao_negativo("memory_set_u32", argv[1], "offset");
    uint32_t valor = esperar_u32("memory_set_u32", argv[2], "value");
    memcpy(base + deslocamento, &valor, sizeof(valor));
    return argv[0];
}

static Valor builtin_string(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("string", argc, 1, 1);
    char *texto = valor_para_texto(argv[0]);
    return valor_texto(texto);
}

static Valor builtin_number(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("number", argc, 1, 1);
    return valor_numero(valor_para_numero(argv[0]));
}

static Valor builtin_int(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("int", argc, 1, 1);
    return valor_numero(trunc(valor_para_numero(argv[0])));
}

static Valor builtin_bool(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("bool", argc, 1, 1);
    return valor_bool(valor_eh_verdadeiro(argv[0]));
}

static Valor builtin_parse_number(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("parse_number", argc, 1, 2);
    const char *texto = esperar_texto("parse_number", argv[0]);
    return valor_numero(parsear_numero_texto("parse_number", texto, false, argc == 2, argc == 2 ? argv[1] : valor_nulo()));
}

static Valor builtin_parse_int(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("parse_int", argc, 1, 2);
    const char *texto = esperar_texto("parse_int", argv[0]);
    return valor_numero(parsear_numero_texto("parse_int", texto, true, argc == 2, argc == 2 ? argv[1] : valor_nulo()));
}

static Valor builtin_len(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("len", argc, 1, 1);
    switch (argv[0].tipo) {
        case VALOR_TEXTO:
            return valor_numero((double)strlen(argv[0].como.texto));
        case VALOR_LISTA:
            return valor_numero((double)argv[0].como.lista->quantidade);
        case VALOR_MAPA:
            return valor_numero((double)argv[0].como.mapa->quantidade);
        default:
            noema_falhar("builtin 'len' aceita string, list ou map");
    }
    return valor_nulo();
}

static Valor builtin_push(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("push", argc, 2, 2);
    lista_adicionar(esperar_lista("push", argv[0]), argv[1]);
    return argv[0];
}

static Valor builtin_pop(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("pop", argc, 1, 1);
    return lista_remover_ultimo(esperar_lista("pop", argv[0]));
}

static Valor builtin_keys(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("keys", argc, 1, 1);
    Mapa *mapa = esperar_mapa("keys", argv[0]);
    Lista *lista = lista_criar();
    for (int i = 0; i < mapa->quantidade; i++) {
        lista_adicionar(lista, valor_texto(mapa->entradas[i].chave));
    }
    return valor_lista(lista);
}

static Valor builtin_values(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("values", argc, 1, 1);
    Mapa *mapa = esperar_mapa("values", argv[0]);
    Lista *lista = lista_criar();
    for (int i = 0; i < mapa->quantidade; i++) {
        lista_adicionar(lista, mapa->entradas[i].valor);
    }
    return valor_lista(lista);
}

static Valor builtin_get(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("get", argc, 2, 3);
    Valor padrao = argc == 3 ? argv[2] : valor_nulo();

    if (argv[0].tipo == VALOR_MAPA) {
        char *chave = valor_para_texto(argv[1]);
        Valor valor;
        if (mapa_obter(argv[0].como.mapa, chave, &valor)) {
            return valor;
        }
        return padrao;
    }
    if (argv[0].tipo == VALOR_LISTA) {
        int indice = (int)valor_para_numero(argv[1]);
        Valor valor = lista_obter(argv[0].como.lista, indice);
        if (valor.tipo == VALOR_NULO) {
            return padrao;
        }
        return valor;
    }
    noema_falhar("builtin 'get' aceita map ou list");
    return valor_nulo();
}

static Valor builtin_has(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("has", argc, 2, 2);
    if (argv[0].tipo == VALOR_MAPA) {
        char *chave = valor_para_texto(argv[1]);
        return valor_bool(mapa_tem(argv[0].como.mapa, chave));
    }
    if (argv[0].tipo == VALOR_LISTA) {
        int indice = (int)valor_para_numero(argv[1]);
        return valor_bool(indice >= 0 && indice < argv[0].como.lista->quantidade);
    }
    if (argv[0].tipo == VALOR_TEXTO) {
        char *fragmento = valor_para_texto(argv[1]);
        return valor_bool(strstr(argv[0].como.texto, fragmento) != NULL);
    }
    noema_falhar("builtin 'has' aceita map, list ou string");
    return valor_nulo();
}

static Valor builtin_range(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("range", argc, 1, 3);
    double inicio = 0.0;
    double fim = 0.0;
    double passo = 1.0;

    if (argc == 1) {
        fim = valor_para_numero(argv[0]);
    } else if (argc == 2) {
        inicio = valor_para_numero(argv[0]);
        fim = valor_para_numero(argv[1]);
    } else {
        inicio = valor_para_numero(argv[0]);
        fim = valor_para_numero(argv[1]);
        passo = valor_para_numero(argv[2]);
    }

    if (fabs(passo) < 1e-9) {
        noema_falhar("builtin 'range' nao aceita passo zero");
    }

    Lista *lista = lista_criar();
    if (passo > 0) {
        for (double valor = inicio; valor < fim; valor += passo) {
            lista_adicionar(lista, valor_numero(valor));
        }
    } else {
        for (double valor = inicio; valor > fim; valor += passo) {
            lista_adicionar(lista, valor_numero(valor));
        }
    }

    return valor_lista(lista);
}

static Valor builtin_map(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("map", argc, 2, 2);
    Lista *entrada = esperar_lista("map", argv[0]);
    Lista *saida = lista_criar();
    for (int i = 0; i < entrada->quantidade; i++) {
        lista_adicionar(saida, chamar_callback_com_item(interpretador, ambiente, argv[1], entrada->itens[i], i));
    }
    return valor_lista(saida);
}

static Valor builtin_filter(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("filter", argc, 2, 2);
    Lista *entrada = esperar_lista("filter", argv[0]);
    Lista *saida = lista_criar();
    for (int i = 0; i < entrada->quantidade; i++) {
        if (valor_eh_verdadeiro(chamar_callback_com_item(interpretador, ambiente, argv[1], entrada->itens[i], i))) {
            lista_adicionar(saida, entrada->itens[i]);
        }
    }
    return valor_lista(saida);
}

static Valor builtin_reduce(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("reduce", argc, 3, 3);
    Lista *entrada = esperar_lista("reduce", argv[0]);
    Valor acumulador = argv[1];
    for (int i = 0; i < entrada->quantidade; i++) {
        Valor argumentos[3];
        argumentos[0] = acumulador;
        argumentos[1] = entrada->itens[i];
        argumentos[2] = valor_numero((double)i);
        acumulador = chamar_valor(interpretador, ambiente, argv[2], 3, argumentos);
    }
    return acumulador;
}

static Valor builtin_find(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("find", argc, 2, 2);
    Lista *entrada = esperar_lista("find", argv[0]);
    for (int i = 0; i < entrada->quantidade; i++) {
        if (valor_eh_verdadeiro(chamar_callback_com_item(interpretador, ambiente, argv[1], entrada->itens[i], i))) {
            return entrada->itens[i];
        }
    }
    return valor_nulo();
}

static Valor builtin_any(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("any", argc, 2, 2);
    Lista *entrada = esperar_lista("any", argv[0]);
    for (int i = 0; i < entrada->quantidade; i++) {
        if (valor_eh_verdadeiro(chamar_callback_com_item(interpretador, ambiente, argv[1], entrada->itens[i], i))) {
            return valor_bool(true);
        }
    }
    return valor_bool(false);
}

static Valor builtin_all(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("all", argc, 2, 2);
    Lista *entrada = esperar_lista("all", argv[0]);
    for (int i = 0; i < entrada->quantidade; i++) {
        if (!valor_eh_verdadeiro(chamar_callback_com_item(interpretador, ambiente, argv[1], entrada->itens[i], i))) {
            return valor_bool(false);
        }
    }
    return valor_bool(true);
}

static Valor builtin_join(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("join", argc, 2, 2);
    Lista *lista = esperar_lista("join", argv[0]);
    const char *separador = esperar_texto("join", argv[1]);
    char *texto = noema_duplicar("");
    size_t capacidade = 1;
    size_t comprimento = 0;
    for (int i = 0; i < lista->quantidade; i++) {
        char *parte = valor_para_texto(lista->itens[i]);
        size_t extra = strlen(parte) + (i > 0 ? strlen(separador) : 0);
        while (comprimento + extra + 1 > capacidade) {
            capacidade = capacidade == 1 ? 64 : capacidade * 2;
            texto = realloc(texto, capacidade);
            if (texto == NULL) {
                noema_falhar("memoria insuficiente");
            }
        }
        if (i > 0) {
            memcpy(texto + comprimento, separador, strlen(separador));
            comprimento += strlen(separador);
        }
        memcpy(texto + comprimento, parte, strlen(parte));
        comprimento += strlen(parte);
        texto[comprimento] = '\0';
    }
    return valor_texto(texto);
}

static Valor builtin_split(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("split", argc, 2, 2);
    const char *texto = esperar_texto("split", argv[0]);
    const char *separador = esperar_texto("split", argv[1]);
    Lista *lista = lista_criar();

    if (separador[0] == '\0') {
        for (size_t i = 0; texto[i] != '\0'; i++) {
            char buffer[2] = {texto[i], '\0'};
            lista_adicionar(lista, valor_texto(buffer));
        }
        return valor_lista(lista);
    }

    const char *cursor = texto;
    const char *achado = strstr(cursor, separador);
    while (achado != NULL) {
        size_t tamanho = (size_t)(achado - cursor);
        char *parte = malloc(tamanho + 1);
        if (parte == NULL) {
            noema_falhar("memoria insuficiente");
        }
        memcpy(parte, cursor, tamanho);
        parte[tamanho] = '\0';
        lista_adicionar(lista, valor_texto(parte));
        cursor = achado + strlen(separador);
        achado = strstr(cursor, separador);
    }
    lista_adicionar(lista, valor_texto(cursor));
    return valor_lista(lista);
}

static Valor builtin_slice(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("slice", argc, 2, 3);
    int inicio = (int)valor_para_numero(argv[1]);

    if (argv[0].tipo == VALOR_LISTA) {
        Lista *entrada = argv[0].como.lista;
        int fim = argc == 3 ? (int)valor_para_numero(argv[2]) : entrada->quantidade;
        inicio = normalizar_indice(inicio, entrada->quantidade);
        fim = normalizar_indice(fim, entrada->quantidade);
        if (fim < inicio) {
            fim = inicio;
        }
        Lista *saida = lista_criar();
        for (int i = inicio; i < fim; i++) {
            lista_adicionar(saida, entrada->itens[i]);
        }
        return valor_lista(saida);
    }

    if (argv[0].tipo == VALOR_TEXTO) {
        const char *texto = argv[0].como.texto;
        int tamanho = (int)strlen(texto);
        int fim = argc == 3 ? (int)valor_para_numero(argv[2]) : tamanho;
        inicio = normalizar_indice(inicio, tamanho);
        fim = normalizar_indice(fim, tamanho);
        if (fim < inicio) {
            fim = inicio;
        }
        char *saida = malloc((size_t)(fim - inicio) + 1);
        if (saida == NULL) {
            noema_falhar("memoria insuficiente");
        }
        memcpy(saida, texto + inicio, (size_t)(fim - inicio));
        saida[fim - inicio] = '\0';
        return valor_texto(saida);
    }

    noema_falhar("builtin 'slice' aceita list ou string");
}

static Valor builtin_reverse(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("reverse", argc, 1, 1);
    if (argv[0].tipo == VALOR_LISTA) {
        Lista *entrada = argv[0].como.lista;
        Lista *saida = lista_criar();
        for (int i = entrada->quantidade - 1; i >= 0; i--) {
            lista_adicionar(saida, entrada->itens[i]);
        }
        return valor_lista(saida);
    }
    if (argv[0].tipo == VALOR_TEXTO) {
        const char *texto = argv[0].como.texto;
        size_t tamanho = strlen(texto);
        char *saida = malloc(tamanho + 1);
        if (saida == NULL) {
            noema_falhar("memoria insuficiente");
        }
        for (size_t i = 0; i < tamanho; i++) {
            saida[i] = texto[tamanho - i - 1];
        }
        saida[tamanho] = '\0';
        return valor_texto(saida);
    }
    noema_falhar("builtin 'reverse' aceita list ou string");
}

static Valor builtin_sum(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("sum", argc, 1, 1);
    Lista *entrada = esperar_lista("sum", argv[0]);
    double soma = 0.0;
    for (int i = 0; i < entrada->quantidade; i++) {
        soma += valor_para_numero(entrada->itens[i]);
    }
    return valor_numero(soma);
}

static Valor builtin_starts_with(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("starts_with", argc, 2, 2);
    const char *texto = esperar_texto("starts_with", argv[0]);
    const char *prefixo = esperar_texto("starts_with", argv[1]);
    size_t tamanho_prefixo = strlen(prefixo);
    return valor_bool(strncmp(texto, prefixo, tamanho_prefixo) == 0);
}

static Valor builtin_ends_with(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("ends_with", argc, 2, 2);
    const char *texto = esperar_texto("ends_with", argv[0]);
    const char *sufixo = esperar_texto("ends_with", argv[1]);
    return valor_bool(noema_tem_sufixo(texto, sufixo));
}

static Valor builtin_contains(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("contains", argc, 2, 2);
    const char *texto = esperar_texto("contains", argv[0]);
    const char *trecho = esperar_texto("contains", argv[1]);
    return valor_bool(strstr(texto, trecho) != NULL);
}

static Valor builtin_index_of(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("index_of", argc, 2, 3);
    const char *texto = esperar_texto("index_of", argv[0]);
    const char *trecho = esperar_texto("index_of", argv[1]);
    int inicio = argc == 3 ? normalizar_indice((int)valor_para_numero(argv[2]), (int)strlen(texto)) : 0;
    const char *achado = strstr(texto + inicio, trecho);
    if (achado == NULL) {
        return valor_numero(-1);
    }
    return valor_numero((double)(achado - texto));
}

static Valor builtin_last_index_of(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("last_index_of", argc, 2, 2);
    const char *texto = esperar_texto("last_index_of", argv[0]);
    const char *trecho = esperar_texto("last_index_of", argv[1]);
    const char *cursor = texto;
    const char *ultimo = NULL;
    while ((cursor = strstr(cursor, trecho)) != NULL) {
        ultimo = cursor;
        cursor++;
    }
    return valor_numero(ultimo == NULL ? -1 : (double)(ultimo - texto));
}

static Valor builtin_repeat(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("repeat", argc, 2, 2);
    const char *texto = esperar_texto("repeat", argv[0]);
    int quantidade = (int)valor_para_numero(argv[1]);
    if (quantidade <= 0) {
        return valor_texto("");
    }
    size_t tamanho = strlen(texto);
    char *saida = malloc(tamanho * (size_t)quantidade + 1);
    if (saida == NULL) {
        noema_falhar("memoria insuficiente");
    }
    size_t cursor = 0;
    for (int i = 0; i < quantidade; i++) {
        memcpy(saida + cursor, texto, tamanho);
        cursor += tamanho;
    }
    saida[cursor] = '\0';
    return valor_texto(saida);
}

static Valor builtin_char_at(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("char_at", argc, 2, 2);
    const char *texto = esperar_texto("char_at", argv[0]);
    int indice = normalizar_indice((int)valor_para_numero(argv[1]), (int)strlen(texto));
    if (indice < 0 || (size_t)indice >= strlen(texto)) {
        return valor_nulo();
    }
    char buffer[2] = {texto[indice], '\0'};
    return valor_texto(buffer);
}

static Valor builtin_ord(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("ord", argc, 1, 1);
    const char *texto = esperar_texto("ord", argv[0]);
    if (texto[0] == '\0') {
        return valor_nulo();
    }
    return valor_numero((double)(unsigned char)texto[0]);
}

static Valor builtin_chr(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("chr", argc, 1, 1);
    char buffer[2];
    buffer[0] = (char)((unsigned int)((int)valor_para_numero(argv[0])) & 0xFFu);
    buffer[1] = '\0';
    return valor_texto(buffer);
}

static Valor builtin_lower(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("lower", argc, 1, 1);
    const char *texto = esperar_texto("lower", argv[0]);
    char *saida = noema_duplicar(texto);
    for (size_t i = 0; saida[i] != '\0'; i++) {
        saida[i] = (char)tolower((unsigned char)saida[i]);
    }
    return valor_texto(saida);
}

static Valor builtin_upper(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("upper", argc, 1, 1);
    const char *texto = esperar_texto("upper", argv[0]);
    char *saida = noema_duplicar(texto);
    for (size_t i = 0; saida[i] != '\0'; i++) {
        saida[i] = (char)toupper((unsigned char)saida[i]);
    }
    return valor_texto(saida);
}

static Valor builtin_trim(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("trim", argc, 1, 1);
    char *saida = recortar_espacos(esperar_texto("trim", argv[0]), true, true);
    return valor_texto(saida);
}

static Valor builtin_trim_left(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("trim_left", argc, 1, 1);
    return valor_texto(recortar_espacos(esperar_texto("trim_left", argv[0]), true, false));
}

static Valor builtin_trim_right(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("trim_right", argc, 1, 1);
    return valor_texto(recortar_espacos(esperar_texto("trim_right", argv[0]), false, true));
}

static Valor builtin_replace(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("replace", argc, 3, 3);
    const char *texto = esperar_texto("replace", argv[0]);
    const char *busca = esperar_texto("replace", argv[1]);
    const char *troca = esperar_texto("replace", argv[2]);

    if (busca[0] == '\0') {
        return valor_texto(texto);
    }

    char *saida = noema_duplicar("");
    size_t capacidade = 1;
    size_t comprimento = 0;
    const char *cursor = texto;
    const char *achado = strstr(cursor, busca);
    while (achado != NULL) {
        size_t trecho = (size_t)(achado - cursor);
        size_t extra = trecho + strlen(troca);
        while (comprimento + extra + 1 > capacidade) {
            capacidade = capacidade == 1 ? 64 : capacidade * 2;
            saida = realloc(saida, capacidade);
            if (saida == NULL) {
                noema_falhar("memoria insuficiente");
            }
        }
        memcpy(saida + comprimento, cursor, trecho);
        comprimento += trecho;
        memcpy(saida + comprimento, troca, strlen(troca));
        comprimento += strlen(troca);
        saida[comprimento] = '\0';
        cursor = achado + strlen(busca);
        achado = strstr(cursor, busca);
    }
    size_t resto = strlen(cursor);
    while (comprimento + resto + 1 > capacidade) {
        capacidade = capacidade == 1 ? 64 : capacidade * 2;
        saida = realloc(saida, capacidade);
        if (saida == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    memcpy(saida + comprimento, cursor, resto);
    comprimento += resto;
    saida[comprimento] = '\0';
    return valor_texto(saida);
}

static Valor builtin_normalize_text(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("normalize_text", argc, 1, 1);
    return valor_texto(normalizar_texto_para_padrao(esperar_texto("normalize_text", argv[0])));
}

static Valor builtin_match_pattern(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("match_pattern", argc, 2, 2);
    const char *padrao = esperar_texto("match_pattern", argv[0]);
    const char *texto = esperar_texto("match_pattern", argv[1]);
    Lista *capturas = lista_criar();
    bool ok = casar_padrao(padrao, texto, capturas);
    Mapa *resultado = mapa_criar();
    mapa_definir(resultado, "ok", valor_bool(ok));
    mapa_definir(resultado, "pattern", valor_texto(padrao));
    mapa_definir(resultado, "text", valor_texto(texto));
    mapa_definir(resultado, "captures", valor_lista(capturas));
    mapa_definir(resultado, "count", valor_numero((double)capturas->quantidade));
    return valor_mapa(resultado);
}

static Valor builtin_render_template(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("render_template", argc, 2, 2);
    const char *modelo = esperar_texto("render_template", argv[0]);
    return valor_texto(renderizar_modelo(modelo, argv[1]));
}

static Valor builtin_rand(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)ambiente;
    (void)argv;
    (void)dados;
    garantir_argumentos("rand", argc, 0, 0);
    return valor_numero((double)(proximo_aleatorio(interpretador) % 1000000u) / 1000000.0);
}

static Valor builtin_rand_int(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)ambiente;
    (void)dados;
    garantir_argumentos("rand_int", argc, 1, 1);
    int limite = (int)valor_para_numero(argv[0]);
    if (limite <= 0) {
        noema_falhar("builtin 'rand_int' exige limite positivo");
    }
    return valor_numero((double)(proximo_aleatorio(interpretador) % (unsigned int)limite));
}

static Valor builtin_choice(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)ambiente;
    (void)dados;
    garantir_argumentos("choice", argc, 1, 1);
    Lista *lista = esperar_lista("choice", argv[0]);
    if (lista->quantidade == 0) {
        return valor_nulo();
    }
    return lista->itens[proximo_aleatorio(interpretador) % (unsigned int)lista->quantidade];
}

static Valor builtin_eval(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("eval", argc, 1, 1);
    const char *codigo = esperar_texto("eval", argv[0]);
    return interpretador_executar_codigo(interpretador, ambiente, codigo, "<eval>");
}

static Valor builtin_try_eval(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("try_eval", argc, 1, 2);
    const char *codigo = esperar_texto("try_eval", argv[0]);
    const char *caminho_virtual = argc == 2 ? esperar_texto("try_eval", argv[1]) : "<try_eval>";
    Valor valor = valor_nulo();
    char *erro = NULL;
    bool ok = interpretador_tentar_executar_codigo(interpretador, ambiente, codigo, caminho_virtual, &valor, &erro);
    Mapa *resultado = mapa_criar();
    mapa_definir(resultado, "ok", valor_bool(ok));
    mapa_definir(resultado, "value", ok ? valor : valor_nulo());
    mapa_definir(resultado, "error", ok ? valor_nulo() : valor_texto(erro));
    return valor_mapa(resultado);
}

static Valor builtin_sin(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("sin", argc, 1, 1);
    return valor_numero(sin(valor_para_numero(argv[0])));
}

static Valor builtin_cos(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("cos", argc, 1, 1);
    return valor_numero(cos(valor_para_numero(argv[0])));
}

static Valor builtin_tan(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("tan", argc, 1, 1);
    return valor_numero(tan(valor_para_numero(argv[0])));
}

static Valor builtin_asin(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("asin", argc, 1, 1);
    return valor_numero(asin(valor_para_numero(argv[0])));
}

static Valor builtin_acos(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("acos", argc, 1, 1);
    return valor_numero(acos(valor_para_numero(argv[0])));
}

static Valor builtin_atan(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("atan", argc, 1, 1);
    return valor_numero(atan(valor_para_numero(argv[0])));
}

static Valor builtin_atan2(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("atan2", argc, 2, 2);
    return valor_numero(atan2(valor_para_numero(argv[0]), valor_para_numero(argv[1])));
}

static Valor builtin_sqrt(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("sqrt", argc, 1, 1);
    return valor_numero(sqrt(valor_para_numero(argv[0])));
}

static Valor builtin_hypot(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("hypot", argc, 2, 2);
    return valor_numero(hypot(valor_para_numero(argv[0]), valor_para_numero(argv[1])));
}

static Valor builtin_abs(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("abs", argc, 1, 1);
    return valor_numero(fabs(valor_para_numero(argv[0])));
}

static Valor builtin_floor(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("floor", argc, 1, 1);
    return valor_numero(floor(valor_para_numero(argv[0])));
}

static Valor builtin_ceil(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("ceil", argc, 1, 1);
    return valor_numero(ceil(valor_para_numero(argv[0])));
}

static Valor builtin_round(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("round", argc, 1, 1);
    return valor_numero(round(valor_para_numero(argv[0])));
}

static Valor builtin_trunc(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("trunc", argc, 1, 1);
    return valor_numero(trunc(valor_para_numero(argv[0])));
}

static Valor builtin_pow(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("pow", argc, 2, 2);
    return valor_numero(pow(valor_para_numero(argv[0]), valor_para_numero(argv[1])));
}

static Valor builtin_mod(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("mod", argc, 2, 2);
    return valor_numero(fmod(valor_para_numero(argv[0]), valor_para_numero(argv[1])));
}

static Valor builtin_sign(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("sign", argc, 1, 1);
    double valor = valor_para_numero(argv[0]);
    if (valor > 0.0) {
        return valor_numero(1);
    }
    if (valor < 0.0) {
        return valor_numero(-1);
    }
    return valor_numero(0);
}

static Valor builtin_min(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("min", argc, 2, 2);
    double a = valor_para_numero(argv[0]);
    double b = valor_para_numero(argv[1]);
    return valor_numero(a < b ? a : b);
}

static Valor builtin_max(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("max", argc, 2, 2);
    double a = valor_para_numero(argv[0]);
    double b = valor_para_numero(argv[1]);
    return valor_numero(a > b ? a : b);
}

static Valor builtin_log(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("log", argc, 1, 1);
    return valor_numero(log(valor_para_numero(argv[0])));
}

static Valor builtin_exp(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("exp", argc, 1, 1);
    return valor_numero(exp(valor_para_numero(argv[0])));
}

static Valor builtin_is_nan(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_nan", argc, 1, 1);
    return valor_bool(isnan(valor_para_numero(argv[0])));
}

static Valor builtin_is_finite(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_finite", argc, 1, 1);
    return valor_bool(isfinite(valor_para_numero(argv[0])));
}

static Valor builtin_read_file(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("read_file", argc, 1, 1);
    const char *caminho = esperar_texto("read_file", argv[0]);
    FILE *arquivo = fopen(caminho, "rb");
    if (arquivo == NULL) {
        noema_falhar("builtin 'read_file' nao conseguiu abrir '%s': %s", caminho, strerror(errno));
    }
    if (fseek(arquivo, 0, SEEK_END) != 0) {
        fclose(arquivo);
        noema_falhar("builtin 'read_file' nao conseguiu ler '%s'", caminho);
    }
    long tamanho_longo = ftell(arquivo);
    if (tamanho_longo < 0) {
        fclose(arquivo);
        noema_falhar("builtin 'read_file' nao conseguiu medir '%s'", caminho);
    }
    rewind(arquivo);
    size_t tamanho = (size_t)tamanho_longo;
    char *conteudo = malloc(tamanho + 1);
    if (conteudo == NULL) {
        fclose(arquivo);
        noema_falhar("memoria insuficiente");
    }
    size_t lidos = fread(conteudo, 1, tamanho, arquivo);
    fclose(arquivo);
    if (lidos != tamanho) {
        noema_falhar("builtin 'read_file' nao conseguiu ler '%s' por completo", caminho);
    }
    conteudo[tamanho] = '\0';
    return valor_texto(conteudo);
}

static Valor builtin_write_file(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("write_file", argc, 2, 2);
    const char *caminho = esperar_texto("write_file", argv[0]);
    char *conteudo = valor_para_texto(argv[1]);
    return escrever_arquivo("write_file", caminho, conteudo, "wb");
}

static Valor builtin_append_file(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("append_file", argc, 2, 2);
    const char *caminho = esperar_texto("append_file", argv[0]);
    char *conteudo = valor_para_texto(argv[1]);
    return escrever_arquivo("append_file", caminho, conteudo, "ab");
}

static Valor builtin_file_exists(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("file_exists", argc, 1, 1);
    return valor_bool(caminho_existe(esperar_texto("file_exists", argv[0])));
}

static Valor builtin_is_file(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_file", argc, 1, 1);
    return valor_bool(caminho_eh_arquivo(esperar_texto("is_file", argv[0])));
}

static Valor builtin_is_dir(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("is_dir", argc, 1, 1);
    return valor_bool(caminho_eh_diretorio(esperar_texto("is_dir", argv[0])));
}

static Valor builtin_list_dir(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("list_dir", argc, 1, 1);
    const char *caminho = esperar_texto("list_dir", argv[0]);
    Lista *saida = lista_criar();

#ifdef _WIN32
    char *padrao = noema_formatar("%s%c*", caminho, noema_obter_separador_caminho());
    WIN32_FIND_DATAA entrada;
    HANDLE busca = FindFirstFileA(padrao, &entrada);
    if (busca == INVALID_HANDLE_VALUE) {
        free(padrao);
        noema_falhar("builtin 'list_dir' nao conseguiu abrir '%s': %s", caminho, strerror(errno));
    }
    free(padrao);

    do {
        if (strcmp(entrada.cFileName, ".") == 0 || strcmp(entrada.cFileName, "..") == 0) {
            continue;
        }
        lista_adicionar(saida, valor_texto(entrada.cFileName));
    } while (FindNextFileA(busca, &entrada) != 0);

    FindClose(busca);
#else
    DIR *diretorio = opendir(caminho);
    if (diretorio == NULL) {
        noema_falhar("builtin 'list_dir' nao conseguiu abrir '%s': %s", caminho, strerror(errno));
    }
    struct dirent *entrada = NULL;
    while ((entrada = readdir(diretorio)) != NULL) {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) {
            continue;
        }
        lista_adicionar(saida, valor_texto(entrada->d_name));
    }
    closedir(diretorio);
#endif

    return valor_lista(saida);
}

static Valor builtin_make_dir(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("make_dir", argc, 1, 2);
    const char *caminho = esperar_texto("make_dir", argv[0]);
    bool recursivo = argc == 2 ? valor_eh_verdadeiro(argv[1]) : false;
    criar_diretorios("make_dir", caminho, recursivo);
    return valor_bool(true);
}

static Valor builtin_cwd(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    (void)argv;
    garantir_argumentos("cwd", argc, 0, 0);
    char caminho[4096];
    if (noema_obter_cwd_sistema(caminho, sizeof(caminho)) == NULL) {
        noema_falhar("builtin 'cwd' nao conseguiu obter diretorio atual: %s", strerror(errno));
    }
    return valor_texto(caminho);
}

static Valor builtin_change_dir(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("change_dir", argc, 1, 1);
    const char *caminho = esperar_texto("change_dir", argv[0]);
    Mapa *resultado = mapa_criar();
    mapa_definir(resultado, "target", valor_texto(caminho));
    if (noema_mudar_diretorio_sistema(caminho) != 0) {
        mapa_definir(resultado, "ok", valor_bool(false));
        mapa_definir(resultado, "path", builtin_cwd(interpretador, ambiente, 0, NULL, NULL));
        mapa_definir(resultado, "error", valor_texto(strerror(errno)));
        return valor_mapa(resultado);
    }
    mapa_definir(resultado, "ok", valor_bool(true));
    mapa_definir(resultado, "path", builtin_cwd(interpretador, ambiente, 0, NULL, NULL));
    mapa_definir(resultado, "error", valor_nulo());
    return valor_mapa(resultado);
}

static Valor builtin_shell_run(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("shell_run", argc, 1, 1);
    const char *comando = esperar_texto("shell_run", argv[0]);
    char *comando_completo = noema_formatar("(%s) 2>&1", comando);
    FILE *processo = noema_abrir_processo(comando_completo);
    if (processo == NULL) {
        noema_falhar("builtin 'shell_run' nao conseguiu iniciar shell: %s", strerror(errno));
    }
    char *saida = ler_fluxo_texto(processo);
    int status = noema_fechar_processo(processo);
    int codigo = status;
#ifndef _WIN32
    if (status >= 0 && WIFEXITED(status)) {
        codigo = WEXITSTATUS(status);
    }
#endif
    Mapa *resultado = mapa_criar();
    mapa_definir(resultado, "command", valor_texto(comando));
    mapa_definir(resultado, "output", valor_texto(saida));
    mapa_definir(resultado, "code", valor_numero((double)codigo));
    mapa_definir(resultado, "ok", valor_bool(codigo == 0));
    return valor_mapa(resultado);
}

static Valor builtin_path_join(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("path_join", argc, 1, 64);
    char *saida = noema_duplicar("");
    size_t capacidade = 1;
    size_t comprimento = 0;

    for (int i = 0; i < argc; i++) {
        const char *parte_original = esperar_texto("path_join", argv[i]);
        if (parte_original[0] == '\0') {
            continue;
        }

        const char *parte = parte_original;
        size_t tamanho = strlen(parte);
        if (comprimento > 0) {
            while (*parte != '\0' && noema_eh_separador_caminho(*parte)) {
                parte++;
            }
            tamanho = strlen(parte);
        }
        if (tamanho == 0) {
            continue;
        }

        bool precisa_barra = comprimento > 0 && !noema_eh_separador_caminho(saida[comprimento - 1]);
        size_t extra = tamanho + (precisa_barra ? 1 : 0);
        while (comprimento + extra + 1 > capacidade) {
            capacidade = capacidade == 1 ? 64 : capacidade * 2;
            saida = realloc(saida, capacidade);
            if (saida == NULL) {
                noema_falhar("memoria insuficiente");
            }
        }
        if (precisa_barra) {
            saida[comprimento++] = noema_obter_separador_caminho();
        }
        memcpy(saida + comprimento, parte, tamanho);
        comprimento += tamanho;
        saida[comprimento] = '\0';
    }

    return valor_texto(saida);
}

static Valor builtin_path_dirname(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("path_dirname", argc, 1, 1);
    return valor_texto(dirname_de_caminho(esperar_texto("path_dirname", argv[0])));
}

static Valor builtin_path_basename(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("path_basename", argc, 1, 1);
    return valor_texto(basename_de_caminho(esperar_texto("path_basename", argv[0])));
}

static Valor builtin_path_extname(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("path_extname", argc, 1, 1);
    return valor_texto(extensao_de_caminho(esperar_texto("path_extname", argv[0])));
}

static Valor builtin_path_stem(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("path_stem", argc, 1, 1);
    return valor_texto(stem_de_caminho(esperar_texto("path_stem", argv[0])));
}

static Valor builtin_make_agent(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("make_agent", argc, 1, 1);
    Mapa *agente = mapa_criar();
    mapa_definir(agente, "kind", valor_texto("agent"));
    mapa_definir(agente, "name", argv[0]);
    mapa_definir(agente, "patterns", valor_lista(lista_criar()));
    mapa_definir(agente, "fallback", valor_texto("Tell me more."));
    mapa_definir(agente, "memory", valor_mapa(mapa_criar()));
    return valor_mapa(agente);
}

static Valor builtin_add_pattern(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("add_pattern", argc, 3, 3);
    Mapa *agente = esperar_mapa("add_pattern", argv[0]);
    Lista *padroes = lista_em(agente, "patterns");
    Mapa *padrao = mapa_criar();
    mapa_definir(padrao, "pattern", argv[1]);
    mapa_definir(padrao, "response", argv[2]);
    lista_adicionar(padroes, valor_mapa(padrao));
    return argv[0];
}

static Valor builtin_set_fallback(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("set_fallback", argc, 2, 2);
    mapa_definir(esperar_mapa("set_fallback", argv[0]), "fallback", argv[1]);
    return argv[0];
}

static Valor builtin_respond(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("respond", argc, 2, 2);
    Mapa *agente = esperar_mapa("respond", argv[0]);
    const char *entrada = esperar_texto("respond", argv[1]);
    Lista *padroes = lista_em(agente, "patterns");
    for (int i = 0; i < padroes->quantidade; i++) {
        Mapa *item = esperar_mapa("respond", padroes->itens[i]);
        Valor padrao_valor;
        Valor resposta;
        if (!mapa_obter(item, "pattern", &padrao_valor) || !mapa_obter(item, "response", &resposta)) {
            continue;
        }
        if (padrao_valor.tipo != VALOR_TEXTO) {
            continue;
        }
        Lista *capturas = lista_criar();
        if (casar_padrao(padrao_valor.como.texto, entrada, capturas)) {
            return responder_com_valor(interpretador, ambiente, resposta, capturas);
        }
    }
    Valor fallback;
    if (!mapa_obter(agente, "fallback", &fallback)) {
        return valor_nulo();
    }
    return responder_com_valor(interpretador, ambiente, fallback, lista_criar());
}

static Valor builtin_remember(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("remember", argc, 3, 3);
    Mapa *agente = esperar_mapa("remember", argv[0]);
    Mapa *memoria = mapa_em(agente, "memory");
    char *chave = valor_para_texto(argv[1]);
    mapa_definir(memoria, chave, argv[2]);
    return argv[0];
}

static Valor builtin_recall(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("recall", argc, 2, 2);
    Mapa *agente = esperar_mapa("recall", argv[0]);
    Mapa *memoria = mapa_em(agente, "memory");
    char *chave = valor_para_texto(argv[1]);
    Valor valor;
    if (mapa_obter(memoria, chave, &valor)) {
        return valor;
    }
    return valor_nulo();
}

static Valor builtin_expert(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("expert", argc, 1, 1);
    Mapa *sistema = mapa_criar();
    mapa_definir(sistema, "kind", valor_texto("expert"));
    mapa_definir(sistema, "name", argv[0]);
    mapa_definir(sistema, "facts", valor_mapa(mapa_criar()));
    mapa_definir(sistema, "rules", valor_lista(lista_criar()));
    mapa_definir(sistema, "trace", valor_lista(lista_criar()));
    return valor_mapa(sistema);
}

static Valor builtin_set_fact(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("set_fact", argc, 3, 3);
    Mapa *sistema = esperar_mapa("set_fact", argv[0]);
    Mapa *fatos = mapa_em(sistema, "facts");
    char *chave = valor_para_texto(argv[1]);
    mapa_definir(fatos, chave, argv[2]);
    return argv[2];
}

static Valor builtin_get_fact(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("get_fact", argc, 2, 2);
    Mapa *sistema = esperar_mapa("get_fact", argv[0]);
    Mapa *fatos = mapa_em(sistema, "facts");
    char *chave = valor_para_texto(argv[1]);
    Valor valor;
    if (mapa_obter(fatos, chave, &valor)) {
        return valor;
    }
    return valor_nulo();
}

static Valor builtin_add_rule(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("add_rule", argc, 4, 4);
    Mapa *sistema = esperar_mapa("add_rule", argv[0]);
    Lista *regras = lista_em(sistema, "rules");
    Mapa *regra = mapa_criar();
    mapa_definir(regra, "name", argv[1]);
    mapa_definir(regra, "condition", argv[2]);
    mapa_definir(regra, "action", argv[3]);
    mapa_definir(regra, "fired", valor_bool(false));
    lista_adicionar(regras, valor_mapa(regra));
    return argv[0];
}

static Valor builtin_run_rules(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)dados;
    garantir_argumentos("run_rules", argc, 1, 2);
    Mapa *sistema = esperar_mapa("run_rules", argv[0]);
    int limite = argc == 2 ? (int)valor_para_numero(argv[1]) : 32;
    Lista *regras = lista_em(sistema, "rules");
    Lista *trilha = lista_em(sistema, "trace");

    for (int ciclo = 0; ciclo < limite; ciclo++) {
        bool mudou = false;
        for (int i = 0; i < regras->quantidade; i++) {
            Mapa *regra = esperar_mapa("run_rules", regras->itens[i]);
            Valor fired;
            mapa_obter(regra, "fired", &fired);
            if (valor_eh_verdadeiro(fired)) {
                continue;
            }
            Valor condicao;
            Valor acao;
            Valor nome;
            mapa_obter(regra, "condition", &condicao);
            mapa_obter(regra, "action", &acao);
            mapa_obter(regra, "name", &nome);
            Valor argumento = argv[0];
            Valor resultado = chamar_valor(interpretador, ambiente, condicao, 1, &argumento);
            if (valor_eh_verdadeiro(resultado)) {
                chamar_valor(interpretador, ambiente, acao, 1, &argumento);
                mapa_definir(regra, "fired", valor_bool(true));
                lista_adicionar(trilha, nome);
                mudou = true;
            }
        }
        if (!mudou) {
            break;
        }
    }
    return argv[0];
}

static Valor builtin_reset_rules(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("reset_rules", argc, 1, 1);
    Mapa *sistema = esperar_mapa("reset_rules", argv[0]);
    Lista *regras = lista_em(sistema, "rules");
    for (int i = 0; i < regras->quantidade; i++) {
        Mapa *regra = esperar_mapa("reset_rules", regras->itens[i]);
        mapa_definir(regra, "fired", valor_bool(false));
    }
    Lista *trilha = lista_em(sistema, "trace");
    trilha->quantidade = 0;
    return argv[0];
}

static Valor builtin_trace(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("trace", argc, 1, 1);
    Mapa *sistema = esperar_mapa("trace", argv[0]);
    return valor_lista(lista_em(sistema, "trace"));
}

static Valor builtin_world(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("world", argc, 1, 1);
    Mapa *mundo = mapa_criar();
    mapa_definir(mundo, "kind", valor_texto("world"));
    mapa_definir(mundo, "name", argv[0]);
    mapa_definir(mundo, "placements", valor_mapa(mapa_criar()));
    return valor_mapa(mundo);
}

static Valor builtin_place(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("place", argc, 3, 3);
    Mapa *mundo = esperar_mapa("place", argv[0]);
    Mapa *colocacoes = mapa_em(mundo, "placements");
    char *objeto = valor_para_texto(argv[1]);
    mapa_definir(colocacoes, objeto, argv[2]);
    return argv[0];
}

static Valor builtin_where(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("where", argc, 2, 2);
    Mapa *mundo = esperar_mapa("where", argv[0]);
    Mapa *colocacoes = mapa_em(mundo, "placements");
    char *objeto = valor_para_texto(argv[1]);
    Valor resultado;
    if (mapa_obter(colocacoes, objeto, &resultado)) {
        return resultado;
    }
    return valor_nulo();
}

static Valor builtin_objects_on(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("objects_on", argc, 2, 2);
    Mapa *mundo = esperar_mapa("objects_on", argv[0]);
    Mapa *colocacoes = mapa_em(mundo, "placements");
    char *suporte = valor_para_texto(argv[1]);
    Lista *lista = lista_criar();
    for (int i = 0; i < colocacoes->quantidade; i++) {
        Valor destino = colocacoes->entradas[i].valor;
        if (destino.tipo == VALOR_TEXTO && strcmp(destino.como.texto, suporte) == 0) {
            lista_adicionar(lista, valor_texto(colocacoes->entradas[i].chave));
        }
    }
    return valor_lista(lista);
}

static Valor builtin_flock(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)argv;
    (void)dados;
    garantir_argumentos("flock", argc, 0, 0);
    Mapa *flock = mapa_criar();
    mapa_definir(flock, "kind", valor_texto("flock"));
    mapa_definir(flock, "boids", valor_lista(lista_criar()));
    return valor_mapa(flock);
}

static Valor builtin_add_boid(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("add_boid", argc, 2, 2);
    Mapa *flock = esperar_mapa("add_boid", argv[0]);
    Lista *boids = lista_em(flock, "boids");
    lista_adicionar(boids, argv[1]);
    return argv[0];
}

static double coordenada(Mapa *boid, const char *chave) {
    Valor valor;
    if (!mapa_obter(boid, chave, &valor)) {
        return 0.0;
    }
    return valor_para_numero(valor);
}

static Valor builtin_step_flock(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    (void)dados;
    garantir_argumentos("step_flock", argc, 7, 7);
    Mapa *flock = esperar_mapa("step_flock", argv[0]);
    Lista *boids = lista_em(flock, "boids");
    double largura = valor_para_numero(argv[1]);
    double altura = valor_para_numero(argv[2]);
    double peso_separacao = valor_para_numero(argv[3]);
    double peso_alinhamento = valor_para_numero(argv[4]);
    double peso_coesao = valor_para_numero(argv[5]);
    double velocidade_maxima = valor_para_numero(argv[6]);
    const double raio = 40.0;
    const double raio_separacao = 14.0;

    Lista *novos = lista_criar();

    for (int i = 0; i < boids->quantidade; i++) {
        Mapa *boid = esperar_mapa("step_flock", boids->itens[i]);
        double x = coordenada(boid, "x");
        double y = coordenada(boid, "y");
        double vx = coordenada(boid, "vx");
        double vy = coordenada(boid, "vy");

        double media_x = 0.0;
        double media_y = 0.0;
        double media_vx = 0.0;
        double media_vy = 0.0;
        double afastar_x = 0.0;
        double afastar_y = 0.0;
        int vizinhos = 0;

        for (int j = 0; j < boids->quantidade; j++) {
            if (i == j) {
                continue;
            }
            Mapa *outro = esperar_mapa("step_flock", boids->itens[j]);
            double ox = coordenada(outro, "x");
            double oy = coordenada(outro, "y");
            double dx = ox - x;
            double dy = oy - y;
            double distancia = sqrt(dx * dx + dy * dy);
            if (distancia <= raio) {
                media_x += ox;
                media_y += oy;
                media_vx += coordenada(outro, "vx");
                media_vy += coordenada(outro, "vy");
                vizinhos++;
            }
            if (distancia > 0.0 && distancia <= raio_separacao) {
                afastar_x -= dx / distancia;
                afastar_y -= dy / distancia;
            }
        }

        if (vizinhos > 0) {
            media_x /= vizinhos;
            media_y /= vizinhos;
            media_vx /= vizinhos;
            media_vy /= vizinhos;

            vx += afastar_x * peso_separacao;
            vy += afastar_y * peso_separacao;
            vx += (media_vx - vx) * peso_alinhamento;
            vy += (media_vy - vy) * peso_alinhamento;
            vx += (media_x - x) * peso_coesao * 0.01;
            vy += (media_y - y) * peso_coesao * 0.01;
        }

        double velocidade = sqrt(vx * vx + vy * vy);
        if (velocidade > velocidade_maxima && velocidade > 0.0) {
            vx = (vx / velocidade) * velocidade_maxima;
            vy = (vy / velocidade) * velocidade_maxima;
        }

        x += vx;
        y += vy;

        while (x < 0.0) x += largura;
        while (y < 0.0) y += altura;
        while (x >= largura) x -= largura;
        while (y >= altura) y -= altura;

        Mapa *novo = mapa_criar();
        mapa_definir(novo, "x", valor_numero(x));
        mapa_definir(novo, "y", valor_numero(y));
        mapa_definir(novo, "vx", valor_numero(vx));
        mapa_definir(novo, "vy", valor_numero(vy));
        lista_adicionar(novos, valor_mapa(novo));
    }

    mapa_definir(flock, "boids", valor_lista(novos));
    return argv[0];
}

void registrar_builtins(Interpretador *interpretador) {
    registrar_nativa(interpretador, "print", builtin_print, NULL);
    registrar_nativa(interpretador, "write", builtin_write, NULL);
    registrar_nativa(interpretador, "flush", builtin_flush, NULL);
    registrar_nativa(interpretador, "read_line", builtin_read_line, NULL);
    registrar_nativa(interpretador, "read_stdin", builtin_read_stdin, NULL);
    registrar_nativa(interpretador, "panic", builtin_panic, NULL);
    registrar_nativa(interpretador, "type", builtin_type, NULL);
    registrar_nativa(interpretador, "is_null", builtin_is_null, NULL);
    registrar_nativa(interpretador, "is_bool", builtin_is_bool, NULL);
    registrar_nativa(interpretador, "is_number", builtin_is_number, NULL);
    registrar_nativa(interpretador, "is_string", builtin_is_string, NULL);
    registrar_nativa(interpretador, "is_list", builtin_is_list, NULL);
    registrar_nativa(interpretador, "is_map", builtin_is_map, NULL);
    registrar_nativa(interpretador, "is_function", builtin_is_function, NULL);
    registrar_nativa(interpretador, "is_pointer", builtin_is_pointer, NULL);
    registrar_nativa(interpretador, "is_syntax", builtin_is_syntax, NULL);
    registrar_nativa(interpretador, "syntax_kind", builtin_syntax_kind, NULL);
    registrar_nativa(interpretador, "syntax_text", builtin_syntax_text, NULL);
    registrar_nativa(interpretador, "syntax_parts", builtin_syntax_parts, NULL);
    registrar_nativa(interpretador, "syntax_identifier", builtin_syntax_identifier, NULL);
    registrar_nativa(interpretador, "syntax_literal", builtin_syntax_literal, NULL);
    registrar_nativa(interpretador, "syntax_block", builtin_syntax_block, NULL);
    registrar_nativa(interpretador, "syntax_let", builtin_syntax_let, NULL);
    registrar_nativa(interpretador, "syntax_const", builtin_syntax_const, NULL);
    registrar_nativa(interpretador, "syntax_function", builtin_syntax_function, NULL);
    registrar_nativa(interpretador, "memory_alloc", builtin_memory_alloc, NULL);
    registrar_nativa(interpretador, "memory_free", builtin_memory_free, NULL);
    registrar_nativa(interpretador, "memory_fill", builtin_memory_fill, NULL);
    registrar_nativa(interpretador, "memory_get_u8", builtin_memory_get_u8, NULL);
    registrar_nativa(interpretador, "memory_get_i32", builtin_memory_get_i32, NULL);
    registrar_nativa(interpretador, "memory_get_u32", builtin_memory_get_u32, NULL);
    registrar_nativa(interpretador, "memory_set_u8", builtin_memory_set_u8, NULL);
    registrar_nativa(interpretador, "memory_set_i32", builtin_memory_set_i32, NULL);
    registrar_nativa(interpretador, "memory_set_u32", builtin_memory_set_u32, NULL);
    registrar_nativa(interpretador, "string", builtin_string, NULL);
    registrar_nativa(interpretador, "number", builtin_number, NULL);
    registrar_nativa(interpretador, "int", builtin_int, NULL);
    registrar_nativa(interpretador, "bool", builtin_bool, NULL);
    registrar_nativa(interpretador, "parse_number", builtin_parse_number, NULL);
    registrar_nativa(interpretador, "parse_int", builtin_parse_int, NULL);
    registrar_nativa(interpretador, "len", builtin_len, NULL);
    registrar_nativa(interpretador, "push", builtin_push, NULL);
    registrar_nativa(interpretador, "pop", builtin_pop, NULL);
    registrar_nativa(interpretador, "keys", builtin_keys, NULL);
    registrar_nativa(interpretador, "values", builtin_values, NULL);
    registrar_nativa(interpretador, "get", builtin_get, NULL);
    registrar_nativa(interpretador, "has", builtin_has, NULL);
    registrar_nativa(interpretador, "range", builtin_range, NULL);
    registrar_nativa(interpretador, "map", builtin_map, NULL);
    registrar_nativa(interpretador, "filter", builtin_filter, NULL);
    registrar_nativa(interpretador, "reduce", builtin_reduce, NULL);
    registrar_nativa(interpretador, "find", builtin_find, NULL);
    registrar_nativa(interpretador, "any", builtin_any, NULL);
    registrar_nativa(interpretador, "all", builtin_all, NULL);
    registrar_nativa(interpretador, "join", builtin_join, NULL);
    registrar_nativa(interpretador, "split", builtin_split, NULL);
    registrar_nativa(interpretador, "slice", builtin_slice, NULL);
    registrar_nativa(interpretador, "reverse", builtin_reverse, NULL);
    registrar_nativa(interpretador, "sum", builtin_sum, NULL);
    registrar_nativa(interpretador, "starts_with", builtin_starts_with, NULL);
    registrar_nativa(interpretador, "ends_with", builtin_ends_with, NULL);
    registrar_nativa(interpretador, "contains", builtin_contains, NULL);
    registrar_nativa(interpretador, "index_of", builtin_index_of, NULL);
    registrar_nativa(interpretador, "last_index_of", builtin_last_index_of, NULL);
    registrar_nativa(interpretador, "repeat", builtin_repeat, NULL);
    registrar_nativa(interpretador, "char_at", builtin_char_at, NULL);
    registrar_nativa(interpretador, "ord", builtin_ord, NULL);
    registrar_nativa(interpretador, "chr", builtin_chr, NULL);
    registrar_nativa(interpretador, "lower", builtin_lower, NULL);
    registrar_nativa(interpretador, "upper", builtin_upper, NULL);
    registrar_nativa(interpretador, "trim", builtin_trim, NULL);
    registrar_nativa(interpretador, "trim_left", builtin_trim_left, NULL);
    registrar_nativa(interpretador, "trim_right", builtin_trim_right, NULL);
    registrar_nativa(interpretador, "replace", builtin_replace, NULL);
    registrar_nativa(interpretador, "normalize_text", builtin_normalize_text, NULL);
    registrar_nativa(interpretador, "match_pattern", builtin_match_pattern, NULL);
    registrar_nativa(interpretador, "render_template", builtin_render_template, NULL);
    registrar_nativa(interpretador, "rand", builtin_rand, NULL);
    registrar_nativa(interpretador, "rand_int", builtin_rand_int, NULL);
    registrar_nativa(interpretador, "choice", builtin_choice, NULL);
    registrar_nativa(interpretador, "eval", builtin_eval, NULL);
    registrar_nativa(interpretador, "try_eval", builtin_try_eval, NULL);
    registrar_nativa(interpretador, "sin", builtin_sin, NULL);
    registrar_nativa(interpretador, "cos", builtin_cos, NULL);
    registrar_nativa(interpretador, "tan", builtin_tan, NULL);
    registrar_nativa(interpretador, "asin", builtin_asin, NULL);
    registrar_nativa(interpretador, "acos", builtin_acos, NULL);
    registrar_nativa(interpretador, "atan", builtin_atan, NULL);
    registrar_nativa(interpretador, "atan2", builtin_atan2, NULL);
    registrar_nativa(interpretador, "sqrt", builtin_sqrt, NULL);
    registrar_nativa(interpretador, "hypot", builtin_hypot, NULL);
    registrar_nativa(interpretador, "abs", builtin_abs, NULL);
    registrar_nativa(interpretador, "floor", builtin_floor, NULL);
    registrar_nativa(interpretador, "ceil", builtin_ceil, NULL);
    registrar_nativa(interpretador, "round", builtin_round, NULL);
    registrar_nativa(interpretador, "trunc", builtin_trunc, NULL);
    registrar_nativa(interpretador, "pow", builtin_pow, NULL);
    registrar_nativa(interpretador, "mod", builtin_mod, NULL);
    registrar_nativa(interpretador, "sign", builtin_sign, NULL);
    registrar_nativa(interpretador, "min", builtin_min, NULL);
    registrar_nativa(interpretador, "max", builtin_max, NULL);
    registrar_nativa(interpretador, "log", builtin_log, NULL);
    registrar_nativa(interpretador, "exp", builtin_exp, NULL);
    registrar_nativa(interpretador, "is_nan", builtin_is_nan, NULL);
    registrar_nativa(interpretador, "is_finite", builtin_is_finite, NULL);
    registrar_nativa(interpretador, "read_file", builtin_read_file, NULL);
    registrar_nativa(interpretador, "write_file", builtin_write_file, NULL);
    registrar_nativa(interpretador, "append_file", builtin_append_file, NULL);
    registrar_nativa(interpretador, "file_exists", builtin_file_exists, NULL);
    registrar_nativa(interpretador, "is_file", builtin_is_file, NULL);
    registrar_nativa(interpretador, "is_dir", builtin_is_dir, NULL);
    registrar_nativa(interpretador, "list_dir", builtin_list_dir, NULL);
    registrar_nativa(interpretador, "make_dir", builtin_make_dir, NULL);
    registrar_nativa(interpretador, "cwd", builtin_cwd, NULL);
    registrar_nativa(interpretador, "change_dir", builtin_change_dir, NULL);
    registrar_nativa(interpretador, "shell_run", builtin_shell_run, NULL);
    registrar_nativa(interpretador, "path_join", builtin_path_join, NULL);
    registrar_nativa(interpretador, "path_dirname", builtin_path_dirname, NULL);
    registrar_nativa(interpretador, "path_basename", builtin_path_basename, NULL);
    registrar_nativa(interpretador, "path_extname", builtin_path_extname, NULL);
    registrar_nativa(interpretador, "path_stem", builtin_path_stem, NULL);

    registrar_nativa(interpretador, "make_agent", builtin_make_agent, NULL);
    registrar_nativa(interpretador, "add_pattern", builtin_add_pattern, NULL);
    registrar_nativa(interpretador, "set_fallback", builtin_set_fallback, NULL);
    registrar_nativa(interpretador, "respond", builtin_respond, NULL);
    registrar_nativa(interpretador, "remember", builtin_remember, NULL);
    registrar_nativa(interpretador, "recall", builtin_recall, NULL);

    registrar_nativa(interpretador, "expert", builtin_expert, NULL);
    registrar_nativa(interpretador, "set_fact", builtin_set_fact, NULL);
    registrar_nativa(interpretador, "get_fact", builtin_get_fact, NULL);
    registrar_nativa(interpretador, "add_rule", builtin_add_rule, NULL);
    registrar_nativa(interpretador, "run_rules", builtin_run_rules, NULL);
    registrar_nativa(interpretador, "reset_rules", builtin_reset_rules, NULL);
    registrar_nativa(interpretador, "trace", builtin_trace, NULL);

    registrar_nativa(interpretador, "world", builtin_world, NULL);
    registrar_nativa(interpretador, "place", builtin_place, NULL);
    registrar_nativa(interpretador, "where", builtin_where, NULL);
    registrar_nativa(interpretador, "objects_on", builtin_objects_on, NULL);

    registrar_nativa(interpretador, "flock", builtin_flock, NULL);
    registrar_nativa(interpretador, "add_boid", builtin_add_boid, NULL);
    registrar_nativa(interpretador, "step_flock", builtin_step_flock, NULL);
}
