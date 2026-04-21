#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "noema.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

typedef struct {
    char *dados;
    size_t comprimento;
    size_t capacidade;
} MontadorTexto;

typedef struct NoemaRecuperacao {
    jmp_buf salto;
    char *mensagem;
    struct NoemaRecuperacao *anterior;
} NoemaRecuperacao;

typedef struct {
    char *nome;
    char **parametros;
    int quantidade_parametros;
    No *corpo;
} MacroExecutavel;

typedef struct AmbienteMacro {
    struct AmbienteMacro *pai;
    MacroExecutavel *itens;
    int quantidade;
    int capacidade;
} AmbienteMacro;

static char *g_noema_diretorio_stdlib = NULL;
static NoemaRecuperacao *g_noema_recuperacao_atual = NULL;

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

static int noema_verificar_acesso_executavel(const char *caminho) {
#ifdef _WIN32
    return _access(caminho, 0);
#else
    return access(caminho, X_OK);
#endif
}

static Valor avaliar(Interpretador *interpretador, Ambiente *ambiente, No *no);
static ResultadoExecucao executar(Interpretador *interpretador, Ambiente *ambiente, No *no);
static ResultadoExecucao executar_bloco(Interpretador *interpretador, Ambiente *ambiente, No *no_bloco, bool criar_escopo);
static Valor chamar_funcao_noema(Interpretador *interpretador, Ambiente *ambiente, FuncaoNoema *funcao, int argc, Valor *argv);
static Valor avaliar_atribuicao(Interpretador *interpretador, Ambiente *ambiente, No *no);
static char *valor_para_texto_interno(Valor valor, int profundidade);
static FuncaoNoema *criar_funcao_noema(No *no, Ambiente *ambiente, const char *nome_padrao);
static char *interpolar_texto(Interpretador *interpretador, Ambiente *ambiente, const char *texto);
static No *clonar_no(No *no);
static char *no_para_texto(No *no);
static No *expandir_no(Interpretador *interpretador, AmbienteMacro *ambiente_macro, No *no, bool contexto_instrucao);

static void montador_inicializar(MontadorTexto *montador) {
    montador->capacidade = 256;
    montador->comprimento = 0;
    montador->dados = malloc(montador->capacidade);
    if (montador->dados == NULL) {
        noema_falhar("memoria insuficiente");
    }
    montador->dados[0] = '\0';
}

