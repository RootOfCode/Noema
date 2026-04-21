#include "noema.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Lexico {
    const char *fonte;
    const char *caminho;
    size_t tamanho;
    size_t inicio;
    size_t atual;
    int linha;
    int coluna;
    bool tem_espiado;
    Token token_espiado;
};

static bool eh_fim(Lexico *lexico) {
    return lexico->atual >= lexico->tamanho;
}

static char atual(Lexico *lexico) {
    if (eh_fim(lexico)) {
        return '\0';
    }
    return lexico->fonte[lexico->atual];
}

static char avancar(Lexico *lexico) {
    char caractere = atual(lexico);
    if (caractere == '\n') {
        lexico->linha++;
        lexico->coluna = 1;
    } else {
        lexico->coluna++;
    }
    lexico->atual++;
    return caractere;
}

static char espiar_proximo(Lexico *lexico) {
    if (lexico->atual + 1 >= lexico->tamanho) {
        return '\0';
    }
    return lexico->fonte[lexico->atual + 1];
}

static bool consumir_se(Lexico *lexico, char esperado) {
    if (atual(lexico) != esperado) {
        return false;
    }
    avancar(lexico);
    return true;
}

static char *fatia(Lexico *lexico, size_t inicio, size_t fim) {
    size_t tamanho = fim - inicio;
    char *texto = malloc(tamanho + 1);
    if (texto == NULL) {
        noema_falhar("memoria insuficiente");
    }
    memcpy(texto, lexico->fonte + inicio, tamanho);
    texto[tamanho] = '\0';
    return texto;
}

static Token token_simples(Lexico *lexico, TipoToken tipo) {
    Token token;
    token.tipo = tipo;
    token.lexema = fatia(lexico, lexico->inicio, lexico->atual);
    token.literal = NULL;
    token.linha = lexico->linha;
    token.coluna = lexico->coluna;
    return token;
}

static TipoToken palavra_chave(const char *texto) {
    if (strcmp(texto, "import") == 0) return TOKEN_IMPORT;
    if (strcmp(texto, "use") == 0) return TOKEN_IMPORT;
    if (strcmp(texto, "macro") == 0) return TOKEN_MACRO;
    if (strcmp(texto, "expand") == 0) return TOKEN_EXPAND;
    if (strcmp(texto, "plugin") == 0) return TOKEN_PLUGIN;
    if (strcmp(texto, "library") == 0) return TOKEN_LIBRARY;
    if (strcmp(texto, "bind") == 0) return TOKEN_BIND;
    if (strcmp(texto, "as") == 0) return TOKEN_AS;
    if (strcmp(texto, "function") == 0) return TOKEN_FUNCTION;
    if (strcmp(texto, "fn") == 0) return TOKEN_FUNCTION;
    if (strcmp(texto, "let") == 0) return TOKEN_LET;
    if (strcmp(texto, "var") == 0) return TOKEN_LET;
    if (strcmp(texto, "const") == 0) return TOKEN_CONST;
    if (strcmp(texto, "if") == 0) return TOKEN_IF;
    if (strcmp(texto, "when") == 0) return TOKEN_IF;
    if (strcmp(texto, "else") == 0) return TOKEN_ELSE;
    if (strcmp(texto, "elif") == 0) return TOKEN_ELIF;
    if (strcmp(texto, "while") == 0) return TOKEN_WHILE;
    if (strcmp(texto, "for") == 0) return TOKEN_FOR;
    if (strcmp(texto, "each") == 0) return TOKEN_FOR;
    if (strcmp(texto, "in") == 0) return TOKEN_IN;
    if (strcmp(texto, "return") == 0) return TOKEN_RETURN;
    if (strcmp(texto, "break") == 0) return TOKEN_BREAK;
    if (strcmp(texto, "continue") == 0) return TOKEN_CONTINUE;
    if (strcmp(texto, "unless") == 0) return TOKEN_UNLESS;
    if (strcmp(texto, "true") == 0) return TOKEN_TRUE;
    if (strcmp(texto, "false") == 0) return TOKEN_FALSE;
    if (strcmp(texto, "null") == 0) return TOKEN_NULL;
    if (strcmp(texto, "nil") == 0) return TOKEN_NULL;
    if (strcmp(texto, "and") == 0) return TOKEN_AND;
    if (strcmp(texto, "or") == 0) return TOKEN_OR;
    if (strcmp(texto, "not") == 0) return TOKEN_NOT;
    return TOKEN_IDENTIFICADOR;
}

