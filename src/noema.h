#ifndef NOEMA_H
#define NOEMA_H

#include <ffi.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Token Token;
typedef struct Lexico Lexico;
typedef struct No No;
typedef struct Programa Programa;
typedef struct Ambiente Ambiente;
typedef struct Interpretador Interpretador;
typedef struct Valor Valor;
typedef struct Lista Lista;
typedef struct Mapa Mapa;
typedef struct FuncaoNoema FuncaoNoema;
typedef struct FuncaoNativa FuncaoNativa;
typedef struct LigacaoPlugin LigacaoPlugin;
typedef struct LigacaoPluginAst LigacaoPluginAst;
typedef struct MacroPre MacroPre;
typedef struct TabelaMacros TabelaMacros;
typedef struct TabelaImportados TabelaImportados;
typedef struct ResultadoExecucao ResultadoExecucao;

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_IDENTIFICADOR,
    TOKEN_NUMERO,
    TOKEN_TEXTO,
    TOKEN_TEXTO_CRU,

    TOKEN_ABRE_PAREN,
    TOKEN_FECHA_PAREN,
    TOKEN_ABRE_CHAVE,
    TOKEN_FECHA_CHAVE,
    TOKEN_ABRE_COLCHETE,
    TOKEN_FECHA_COLCHETE,
    TOKEN_VIRGULA,
    TOKEN_PONTO,
    TOKEN_PONTO_E_VIRGULA,
    TOKEN_DOIS_PONTOS,
    TOKEN_DOLAR,

    TOKEN_MAIS,
    TOKEN_MENOS,
    TOKEN_ASTERISCO,
    TOKEN_BARRA,
    TOKEN_PERCENTUAL,

    TOKEN_ATRIBUICAO,
    TOKEN_IGUAL,
    TOKEN_DIFERENTE,
    TOKEN_MENOR,
    TOKEN_MENOR_IGUAL,
    TOKEN_MAIOR,
    TOKEN_MAIOR_IGUAL,
    TOKEN_SETA,
    TOKEN_SETA_GROSSA,

    TOKEN_IMPORT,
    TOKEN_MACRO,
    TOKEN_EXPAND,
    TOKEN_SYNTAX,
    TOKEN_PLUGIN,
    TOKEN_LIBRARY,
    TOKEN_BIND,
    TOKEN_AS,
    TOKEN_FUNCTION,
    TOKEN_LET,
    TOKEN_CONST,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_UNLESS,
    TOKEN_ELIF,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT
} TipoToken;

struct Token {
    TipoToken tipo;
    char *lexema;
    char *literal;
    int linha;
    int coluna;
};

typedef enum {
    VALOR_NULO = 0,
    VALOR_BOOL,
    VALOR_NUMERO,
    VALOR_TEXTO,
    VALOR_LISTA,
    VALOR_MAPA,
    VALOR_PONTEIRO,
    VALOR_SINTAXE,
    VALOR_FUNCAO,
    VALOR_NATIVA
} TipoValor;

struct Lista {
    Valor *itens;
    int quantidade;
    int capacidade;
};

typedef Valor (*FuncaoNativaBruta)(Interpretador *, Ambiente *, int, Valor *, void *);

struct FuncaoNativa {
    char *nome;
    FuncaoNativaBruta funcao;
    void *dados;
};

struct FuncaoNoema {
    char *nome;
    char **parametros;
    int quantidade_parametros;
    No *corpo;
    Ambiente *fecho;
};

struct Valor {
    TipoValor tipo;
    union {
        bool booleano;
        double numero;
        char *texto;
        Lista *lista;
        Mapa *mapa;
        void *ponteiro;
        No *sintaxe;
        FuncaoNoema *funcao;
        FuncaoNativa *nativa;
    } como;
};

typedef struct {
    char *chave;
    Valor valor;
} EntradaMapa;

struct Mapa {
    EntradaMapa *entradas;
    int quantidade;
    int capacidade;
};