static void montador_anexar(MontadorTexto *montador, const char *texto) {
    size_t extra = strlen(texto);
    while (montador->comprimento + extra + 1 > montador->capacidade) {
        montador->capacidade *= 2;
        montador->dados = realloc(montador->dados, montador->capacidade);
        if (montador->dados == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    memcpy(montador->dados + montador->comprimento, texto, extra + 1);
    montador->comprimento += extra;
}

static char *montador_finalizar(MontadorTexto *montador) {
    return montador->dados;
}

static char *noema_vformatar(const char *formato, va_list args) {
    va_list copia;
    va_copy(copia, args);
    int tamanho = vsnprintf(NULL, 0, formato, copia);
    va_end(copia);
    if (tamanho < 0) {
        fprintf(stderr, "erro noema: falha ao formatar texto\n");
        exit(1);
    }
    char *saida = malloc((size_t)tamanho + 1);
    if (saida == NULL) {
        fprintf(stderr, "erro noema: memoria insuficiente\n");
        exit(1);
    }
    vsnprintf(saida, (size_t)tamanho + 1, formato, args);
    return saida;
}

static void noema_empilhar_recuperacao(NoemaRecuperacao *recuperacao) {
    recuperacao->mensagem = NULL;
    recuperacao->anterior = g_noema_recuperacao_atual;
    g_noema_recuperacao_atual = recuperacao;
}

static void noema_desempilhar_recuperacao(NoemaRecuperacao *recuperacao) {
    if (g_noema_recuperacao_atual == recuperacao) {
        g_noema_recuperacao_atual = recuperacao->anterior;
    }
}

void noema_falhar(const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    char *mensagem = noema_vformatar(formato, args);
    va_end(args);
    if (g_noema_recuperacao_atual != NULL) {
        NoemaRecuperacao *recuperacao = g_noema_recuperacao_atual;
        recuperacao->mensagem = mensagem;
        g_noema_recuperacao_atual = recuperacao->anterior;
        longjmp(recuperacao->salto, 1);
    }
    fprintf(stderr, "erro noema: %s\n", mensagem);
    free(mensagem);
    exit(1);
}

char *noema_duplicar(const char *texto) {
    size_t tamanho = strlen(texto) + 1;
    char *copia = malloc(tamanho);
    if (copia == NULL) {
        noema_falhar("memoria insuficiente");
    }
    memcpy(copia, texto, tamanho);
    return copia;
}

char *noema_formatar(const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    char *saida = noema_vformatar(formato, args);
    va_end(args);
    return saida;
}

static No *runtime_novo_no(TipoNo tipo, int linha) {
    No *no = calloc(1, sizeof(No));
    if (no == NULL) {
        noema_falhar("memoria insuficiente");
    }
    no->tipo = tipo;
    no->linha = linha;
    return no;
}

static bool no_eh_expressao(TipoNo tipo) {
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

static const char *token_operador_texto(TipoToken operador) {
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

static void bloco_adicionar_item_runtime(No *bloco, No *item) {
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

static void lista_literal_adicionar_item_runtime(No *lista, No *item) {
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

static void mapa_literal_adicionar_item_runtime(No *mapa, const char *chave, No *valor) {
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

static void chamada_adicionar_argumento_runtime(No *chamada, No *argumento) {
    if (chamada->como.chamada.quantidade_argumentos >= chamada->como.chamada.capacidade_argumentos) {
        chamada->como.chamada.capacidade_argumentos =
            chamada->como.chamada.capacidade_argumentos == 0 ? 4 : chamada->como.chamada.capacidade_argumentos * 2;
        chamada->como.chamada.argumentos = realloc(chamada->como.chamada.argumentos,
                                                   (size_t)chamada->como.chamada.capacidade_argumentos * sizeof(No *));
        if (chamada->como.chamada.argumentos == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    chamada->como.chamada.argumentos[chamada->como.chamada.quantidade_argumentos++] = argumento;
}

static void expansao_macro_adicionar_argumento_runtime(No *expansao, No *argumento) {
    if (expansao->como.expansao_macro.quantidade_argumentos >= expansao->como.expansao_macro.capacidade_argumentos) {
        expansao->como.expansao_macro.capacidade_argumentos =
            expansao->como.expansao_macro.capacidade_argumentos == 0 ? 4 : expansao->como.expansao_macro.capacidade_argumentos * 2;
        expansao->como.expansao_macro.argumentos = realloc(
            expansao->como.expansao_macro.argumentos,
            (size_t)expansao->como.expansao_macro.capacidade_argumentos * sizeof(No *));
        if (expansao->como.expansao_macro.argumentos == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    expansao->como.expansao_macro.argumentos[expansao->como.expansao_macro.quantidade_argumentos++] = argumento;
}

static char **clonar_lista_textos(char **itens, int quantidade) {
    if (quantidade <= 0) {
        return NULL;
    }
    char **copia = calloc((size_t)quantidade, sizeof(char *));
    if (copia == NULL) {
        noema_falhar("memoria insuficiente");
    }
    for (int i = 0; i < quantidade; i++) {
        copia[i] = noema_duplicar(itens[i]);
    }
    return copia;
}

static Valor clonar_valor_literal(Valor valor) {
    switch (valor.tipo) {
        case VALOR_NULO:
            return valor_nulo();
        case VALOR_BOOL:
            return valor_bool(valor.como.booleano);
        case VALOR_NUMERO:
            return valor_numero(valor.como.numero);
        case VALOR_TEXTO:
            return valor_texto(valor.como.texto);
        case VALOR_PONTEIRO:
            return valor_ponteiro(valor.como.ponteiro);
        case VALOR_SINTAXE:
            return valor_sintaxe(clonar_no(valor.como.sintaxe));
        case VALOR_LISTA:
        case VALOR_MAPA:
        case VALOR_FUNCAO:
        case VALOR_NATIVA:
            return valor;
    }
    return valor_nulo();
}

static No *clonar_no(No *no) {
    if (no == NULL) {
        return NULL;
    }

    No *copia = runtime_novo_no(no->tipo, no->linha);
    switch (no->tipo) {
        case NO_BLOCO:
            for (int i = 0; i < no->como.bloco.quantidade; i++) {
                bloco_adicionar_item_runtime(copia, clonar_no(no->como.bloco.itens[i]));
            }
            break;
        case NO_LITERAL:
            copia->como.literal.valor = clonar_valor_literal(no->como.literal.valor);
            break;
        case NO_VARIAVEL:
            copia->como.variavel.nome = noema_duplicar(no->como.variavel.nome);
            break;
        case NO_LISTA_LITERAL:
            for (int i = 0; i < no->como.lista.quantidade; i++) {
                lista_literal_adicionar_item_runtime(copia, clonar_no(no->como.lista.itens[i]));
            }
            break;
        case NO_MAPA_LITERAL:
            copia->como.mapa.quantidade = no->como.mapa.quantidade;
            copia->como.mapa.capacidade = no->como.mapa.quantidade;
            if (no->como.mapa.quantidade > 0) {
                copia->como.mapa.chaves = calloc((size_t)no->como.mapa.quantidade, sizeof(char *));
                copia->como.mapa.valores = calloc((size_t)no->como.mapa.quantidade, sizeof(No *));
                if (copia->como.mapa.chaves == NULL || copia->como.mapa.valores == NULL) {
                    noema_falhar("memoria insuficiente");
                }
                for (int i = 0; i < no->como.mapa.quantidade; i++) {
                    copia->como.mapa.chaves[i] = noema_duplicar(no->como.mapa.chaves[i]);
                    copia->como.mapa.valores[i] = clonar_no(no->como.mapa.valores[i]);
                }
            }
            break;
        case NO_BINARIO:
            copia->como.binario.esquerda = clonar_no(no->como.binario.esquerda);
            copia->como.binario.operador = no->como.binario.operador;
            copia->como.binario.direita = clonar_no(no->como.binario.direita);
            break;
        case NO_UNARIO:
            copia->como.unario.operador = no->como.unario.operador;
            copia->como.unario.direita = clonar_no(no->como.unario.direita);
            break;
        case NO_CHAMADA:
            copia->como.chamada.alvo = clonar_no(no->como.chamada.alvo);
            for (int i = 0; i < no->como.chamada.quantidade_argumentos; i++) {
                chamada_adicionar_argumento_runtime(copia, clonar_no(no->como.chamada.argumentos[i]));
            }
            break;
        case NO_MEMBRO:
            copia->como.membro.objeto = clonar_no(no->como.membro.objeto);
            copia->como.membro.nome = noema_duplicar(no->como.membro.nome);
            break;
        case NO_INDICE:
            copia->como.indice.objeto = clonar_no(no->como.indice.objeto);
            copia->como.indice.indice = clonar_no(no->como.indice.indice);
            break;
        case NO_ATRIBUICAO:
            copia->como.atribuicao.alvo = clonar_no(no->como.atribuicao.alvo);
            copia->como.atribuicao.valor = clonar_no(no->como.atribuicao.valor);
            break;
        case NO_DECL_LET:
        case NO_DECL_CONST:
            copia->como.declaracao_variavel.nome = noema_duplicar(no->como.declaracao_variavel.nome);
            copia->como.declaracao_variavel.inicializador = clonar_no(no->como.declaracao_variavel.inicializador);
            break;
        case NO_DECL_FUNCAO:
        case NO_FUNCAO_ANONIMA:
            copia->como.declaracao_funcao.nome =
                no->como.declaracao_funcao.nome != NULL ? noema_duplicar(no->como.declaracao_funcao.nome) : NULL;
            copia->como.declaracao_funcao.parametros =
                clonar_lista_textos(no->como.declaracao_funcao.parametros, no->como.declaracao_funcao.quantidade_parametros);
            copia->como.declaracao_funcao.quantidade_parametros = no->como.declaracao_funcao.quantidade_parametros;
            copia->como.declaracao_funcao.corpo = clonar_no(no->como.declaracao_funcao.corpo);
            break;
        case NO_DECL_MACRO:
            copia->como.declaracao_macro.nome = noema_duplicar(no->como.declaracao_macro.nome);
            copia->como.declaracao_macro.parametros =
                clonar_lista_textos(no->como.declaracao_macro.parametros, no->como.declaracao_macro.quantidade_parametros);
            copia->como.declaracao_macro.quantidade_parametros = no->como.declaracao_macro.quantidade_parametros;
            copia->como.declaracao_macro.corpo = clonar_no(no->como.declaracao_macro.corpo);
            break;
        case NO_EXPR_STMT:
            copia->como.expressao.expressao = clonar_no(no->como.expressao.expressao);
            break;
        case NO_EXPAND:
            copia->como.expansao_macro.nome = noema_duplicar(no->como.expansao_macro.nome);
            for (int i = 0; i < no->como.expansao_macro.quantidade_argumentos; i++) {
                expansao_macro_adicionar_argumento_runtime(copia, clonar_no(no->como.expansao_macro.argumentos[i]));
            }
            break;
        case NO_SYNTAX_TEMPLATE:
            copia->como.template_sintaxe.corpo = clonar_no(no->como.template_sintaxe.corpo);
            break;
        case NO_SYNTAX_PLACEHOLDER:
            copia->como.placeholder_sintaxe.nome =
                no->como.placeholder_sintaxe.nome != NULL ? noema_duplicar(no->como.placeholder_sintaxe.nome) : NULL;
            copia->como.placeholder_sintaxe.expressao = clonar_no(no->como.placeholder_sintaxe.expressao);
            break;
        case NO_IF:
            copia->como.condicional.condicao = clonar_no(no->como.condicional.condicao);
            copia->como.condicional.ramo_entao = clonar_no(no->como.condicional.ramo_entao);
            copia->como.condicional.ramo_senao = clonar_no(no->como.condicional.ramo_senao);
            break;
        case NO_WHILE:
            copia->como.enquanto.condicao = clonar_no(no->como.enquanto.condicao);
            copia->como.enquanto.corpo = clonar_no(no->como.enquanto.corpo);
            break;
        case NO_FOR:
            copia->como.para.variavel = noema_duplicar(no->como.para.variavel);
            copia->como.para.iteravel = clonar_no(no->como.para.iteravel);
            copia->como.para.corpo = clonar_no(no->como.para.corpo);
            break;
        case NO_RETURN:
            copia->como.retorno.valor = clonar_no(no->como.retorno.valor);
            break;
        case NO_BREAK:
        case NO_CONTINUE:
            break;
        case NO_DECL_PLUGIN:
            copia->como.plugin.nome = noema_duplicar(no->como.plugin.nome);
            copia->como.plugin.bibliotecas =
                clonar_lista_textos(no->como.plugin.bibliotecas, no->como.plugin.quantidade_bibliotecas);
            copia->como.plugin.quantidade_bibliotecas = no->como.plugin.quantidade_bibliotecas;
            copia->como.plugin.quantidade_ligacoes = no->como.plugin.quantidade_ligacoes;
            if (no->como.plugin.quantidade_ligacoes > 0) {
                copia->como.plugin.ligacoes = calloc((size_t)no->como.plugin.quantidade_ligacoes, sizeof(LigacaoPluginAst));
                if (copia->como.plugin.ligacoes == NULL) {
                    noema_falhar("memoria insuficiente");
                }
                for (int i = 0; i < no->como.plugin.quantidade_ligacoes; i++) {
                    copia->como.plugin.ligacoes[i].nome_noema = noema_duplicar(no->como.plugin.ligacoes[i].nome_noema);
                    copia->como.plugin.ligacoes[i].nome_simbolo = noema_duplicar(no->como.plugin.ligacoes[i].nome_simbolo);
                    copia->como.plugin.ligacoes[i].retorno = no->como.plugin.ligacoes[i].retorno;
                    copia->como.plugin.ligacoes[i].quantidade_argumentos = no->como.plugin.ligacoes[i].quantidade_argumentos;
                    if (no->como.plugin.ligacoes[i].quantidade_argumentos > 0) {
                        copia->como.plugin.ligacoes[i].argumentos =
                            calloc((size_t)no->como.plugin.ligacoes[i].quantidade_argumentos, sizeof(TipoFfi));
                        if (copia->como.plugin.ligacoes[i].argumentos == NULL) {
                            noema_falhar("memoria insuficiente");
                        }
                        memcpy(copia->como.plugin.ligacoes[i].argumentos,
                               no->como.plugin.ligacoes[i].argumentos,
                               (size_t)no->como.plugin.ligacoes[i].quantidade_argumentos * sizeof(TipoFfi));
                    }
                }
            }
            break;
    }

    return copia;
}

static char *no_para_texto(No *no) {
    if (no == NULL) {
        return noema_duplicar("null");
    }

    MontadorTexto montador;
    montador_inicializar(&montador);

    switch (no->tipo) {
        case NO_BLOCO:
            montador_anexar(&montador, "{");
            for (int i = 0; i < no->como.bloco.quantidade; i++) {
                if (i == 0) {
                    montador_anexar(&montador, "\n");
                }
                char *item = no_para_texto(no->como.bloco.itens[i]);
                montador_anexar(&montador, item);
                montador_anexar(&montador, "\n");
            }
            montador_anexar(&montador, "}");
            return montador_finalizar(&montador);
        case NO_LITERAL:
            return valor_para_texto_interno(no->como.literal.valor, 0);
        case NO_VARIAVEL:
            return noema_duplicar(no->como.variavel.nome);
        case NO_LISTA_LITERAL: {
            montador_anexar(&montador, "[");
            for (int i = 0; i < no->como.lista.quantidade; i++) {
                if (i > 0) {
                    montador_anexar(&montador, ", ");
                }
                char *item = no_para_texto(no->como.lista.itens[i]);
                montador_anexar(&montador, item);
            }
            montador_anexar(&montador, "]");
            return montador_finalizar(&montador);
        }
        case NO_MAPA_LITERAL: {
            montador_anexar(&montador, "{");
            for (int i = 0; i < no->como.mapa.quantidade; i++) {
                if (i > 0) {
                    montador_anexar(&montador, ", ");
                }
                montador_anexar(&montador, no->como.mapa.chaves[i]);
                montador_anexar(&montador, ": ");
                char *valor = no_para_texto(no->como.mapa.valores[i]);
                montador_anexar(&montador, valor);
            }
            montador_anexar(&montador, "}");
            return montador_finalizar(&montador);
        }
        case NO_BINARIO: {
            char *esquerda = no_para_texto(no->como.binario.esquerda);
            char *direita = no_para_texto(no->como.binario.direita);
            return noema_formatar("(%s %s %s)", esquerda, token_operador_texto(no->como.binario.operador), direita);
        }
        case NO_UNARIO: {
            char *direita = no_para_texto(no->como.unario.direita);
            return noema_formatar("(%s %s)", token_operador_texto(no->como.unario.operador), direita);
        }
        case NO_CHAMADA: {
            char *alvo = no_para_texto(no->como.chamada.alvo);
            montador_anexar(&montador, alvo);
            montador_anexar(&montador, "(");
            for (int i = 0; i < no->como.chamada.quantidade_argumentos; i++) {
                if (i > 0) {
                    montador_anexar(&montador, ", ");
                }
                char *arg = no_para_texto(no->como.chamada.argumentos[i]);
                montador_anexar(&montador, arg);
            }
            montador_anexar(&montador, ")");
            return montador_finalizar(&montador);
        }
        case NO_MEMBRO: {
            char *objeto = no_para_texto(no->como.membro.objeto);
            return noema_formatar("%s.%s", objeto, no->como.membro.nome);
        }
        case NO_INDICE: {
            char *objeto = no_para_texto(no->como.indice.objeto);
            char *indice = no_para_texto(no->como.indice.indice);
            return noema_formatar("%s[%s]", objeto, indice);
        }
        case NO_ATRIBUICAO: {
            char *alvo = no_para_texto(no->como.atribuicao.alvo);
            char *valor = no_para_texto(no->como.atribuicao.valor);
            return noema_formatar("%s = %s", alvo, valor);
        }
        case NO_DECL_LET:
        case NO_DECL_CONST: {
            const char *palavra = no->tipo == NO_DECL_CONST ? "const" : "var";
            if (no->como.declaracao_variavel.inicializador == NULL) {
                return noema_formatar("%s %s", palavra, no->como.declaracao_variavel.nome);
            }
            char *init = no_para_texto(no->como.declaracao_variavel.inicializador);
            return noema_formatar("%s %s = %s", palavra, no->como.declaracao_variavel.nome, init);
        }
        case NO_DECL_FUNCAO:
        case NO_FUNCAO_ANONIMA: {
            montador_anexar(&montador, "fn");
            if (no->como.declaracao_funcao.nome != NULL) {
                montador_anexar(&montador, " ");
                montador_anexar(&montador, no->como.declaracao_funcao.nome);
            }
            montador_anexar(&montador, "(");
            for (int i = 0; i < no->como.declaracao_funcao.quantidade_parametros; i++) {
                if (i > 0) {
                    montador_anexar(&montador, ", ");
                }
                montador_anexar(&montador, no->como.declaracao_funcao.parametros[i]);
            }
            montador_anexar(&montador, ") ");
            char *corpo = no_para_texto(no->como.declaracao_funcao.corpo);
            montador_anexar(&montador, corpo);
            return montador_finalizar(&montador);
        }
        case NO_DECL_MACRO: {
            montador_anexar(&montador, "macro ");
            montador_anexar(&montador, no->como.declaracao_macro.nome);
            montador_anexar(&montador, "(");
            for (int i = 0; i < no->como.declaracao_macro.quantidade_parametros; i++) {
                if (i > 0) {
                    montador_anexar(&montador, ", ");
                }
                montador_anexar(&montador, no->como.declaracao_macro.parametros[i]);
            }
            montador_anexar(&montador, ") ");
            char *corpo = no_para_texto(no->como.declaracao_macro.corpo);
            montador_anexar(&montador, corpo);
            return montador_finalizar(&montador);
        }
        case NO_EXPR_STMT:
            return no_para_texto(no->como.expressao.expressao);
        case NO_EXPAND: {
            montador_anexar(&montador, "expand ");
            montador_anexar(&montador, no->como.expansao_macro.nome);
            montador_anexar(&montador, "(");
            for (int i = 0; i < no->como.expansao_macro.quantidade_argumentos; i++) {
                if (i > 0) {
                    montador_anexar(&montador, ", ");
                }
                char *arg = no_para_texto(no->como.expansao_macro.argumentos[i]);
                montador_anexar(&montador, arg);
            }
            montador_anexar(&montador, ")");
            return montador_finalizar(&montador);
        }
        case NO_SYNTAX_TEMPLATE: {
            char *corpo = no_para_texto(no->como.template_sintaxe.corpo);
            return noema_formatar("syntax %s", corpo);
        }
        case NO_SYNTAX_PLACEHOLDER:
            if (no->como.placeholder_sintaxe.nome != NULL) {
                return noema_formatar("$%s", no->como.placeholder_sintaxe.nome);
            }
            return noema_formatar("$(%s)", no_para_texto(no->como.placeholder_sintaxe.expressao));
        case NO_IF: {
            char *condicao = no_para_texto(no->como.condicional.condicao);
            char *entao = no_para_texto(no->como.condicional.ramo_entao);
            if (no->como.condicional.ramo_senao == NULL) {
                return noema_formatar("if %s %s", condicao, entao);
            }
            char *senao = no_para_texto(no->como.condicional.ramo_senao);
            return noema_formatar("if %s %s else %s", condicao, entao, senao);
        }
        case NO_WHILE: {
            char *condicao = no_para_texto(no->como.enquanto.condicao);
            char *corpo = no_para_texto(no->como.enquanto.corpo);
            return noema_formatar("while %s %s", condicao, corpo);
        }
        case NO_FOR: {
            char *iteravel = no_para_texto(no->como.para.iteravel);
            char *corpo = no_para_texto(no->como.para.corpo);
            return noema_formatar("each %s in %s %s", no->como.para.variavel, iteravel, corpo);
        }
        case NO_RETURN:
            if (no->como.retorno.valor == NULL) {
                return noema_duplicar("return");
            }
            return noema_formatar("return %s", no_para_texto(no->como.retorno.valor));
        case NO_BREAK:
            return noema_duplicar("break");
        case NO_CONTINUE:
            return noema_duplicar("continue");
        case NO_DECL_PLUGIN:
            montador_anexar(&montador, "plugin ");
            montador_anexar(&montador, no->como.plugin.nome);
            montador_anexar(&montador, " {\n");
            for (int i = 0; i < no->como.plugin.quantidade_bibliotecas; i++) {
                montador_anexar(&montador, "library \"");
                montador_anexar(&montador, no->como.plugin.bibliotecas[i]);
                montador_anexar(&montador, "\"\n");
            }
            for (int i = 0; i < no->como.plugin.quantidade_ligacoes; i++) {
                montador_anexar(&montador, "bind ");
                montador_anexar(&montador, no->como.plugin.ligacoes[i].nome_noema);
                montador_anexar(&montador, "(...)");
                montador_anexar(&montador, "\n");
            }
            montador_anexar(&montador, "}");
            return montador_finalizar(&montador);
    }

    return noema_duplicar("<syntax>");
}

static void ambiente_macro_inicializar(AmbienteMacro *ambiente, AmbienteMacro *pai) {
    memset(ambiente, 0, sizeof(*ambiente));
    ambiente->pai = pai;
}

static MacroExecutavel *ambiente_macro_buscar(AmbienteMacro *ambiente, const char *nome) {
    for (AmbienteMacro *cursor = ambiente; cursor != NULL; cursor = cursor->pai) {
        for (int i = 0; i < cursor->quantidade; i++) {
            if (strcmp(cursor->itens[i].nome, nome) == 0) {
                return &cursor->itens[i];
            }
        }
    }
    return NULL;
}

static void ambiente_macro_registrar(AmbienteMacro *ambiente, No *macro) {
    for (int i = 0; i < ambiente->quantidade; i++) {
        if (strcmp(ambiente->itens[i].nome, macro->como.declaracao_macro.nome) == 0) {
            ambiente->itens[i].parametros = macro->como.declaracao_macro.parametros;
            ambiente->itens[i].quantidade_parametros = macro->como.declaracao_macro.quantidade_parametros;
            ambiente->itens[i].corpo = macro->como.declaracao_macro.corpo;
            return;
        }
    }

    if (ambiente->quantidade >= ambiente->capacidade) {
        ambiente->capacidade = ambiente->capacidade == 0 ? 8 : ambiente->capacidade * 2;
        ambiente->itens = realloc(ambiente->itens, (size_t)ambiente->capacidade * sizeof(MacroExecutavel));
        if (ambiente->itens == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }

    MacroExecutavel *item = &ambiente->itens[ambiente->quantidade++];
    item->nome = macro->como.declaracao_macro.nome;
    item->parametros = macro->como.declaracao_macro.parametros;
    item->quantidade_parametros = macro->como.declaracao_macro.quantidade_parametros;
    item->corpo = macro->como.declaracao_macro.corpo;
}

static No *garantir_no_expressao(No *no, const char *contexto) {
    if (no == NULL || !no_eh_expressao(no->tipo)) {
        noema_falhar("%s exige uma expressao de sintaxe", contexto);
    }
    return no;
}

static Valor avaliar_placeholder_sintaxe(Interpretador *interpretador, Ambiente *ambiente, No *placeholder) {
    if (placeholder->como.placeholder_sintaxe.nome != NULL) {
        Valor valor;
        if (!ambiente_obter(ambiente, placeholder->como.placeholder_sintaxe.nome, &valor)) {
            noema_falhar("placeholder '$%s' nao esta definido", placeholder->como.placeholder_sintaxe.nome);
        }
        return valor;
    }
    return avaliar(interpretador, ambiente, placeholder->como.placeholder_sintaxe.expressao);
}

static No *valor_para_no_sintaxe(Valor valor, int linha) {
    switch (valor.tipo) {
        case VALOR_SINTAXE:
            return clonar_no(valor.como.sintaxe);
        case VALOR_NULO:
        case VALOR_BOOL:
        case VALOR_NUMERO:
        case VALOR_TEXTO: {
            No *literal = runtime_novo_no(NO_LITERAL, linha);
            literal->como.literal.valor = clonar_valor_literal(valor);
            return literal;
        }
        case VALOR_LISTA: {
            No *lista = runtime_novo_no(NO_LISTA_LITERAL, linha);
            for (int i = 0; i < valor.como.lista->quantidade; i++) {
                lista_literal_adicionar_item_runtime(lista,
                                                    valor_para_no_sintaxe(valor.como.lista->itens[i], linha));
            }
            return lista;
        }
        case VALOR_MAPA: {
            No *mapa = runtime_novo_no(NO_MAPA_LITERAL, linha);
            for (int i = 0; i < valor.como.mapa->quantidade; i++) {
                mapa_literal_adicionar_item_runtime(mapa,
                                                   valor.como.mapa->entradas[i].chave,
                                                   valor_para_no_sintaxe(valor.como.mapa->entradas[i].valor, linha));
            }
            return mapa;
        }
        default:
            noema_falhar("nao foi possivel converter %s em sintaxe", valor_tipo_nome(valor));
    }
    return NULL;
}

static No *normalizar_no_para_instrucao(No *no, int linha) {
    if (no == NULL) {
        return NULL;
    }
    if (no_eh_expressao(no->tipo)) {
        No *stmt = runtime_novo_no(NO_EXPR_STMT, linha);
        stmt->como.expressao.expressao = no;
        return stmt;
    }
    return no;
}

static No *valor_para_no_instrucao(Valor valor, int linha) {
    if (valor.tipo == VALOR_LISTA) {
        No *bloco = runtime_novo_no(NO_BLOCO, linha);
        for (int i = 0; i < valor.como.lista->quantidade; i++) {
            No *item = valor_para_no_sintaxe(valor.como.lista->itens[i], linha);
            bloco_adicionar_item_runtime(bloco, normalizar_no_para_instrucao(item, linha));
        }
        return bloco;
    }
    return normalizar_no_para_instrucao(valor_para_no_sintaxe(valor, linha), linha);
}

static bool no_expr_stmt_placeholder(No *no, No **placeholder) {
    if (no != NULL &&
        no->tipo == NO_EXPR_STMT &&
        no->como.expressao.expressao != NULL &&
        no->como.expressao.expressao->tipo == NO_SYNTAX_PLACEHOLDER) {
        if (placeholder != NULL) {
            *placeholder = no->como.expressao.expressao;
        }
        return true;
    }
    return false;
}

static No *avaliar_template_sintaxe_no(Interpretador *interpretador, Ambiente *ambiente, No *no);

static void template_adicionar_item_bloco(Interpretador *interpretador, Ambiente *ambiente, No *bloco, No *item) {
    No *placeholder = NULL;
    if (no_expr_stmt_placeholder(item, &placeholder)) {
        Valor valor = avaliar_placeholder_sintaxe(interpretador, ambiente, placeholder);
        if (valor.tipo == VALOR_SINTAXE && valor.como.sintaxe != NULL && valor.como.sintaxe->tipo == NO_BLOCO) {
            for (int i = 0; i < valor.como.sintaxe->como.bloco.quantidade; i++) {
                No *filho = clonar_no(valor.como.sintaxe->como.bloco.itens[i]);
                bloco_adicionar_item_runtime(bloco, normalizar_no_para_instrucao(filho, item->linha));
            }
            return;
        }
        if (valor.tipo == VALOR_LISTA) {
            for (int i = 0; i < valor.como.lista->quantidade; i++) {
                No *filho = valor_para_no_sintaxe(valor.como.lista->itens[i], item->linha);
                bloco_adicionar_item_runtime(bloco, normalizar_no_para_instrucao(filho, item->linha));
            }
            return;
        }
        bloco_adicionar_item_runtime(bloco, valor_para_no_instrucao(valor, item->linha));
        return;
    }

    No *expandido = avaliar_template_sintaxe_no(interpretador, ambiente, item);
    bloco_adicionar_item_runtime(bloco, normalizar_no_para_instrucao(expandido, item->linha));
}

static void template_adicionar_item_lista(Interpretador *interpretador, Ambiente *ambiente, No *lista, No *item) {
    if (item->tipo == NO_SYNTAX_PLACEHOLDER) {
        Valor valor = avaliar_placeholder_sintaxe(interpretador, ambiente, item);
        if (valor.tipo == VALOR_SINTAXE && valor.como.sintaxe != NULL && valor.como.sintaxe->tipo == NO_LISTA_LITERAL) {
            for (int i = 0; i < valor.como.sintaxe->como.lista.quantidade; i++) {
                No *filho = clonar_no(valor.como.sintaxe->como.lista.itens[i]);
                lista_literal_adicionar_item_runtime(lista, garantir_no_expressao(filho, "placeholder em lista"));
            }
            return;
        }
        if (valor.tipo == VALOR_LISTA) {
            for (int i = 0; i < valor.como.lista->quantidade; i++) {
                No *filho = valor_para_no_sintaxe(valor.como.lista->itens[i], item->linha);
                lista_literal_adicionar_item_runtime(lista, garantir_no_expressao(filho, "placeholder em lista"));
            }
            return;
        }
        lista_literal_adicionar_item_runtime(lista,
                                             garantir_no_expressao(valor_para_no_sintaxe(valor, item->linha),
                                                                   "placeholder em lista"));
        return;
    }

    lista_literal_adicionar_item_runtime(lista,
                                         garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, item),
                                                               "item de lista"));
}

static void template_adicionar_argumento_chamada(Interpretador *interpretador, Ambiente *ambiente, No *chamada, No *arg) {
    if (arg->tipo == NO_SYNTAX_PLACEHOLDER) {
        Valor valor = avaliar_placeholder_sintaxe(interpretador, ambiente, arg);
        if (valor.tipo == VALOR_SINTAXE && valor.como.sintaxe != NULL && valor.como.sintaxe->tipo == NO_LISTA_LITERAL) {
            for (int i = 0; i < valor.como.sintaxe->como.lista.quantidade; i++) {
                No *filho = clonar_no(valor.como.sintaxe->como.lista.itens[i]);
                chamada_adicionar_argumento_runtime(chamada, garantir_no_expressao(filho, "placeholder em chamada"));
            }
            return;
        }
        if (valor.tipo == VALOR_LISTA) {
            for (int i = 0; i < valor.como.lista->quantidade; i++) {
                No *filho = valor_para_no_sintaxe(valor.como.lista->itens[i], arg->linha);
                chamada_adicionar_argumento_runtime(chamada, garantir_no_expressao(filho, "placeholder em chamada"));
            }
            return;
        }
        chamada_adicionar_argumento_runtime(chamada,
                                            garantir_no_expressao(valor_para_no_sintaxe(valor, arg->linha),
                                                                  "placeholder em chamada"));
        return;
    }

    chamada_adicionar_argumento_runtime(
        chamada,
        garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, arg), "argumento de chamada"));
}

static No *avaliar_template_sintaxe_no(Interpretador *interpretador, Ambiente *ambiente, No *no) {
    if (no == NULL) {
        return NULL;
    }

    switch (no->tipo) {
        case NO_BLOCO: {
            No *bloco = runtime_novo_no(NO_BLOCO, no->linha);
            for (int i = 0; i < no->como.bloco.quantidade; i++) {
                template_adicionar_item_bloco(interpretador, ambiente, bloco, no->como.bloco.itens[i]);
            }
            return bloco;
        }
        case NO_LITERAL:
        case NO_VARIAVEL:
        case NO_SYNTAX_TEMPLATE:
            return clonar_no(no);
        case NO_LISTA_LITERAL: {
            No *lista = runtime_novo_no(NO_LISTA_LITERAL, no->linha);
            for (int i = 0; i < no->como.lista.quantidade; i++) {
                template_adicionar_item_lista(interpretador, ambiente, lista, no->como.lista.itens[i]);
            }
            return lista;
        }
        case NO_MAPA_LITERAL: {
            No *mapa = runtime_novo_no(NO_MAPA_LITERAL, no->linha);
            for (int i = 0; i < no->como.mapa.quantidade; i++) {
                mapa_literal_adicionar_item_runtime(mapa,
                                                   no->como.mapa.chaves[i],
                                                   garantir_no_expressao(
                                                       avaliar_template_sintaxe_no(interpretador,
                                                                                   ambiente,
                                                                                   no->como.mapa.valores[i]),
                                                       "valor de mapa"));
            }
            return mapa;
        }
        case NO_BINARIO: {
            No *copia = runtime_novo_no(NO_BINARIO, no->linha);
            copia->como.binario.operador = no->como.binario.operador;
            copia->como.binario.esquerda =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.binario.esquerda),
                                      "operando esquerdo");
            copia->como.binario.direita =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.binario.direita),
                                      "operando direito");
            return copia;
        }
        case NO_UNARIO: {
            No *copia = runtime_novo_no(NO_UNARIO, no->linha);
            copia->como.unario.operador = no->como.unario.operador;
            copia->como.unario.direita =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.unario.direita),
                                      "operando unario");
            return copia;
        }
        case NO_CHAMADA: {
            No *copia = runtime_novo_no(NO_CHAMADA, no->linha);
            copia->como.chamada.alvo =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.chamada.alvo),
                                      "alvo de chamada");
            for (int i = 0; i < no->como.chamada.quantidade_argumentos; i++) {
                template_adicionar_argumento_chamada(interpretador, ambiente, copia, no->como.chamada.argumentos[i]);
            }
            return copia;
        }
        case NO_MEMBRO: {
            No *copia = runtime_novo_no(NO_MEMBRO, no->linha);
            copia->como.membro.objeto =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.membro.objeto),
                                      "objeto de membro");
            copia->como.membro.nome = noema_duplicar(no->como.membro.nome);
            return copia;
        }
        case NO_INDICE: {
            No *copia = runtime_novo_no(NO_INDICE, no->linha);
            copia->como.indice.objeto =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.indice.objeto),
                                      "objeto de indice");
            copia->como.indice.indice =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.indice.indice),
                                      "indice");
            return copia;
        }
        case NO_ATRIBUICAO: {
            No *copia = runtime_novo_no(NO_ATRIBUICAO, no->linha);
            copia->como.atribuicao.alvo =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.atribuicao.alvo),
                                      "alvo de atribuicao");
            copia->como.atribuicao.valor =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.atribuicao.valor),
                                      "valor de atribuicao");
            return copia;
        }
        case NO_DECL_LET:
        case NO_DECL_CONST: {
            No *copia = runtime_novo_no(no->tipo, no->linha);
            copia->como.declaracao_variavel.nome = noema_duplicar(no->como.declaracao_variavel.nome);
            copia->como.declaracao_variavel.inicializador =
                no->como.declaracao_variavel.inicializador != NULL
                    ? garantir_no_expressao(
                          avaliar_template_sintaxe_no(interpretador,
                                                      ambiente,
                                                      no->como.declaracao_variavel.inicializador),
                          "inicializador")
                    : NULL;
            return copia;
        }
        case NO_DECL_FUNCAO:
        case NO_FUNCAO_ANONIMA: {
            No *copia = runtime_novo_no(no->tipo, no->linha);
            copia->como.declaracao_funcao.nome =
                no->como.declaracao_funcao.nome != NULL ? noema_duplicar(no->como.declaracao_funcao.nome) : NULL;
            copia->como.declaracao_funcao.parametros =
                clonar_lista_textos(no->como.declaracao_funcao.parametros, no->como.declaracao_funcao.quantidade_parametros);
            copia->como.declaracao_funcao.quantidade_parametros = no->como.declaracao_funcao.quantidade_parametros;
            copia->como.declaracao_funcao.corpo = avaliar_template_sintaxe_no(interpretador,
                                                                               ambiente,
                                                                               no->como.declaracao_funcao.corpo);
            return copia;
        }
        case NO_DECL_MACRO: {
            No *copia = runtime_novo_no(NO_DECL_MACRO, no->linha);
            copia->como.declaracao_macro.nome = noema_duplicar(no->como.declaracao_macro.nome);
            copia->como.declaracao_macro.parametros =
                clonar_lista_textos(no->como.declaracao_macro.parametros, no->como.declaracao_macro.quantidade_parametros);
            copia->como.declaracao_macro.quantidade_parametros = no->como.declaracao_macro.quantidade_parametros;
            copia->como.declaracao_macro.corpo =
                avaliar_template_sintaxe_no(interpretador, ambiente, no->como.declaracao_macro.corpo);
            return copia;
        }
        case NO_EXPR_STMT: {
            if (no->como.expressao.expressao != NULL &&
                no->como.expressao.expressao->tipo == NO_SYNTAX_PLACEHOLDER) {
                Valor valor = avaliar_placeholder_sintaxe(interpretador, ambiente, no->como.expressao.expressao);
                return valor_para_no_instrucao(valor, no->linha);
            }
            No *stmt = runtime_novo_no(NO_EXPR_STMT, no->linha);
            stmt->como.expressao.expressao =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador,
                                                                  ambiente,
                                                                  no->como.expressao.expressao),
                                      "instrucao de expressao");
            return stmt;
        }
        case NO_EXPAND: {
            No *copia = runtime_novo_no(NO_EXPAND, no->linha);
            copia->como.expansao_macro.nome = noema_duplicar(no->como.expansao_macro.nome);
            for (int i = 0; i < no->como.expansao_macro.quantidade_argumentos; i++) {
                expansao_macro_adicionar_argumento_runtime(
                    copia,
                    avaliar_template_sintaxe_no(interpretador, ambiente, no->como.expansao_macro.argumentos[i]));
            }
            return copia;
        }
        case NO_SYNTAX_PLACEHOLDER:
            return valor_para_no_sintaxe(avaliar_placeholder_sintaxe(interpretador, ambiente, no), no->linha);
        case NO_IF: {
            No *copia = runtime_novo_no(NO_IF, no->linha);
            copia->como.condicional.condicao =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.condicional.condicao),
                                      "condicao");
            copia->como.condicional.ramo_entao =
                avaliar_template_sintaxe_no(interpretador, ambiente, no->como.condicional.ramo_entao);
            copia->como.condicional.ramo_senao =
                avaliar_template_sintaxe_no(interpretador, ambiente, no->como.condicional.ramo_senao);
            return copia;
        }
        case NO_WHILE: {
            No *copia = runtime_novo_no(NO_WHILE, no->linha);
            copia->como.enquanto.condicao =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.enquanto.condicao),
                                      "condicao do while");
            copia->como.enquanto.corpo = avaliar_template_sintaxe_no(interpretador, ambiente, no->como.enquanto.corpo);
            return copia;
        }
        case NO_FOR: {
            No *copia = runtime_novo_no(NO_FOR, no->linha);
            copia->como.para.variavel = noema_duplicar(no->como.para.variavel);
            copia->como.para.iteravel =
                garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.para.iteravel),
                                      "iteravel do for");
            copia->como.para.corpo = avaliar_template_sintaxe_no(interpretador, ambiente, no->como.para.corpo);
            return copia;
        }
        case NO_RETURN: {
            No *copia = runtime_novo_no(NO_RETURN, no->linha);
            copia->como.retorno.valor =
                no->como.retorno.valor != NULL
                    ? garantir_no_expressao(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.retorno.valor),
                                            "valor de retorno")
                    : NULL;
            return copia;
        }
        case NO_BREAK:
        case NO_CONTINUE:
        case NO_DECL_PLUGIN:
            return clonar_no(no);
    }

    return clonar_no(no);
}

