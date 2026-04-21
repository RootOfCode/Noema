#include "noema.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *dados;
    size_t comprimento;
    size_t capacidade;
} MontadorTexto;

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

static void montador_anexar_linhas(MontadorTexto *montador, int quantidade) {
    for (int i = 0; i < quantidade; i++) {
        montador_anexar(montador, "\n");
    }
}

static bool token_eh_palavra(TipoToken tipo) {
    return tipo == TOKEN_IDENTIFICADOR ||
           tipo == TOKEN_NUMERO ||
           tipo == TOKEN_TEXTO ||
           tipo == TOKEN_TEXTO_CRU ||
           (tipo >= TOKEN_IMPORT && tipo <= TOKEN_NOT);
}

static bool precisa_espaco(TipoToken anterior, TipoToken atual) {
    if (anterior == TOKEN_EOF || atual == TOKEN_EOF) {
        return false;
    }
    if (token_eh_palavra(anterior) && token_eh_palavra(atual)) {
        return true;
    }
    if (token_eh_palavra(anterior) && atual == TOKEN_DOLAR) {
        return true;
    }
    if ((anterior == TOKEN_FECHA_PAREN || anterior == TOKEN_FECHA_COLCHETE) && token_eh_palavra(atual)) {
        return true;
    }
    if ((anterior == TOKEN_FECHA_PAREN || anterior == TOKEN_FECHA_COLCHETE) && atual == TOKEN_DOLAR) {
        return true;
    }
    if (token_eh_palavra(anterior) && (atual == TOKEN_ABRE_PAREN || atual == TOKEN_ABRE_COLCHETE)) {
        return false;
    }
    return false;
}

static bool importado_ja_existe(TabelaImportados *importados, const char *caminho) {
    for (int i = 0; i < importados->quantidade; i++) {
        if (strcmp(importados->itens[i], caminho) == 0) {
            return true;
        }
    }
    return false;
}

static void registrar_importado(TabelaImportados *importados, const char *caminho) {
    if (importado_ja_existe(importados, caminho)) {
        return;
    }

    if (importados->quantidade >= importados->capacidade) {
        importados->capacidade = importados->capacidade == 0 ? 8 : importados->capacidade * 2;
        importados->itens = realloc(importados->itens, (size_t)importados->capacidade * sizeof(char *));
        if (importados->itens == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }

    importados->itens[importados->quantidade++] = noema_duplicar(caminho);
}

static Token consumir(Lexico *lexico, TipoToken esperado, const char *mensagem) {
    Token token = lexico_proximo(lexico);
    if (token.tipo != esperado) {
        noema_falhar("%s", mensagem);
    }
    return token;
}

static void consumir_terminador_preprocessador(Lexico *lexico, Token ultimo, const char *mensagem) {
    Token proximo = lexico_espiar(lexico);
    if (proximo.tipo == TOKEN_PONTO_E_VIRGULA) {
        lexico_proximo(lexico);
        return;
    }
    if (proximo.tipo == TOKEN_EOF || proximo.tipo == TOKEN_FECHA_CHAVE || proximo.linha > ultimo.linha) {
        return;
    }
    noema_falhar("%s", mensagem);
}

FonteProcessada preprocessar_texto(const char *texto,
                                   const char *caminho_virtual,
                                   const char *diretorio_base,
                                   TabelaMacros *macros,
                                   TabelaImportados *importados) {
    Lexico *lexico = lexico_criar(texto, caminho_virtual);
    MontadorTexto saida;
    montador_inicializar(&saida);
    TipoToken anterior = TOKEN_EOF;
    int linha_anterior = 0;

    for (;;) {
        Token token = lexico_proximo(lexico);
        if (token.tipo == TOKEN_EOF) {
            break;
        }

        if (token.tipo == TOKEN_IMPORT) {
            Token caminho = consumir(lexico, TOKEN_TEXTO, "import exige um caminho em texto");
            consumir_terminador_preprocessador(lexico, caminho, "import exige terminador");
            char *resolvido = noema_resolver_importacao(diretorio_base, caminho.literal);
            if (!importado_ja_existe(importados, resolvido)) {
                FonteProcessada importado = preprocessar_arquivo(resolvido, macros, importados);
                montador_anexar(&saida, importado.codigo);
                montador_anexar(&saida, "\n");
                anterior = TOKEN_EOF;
                linha_anterior = 0;
            }
            continue;
        }

        if (linha_anterior != 0 && token.linha > linha_anterior) {
            montador_anexar_linhas(&saida, token.linha - linha_anterior);
        } else if (precisa_espaco(anterior, token.tipo)) {
            montador_anexar(&saida, " ");
        }
        montador_anexar(&saida, token.lexema);
        anterior = token.tipo;
        linha_anterior = token.linha;
    }

    FonteProcessada fonte;
    fonte.codigo = montador_finalizar(&saida);
    fonte.caminho = noema_duplicar(caminho_virtual);
    return fonte;
}

FonteProcessada preprocessar_arquivo(const char *caminho, TabelaMacros *macros, TabelaImportados *importados) {
    if (importado_ja_existe(importados, caminho)) {
        FonteProcessada vazio;
        vazio.codigo = noema_duplicar("");
        vazio.caminho = noema_duplicar(caminho);
        return vazio;
    }
    registrar_importado(importados, caminho);
    char *conteudo = noema_ler_arquivo(caminho);
    char *diretorio = noema_diretorio_de(caminho);
    return preprocessar_texto(conteudo, caminho, diretorio, macros, importados);
}