typedef enum {
    NO_BLOCO = 0,
    NO_LITERAL,
    NO_VARIAVEL,
    NO_LISTA_LITERAL,
    NO_MAPA_LITERAL,
    NO_BINARIO,
    NO_UNARIO,
    NO_CHAMADA,
    NO_MEMBRO,
    NO_INDICE,
    NO_ATRIBUICAO,
    NO_DECL_LET,
    NO_DECL_CONST,
    NO_DECL_FUNCAO,
    NO_DECL_MACRO,
    NO_FUNCAO_ANONIMA,
    NO_EXPR_STMT,
    NO_EXPAND,
    NO_SYNTAX_TEMPLATE,
    NO_SYNTAX_PLACEHOLDER,
    NO_IF,
    NO_WHILE,
    NO_FOR,
    NO_RETURN,
    NO_BREAK,
    NO_CONTINUE,
    NO_DECL_PLUGIN
} TipoNo;

typedef enum {
    FFI_NUMERO = 0,
    FFI_INT,
    FFI_TEXTO,
    FFI_BOOL,
    FFI_PONTEIRO,
    FFI_VAZIO
} TipoFfi;

struct LigacaoPluginAst {
    char *nome_noema;
    char *nome_simbolo;
    TipoFfi retorno;
    TipoFfi *argumentos;
    int quantidade_argumentos;
};

struct No {
    TipoNo tipo;
    int linha;
    union {
        struct {
            No **itens;
            int quantidade;
            int capacidade;
        } bloco;
        struct {
            Valor valor;
        } literal;
        struct {
            char *nome;
        } variavel;
        struct {
            No **itens;
            int quantidade;
            int capacidade;
        } lista;
        struct {
            char **chaves;
            No **valores;
            int quantidade;
            int capacidade;
        } mapa;
        struct {
            No *esquerda;
            TipoToken operador;
            No *direita;
        } binario;
        struct {
            TipoToken operador;
            No *direita;
        } unario;
        struct {
            No *alvo;
            No **argumentos;
            int quantidade_argumentos;
            int capacidade_argumentos;
        } chamada;
        struct {
            No *objeto;
            char *nome;
        } membro;
        struct {
            No *objeto;
            No *indice;
        } indice;
        struct {
            No *alvo;
            No *valor;
        } atribuicao;
        struct {
            char *nome;
            No *inicializador;
        } declaracao_variavel;
        struct {
            char *nome;
            char **parametros;
            int quantidade_parametros;
            No *corpo;
        } declaracao_funcao;
        struct {
            char *nome;
            char **parametros;
            int quantidade_parametros;
            No *corpo;
        } declaracao_macro;
        struct {
            No *expressao;
        } expressao;
        struct {
            char *nome;
            No **argumentos;
            int quantidade_argumentos;
            int capacidade_argumentos;
        } expansao_macro;
        struct {
            No *corpo;
        } template_sintaxe;
        struct {
            char *nome;
            No *expressao;
        } placeholder_sintaxe;
        struct {
            No *condicao;
            No *ramo_entao;
            No *ramo_senao;
        } condicional;
        struct {
            No *condicao;
            No *corpo;
        } enquanto;
        struct {
            char *variavel;
            No *iteravel;
            No *corpo;
        } para;
        struct {
            No *valor;
        } retorno;
        struct {
            char *nome;
            char **bibliotecas;
            int quantidade_bibliotecas;
            LigacaoPluginAst *ligacoes;
            int quantidade_ligacoes;
        } plugin;
    } como;
};

struct Programa {
    No **nos;
    int quantidade;
    int capacidade;
    char *caminho;
};

typedef struct {
    char *nome;
    Valor valor;
    bool constante;
} EntradaAmbiente;

struct Ambiente {
    Ambiente *pai;
    EntradaAmbiente *entradas;
    int quantidade;
    int capacidade;
};

typedef enum {
    EXEC_NORMAL = 0,
    EXEC_RETURN,
    EXEC_BREAK,
    EXEC_CONTINUE
} SinalExecucao;

struct ResultadoExecucao {
    SinalExecucao sinal;
    Valor valor;
};

struct MacroPre {
    char *nome;
    char **parametros;
    int quantidade_parametros;
    char *modelo;
};

struct TabelaMacros {
    MacroPre *itens;
    int quantidade;
    int capacidade;
};

struct TabelaImportados {
    char **itens;
    int quantidade;
    int capacidade;
};

typedef struct {
    char *codigo;
    char *caminho;
} FonteProcessada;

struct LigacaoPlugin {
    char *nome_noema;
    char *nome_simbolo;
    TipoFfi retorno;
    TipoFfi *argumentos;
    int quantidade_argumentos;
    void *alca_biblioteca;
    void *simbolo;
    ffi_cif cif;
    ffi_type **tipos_argumentos_ffi;
    ffi_type *tipo_retorno_ffi;
};