static No *expandir_invocacao_macro(Interpretador *interpretador,
                                    AmbienteMacro *ambiente_macro,
                                    const char *nome,
                                    No **argumentos,
                                    int quantidade_argumentos,
                                    int linha,
                                    bool contexto_instrucao) {
    MacroExecutavel *macro = ambiente_macro_buscar(ambiente_macro, nome);
    if (macro == NULL) {
        noema_falhar("macro indefinida: %s", nome);
    }
    if (quantidade_argumentos != macro->quantidade_parametros) {
        noema_falhar("macro '%s' esperava %d argumentos e recebeu %d",
                     nome,
                     macro->quantidade_parametros,
                     quantidade_argumentos);
    }

    Ambiente *local = ambiente_criar(interpretador->global);
    for (int i = 0; i < macro->quantidade_parametros; i++) {
        ambiente_definir(local, macro->parametros[i], valor_sintaxe(clonar_no(argumentos[i])), false);
    }

    ResultadoExecucao resultado = executar(interpretador, local, macro->corpo);
    if (resultado.sinal == EXEC_BREAK || resultado.sinal == EXEC_CONTINUE) {
        noema_falhar("macro '%s' nao pode usar break/continue no topo", nome);
    }

    Valor valor = resultado.valor;
    if (resultado.sinal == EXEC_RETURN) {
        valor = resultado.valor;
    }
    if (valor.tipo != VALOR_SINTAXE) {
        noema_falhar("macro '%s' precisa retornar syntax", nome);
    }

    No *expandido = expandir_no(interpretador, ambiente_macro, clonar_no(valor.como.sintaxe), contexto_instrucao);
    if (expandido == NULL) {
        if (contexto_instrucao) {
            return NULL;
        }
        noema_falhar("macro '%s' precisa expandir para uma expressao", nome);
    }
    if (!contexto_instrucao && !no_eh_expressao(expandido->tipo)) {
        noema_falhar("macro '%s' precisa expandir para uma expressao", nome);
    }
    (void)linha;
    return expandido;
}