static void ignorar_espacos_e_comentarios(Lexico *lexico) {
    for (;;) {
        char c = atual(lexico);
        if (isspace((unsigned char)c)) {
            avancar(lexico);
            continue;
        }

        if (c == '/' && espiar_proximo(lexico) == '/') {
            while (!eh_fim(lexico) && atual(lexico) != '\n') {
                avancar(lexico);
            }
            continue;
        }

        if (c == '/' && espiar_proximo(lexico) == '*') {
            avancar(lexico);
            avancar(lexico);
            while (!eh_fim(lexico)) {
                if (atual(lexico) == '*' && espiar_proximo(lexico) == '/') {
                    avancar(lexico);
                    avancar(lexico);
                    break;
                }
                avancar(lexico);
            }
            continue;
        }

        if (c == '#') {
            while (!eh_fim(lexico) && atual(lexico) != '\n') {
                avancar(lexico);
            }
            continue;
        }

        break;
    }
}

static Token ler_numero(Lexico *lexico) {
    while (isdigit((unsigned char)atual(lexico))) {
        avancar(lexico);
    }

    if (atual(lexico) == '.' && isdigit((unsigned char)espiar_proximo(lexico))) {
        avancar(lexico);
        while (isdigit((unsigned char)atual(lexico))) {
            avancar(lexico);
        }
    }

    Token token = token_simples(lexico, TOKEN_NUMERO);
    token.literal = noema_duplicar(token.lexema);
    return token;
}

static Token ler_identificador(Lexico *lexico) {
    while (isalnum((unsigned char)atual(lexico)) || atual(lexico) == '_') {
        avancar(lexico);
    }
    Token token = token_simples(lexico, TOKEN_IDENTIFICADOR);
    token.tipo = palavra_chave(token.lexema);
    token.literal = noema_duplicar(token.lexema);
    return token;
}

static Token ler_texto(Lexico *lexico, char delimitador) {
    size_t inicio_literal = lexico->atual;
    while (!eh_fim(lexico) && atual(lexico) != delimitador) {
        if (atual(lexico) == '\\' && !eh_fim(lexico)) {
            avancar(lexico);
        }
        avancar(lexico);
    }
    if (eh_fim(lexico)) {
        noema_falhar("texto nao finalizado em %s", lexico->caminho);
    }
    size_t fim_literal = lexico->atual;
    avancar(lexico);

    char *cru = fatia(lexico, inicio_literal, fim_literal);
    char *decodificado = malloc(strlen(cru) + 1);
    if (decodificado == NULL) {
        noema_falhar("memoria insuficiente");
    }

    size_t j = 0;
    for (size_t i = 0; cru[i] != '\0'; i++) {
        if (cru[i] == '\\') {
            i++;
            switch (cru[i]) {
                case 'n':
                    decodificado[j++] = '\n';
                    break;
                case 't':
                    decodificado[j++] = '\t';
                    break;
                case 'r':
                    decodificado[j++] = '\r';
                    break;
                case '\'':
                    decodificado[j++] = '\'';
                    break;
                case '"':
                    decodificado[j++] = '"';
                    break;
                case '\\':
                    decodificado[j++] = '\\';
                    break;
                case '\0':
                    decodificado[j++] = '\\';
                    i--;
                    break;
                default:
                    decodificado[j++] = cru[i];
                    break;
            }
        } else {
            decodificado[j++] = cru[i];
        }
    }
    decodificado[j] = '\0';

    Token token = token_simples(lexico, TOKEN_TEXTO);
    token.literal = decodificado;
    return token;
}

static Token ler_texto_cru(Lexico *lexico) {
    size_t inicio_literal = lexico->atual;
    while (!eh_fim(lexico) && atual(lexico) != '`') {
        avancar(lexico);
    }
    if (eh_fim(lexico)) {
        noema_falhar("texto cru nao finalizado em %s", lexico->caminho);
    }
    size_t fim_literal = lexico->atual;
    avancar(lexico);

    Token token = token_simples(lexico, TOKEN_TEXTO_CRU);
    token.literal = fatia(lexico, inicio_literal, fim_literal);
    return token;
}

