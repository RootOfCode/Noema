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

static MacroPre *buscar_macro(TabelaMacros *macros, const char *nome) {
    for (int i = 0; i < macros->quantidade; i++) {
        if (strcmp(macros->itens[i].nome, nome) == 0) {
            return &macros->itens[i];
        }
    }
    return NULL;
}

static void registrar_macro(TabelaMacros *macros, const char *nome, char **parametros, int quantidade_parametros, const char *modelo) {
    MacroPre *existente = buscar_macro(macros, nome);
    if (existente != NULL) {
        existente->parametros = parametros;
        existente->quantidade_parametros = quantidade_parametros;
        existente->modelo = noema_duplicar(modelo);
        return;
    }

    if (macros->quantidade >= macros->capacidade) {
        macros->capacidade = macros->capacidade == 0 ? 8 : macros->capacidade * 2;
        macros->itens = realloc(macros->itens, (size_t)macros->capacidade * sizeof(MacroPre));
        if (macros->itens == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }

    MacroPre *macro = &macros->itens[macros->quantidade++];
    macro->nome = noema_duplicar(nome);
    macro->parametros = parametros;
    macro->quantidade_parametros = quantidade_parametros;
    macro->modelo = noema_duplicar(modelo);
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

static char *reconstruir_tokens(Token *tokens, int quantidade) {
    MontadorTexto montador;
    montador_inicializar(&montador);
    TipoToken anterior = TOKEN_EOF;
    int linha_anterior = 0;
    for (int i = 0; i < quantidade; i++) {
        if (linha_anterior != 0 && tokens[i].linha > linha_anterior) {
            montador_anexar_linhas(&montador, tokens[i].linha - linha_anterior);
        } else if (precisa_espaco(anterior, tokens[i].tipo)) {
            montador_anexar(&montador, " ");
        }
        montador_anexar(&montador, tokens[i].lexema);
        anterior = tokens[i].tipo;
        linha_anterior = tokens[i].linha;
    }
    return montador_finalizar(&montador);
}

static char *ler_corpo_macro_em_bloco(Lexico *lexico, Token *fechamento) {
    consumir(lexico, TOKEN_ABRE_CHAVE, "macro exige '{'");

    Token *buffer = NULL;
    int quantidade_buffer = 0;
    int capacidade_buffer = 0;
    int profundidade = 1;

    for (;;) {
        Token atual = lexico_proximo(lexico);
        if (atual.tipo == TOKEN_EOF) {
            noema_falhar("macro sem fechamento de bloco");
        }

        if (atual.tipo == TOKEN_ABRE_CHAVE) {
            profundidade++;
        } else if (atual.tipo == TOKEN_FECHA_CHAVE) {
            profundidade--;
            if (profundidade == 0) {
                *fechamento = atual;
                break;
            }
        }

        if (capacidade_buffer <= quantidade_buffer) {
            capacidade_buffer = capacidade_buffer == 0 ? 8 : capacidade_buffer * 2;
            buffer = realloc(buffer, (size_t)capacidade_buffer * sizeof(Token));
            if (buffer == NULL) {
                noema_falhar("memoria insuficiente");
            }
        }
        buffer[quantidade_buffer++] = atual;
    }

    return reconstruir_tokens(buffer, quantidade_buffer);
}

static char **ler_argumentos_expand(Lexico *lexico, int *quantidade_argumentos, Token *fechamento) {
    Token token = lexico_espiar(lexico);
    char **argumentos = NULL;
    int quantidade = 0;
    int capacidade = 0;

    if (token.tipo == TOKEN_FECHA_PAREN) {
        *fechamento = lexico_proximo(lexico);
        *quantidade_argumentos = 0;
        return NULL;
    }

    Token *buffer = NULL;
    int quantidade_buffer = 0;
    int capacidade_buffer = 0;
    int profundidade = 0;

    for (;;) {
        Token atual = lexico_proximo(lexico);
        if (atual.tipo == TOKEN_EOF) {
            noema_falhar("expand sem fechamento de parenteses");
        }

        if (atual.tipo == TOKEN_ABRE_PAREN || atual.tipo == TOKEN_ABRE_CHAVE || atual.tipo == TOKEN_ABRE_COLCHETE) {
            profundidade++;
        } else if (atual.tipo == TOKEN_FECHA_PAREN || atual.tipo == TOKEN_FECHA_CHAVE || atual.tipo == TOKEN_FECHA_COLCHETE) {
            if (atual.tipo == TOKEN_FECHA_PAREN && profundidade == 0) {
                *fechamento = atual;
                if (capacidade <= quantidade) {
                    capacidade = capacidade == 0 ? 4 : capacidade * 2;
                    argumentos = realloc(argumentos, (size_t)capacidade * sizeof(char *));
                    if (argumentos == NULL) {
                        noema_falhar("memoria insuficiente");
                    }
                }
                argumentos[quantidade++] = reconstruir_tokens(buffer, quantidade_buffer);
                break;
            }
            profundidade--;
        }

        if (atual.tipo == TOKEN_VIRGULA && profundidade == 0) {
            if (capacidade <= quantidade) {
                capacidade = capacidade == 0 ? 4 : capacidade * 2;
                argumentos = realloc(argumentos, (size_t)capacidade * sizeof(char *));
                if (argumentos == NULL) {
                    noema_falhar("memoria insuficiente");
                }
            }
            argumentos[quantidade++] = reconstruir_tokens(buffer, quantidade_buffer);
            quantidade_buffer = 0;
            continue;
        }

        if (capacidade_buffer <= quantidade_buffer) {
            capacidade_buffer = capacidade_buffer == 0 ? 8 : capacidade_buffer * 2;
            buffer = realloc(buffer, (size_t)capacidade_buffer * sizeof(Token));
            if (buffer == NULL) {
                noema_falhar("memoria insuficiente");
            }
        }
        buffer[quantidade_buffer++] = atual;
    }

    *quantidade_argumentos = quantidade;
    return argumentos;
}

static char *substituir_parametros(MacroPre *macro, char **argumentos, int quantidade_argumentos) {
    if (quantidade_argumentos != macro->quantidade_parametros) {
        noema_falhar("macro '%s' esperava %d argumentos e recebeu %d",
                     macro->nome,
                     macro->quantidade_parametros,
                     quantidade_argumentos);
    }

    MontadorTexto montador;
    montador_inicializar(&montador);

    const char *cursor = macro->modelo;
    while (*cursor != '\0') {
        if (*cursor == '$') {
            cursor++;
            char nome[256];
            size_t tamanho = 0;

            if (*cursor == '{') {
                cursor++;
                while (*cursor != '\0' && *cursor != '}' && tamanho + 1 < sizeof(nome)) {
                    nome[tamanho++] = *cursor++;
                }
                if (*cursor == '}') {
                    cursor++;
                }
            } else {
                while ((isalnum((unsigned char)*cursor) || *cursor == '_') && tamanho + 1 < sizeof(nome)) {
                    nome[tamanho++] = *cursor++;
                }
            }

            nome[tamanho] = '\0';

            bool encontrado = false;
            for (int i = 0; i < macro->quantidade_parametros; i++) {
                if (strcmp(macro->parametros[i], nome) == 0) {
                    montador_anexar(&montador, argumentos[i]);
                    encontrado = true;
                    break;
                }
            }

            if (!encontrado) {
                montador_anexar(&montador, "$");
                montador_anexar(&montador, nome);
            }

            continue;
        }

        char buffer[2];
        buffer[0] = *cursor++;
        buffer[1] = '\0';
        montador_anexar(&montador, buffer);
    }

    return montador_finalizar(&montador);
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

        if (token.tipo == TOKEN_MACRO) {
            Token nome = consumir(lexico, TOKEN_IDENTIFICADOR, "macro exige nome");
            consumir(lexico, TOKEN_ABRE_PAREN, "macro exige '('");

            char **parametros = NULL;
            int quantidade_parametros = 0;
            int capacidade_parametros = 0;

            Token proximo = lexico_espiar(lexico);
            if (proximo.tipo != TOKEN_FECHA_PAREN) {
                for (;;) {
                    Token parametro = consumir(lexico, TOKEN_IDENTIFICADOR, "parametro invalido em macro");
                    if (capacidade_parametros <= quantidade_parametros) {
                        capacidade_parametros = capacidade_parametros == 0 ? 4 : capacidade_parametros * 2;
                        parametros = realloc(parametros, (size_t)capacidade_parametros * sizeof(char *));
                        if (parametros == NULL) {
                            noema_falhar("memoria insuficiente");
                        }
                    }
                    parametros[quantidade_parametros++] = noema_duplicar(parametro.lexema);

                    proximo = lexico_espiar(lexico);
                    if (proximo.tipo == TOKEN_VIRGULA) {
                        lexico_proximo(lexico);
                        continue;
                    }
                    break;
                }
            }

            consumir(lexico, TOKEN_FECHA_PAREN, "macro exige ')'");

            Token ultimo_modelo = {0};
            char *modelo = NULL;
            Token inicio_modelo = lexico_espiar(lexico);
            if (inicio_modelo.tipo == TOKEN_SETA_GROSSA) {
                lexico_proximo(lexico);
                inicio_modelo = lexico_espiar(lexico);
            }

            if (inicio_modelo.tipo == TOKEN_TEXTO_CRU) {
                Token texto_cru = consumir(lexico, TOKEN_TEXTO_CRU, "macro exige texto cru entre crases ou corpo em bloco");
                ultimo_modelo = texto_cru;
                modelo = texto_cru.literal;
            } else if (inicio_modelo.tipo == TOKEN_ABRE_CHAVE) {
                modelo = ler_corpo_macro_em_bloco(lexico, &ultimo_modelo);
            } else {
                noema_falhar("macro exige corpo em bloco '{...}' ou texto cru entre crases");
            }

            consumir_terminador_preprocessador(lexico, ultimo_modelo, "macro exige terminador");
            registrar_macro(macros, nome.lexema, parametros, quantidade_parametros, modelo);
            continue;
        }

        if (token.tipo == TOKEN_EXPAND) {
            Token nome = consumir(lexico, TOKEN_IDENTIFICADOR, "expand exige nome de macro");
            consumir(lexico, TOKEN_ABRE_PAREN, "expand exige '('");
            int quantidade_argumentos = 0;
            Token fechamento = {0};
            char **argumentos = ler_argumentos_expand(lexico, &quantidade_argumentos, &fechamento);
            consumir_terminador_preprocessador(lexico, fechamento, "expand exige terminador");

            MacroPre *macro = buscar_macro(macros, nome.lexema);
            if (macro == NULL) {
                noema_falhar("macro '%s' nao foi definida", nome.lexema);
            }

            char *expandido = substituir_parametros(macro, argumentos, quantidade_argumentos);
            FonteProcessada trecho = preprocessar_texto(expandido, caminho_virtual, diretorio_base, macros, importados);
            montador_anexar(&saida, trecho.codigo);
            montador_anexar(&saida, "\n");
            anterior = TOKEN_EOF;
            linha_anterior = 0;
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