static No *expandir_em_contexto_expressao(Interpretador *interpretador,
                                          AmbienteMacro *ambiente_macro,
                                          No *no,
                                          const char *contexto) {
    No *expandido = expandir_no(interpretador, ambiente_macro, no, false);
    return garantir_no_expressao(expandido, contexto);
}

static No *expandir_no(Interpretador *interpretador, AmbienteMacro *ambiente_macro, No *no, bool contexto_instrucao) {
    if (no == NULL) {
        return NULL;
    }

    switch (no->tipo) {
        case NO_BLOCO: {
            AmbienteMacro local;
            ambiente_macro_inicializar(&local, ambiente_macro);
            No *bloco = runtime_novo_no(NO_BLOCO, no->linha);
            for (int i = 0; i < no->como.bloco.quantidade; i++) {
                No *item = expandir_no(interpretador, &local, no->como.bloco.itens[i], true);
                if (item != NULL) {
                    bloco_adicionar_item_runtime(bloco, item);
                }
            }
            return bloco;
        }
        case NO_LITERAL:
        case NO_VARIAVEL:
        case NO_DECL_PLUGIN:
        case NO_SYNTAX_TEMPLATE:
            return clonar_no(no);
        case NO_LISTA_LITERAL: {
            No *lista = runtime_novo_no(NO_LISTA_LITERAL, no->linha);
            for (int i = 0; i < no->como.lista.quantidade; i++) {
                lista_literal_adicionar_item_runtime(
                    lista,
                    expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.lista.itens[i], "item de lista"));
            }
            return lista;
        }
        case NO_MAPA_LITERAL: {
            No *mapa = runtime_novo_no(NO_MAPA_LITERAL, no->linha);
            for (int i = 0; i < no->como.mapa.quantidade; i++) {
                mapa_literal_adicionar_item_runtime(
                    mapa,
                    no->como.mapa.chaves[i],
                    expandir_em_contexto_expressao(
                        interpretador, ambiente_macro, no->como.mapa.valores[i], "valor de mapa"));
            }
            return mapa;
        }
        case NO_BINARIO: {
            No *copia = runtime_novo_no(NO_BINARIO, no->linha);
            copia->como.binario.operador = no->como.binario.operador;
            copia->como.binario.esquerda =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.binario.esquerda, "operando esquerdo");
            copia->como.binario.direita =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.binario.direita, "operando direito");
            return copia;
        }
        case NO_UNARIO: {
            No *copia = runtime_novo_no(NO_UNARIO, no->linha);
            copia->como.unario.operador = no->como.unario.operador;
            copia->como.unario.direita =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.unario.direita, "operando unario");
            return copia;
        }
        case NO_CHAMADA: {
            No *copia = runtime_novo_no(NO_CHAMADA, no->linha);
            copia->como.chamada.alvo =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.chamada.alvo, "alvo de chamada");
            for (int i = 0; i < no->como.chamada.quantidade_argumentos; i++) {
                chamada_adicionar_argumento_runtime(
                    copia,
                    expandir_em_contexto_expressao(
                        interpretador, ambiente_macro, no->como.chamada.argumentos[i], "argumento de chamada"));
            }
            return copia;
        }
        case NO_MEMBRO: {
            No *copia = runtime_novo_no(NO_MEMBRO, no->linha);
            copia->como.membro.objeto =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.membro.objeto, "objeto de membro");
            copia->como.membro.nome = noema_duplicar(no->como.membro.nome);
            return copia;
        }
        case NO_INDICE: {
            No *copia = runtime_novo_no(NO_INDICE, no->linha);
            copia->como.indice.objeto =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.indice.objeto, "objeto de indice");
            copia->como.indice.indice =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.indice.indice, "indice");
            return copia;
        }
        case NO_ATRIBUICAO: {
            No *copia = runtime_novo_no(NO_ATRIBUICAO, no->linha);
            copia->como.atribuicao.alvo =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.atribuicao.alvo, "alvo de atribuicao");
            copia->como.atribuicao.valor =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.atribuicao.valor, "valor de atribuicao");
            return copia;
        }
        case NO_DECL_LET:
        case NO_DECL_CONST: {
            No *copia = runtime_novo_no(no->tipo, no->linha);
            copia->como.declaracao_variavel.nome = noema_duplicar(no->como.declaracao_variavel.nome);
            copia->como.declaracao_variavel.inicializador =
                no->como.declaracao_variavel.inicializador != NULL
                    ? expandir_em_contexto_expressao(
                          interpretador, ambiente_macro, no->como.declaracao_variavel.inicializador, "inicializador")
                    : NULL;
            return copia;
        }
        case NO_DECL_FUNCAO:
        case NO_FUNCAO_ANONIMA: {
            No *copia = runtime_novo_no(no->tipo, no->linha);
            copia->como.declaracao_funcao.nome =
                no->como.declaracao_funcao.nome != NULL ? noema_duplicar(no->como.declaracao_funcao.nome) : NULL;
            copia->como.declaracao_funcao.parametros =
                clonar_lista_textos(no->como.declaracao_funcao.parametros, no->como.declaracao_funcao.quantidade_parametros);
            copia->como.declaracao_funcao.quantidade_parametros = no->como.declaracao_funcao.quantidade_parametros;
            copia->como.declaracao_funcao.corpo =
                expandir_no(interpretador, ambiente_macro, no->como.declaracao_funcao.corpo, true);
            return copia;
        }
        case NO_DECL_MACRO:
            ambiente_macro_registrar(ambiente_macro, no);
            return NULL;
        case NO_EXPR_STMT: {
            No *expandido = expandir_no(interpretador, ambiente_macro, no->como.expressao.expressao, true);
            if (expandido == NULL) {
                return NULL;
            }
            if (no_eh_expressao(expandido->tipo)) {
                No *stmt = runtime_novo_no(NO_EXPR_STMT, no->linha);
                stmt->como.expressao.expressao = expandido;
                return stmt;
            }
            return expandido;
        }
        case NO_EXPAND:
            return expandir_invocacao_macro(interpretador,
                                            ambiente_macro,
                                            no->como.expansao_macro.nome,
                                            no->como.expansao_macro.argumentos,
                                            no->como.expansao_macro.quantidade_argumentos,
                                            no->linha,
                                            contexto_instrucao);
        case NO_SYNTAX_PLACEHOLDER:
            noema_falhar("placeholder de sintaxe fora de syntax");
            return NULL;
        case NO_IF: {
            No *copia = runtime_novo_no(NO_IF, no->linha);
            copia->como.condicional.condicao =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.condicional.condicao, "condicao");
            copia->como.condicional.ramo_entao =
                expandir_no(interpretador, ambiente_macro, no->como.condicional.ramo_entao, true);
            copia->como.condicional.ramo_senao =
                expandir_no(interpretador, ambiente_macro, no->como.condicional.ramo_senao, true);
            return copia;
        }
        case NO_WHILE: {
            No *copia = runtime_novo_no(NO_WHILE, no->linha);
            copia->como.enquanto.condicao =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.enquanto.condicao, "condicao do while");
            copia->como.enquanto.corpo = expandir_no(interpretador, ambiente_macro, no->como.enquanto.corpo, true);
            return copia;
        }
        case NO_FOR: {
            No *copia = runtime_novo_no(NO_FOR, no->linha);
            copia->como.para.variavel = noema_duplicar(no->como.para.variavel);
            copia->como.para.iteravel =
                expandir_em_contexto_expressao(interpretador, ambiente_macro, no->como.para.iteravel, "iteravel do for");
            copia->como.para.corpo = expandir_no(interpretador, ambiente_macro, no->como.para.corpo, true);
            return copia;
        }
        case NO_RETURN: {
            No *copia = runtime_novo_no(NO_RETURN, no->linha);
            copia->como.retorno.valor =
                no->como.retorno.valor != NULL
                    ? expandir_em_contexto_expressao(
                          interpretador, ambiente_macro, no->como.retorno.valor, "valor de retorno")
                    : NULL;
            return copia;
        }
        case NO_BREAK:
        case NO_CONTINUE:
            return clonar_no(no);
    }

    return clonar_no(no);
}