static Token lexico_ler(Lexico *lexico) {
    ignorar_espacos_e_comentarios(lexico);
    lexico->inicio = lexico->atual;

    if (eh_fim(lexico)) {
        Token token;
        token.tipo = TOKEN_EOF;
        token.lexema = noema_duplicar("");
        token.literal = NULL;
        token.linha = lexico->linha;
        token.coluna = lexico->coluna;
        return token;
    }

    char c = avancar(lexico);

    if (isdigit((unsigned char)c)) {
        return ler_numero(lexico);
    }

    if (isalpha((unsigned char)c) || c == '_') {
        return ler_identificador(lexico);
    }

    switch (c) {
        case '(':
            return token_simples(lexico, TOKEN_ABRE_PAREN);
        case ')':
            return token_simples(lexico, TOKEN_FECHA_PAREN);
        case '{':
            return token_simples(lexico, TOKEN_ABRE_CHAVE);
        case '}':
            return token_simples(lexico, TOKEN_FECHA_CHAVE);
        case '[':
            return token_simples(lexico, TOKEN_ABRE_COLCHETE);
        case ']':
            return token_simples(lexico, TOKEN_FECHA_COLCHETE);
        case ',':
            return token_simples(lexico, TOKEN_VIRGULA);
        case '.':
            return token_simples(lexico, TOKEN_PONTO);
        case ';':
            return token_simples(lexico, TOKEN_PONTO_E_VIRGULA);
        case ':':
            return token_simples(lexico, TOKEN_DOIS_PONTOS);
        case '$':
            return token_simples(lexico, TOKEN_DOLAR);
        case '+':
            return token_simples(lexico, TOKEN_MAIS);
        case '-':
            if (consumir_se(lexico, '>')) {
                return token_simples(lexico, TOKEN_SETA);
            }
            return token_simples(lexico, TOKEN_MENOS);
        case '*':
            return token_simples(lexico, TOKEN_ASTERISCO);
        case '/':
            return token_simples(lexico, TOKEN_BARRA);
        case '%':
            return token_simples(lexico, TOKEN_PERCENTUAL);
        case '=':
            if (consumir_se(lexico, '=')) {
                return token_simples(lexico, TOKEN_IGUAL);
            }
            if (consumir_se(lexico, '>')) {
                return token_simples(lexico, TOKEN_SETA_GROSSA);
            }
            return token_simples(lexico, TOKEN_ATRIBUICAO);
        case '!':
            if (consumir_se(lexico, '=')) {
                return token_simples(lexico, TOKEN_DIFERENTE);
            }
            break;
        case '<':
            if (consumir_se(lexico, '=')) {
                return token_simples(lexico, TOKEN_MENOR_IGUAL);
            }
            return token_simples(lexico, TOKEN_MENOR);
        case '>':
            if (consumir_se(lexico, '=')) {
                return token_simples(lexico, TOKEN_MAIOR_IGUAL);
            }
            return token_simples(lexico, TOKEN_MAIOR);
        case '"':
            return ler_texto(lexico, '"');
        case '\'':
            return ler_texto(lexico, '\'');
        case '`':
            return ler_texto_cru(lexico);
    }

    noema_falhar("caractere inesperado '%c' em %s", c, lexico->caminho);
    return token_simples(lexico, TOKEN_EOF);
}

Lexico *lexico_criar(const char *fonte, const char *caminho) {
    Lexico *lexico = calloc(1, sizeof(Lexico));
    if (lexico == NULL) {
        noema_falhar("memoria insuficiente");
    }
    lexico->fonte = fonte;
    lexico->caminho = caminho;
    lexico->tamanho = strlen(fonte);
    lexico->linha = 1;
    lexico->coluna = 1;
    return lexico;
}

Token lexico_proximo(Lexico *lexico) {
    if (lexico->tem_espiado) {
        lexico->tem_espiado = false;
        return lexico->token_espiado;
    }
    return lexico_ler(lexico);
}

Token lexico_espiar(Lexico *lexico) {
    if (!lexico->tem_espiado) {
        lexico->token_espiado = lexico_ler(lexico);
        lexico->tem_espiado = true;
    }
    return lexico->token_espiado;
}

void lexico_destruir(Lexico *lexico) {
    free(lexico);
}