struct Interpretador {
    Ambiente *global;
    TabelaMacros macros;
    TabelaImportados importados;
    char *caminho_atual;
    unsigned int semente_aleatoria;
};

void noema_falhar(const char *formato, ...) __attribute__((noreturn, format(printf, 1, 2)));
char *noema_duplicar(const char *texto);
char *noema_formatar(const char *formato, ...);
char *noema_ler_arquivo(const char *caminho);
char *noema_diretorio_de(const char *caminho);
char *noema_juntar_caminhos(const char *base, const char *relativo);
bool noema_caminho_absoluto(const char *caminho);
bool noema_arquivo_existe(const char *caminho);
bool noema_tem_sufixo(const char *texto, const char *sufixo);
char *noema_normalizar_caminho_modulo(const char *caminho);
void noema_configurar_diretorio_stdlib(const char *caminho);
const char *noema_obter_diretorio_stdlib(void);
char *noema_resolver_importacao(const char *base, const char *relativo);
char *noema_descobrir_diretorio_stdlib(const char *argv0);

Lexico *lexico_criar(const char *fonte, const char *caminho);
Token lexico_proximo(Lexico *lexico);
Token lexico_espiar(Lexico *lexico);
void lexico_destruir(Lexico *lexico);

FonteProcessada preprocessar_arquivo(const char *caminho, TabelaMacros *macros, TabelaImportados *importados);
FonteProcessada preprocessar_texto(const char *texto, const char *caminho_virtual, const char *diretorio_base, TabelaMacros *macros, TabelaImportados *importados);

Programa *parser_analisar(const char *codigo, const char *caminho);

Interpretador *interpretador_criar(void);
void interpretador_destruir(Interpretador *interpretador);
Valor interpretador_executar_programa(Interpretador *interpretador, Programa *programa, Ambiente *ambiente);
Valor interpretador_executar_codigo(Interpretador *interpretador, Ambiente *ambiente, const char *codigo, const char *caminho_virtual);
bool interpretador_tentar_executar_codigo(Interpretador *interpretador,
                                          Ambiente *ambiente,
                                          const char *codigo,
                                          const char *caminho_virtual,
                                          Valor *saida,
                                          char **erro);
Valor chamar_valor(Interpretador *interpretador, Ambiente *ambiente, Valor alvo, int argc, Valor *argv);

Ambiente *ambiente_criar(Ambiente *pai);
void ambiente_definir(Ambiente *ambiente, const char *nome, Valor valor, bool constante);
bool ambiente_atribuir(Ambiente *ambiente, const char *nome, Valor valor);
bool ambiente_obter(Ambiente *ambiente, const char *nome, Valor *saida);

Valor valor_nulo(void);
Valor valor_bool(bool valor);
Valor valor_numero(double valor);
Valor valor_texto(const char *valor);
Valor valor_lista(Lista *lista);
Valor valor_mapa(Mapa *mapa);
Valor valor_ponteiro(void *ponteiro);
Valor valor_sintaxe(No *sintaxe);
Valor valor_funcao(FuncaoNoema *funcao);
Valor valor_nativa(FuncaoNativa *nativa);

bool valor_eh_verdadeiro(Valor valor);
bool valor_iguais(Valor a, Valor b);
char *valor_para_texto(Valor valor);
double valor_para_numero(Valor valor);
const char *valor_tipo_nome(Valor valor);

Lista *lista_criar(void);
void lista_adicionar(Lista *lista, Valor valor);
Valor lista_obter(Lista *lista, int indice);
void lista_definir(Lista *lista, int indice, Valor valor);
Valor lista_remover_ultimo(Lista *lista);

Mapa *mapa_criar(void);
void mapa_definir(Mapa *mapa, const char *chave, Valor valor);
bool mapa_obter(Mapa *mapa, const char *chave, Valor *saida);
bool mapa_tem(Mapa *mapa, const char *chave);
char **mapa_chaves(Mapa *mapa, int *quantidade);

void registrar_builtins(Interpretador *interpretador);
Valor ffi_invocar(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados);
void plugin_registrar_no_ambiente(Interpretador *interpretador, Ambiente *ambiente, No *no_plugin);

#endif