char *noema_ler_arquivo(const char *caminho) {
    FILE *arquivo = fopen(caminho, "rb");
    if (arquivo == NULL) {
        noema_falhar("nao foi possivel abrir '%s': %s", caminho, strerror(errno));
    }

    if (fseek(arquivo, 0, SEEK_END) != 0) {
        fclose(arquivo);
        noema_falhar("nao foi possivel posicionar '%s'", caminho);
    }

    long tamanho = ftell(arquivo);
    if (tamanho < 0) {
        fclose(arquivo);
        noema_falhar("nao foi possivel medir '%s'", caminho);
    }

    rewind(arquivo);

    char *conteudo = malloc((size_t)tamanho + 1);
    if (conteudo == NULL) {
        fclose(arquivo);
        noema_falhar("memoria insuficiente");
    }

    size_t lidos = fread(conteudo, 1, (size_t)tamanho, arquivo);
    fclose(arquivo);

    if (lidos != (size_t)tamanho) {
        noema_falhar("nao foi possivel ler '%s' por completo", caminho);
    }

    conteudo[tamanho] = '\0';
    return conteudo;
}

bool noema_caminho_absoluto(const char *caminho) {
    return noema_comprimento_raiz_caminho(caminho) > 0;
}

bool noema_arquivo_existe(const char *caminho) {
    FILE *arquivo = fopen(caminho, "rb");
    if (arquivo == NULL) {
        return false;
    }
    fclose(arquivo);
    return true;
}

bool noema_tem_sufixo(const char *texto, const char *sufixo) {
    size_t tamanho_texto = strlen(texto);
    size_t tamanho_sufixo = strlen(sufixo);
    if (tamanho_sufixo > tamanho_texto) {
        return false;
    }
    return strcmp(texto + tamanho_texto - tamanho_sufixo, sufixo) == 0;
}

char *noema_normalizar_caminho_modulo(const char *caminho) {
    if (noema_tem_sufixo(caminho, ".noe")) {
        return noema_duplicar(caminho);
    }
    return noema_formatar("%s.noe", caminho);
}

char *noema_diretorio_de(const char *caminho) {
    size_t tamanho = strlen(caminho);
    size_t raiz = noema_comprimento_raiz_caminho(caminho);

    while (tamanho > raiz && noema_eh_separador_caminho(caminho[tamanho - 1])) {
        tamanho--;
    }

    size_t cursor = tamanho;
    while (cursor > raiz && !noema_eh_separador_caminho(caminho[cursor - 1])) {
        cursor--;
    }

    if (cursor <= raiz) {
        if (raiz > 0) {
            return noema_formatar("%.*s", (int)raiz, caminho);
        }
        return noema_duplicar(".");
    }

    while (cursor > raiz && noema_eh_separador_caminho(caminho[cursor - 1])) {
        cursor--;
    }

    char *saida = malloc(cursor + 1);
    if (saida == NULL) {
        noema_falhar("memoria insuficiente");
    }
    memcpy(saida, caminho, cursor);
    saida[cursor] = '\0';
    return saida;
}

char *noema_juntar_caminhos(const char *base, const char *relativo) {
    if (noema_caminho_absoluto(relativo)) {
        return noema_duplicar(relativo);
    }

    if (strcmp(base, ".") == 0) {
        return noema_duplicar(relativo);
    }

    char separador = noema_obter_separador_caminho();
    size_t tamanho_base = strlen(base);
    while (tamanho_base > 0 && noema_eh_separador_caminho(base[tamanho_base - 1])) {
        tamanho_base--;
    }
    while (*relativo != '\0' && noema_eh_separador_caminho(*relativo)) {
        relativo++;
    }
    return noema_formatar("%.*s%c%s", (int)tamanho_base, base, separador, relativo);
}

static bool noema_diretorio_tem_prelude(const char *diretorio) {
    char *caminho = noema_juntar_caminhos(diretorio, "prelude.noe");
    bool existe = noema_arquivo_existe(caminho);
    free(caminho);
    return existe;
}

static char *noema_resolver_executavel_no_path(const char *argv0) {
    const char *path = getenv("PATH");
    if (argv0 == NULL || argv0[0] == '\0' || path == NULL || path[0] == '\0') {
        return NULL;
    }

    const char *inicio = path;
    while (true) {
        const char *fim = strchr(inicio, ':');
        size_t tamanho = fim != NULL ? (size_t)(fim - inicio) : strlen(inicio);
        char *diretorio = NULL;

        if (tamanho == 0) {
            diretorio = noema_duplicar(".");
        } else {
            diretorio = malloc(tamanho + 1);
            if (diretorio == NULL) {
                noema_falhar("memoria insuficiente");
            }
            memcpy(diretorio, inicio, tamanho);
            diretorio[tamanho] = '\0';
        }

        char *candidato = noema_juntar_caminhos(diretorio, argv0);
        free(diretorio);

        if (noema_verificar_acesso_executavel(candidato) == 0) {
            return candidato;
        }

        free(candidato);
        if (fim == NULL) {
            break;
        }
        inicio = fim + 1;
    }

    return NULL;
}

static char *noema_descobrir_caminho_executavel(const char *argv0) {
#ifdef _WIN32
    DWORD capacidade = MAX_PATH;
    for (;;) {
        char *caminho = malloc((size_t)capacidade + 1);
        if (caminho == NULL) {
            noema_falhar("memoria insuficiente");
        }

        DWORD tamanho = GetModuleFileNameA(NULL, caminho, capacidade);
        if (tamanho == 0) {
            free(caminho);
            break;
        }
        if (tamanho < capacidade) {
            caminho[tamanho] = '\0';
            return caminho;
        }

        free(caminho);
        capacidade *= 2;
        if (capacidade > 32768) {
            break;
        }
    }
#else
#ifdef __linux__
    size_t capacidade = 256;
    while (capacidade <= 16384) {
        char *caminho = malloc(capacidade);
        if (caminho == NULL) {
            noema_falhar("memoria insuficiente");
        }

        ssize_t lido = readlink("/proc/self/exe", caminho, capacidade - 1);
        if (lido >= 0 && (size_t)lido < capacidade - 1) {
            caminho[lido] = '\0';
            return caminho;
        }

        free(caminho);
        if (lido < 0) {
            break;
        }
        capacidade *= 2;
    }
#endif
#endif

    if (argv0 == NULL || argv0[0] == '\0') {
        return NULL;
    }

    if (strchr(argv0, '/') != NULL) {
        return noema_duplicar(argv0);
    }

    return noema_resolver_executavel_no_path(argv0);
}

void noema_configurar_diretorio_stdlib(const char *caminho) {
    if (caminho == NULL) {
        g_noema_diretorio_stdlib = NULL;
        return;
    }
    g_noema_diretorio_stdlib = noema_duplicar(caminho);
}

const char *noema_obter_diretorio_stdlib(void) {
    return g_noema_diretorio_stdlib;
}

char *noema_resolver_importacao(const char *base, const char *relativo) {
    char *normalizado = noema_normalizar_caminho_modulo(relativo);
    if (strncmp(normalizado, "std/", 4) == 0) {
        if (g_noema_diretorio_stdlib == NULL) {
            noema_falhar("stdlib nao configurada para resolver '%s'", relativo);
        }
        return noema_juntar_caminhos(g_noema_diretorio_stdlib, normalizado + 4);
    }
    return noema_juntar_caminhos(base, normalizado);
}

char *noema_descobrir_diretorio_stdlib(const char *argv0) {
    const char *env = getenv("NOEMA_STDLIB");
    if (env != NULL && env[0] != '\0') {
        return noema_duplicar(env);
    }

    char *fallback = NULL;
    char *caminho_executavel = noema_descobrir_caminho_executavel(argv0);
    if (caminho_executavel != NULL) {
        char *diretorio_executavel = noema_diretorio_de(caminho_executavel);
        free(caminho_executavel);

        char *candidato_share = noema_juntar_caminhos(
            diretorio_executavel,
#ifdef _WIN32
            "..\\share\\noema\\stdlib"
#else
            "../share/noema/stdlib"
#endif
        );
        if (noema_diretorio_tem_prelude(candidato_share)) {
            free(diretorio_executavel);
            return candidato_share;
        }
        fallback = candidato_share;

        char *candidato_repositorio = noema_juntar_caminhos(
            diretorio_executavel,
#ifdef _WIN32
            "..\\stdlib"
#else
            "../stdlib"
#endif
        );
        if (noema_diretorio_tem_prelude(candidato_repositorio)) {
            free(fallback);
            free(diretorio_executavel);
            return candidato_repositorio;
        }
        free(candidato_repositorio);

        char *candidato_local = noema_juntar_caminhos(diretorio_executavel, "stdlib");
        if (noema_diretorio_tem_prelude(candidato_local)) {
            free(fallback);
            free(diretorio_executavel);
            return candidato_local;
        }
        free(candidato_local);
        free(diretorio_executavel);
    }

    char *local = noema_duplicar("stdlib");
    if (noema_diretorio_tem_prelude(local)) {
        free(fallback);
        return local;
    }
    free(local);

    if (fallback != NULL) {
        return fallback;
    }

    return noema_duplicar("stdlib");
}

Valor valor_nulo(void) {
    Valor valor;
    valor.tipo = VALOR_NULO;
    return valor;
}

