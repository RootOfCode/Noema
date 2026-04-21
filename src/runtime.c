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
        case NO_DECL_LET:
        case NO_DECL_CONST:
        case NO_DECL_FUNCAO:
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
    for (int i = 0; i < programa->quantidade; i++) {
        ResultadoExecucao resultado = executar(interpretador, ambiente, programa->nos[i]);
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