Valor valor_bool(bool valor_bool_local) {
    Valor valor;
    valor.tipo = VALOR_BOOL;
    valor.como.booleano = valor_bool_local;
    return valor;
}

Valor valor_numero(double valor_numero_local) {
    Valor valor;
    valor.tipo = VALOR_NUMERO;
    valor.como.numero = valor_numero_local;
    return valor;
}

Valor valor_texto(const char *valor_texto_local) {
    Valor valor;
    valor.tipo = VALOR_TEXTO;
    valor.como.texto = noema_duplicar(valor_texto_local);
    return valor;
}

Valor valor_lista(Lista *lista) {
    Valor valor;
    valor.tipo = VALOR_LISTA;
    valor.como.lista = lista;
    return valor;
}

Valor valor_mapa(Mapa *mapa) {
    Valor valor;
    valor.tipo = VALOR_MAPA;
    valor.como.mapa = mapa;
    return valor;
}

Valor valor_ponteiro(void *ponteiro) {
    Valor valor;
    valor.tipo = VALOR_PONTEIRO;
    valor.como.ponteiro = ponteiro;
    return valor;
}

Valor valor_sintaxe(No *sintaxe) {
    Valor valor;
    valor.tipo = VALOR_SINTAXE;
    valor.como.sintaxe = sintaxe;
    return valor;
}

Valor valor_funcao(FuncaoNoema *funcao) {
    Valor valor;
    valor.tipo = VALOR_FUNCAO;
    valor.como.funcao = funcao;
    return valor;
}

Valor valor_nativa(FuncaoNativa *nativa) {
    Valor valor;
    valor.tipo = VALOR_NATIVA;
    valor.como.nativa = nativa;
    return valor;
}

Lista *lista_criar(void) {
    Lista *lista = calloc(1, sizeof(Lista));
    if (lista == NULL) {
        noema_falhar("memoria insuficiente");
    }
    lista->capacidade = 8;
    lista->itens = calloc((size_t)lista->capacidade, sizeof(Valor));
    if (lista->itens == NULL) {
        noema_falhar("memoria insuficiente");
    }
    return lista;
}

void lista_adicionar(Lista *lista, Valor valor) {
    if (lista->quantidade >= lista->capacidade) {
        lista->capacidade *= 2;
        lista->itens = realloc(lista->itens, (size_t)lista->capacidade * sizeof(Valor));
        if (lista->itens == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    lista->itens[lista->quantidade++] = valor;
}

Valor lista_obter(Lista *lista, int indice) {
    if (indice < 0 || indice >= lista->quantidade) {
        return valor_nulo();
    }
    return lista->itens[indice];
}

void lista_definir(Lista *lista, int indice, Valor valor) {
    if (indice < 0) {
        noema_falhar("indice negativo em lista");
    }
    while (indice >= lista->capacidade) {
        lista->capacidade *= 2;
        lista->itens = realloc(lista->itens, (size_t)lista->capacidade * sizeof(Valor));
        if (lista->itens == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    while (lista->quantidade <= indice) {
        lista->itens[lista->quantidade++] = valor_nulo();
    }
    lista->itens[indice] = valor;
}

Valor lista_remover_ultimo(Lista *lista) {
    if (lista->quantidade == 0) {
        return valor_nulo();
    }
    return lista->itens[--lista->quantidade];
}

Mapa *mapa_criar(void) {
    Mapa *mapa = calloc(1, sizeof(Mapa));
    if (mapa == NULL) {
        noema_falhar("memoria insuficiente");
    }
    mapa->capacidade = 8;
    mapa->entradas = calloc((size_t)mapa->capacidade, sizeof(EntradaMapa));
    if (mapa->entradas == NULL) {
        noema_falhar("memoria insuficiente");
    }
    return mapa;
}

static int mapa_indice(Mapa *mapa, const char *chave) {
    for (int i = 0; i < mapa->quantidade; i++) {
        if (strcmp(mapa->entradas[i].chave, chave) == 0) {
            return i;
        }
    }
    return -1;
}

void mapa_definir(Mapa *mapa, const char *chave, Valor valor) {
    int indice = mapa_indice(mapa, chave);
    if (indice >= 0) {
        mapa->entradas[indice].valor = valor;
        return;
    }

    if (mapa->quantidade >= mapa->capacidade) {
        mapa->capacidade *= 2;
        mapa->entradas = realloc(mapa->entradas, (size_t)mapa->capacidade * sizeof(EntradaMapa));
        if (mapa->entradas == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }

    mapa->entradas[mapa->quantidade].chave = noema_duplicar(chave);
    mapa->entradas[mapa->quantidade].valor = valor;
    mapa->quantidade++;
}

bool mapa_obter(Mapa *mapa, const char *chave, Valor *saida) {
    int indice = mapa_indice(mapa, chave);
    if (indice < 0) {
        return false;
    }
    *saida = mapa->entradas[indice].valor;
    return true;
}

bool mapa_tem(Mapa *mapa, const char *chave) {
    return mapa_indice(mapa, chave) >= 0;
}

char **mapa_chaves(Mapa *mapa, int *quantidade) {
    char **chaves = calloc((size_t)mapa->quantidade, sizeof(char *));
    if (chaves == NULL) {
        noema_falhar("memoria insuficiente");
    }
    for (int i = 0; i < mapa->quantidade; i++) {
        chaves[i] = noema_duplicar(mapa->entradas[i].chave);
    }
    *quantidade = mapa->quantidade;
    return chaves;
}

Ambiente *ambiente_criar(Ambiente *pai) {
    Ambiente *ambiente = calloc(1, sizeof(Ambiente));
    if (ambiente == NULL) {
        noema_falhar("memoria insuficiente");
    }
    ambiente->pai = pai;
    ambiente->capacidade = 8;
    ambiente->entradas = calloc((size_t)ambiente->capacidade, sizeof(EntradaAmbiente));
    if (ambiente->entradas == NULL) {
        noema_falhar("memoria insuficiente");
    }
    return ambiente;
}

static int ambiente_indice_local(Ambiente *ambiente, const char *nome) {
    for (int i = 0; i < ambiente->quantidade; i++) {
        if (strcmp(ambiente->entradas[i].nome, nome) == 0) {
            return i;
        }
    }
    return -1;
}

void ambiente_definir(Ambiente *ambiente, const char *nome, Valor valor, bool constante) {
    int indice = ambiente_indice_local(ambiente, nome);
    if (indice >= 0) {
        if (ambiente->entradas[indice].constante) {
            noema_falhar("'%s' ja foi definido como constante", nome);
        }
        ambiente->entradas[indice].valor = valor;
        ambiente->entradas[indice].constante = constante;
        return;
    }

    if (ambiente->quantidade >= ambiente->capacidade) {
        ambiente->capacidade *= 2;
        ambiente->entradas = realloc(ambiente->entradas, (size_t)ambiente->capacidade * sizeof(EntradaAmbiente));
        if (ambiente->entradas == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }

    ambiente->entradas[ambiente->quantidade].nome = noema_duplicar(nome);
    ambiente->entradas[ambiente->quantidade].valor = valor;
    ambiente->entradas[ambiente->quantidade].constante = constante;
    ambiente->quantidade++;
}

bool ambiente_atribuir(Ambiente *ambiente, const char *nome, Valor valor) {
    for (Ambiente *cursor = ambiente; cursor != NULL; cursor = cursor->pai) {
        int indice = ambiente_indice_local(cursor, nome);
        if (indice >= 0) {
            if (cursor->entradas[indice].constante) {
                noema_falhar("'%s' e constante e nao pode ser reatribuido", nome);
            }
            cursor->entradas[indice].valor = valor;
            return true;
        }
    }
    return false;
}

bool ambiente_obter(Ambiente *ambiente, const char *nome, Valor *saida) {
    for (Ambiente *cursor = ambiente; cursor != NULL; cursor = cursor->pai) {
        int indice = ambiente_indice_local(cursor, nome);
        if (indice >= 0) {
            *saida = cursor->entradas[indice].valor;
            return true;
        }
    }
    return false;
}

bool valor_eh_verdadeiro(Valor valor) {
    switch (valor.tipo) {
        case VALOR_NULO:
            return false;
        case VALOR_BOOL:
            return valor.como.booleano;
        case VALOR_NUMERO:
            return fabs(valor.como.numero) > 1e-9;
        case VALOR_TEXTO:
            return valor.como.texto[0] != '\0';
        case VALOR_LISTA:
            return valor.como.lista->quantidade > 0;
        case VALOR_MAPA:
            return valor.como.mapa->quantidade > 0;
        case VALOR_PONTEIRO:
            return valor.como.ponteiro != NULL;
        case VALOR_SINTAXE:
        case VALOR_FUNCAO:
        case VALOR_NATIVA:
            return true;
    }
    return false;
}

bool valor_iguais(Valor a, Valor b) {
    if (a.tipo != b.tipo) {
        return false;
    }

    switch (a.tipo) {
        case VALOR_NULO:
            return true;
        case VALOR_BOOL:
            return a.como.booleano == b.como.booleano;
        case VALOR_NUMERO:
            return fabs(a.como.numero - b.como.numero) < 1e-9;
        case VALOR_TEXTO:
            return strcmp(a.como.texto, b.como.texto) == 0;
        case VALOR_LISTA:
            return a.como.lista == b.como.lista;
        case VALOR_MAPA:
            return a.como.mapa == b.como.mapa;
        case VALOR_PONTEIRO:
            return a.como.ponteiro == b.como.ponteiro;
        case VALOR_SINTAXE: {
            char *texto_a = no_para_texto(a.como.sintaxe);
            char *texto_b = no_para_texto(b.como.sintaxe);
            return strcmp(texto_a, texto_b) == 0;
        }
        case VALOR_FUNCAO:
            return a.como.funcao == b.como.funcao;
        case VALOR_NATIVA:
            return a.como.nativa == b.como.nativa;
    }
    return false;
}

double valor_para_numero(Valor valor) {
    switch (valor.tipo) {
        case VALOR_NUMERO:
            return valor.como.numero;
        case VALOR_BOOL:
            return valor.como.booleano ? 1.0 : 0.0;
        case VALOR_TEXTO:
            return strtod(valor.como.texto, NULL);
        case VALOR_NULO:
            return 0.0;
        case VALOR_PONTEIRO:
            return (double)(uintptr_t)valor.como.ponteiro;
        case VALOR_SINTAXE:
            noema_falhar("nao foi possivel converter syntax para numero");
        default:
            noema_falhar("nao foi possivel converter %s para numero", valor_tipo_nome(valor));
    }
    return 0.0;
}

const char *valor_tipo_nome(Valor valor) {
    switch (valor.tipo) {
        case VALOR_NULO:
            return "null";
        case VALOR_BOOL:
            return "bool";
        case VALOR_NUMERO:
            return "number";
        case VALOR_TEXTO:
            return "string";
        case VALOR_LISTA:
            return "list";
        case VALOR_MAPA:
            return "map";
        case VALOR_PONTEIRO:
            return "pointer";
        case VALOR_SINTAXE:
            return "syntax";
        case VALOR_FUNCAO:
            return "function";
        case VALOR_NATIVA:
            return "native";
    }
    return "unknown";
}

static char *valor_para_texto_interno(Valor valor, int profundidade) {
    if (profundidade > 4) {
        return noema_duplicar("...");
    }

    switch (valor.tipo) {
        case VALOR_NULO:
            return noema_duplicar("null");
        case VALOR_BOOL:
            return noema_duplicar(valor.como.booleano ? "true" : "false");
        case VALOR_NUMERO:
            return noema_formatar("%.15g", valor.como.numero);
        case VALOR_TEXTO:
            return noema_duplicar(valor.como.texto);
        case VALOR_PONTEIRO:
            return noema_formatar("<pointer %p>", valor.como.ponteiro);
        case VALOR_SINTAXE:
            return no_para_texto(valor.como.sintaxe);
        case VALOR_FUNCAO:
            return noema_formatar("<function %s>", valor.como.funcao->nome);
        case VALOR_NATIVA:
            return noema_formatar("<native %s>", valor.como.nativa->nome);
        case VALOR_LISTA: {
            MontadorTexto montador;
            montador_inicializar(&montador);
            montador_anexar(&montador, "[");
            for (int i = 0; i < valor.como.lista->quantidade; i++) {
                if (i > 0) {
                    montador_anexar(&montador, ", ");
                }
                char *texto_item = valor_para_texto_interno(valor.como.lista->itens[i], profundidade + 1);
                montador_anexar(&montador, texto_item);
            }
            montador_anexar(&montador, "]");
            return montador_finalizar(&montador);
        }
        case VALOR_MAPA: {
            MontadorTexto montador;
            montador_inicializar(&montador);
            montador_anexar(&montador, "{");
            for (int i = 0; i < valor.como.mapa->quantidade; i++) {
                if (i > 0) {
                    montador_anexar(&montador, ", ");
                }
                montador_anexar(&montador, valor.como.mapa->entradas[i].chave);
                montador_anexar(&montador, ": ");
                char *texto_item = valor_para_texto_interno(valor.como.mapa->entradas[i].valor, profundidade + 1);
                montador_anexar(&montador, texto_item);
            }
            montador_anexar(&montador, "}");
            return montador_finalizar(&montador);
        }
    }

    return noema_duplicar("<unknown>");
}

char *valor_para_texto(Valor valor) {
    return valor_para_texto_interno(valor, 0);
}

Interpretador *interpretador_criar(void) {
    Interpretador *interpretador = calloc(1, sizeof(Interpretador));
    if (interpretador == NULL) {
        noema_falhar("memoria insuficiente");
    }
    interpretador->global = ambiente_criar(NULL);
    interpretador->semente_aleatoria = (unsigned int)time(NULL);
    registrar_builtins(interpretador);
    if (g_noema_diretorio_stdlib != NULL) {
        char *prelude = noema_resolver_importacao(".", "std/prelude");
        if (!noema_arquivo_existe(prelude)) {
            noema_falhar("prelude da stdlib nao encontrado em '%s'", prelude);
        }
        FonteProcessada fonte = preprocessar_arquivo(prelude, &interpretador->macros, &interpretador->importados);
        Programa *programa = parser_analisar(fonte.codigo, fonte.caminho);
        interpretador_executar_programa(interpretador, programa, interpretador->global);
    }
    return interpretador;
}

void interpretador_destruir(Interpretador *interpretador) {
    free(interpretador);
}

static ResultadoExecucao resultado_normal(Valor valor) {
    ResultadoExecucao resultado;
    resultado.sinal = EXEC_NORMAL;
    resultado.valor = valor;
    return resultado;
}

static ResultadoExecucao resultado_com_sinal(SinalExecucao sinal, Valor valor) {
    ResultadoExecucao resultado;
    resultado.sinal = sinal;
    resultado.valor = valor;
    return resultado;
}

static FuncaoNoema *criar_funcao_noema(No *no, Ambiente *ambiente, const char *nome_padrao) {
    FuncaoNoema *funcao = calloc(1, sizeof(FuncaoNoema));
    if (funcao == NULL) {
        noema_falhar("memoria insuficiente");
    }
    funcao->nome = noema_duplicar(no->como.declaracao_funcao.nome != NULL ? no->como.declaracao_funcao.nome : nome_padrao);
    funcao->quantidade_parametros = no->como.declaracao_funcao.quantidade_parametros;
    funcao->parametros = calloc((size_t)funcao->quantidade_parametros, sizeof(char *));
    if (funcao->parametros == NULL && funcao->quantidade_parametros > 0) {
        noema_falhar("memoria insuficiente");
    }
    for (int i = 0; i < funcao->quantidade_parametros; i++) {
        funcao->parametros[i] = noema_duplicar(no->como.declaracao_funcao.parametros[i]);
    }
    funcao->corpo = no->como.declaracao_funcao.corpo;
    funcao->fecho = ambiente;
    return funcao;
}

static char *fatia_texto(const char *texto, size_t inicio, size_t fim) {
    size_t tamanho = fim - inicio;
    char *saida = malloc(tamanho + 1);
    if (saida == NULL) {
        noema_falhar("memoria insuficiente");
    }
    memcpy(saida, texto + inicio, tamanho);
    saida[tamanho] = '\0';
    return saida;
}

static char *interpolar_texto(Interpretador *interpretador, Ambiente *ambiente, const char *texto) {
    if (strstr(texto, "#{") == NULL) {
        return noema_duplicar(texto);
    }

    MontadorTexto montador;
    montador_inicializar(&montador);

    for (size_t i = 0; texto[i] != '\0'; i++) {
        if (texto[i] == '#' && texto[i + 1] == '{') {
            size_t inicio = i + 2;
            size_t fim = inicio;
            int profundidade = 1;
            while (texto[fim] != '\0' && profundidade > 0) {
                if (texto[fim] == '{') {
                    profundidade++;
                } else if (texto[fim] == '}') {
                    profundidade--;
                }
                fim++;
            }
            if (profundidade != 0) {
                noema_falhar("interpolacao de string sem fechamento");
            }

            char *expressao = fatia_texto(texto, inicio, fim - 1);
            char *codigo = noema_formatar("%s;", expressao);
            Valor valor = interpretador_executar_codigo(interpretador, ambiente, codigo, "<interpolacao>");
            char *texto_valor = valor_para_texto(valor);
            montador_anexar(&montador, texto_valor);
            i = fim - 1;
            continue;
        }

        char buffer[2];
        buffer[0] = texto[i];
        buffer[1] = '\0';
        montador_anexar(&montador, buffer);
    }

    return montador_finalizar(&montador);
}

static Valor obter_membro(Valor objeto, const char *nome) {
    Valor saida;
    switch (objeto.tipo) {
        case VALOR_MAPA:
            if (mapa_obter(objeto.como.mapa, nome, &saida)) {
                return saida;
            }
            return valor_nulo();
        case VALOR_LISTA:
            if (strcmp(nome, "length") == 0) {
                return valor_numero((double)objeto.como.lista->quantidade);
            }
            break;
        case VALOR_TEXTO:
            if (strcmp(nome, "length") == 0) {
                return valor_numero((double)strlen(objeto.como.texto));
            }
            break;
        default:
            break;
    }

    noema_falhar("o tipo %s nao suporta acesso a membro .%s", valor_tipo_nome(objeto), nome);
    return valor_nulo();
}

static char *indice_para_chave(Valor indice) {
    if (indice.tipo == VALOR_TEXTO) {
        return noema_duplicar(indice.como.texto);
    }
    return valor_para_texto(indice);
}

static Valor obter_indice(Valor objeto, Valor indice) {
    switch (objeto.tipo) {
        case VALOR_LISTA: {
            int posicao = (int)valor_para_numero(indice);
            if (posicao < 0) {
                posicao += objeto.como.lista->quantidade;
            }
            return lista_obter(objeto.como.lista, posicao);
        }
        case VALOR_MAPA: {
            char *chave = indice_para_chave(indice);
            Valor saida;
            if (mapa_obter(objeto.como.mapa, chave, &saida)) {
                return saida;
            }
            return valor_nulo();
        }
        case VALOR_TEXTO: {
            int posicao = (int)valor_para_numero(indice);
            size_t tamanho = strlen(objeto.como.texto);
            if (posicao < 0) {
                posicao += (int)tamanho;
            }
            if (posicao < 0 || (size_t)posicao >= tamanho) {
                return valor_nulo();
            }
            char buffer[2];
            buffer[0] = objeto.como.texto[posicao];
            buffer[1] = '\0';
            return valor_texto(buffer);
        }
        default:
            noema_falhar("o tipo %s nao suporta indexacao", valor_tipo_nome(objeto));
    }
    return valor_nulo();
}

static void atribuir_em_membro(Valor objeto, const char *nome, Valor valor) {
    if (objeto.tipo != VALOR_MAPA) {
        noema_falhar("atribuicao por membro exige map");
    }
    mapa_definir(objeto.como.mapa, nome, valor);
}

static void atribuir_em_indice(Valor objeto, Valor indice, Valor valor) {
    switch (objeto.tipo) {
        case VALOR_LISTA: {
            int posicao = (int)valor_para_numero(indice);
            if (posicao < 0) {
                posicao += objeto.como.lista->quantidade;
            }
            lista_definir(objeto.como.lista, posicao, valor);
            return;
        }
        case VALOR_MAPA: {
            char *chave = indice_para_chave(indice);
            mapa_definir(objeto.como.mapa, chave, valor);
            return;
        }
        default:
            noema_falhar("atribuicao por indice exige list ou map");
    }
}

static Valor operar_binario(Interpretador *interpretador, Ambiente *ambiente, No *no) {
    (void)interpretador;
    (void)ambiente;

    if (no->como.binario.operador == TOKEN_AND) {
        Valor esquerdo = avaliar(interpretador, ambiente, no->como.binario.esquerda);
        if (!valor_eh_verdadeiro(esquerdo)) {
            return valor_bool(false);
        }
        Valor direito = avaliar(interpretador, ambiente, no->como.binario.direita);
        return valor_bool(valor_eh_verdadeiro(direito));
    }

    if (no->como.binario.operador == TOKEN_OR) {
        Valor esquerdo = avaliar(interpretador, ambiente, no->como.binario.esquerda);
        if (valor_eh_verdadeiro(esquerdo)) {
            return valor_bool(true);
        }
        Valor direito = avaliar(interpretador, ambiente, no->como.binario.direita);
        return valor_bool(valor_eh_verdadeiro(direito));
    }

    Valor esquerdo = avaliar(interpretador, ambiente, no->como.binario.esquerda);
    Valor direito = avaliar(interpretador, ambiente, no->como.binario.direita);

    switch (no->como.binario.operador) {
        case TOKEN_MAIS:
            if (esquerdo.tipo == VALOR_TEXTO || direito.tipo == VALOR_TEXTO) {
                char *a = valor_para_texto(esquerdo);
                char *b = valor_para_texto(direito);
                char *juncao = noema_formatar("%s%s", a, b);
                return valor_texto(juncao);
            }
            return valor_numero(valor_para_numero(esquerdo) + valor_para_numero(direito));
        case TOKEN_MENOS:
            return valor_numero(valor_para_numero(esquerdo) - valor_para_numero(direito));
        case TOKEN_ASTERISCO:
            return valor_numero(valor_para_numero(esquerdo) * valor_para_numero(direito));
        case TOKEN_BARRA:
            return valor_numero(valor_para_numero(esquerdo) / valor_para_numero(direito));
        case TOKEN_PERCENTUAL:
            return valor_numero(fmod(valor_para_numero(esquerdo), valor_para_numero(direito)));
        case TOKEN_IGUAL:
            return valor_bool(valor_iguais(esquerdo, direito));
        case TOKEN_DIFERENTE:
            return valor_bool(!valor_iguais(esquerdo, direito));
        case TOKEN_MENOR:
            return valor_bool(valor_para_numero(esquerdo) < valor_para_numero(direito));
        case TOKEN_MENOR_IGUAL:
            return valor_bool(valor_para_numero(esquerdo) <= valor_para_numero(direito));
        case TOKEN_MAIOR:
            return valor_bool(valor_para_numero(esquerdo) > valor_para_numero(direito));
        case TOKEN_MAIOR_IGUAL:
            return valor_bool(valor_para_numero(esquerdo) >= valor_para_numero(direito));
        default:
            noema_falhar("operador binario nao suportado");
    }
    return valor_nulo();
}

static Valor avaliar(Interpretador *interpretador, Ambiente *ambiente, No *no) {
    switch (no->tipo) {
        case NO_LITERAL:
            if (no->como.literal.valor.tipo == VALOR_TEXTO) {
                char *texto = interpolar_texto(interpretador, ambiente, no->como.literal.valor.como.texto);
                return valor_texto(texto);
            }
            return no->como.literal.valor;
        case NO_VARIAVEL: {
            Valor valor;
            if (!ambiente_obter(ambiente, no->como.variavel.nome, &valor)) {
                noema_falhar("variavel indefinida: %s", no->como.variavel.nome);
            }
            return valor;
        }
        case NO_LISTA_LITERAL: {
            Lista *lista = lista_criar();
            for (int i = 0; i < no->como.lista.quantidade; i++) {
                lista_adicionar(lista, avaliar(interpretador, ambiente, no->como.lista.itens[i]));
            }
            return valor_lista(lista);
        }
        case NO_MAPA_LITERAL: {
            Mapa *mapa = mapa_criar();
            for (int i = 0; i < no->como.mapa.quantidade; i++) {
                mapa_definir(mapa, no->como.mapa.chaves[i], avaliar(interpretador, ambiente, no->como.mapa.valores[i]));
            }
            return valor_mapa(mapa);
        }
        case NO_BINARIO:
            return operar_binario(interpretador, ambiente, no);
        case NO_UNARIO: {
            Valor valor = avaliar(interpretador, ambiente, no->como.unario.direita);
            if (no->como.unario.operador == TOKEN_MENOS) {
                return valor_numero(-valor_para_numero(valor));
            }
            if (no->como.unario.operador == TOKEN_NOT) {
                return valor_bool(!valor_eh_verdadeiro(valor));
            }
            noema_falhar("operador unario nao suportado");
            return valor_nulo();
        }
        case NO_CHAMADA: {
            Valor alvo = avaliar(interpretador, ambiente, no->como.chamada.alvo);
            int argc = no->como.chamada.quantidade_argumentos;
            Valor *argv = calloc((size_t)argc, sizeof(Valor));
            if (argv == NULL && argc > 0) {
                noema_falhar("memoria insuficiente");
            }
            for (int i = 0; i < argc; i++) {
                argv[i] = avaliar(interpretador, ambiente, no->como.chamada.argumentos[i]);
            }
            return chamar_valor(interpretador, ambiente, alvo, argc, argv);
        }
        case NO_MEMBRO:
            return obter_membro(avaliar(interpretador, ambiente, no->como.membro.objeto), no->como.membro.nome);
        case NO_INDICE:
            return obter_indice(avaliar(interpretador, ambiente, no->como.indice.objeto),
                                avaliar(interpretador, ambiente, no->como.indice.indice));
        case NO_ATRIBUICAO:
            return avaliar_atribuicao(interpretador, ambiente, no);
        case NO_FUNCAO_ANONIMA:
            return valor_funcao(criar_funcao_noema(no, ambiente, "<anonymous>"));
        case NO_SYNTAX_TEMPLATE:
            return valor_sintaxe(avaliar_template_sintaxe_no(interpretador, ambiente, no->como.template_sintaxe.corpo));
        case NO_EXPAND:
            noema_falhar("'expand' so pode ser usado durante a expansao de macros");
            return valor_nulo();
        case NO_SYNTAX_PLACEHOLDER:
            noema_falhar("placeholder de sintaxe fora de syntax");
            return valor_nulo();
        case NO_DECL_LET:
        case NO_DECL_CONST:
        case NO_DECL_FUNCAO:
        case NO_DECL_MACRO:
        case NO_EXPR_STMT:
        case NO_IF:
        case NO_WHILE:
        case NO_FOR:
        case NO_RETURN:
        case NO_BREAK:
        case NO_CONTINUE:
        case NO_DECL_PLUGIN:
        case NO_BLOCO:
            noema_falhar("no do tipo %d nao pode ser avaliado como expressao", (int)no->tipo);
    }
    return valor_nulo();
}

static Valor avaliar_atribuicao(Interpretador *interpretador, Ambiente *ambiente, No *no) {
    Valor valor = avaliar(interpretador, ambiente, no->como.atribuicao.valor);
    No *alvo = no->como.atribuicao.alvo;

    if (alvo->tipo == NO_VARIAVEL) {
        if (!ambiente_atribuir(ambiente, alvo->como.variavel.nome, valor)) {
            noema_falhar("variavel indefinida em atribuicao: %s", alvo->como.variavel.nome);
        }
        return valor;
    }

    if (alvo->tipo == NO_MEMBRO) {
        Valor objeto = avaliar(interpretador, ambiente, alvo->como.membro.objeto);
        atribuir_em_membro(objeto, alvo->como.membro.nome, valor);
        return valor;
    }

    if (alvo->tipo == NO_INDICE) {
        Valor objeto = avaliar(interpretador, ambiente, alvo->como.indice.objeto);
        Valor indice = avaliar(interpretador, ambiente, alvo->como.indice.indice);
        atribuir_em_indice(objeto, indice, valor);
        return valor;
    }

    noema_falhar("alvo invalido em atribuicao");
    return valor_nulo();
}

static ResultadoExecucao executar_bloco(Interpretador *interpretador, Ambiente *ambiente, No *no_bloco, bool criar_escopo) {
    Ambiente *escopo = ambiente;
    if (criar_escopo) {
        escopo = ambiente_criar(ambiente);
    }

    Valor ultimo = valor_nulo();
    for (int i = 0; i < no_bloco->como.bloco.quantidade; i++) {
        ResultadoExecucao resultado = executar(interpretador, escopo, no_bloco->como.bloco.itens[i]);
        if (resultado.sinal != EXEC_NORMAL) {
            return resultado;
        }
        ultimo = resultado.valor;
    }
    return resultado_normal(ultimo);
}

static ResultadoExecucao executar(Interpretador *interpretador, Ambiente *ambiente, No *no) {
    switch (no->tipo) {
        case NO_BLOCO:
            return executar_bloco(interpretador, ambiente, no, true);
        case NO_DECL_LET:
        case NO_DECL_CONST: {
            Valor valor = no->como.declaracao_variavel.inicializador != NULL
                              ? avaliar(interpretador, ambiente, no->como.declaracao_variavel.inicializador)
                              : valor_nulo();
            ambiente_definir(ambiente, no->como.declaracao_variavel.nome, valor, no->tipo == NO_DECL_CONST);
            return resultado_normal(valor);
        }
        case NO_DECL_FUNCAO: {
            FuncaoNoema *funcao = criar_funcao_noema(no, ambiente, "<function>");
            ambiente_definir(ambiente, funcao->nome, valor_funcao(funcao), true);
            return resultado_normal(valor_funcao(funcao));
        }
        case NO_DECL_MACRO:
            return resultado_normal(valor_nulo());
        case NO_EXPR_STMT:
            return resultado_normal(avaliar(interpretador, ambiente, no->como.expressao.expressao));
        case NO_IF: {
            Valor condicao = avaliar(interpretador, ambiente, no->como.condicional.condicao);
            if (valor_eh_verdadeiro(condicao)) {
                return executar(interpretador, ambiente, no->como.condicional.ramo_entao);
            }
            if (no->como.condicional.ramo_senao != NULL) {
                return executar(interpretador, ambiente, no->como.condicional.ramo_senao);
            }
            return resultado_normal(valor_nulo());
        }
        case NO_WHILE: {
            while (valor_eh_verdadeiro(avaliar(interpretador, ambiente, no->como.enquanto.condicao))) {
                ResultadoExecucao resultado = executar(interpretador, ambiente, no->como.enquanto.corpo);
                if (resultado.sinal == EXEC_BREAK) {
                    return resultado_normal(valor_nulo());
                }
                if (resultado.sinal == EXEC_CONTINUE) {
                    continue;
                }
                if (resultado.sinal != EXEC_NORMAL) {
                    return resultado;
                }
            }
            return resultado_normal(valor_nulo());
        }
        case NO_FOR: {
            Valor iteravel = avaliar(interpretador, ambiente, no->como.para.iteravel);
            if (iteravel.tipo == VALOR_LISTA) {
                for (int i = 0; i < iteravel.como.lista->quantidade; i++) {
                    Ambiente *local = ambiente_criar(ambiente);
                    ambiente_definir(local, no->como.para.variavel, iteravel.como.lista->itens[i], false);
                    ResultadoExecucao resultado = executar(interpretador, local, no->como.para.corpo);
                    if (resultado.sinal == EXEC_BREAK) {
                        return resultado_normal(valor_nulo());
                    }
                    if (resultado.sinal == EXEC_CONTINUE) {
                        continue;
                    }
                    if (resultado.sinal != EXEC_NORMAL) {
                        return resultado;
                    }
                }
                return resultado_normal(valor_nulo());
            }

            if (iteravel.tipo == VALOR_MAPA) {
                for (int i = 0; i < iteravel.como.mapa->quantidade; i++) {
                    Ambiente *local = ambiente_criar(ambiente);
                    ambiente_definir(local, no->como.para.variavel, valor_texto(iteravel.como.mapa->entradas[i].chave), false);
                    ResultadoExecucao resultado = executar(interpretador, local, no->como.para.corpo);
                    if (resultado.sinal == EXEC_BREAK) {
                        return resultado_normal(valor_nulo());
                    }
                    if (resultado.sinal == EXEC_CONTINUE) {
                        continue;
                    }
                    if (resultado.sinal != EXEC_NORMAL) {
                        return resultado;
                    }
                }
                return resultado_normal(valor_nulo());
            }

            noema_falhar("for exige list ou map");
            return resultado_normal(valor_nulo());
        }
        case NO_RETURN: {
            Valor valor = no->como.retorno.valor != NULL ? avaliar(interpretador, ambiente, no->como.retorno.valor) : valor_nulo();
            return resultado_com_sinal(EXEC_RETURN, valor);
        }
        case NO_BREAK:
            return resultado_com_sinal(EXEC_BREAK, valor_nulo());
        case NO_CONTINUE:
            return resultado_com_sinal(EXEC_CONTINUE, valor_nulo());
        case NO_DECL_PLUGIN:
            plugin_registrar_no_ambiente(interpretador, ambiente, no);
            return resultado_normal(valor_nulo());
        case NO_EXPAND:
            noema_falhar("'expand' so pode ser usado durante a expansao de macros");
            return resultado_normal(valor_nulo());
        case NO_SYNTAX_TEMPLATE:
            return resultado_normal(avaliar(interpretador, ambiente, no));
        case NO_SYNTAX_PLACEHOLDER:
            noema_falhar("placeholder de sintaxe fora de syntax");
            return resultado_normal(valor_nulo());
        case NO_FUNCAO_ANONIMA:
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
            return resultado_normal(avaliar(interpretador, ambiente, no));
    }

    return resultado_normal(valor_nulo());
}

static Valor chamar_funcao_noema(Interpretador *interpretador, Ambiente *ambiente, FuncaoNoema *funcao, int argc, Valor *argv) {
    (void)ambiente;
    Ambiente *local = ambiente_criar(funcao->fecho);
    for (int i = 0; i < funcao->quantidade_parametros; i++) {
        Valor argumento = i < argc ? argv[i] : valor_nulo();
        ambiente_definir(local, funcao->parametros[i], argumento, false);
    }
    ResultadoExecucao resultado = executar(interpretador, local, funcao->corpo);
    if (resultado.sinal == EXEC_RETURN) {
        return resultado.valor;
    }
    if (resultado.sinal == EXEC_BREAK || resultado.sinal == EXEC_CONTINUE) {
        noema_falhar("break/continue fora de loop");
    }
    return resultado.valor;
}

Valor chamar_valor(Interpretador *interpretador, Ambiente *ambiente, Valor alvo, int argc, Valor *argv) {
    if (alvo.tipo == VALOR_FUNCAO) {
        return chamar_funcao_noema(interpretador, ambiente, alvo.como.funcao, argc, argv);
    }
    if (alvo.tipo == VALOR_NATIVA) {
        return alvo.como.nativa->funcao(interpretador, ambiente, argc, argv, alvo.como.nativa->dados);
    }
    noema_falhar("o valor do tipo %s nao e chamavel", valor_tipo_nome(alvo));
    return valor_nulo();
}

Valor interpretador_executar_programa(Interpretador *interpretador, Programa *programa, Ambiente *ambiente) {
    Valor ultimo = valor_nulo();
    AmbienteMacro macros;
    ambiente_macro_inicializar(&macros, NULL);
    for (int i = 0; i < programa->quantidade; i++) {
        No *expandido = expandir_no(interpretador, &macros, programa->nos[i], true);
        if (expandido == NULL) {
            continue;
        }
        ResultadoExecucao resultado = executar(interpretador, ambiente, expandido);
        if (resultado.sinal == EXEC_RETURN) {
            return resultado.valor;
        }
        if (resultado.sinal != EXEC_NORMAL) {
            noema_falhar("sinal de controle invalido no topo do programa");
        }
        ultimo = resultado.valor;
    }
    return ultimo;
}

Valor interpretador_executar_codigo(Interpretador *interpretador, Ambiente *ambiente, const char *codigo, const char *caminho_virtual) {
    char *diretorio = noema_diretorio_de(caminho_virtual);
    FonteProcessada fonte = preprocessar_texto(codigo, caminho_virtual, diretorio, &interpretador->macros, &interpretador->importados);
    Programa *programa = parser_analisar(fonte.codigo, fonte.caminho);
    return interpretador_executar_programa(interpretador, programa, ambiente);
}

bool interpretador_tentar_executar_codigo(Interpretador *interpretador,
                                          Ambiente *ambiente,
                                          const char *codigo,
                                          const char *caminho_virtual,
                                          Valor *saida,
                                          char **erro) {
    NoemaRecuperacao recuperacao = {0};
    noema_empilhar_recuperacao(&recuperacao);
    if (setjmp(recuperacao.salto) == 0) {
        Valor valor = interpretador_executar_codigo(interpretador, ambiente, codigo, caminho_virtual);
        noema_desempilhar_recuperacao(&recuperacao);
        if (saida != NULL) {
            *saida = valor;
        }
        if (erro != NULL) {
            *erro = NULL;
        }
        return true;
    }

    noema_desempilhar_recuperacao(&recuperacao);
    if (saida != NULL) {
        *saida = valor_nulo();
    }
    if (erro != NULL) {
        *erro = recuperacao.mensagem != NULL ? recuperacao.mensagem : noema_duplicar("erro desconhecido");
    } else {
        free(recuperacao.mensagem);
    }
    return false;
}
