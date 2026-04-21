#include "noema.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    Lexico *lexico;
    Token atual;
    Token anterior;
    bool tem_atual;
    const char *caminho;
} Parser;

static Token espiar(Parser *parser);
static void consumir_terminador_instrucao(Parser *parser, const char *mensagem);
static No *parsear_funcao_anonima(Parser *parser);
static No *parsear_if(Parser *parser, bool inverter_condicao);
static No *parsear_while(Parser *parser);
static No *parsear_for(Parser *parser);
static No *parsear_declaracao(Parser *parser);
static No *parsear_instrucao(Parser *parser);
static No *parsear_expressao(Parser *parser);
static No *parsear_bloco(Parser *parser);

static No *novo_no(TipoNo tipo, int linha) {
    No *no = calloc(1, sizeof(No));
    if (no == NULL) {
        noema_falhar("memoria insuficiente");
    }
    no->tipo = tipo;
    no->linha = linha;
    return no;
}

static Programa *novo_programa(const char *caminho) {
    Programa *programa = calloc(1, sizeof(Programa));
    if (programa == NULL) {
        noema_falhar("memoria insuficiente");
    }
    programa->capacidade = 16;
    programa->nos = calloc((size_t)programa->capacidade, sizeof(No *));
    if (programa->nos == NULL) {
        noema_falhar("memoria insuficiente");
    }
    programa->caminho = noema_duplicar(caminho);
    return programa;
}

static void programa_adicionar(Programa *programa, No *no) {
    if (programa->quantidade >= programa->capacidade) {
        programa->capacidade *= 2;
        programa->nos = realloc(programa->nos, (size_t)programa->capacidade * sizeof(No *));
        if (programa->nos == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    programa->nos[programa->quantidade++] = no;
}

static void bloco_adicionar(No *bloco, No *item) {
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

static void lista_literal_adicionar(No *lista, No *item) {
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

static void mapa_literal_adicionar(No *mapa, const char *chave, No *valor) {
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

static void chamada_adicionar_argumento(No *chamada, No *argumento) {
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

static Token avancar(Parser *parser) {
    parser->anterior = espiar(parser);
    parser->atual = lexico_proximo(parser->lexico);
    parser->tem_atual = true;
    return parser->anterior;
}

static Token espiar(Parser *parser) {
    if (!parser->tem_atual) {
        parser->atual = lexico_proximo(parser->lexico);
        parser->tem_atual = true;
    }
    return parser->atual;
}

static bool conferir(Parser *parser, TipoToken tipo) {
    return espiar(parser).tipo == tipo;
}

static bool combinar(Parser *parser, TipoToken tipo) {
    if (!conferir(parser, tipo)) {
        return false;
    }
    avancar(parser);
    return true;
}

static Token consumir(Parser *parser, TipoToken tipo, const char *mensagem) {
    if (!conferir(parser, tipo)) {
        noema_falhar("%s em %s", mensagem, parser->caminho);
    }
    return avancar(parser);
}

static bool ha_quebra_de_linha(Parser *parser) {
    if (!parser->tem_atual) {
        return false;
    }
    return espiar(parser).linha > parser->anterior.linha;
}

static void consumir_terminador_instrucao(Parser *parser, const char *mensagem) {
    if (combinar(parser, TOKEN_PONTO_E_VIRGULA)) {
        return;
    }
    if (conferir(parser, TOKEN_EOF) || conferir(parser, TOKEN_FECHA_CHAVE) || ha_quebra_de_linha(parser)) {
        return;
    }
    noema_falhar("%s em %s", mensagem, parser->caminho);
}

static void parsear_parametros_funcao(Parser *parser, char ***parametros, int *quantidade_parametros) {
    *parametros = NULL;
    *quantidade_parametros = 0;
    int capacidade_parametros = 0;

    consumir(parser, TOKEN_ABRE_PAREN, "funcao exige '('");
    if (!conferir(parser, TOKEN_FECHA_PAREN)) {
        for (;;) {
            Token parametro = consumir(parser, TOKEN_IDENTIFICADOR, "parametro invalido");
            if (capacidade_parametros <= *quantidade_parametros) {
                capacidade_parametros = capacidade_parametros == 0 ? 4 : capacidade_parametros * 2;
                *parametros = realloc(*parametros, (size_t)capacidade_parametros * sizeof(char *));
                if (*parametros == NULL) {
                    noema_falhar("memoria insuficiente");
                }
            }
            (*parametros)[(*quantidade_parametros)++] = noema_duplicar(parametro.lexema);
            if (!combinar(parser, TOKEN_VIRGULA)) {
                break;
            }
            if (conferir(parser, TOKEN_FECHA_PAREN)) {
                break;
            }
        }
    }
    consumir(parser, TOKEN_FECHA_PAREN, "funcao exige ')'");
}

static No *embrulhar_expressao_em_retorno(No *expressao, int linha) {
    No *bloco = novo_no(NO_BLOCO, linha);
    No *retorno = novo_no(NO_RETURN, linha);
    retorno->como.retorno.valor = expressao;
    bloco_adicionar(bloco, retorno);
    return bloco;
}

static No *parsear_corpo_funcao(Parser *parser, bool *corpo_em_linha) {
    if (combinar(parser, TOKEN_SETA_GROSSA)) {
        *corpo_em_linha = true;
        return embrulhar_expressao_em_retorno(parsear_expressao(parser), parser->anterior.linha);
    }
    *corpo_em_linha = false;
    return parsear_bloco(parser);
}

static No *criar_negacao(No *condicao, int linha) {
    No *no = novo_no(NO_UNARIO, linha);
    no->como.unario.operador = TOKEN_NOT;
    no->como.unario.direita = condicao;
    return no;
}

static No *parsear_condicao_parenteses_opcional(Parser *parser, const char *nome) {
    if (combinar(parser, TOKEN_ABRE_PAREN)) {
        No *condicao = parsear_expressao(parser);
        consumir(parser, TOKEN_FECHA_PAREN, nome);
        return condicao;
    }
    return parsear_expressao(parser);
}

static TipoFfi parsear_tipo_ffi(const char *nome) {
    if (strcmp(nome, "number") == 0) return FFI_NUMERO;
    if (strcmp(nome, "int") == 0) return FFI_INT;
    if (strcmp(nome, "string") == 0) return FFI_TEXTO;
    if (strcmp(nome, "bool") == 0) return FFI_BOOL;
    if (strcmp(nome, "pointer") == 0) return FFI_PONTEIRO;
    if (strcmp(nome, "void") == 0) return FFI_VAZIO;
    noema_falhar("tipo ffi desconhecido: %s", nome);
    return FFI_VAZIO;
}

static No *parsear_bloco(Parser *parser) {
    Token abertura = consumir(parser, TOKEN_ABRE_CHAVE, "bloco exige '{'");
    No *bloco = novo_no(NO_BLOCO, abertura.linha);
    while (!conferir(parser, TOKEN_FECHA_CHAVE) && !conferir(parser, TOKEN_EOF)) {
        bloco_adicionar(bloco, parsear_declaracao(parser));
    }
    consumir(parser, TOKEN_FECHA_CHAVE, "bloco exige '}'");
    return bloco;
}

static No *parsear_declaracao_variavel(Parser *parser, TipoNo tipo_no) {
    Token nome = consumir(parser, TOKEN_IDENTIFICADOR, "declaracao exige nome");
    No *declaracao = novo_no(tipo_no, nome.linha);
    declaracao->como.declaracao_variavel.nome = noema_duplicar(nome.lexema);
    declaracao->como.declaracao_variavel.inicializador = NULL;
    if (combinar(parser, TOKEN_ATRIBUICAO)) {
        declaracao->como.declaracao_variavel.inicializador = parsear_expressao(parser);
    }
    consumir_terminador_instrucao(parser, "declaracao exige terminador");
    return declaracao;
}

static No *parsear_declaracao_funcao(Parser *parser) {
    Token nome = consumir(parser, TOKEN_IDENTIFICADOR, "function exige nome");
    char **parametros = NULL;
    int quantidade_parametros = 0;
    bool corpo_em_linha = false;
    parsear_parametros_funcao(parser, &parametros, &quantidade_parametros);
    No *corpo = parsear_corpo_funcao(parser, &corpo_em_linha);

    No *funcao = novo_no(NO_DECL_FUNCAO, nome.linha);
    funcao->como.declaracao_funcao.nome = noema_duplicar(nome.lexema);
    funcao->como.declaracao_funcao.parametros = parametros;
    funcao->como.declaracao_funcao.quantidade_parametros = quantidade_parametros;
    funcao->como.declaracao_funcao.corpo = corpo;
    if (corpo_em_linha) {
        consumir_terminador_instrucao(parser, "funcao exige terminador");
    }
    return funcao;
}

static No *parsear_funcao_anonima(Parser *parser) {
    int linha = parser->anterior.linha;
    char **parametros = NULL;
    int quantidade_parametros = 0;
    bool corpo_em_linha = false;
    parsear_parametros_funcao(parser, &parametros, &quantidade_parametros);
    No *corpo = parsear_corpo_funcao(parser, &corpo_em_linha);

    No *funcao = novo_no(NO_FUNCAO_ANONIMA, linha);
    funcao->como.declaracao_funcao.nome = NULL;
    funcao->como.declaracao_funcao.parametros = parametros;
    funcao->como.declaracao_funcao.quantidade_parametros = quantidade_parametros;
    funcao->como.declaracao_funcao.corpo = corpo;
    return funcao;
}

static No *parsear_declaracao_plugin(Parser *parser) {
    Token nome = consumir(parser, TOKEN_IDENTIFICADOR, "plugin exige nome");
    consumir(parser, TOKEN_ABRE_CHAVE, "plugin exige '{'");

    No *plugin = novo_no(NO_DECL_PLUGIN, nome.linha);
    plugin->como.plugin.nome = noema_duplicar(nome.lexema);

    int capacidade_ligacoes = 0;

    while (!conferir(parser, TOKEN_FECHA_CHAVE) && !conferir(parser, TOKEN_EOF)) {
        if (combinar(parser, TOKEN_LIBRARY)) {
            Token biblioteca = consumir(parser, TOKEN_TEXTO, "library exige um texto");
            plugin->como.plugin.biblioteca = noema_duplicar(biblioteca.literal);
            consumir_terminador_instrucao(parser, "library exige terminador");
            continue;
        }

        if (combinar(parser, TOKEN_BIND)) {
            Token local = consumir(parser, TOKEN_IDENTIFICADOR, "bind exige nome local");
            consumir(parser, TOKEN_ABRE_PAREN, "bind exige '('");

            TipoFfi *argumentos = NULL;
            int quantidade_argumentos = 0;
            int capacidade_argumentos = 0;

            if (!conferir(parser, TOKEN_FECHA_PAREN)) {
                for (;;) {
                    Token tipo = consumir(parser, TOKEN_IDENTIFICADOR, "tipo ffi invalido");
                    if (capacidade_argumentos <= quantidade_argumentos) {
                        capacidade_argumentos = capacidade_argumentos == 0 ? 4 : capacidade_argumentos * 2;
                        argumentos = realloc(argumentos, (size_t)capacidade_argumentos * sizeof(TipoFfi));
                        if (argumentos == NULL) {
                            noema_falhar("memoria insuficiente");
                        }
                    }
                    argumentos[quantidade_argumentos++] = parsear_tipo_ffi(tipo.lexema);
                    if (!combinar(parser, TOKEN_VIRGULA)) {
                        break;
                    }
                }
            }

            consumir(parser, TOKEN_FECHA_PAREN, "bind exige ')'");
            consumir(parser, TOKEN_SETA, "bind exige '->'");
            Token retorno = consumir(parser, TOKEN_IDENTIFICADOR, "bind exige tipo de retorno");
            consumir(parser, TOKEN_AS, "bind exige 'as'");
            Token simbolo = consumir(parser, TOKEN_TEXTO, "bind exige o simbolo em texto");
            consumir_terminador_instrucao(parser, "bind exige terminador");

            if (plugin->como.plugin.quantidade_ligacoes >= capacidade_ligacoes) {
                capacidade_ligacoes = capacidade_ligacoes == 0 ? 4 : capacidade_ligacoes * 2;
                plugin->como.plugin.ligacoes =
                    realloc(plugin->como.plugin.ligacoes, (size_t)capacidade_ligacoes * sizeof(LigacaoPluginAst));
                if (plugin->como.plugin.ligacoes == NULL) {
                    noema_falhar("memoria insuficiente");
                }
            }

            LigacaoPluginAst *ligacao = &plugin->como.plugin.ligacoes[plugin->como.plugin.quantidade_ligacoes++];
            ligacao->nome_noema = noema_duplicar(local.lexema);
            ligacao->nome_simbolo = noema_duplicar(simbolo.literal);
            ligacao->retorno = parsear_tipo_ffi(retorno.lexema);
            ligacao->argumentos = argumentos;
            ligacao->quantidade_argumentos = quantidade_argumentos;
            continue;
        }

        noema_falhar("plugin '%s' aceita apenas 'library' e 'bind'", plugin->como.plugin.nome);
    }

    consumir(parser, TOKEN_FECHA_CHAVE, "plugin exige '}'");
    if (plugin->como.plugin.biblioteca == NULL) {
        noema_falhar("plugin '%s' precisa de uma clausula library", plugin->como.plugin.nome);
    }
    return plugin;
}

static No *parsear_if(Parser *parser, bool inverter_condicao) {
    Token inicio = parser->anterior;
    No *condicao = parsear_condicao_parenteses_opcional(parser, "if exige ')'");
    if (inverter_condicao) {
        condicao = criar_negacao(condicao, inicio.linha);
    }
    No *ramo_entao = parsear_instrucao(parser);
    No *ramo_senao = NULL;
    if (combinar(parser, TOKEN_ELIF)) {
        ramo_senao = parsear_if(parser, false);
    } else if (combinar(parser, TOKEN_ELSE)) {
        if (combinar(parser, TOKEN_IF)) {
            ramo_senao = parsear_if(parser, false);
        } else if (combinar(parser, TOKEN_UNLESS)) {
            ramo_senao = parsear_if(parser, true);
        } else {
            ramo_senao = parsear_instrucao(parser);
        }
    }
    No *no = novo_no(NO_IF, inicio.linha);
    no->como.condicional.condicao = condicao;
    no->como.condicional.ramo_entao = ramo_entao;
    no->como.condicional.ramo_senao = ramo_senao;
    return no;
}

static No *parsear_while(Parser *parser) {
    Token inicio = parser->anterior;
    No *condicao = parsear_condicao_parenteses_opcional(parser, "while exige ')'");
    No *corpo = parsear_instrucao(parser);
    No *no = novo_no(NO_WHILE, inicio.linha);
    no->como.enquanto.condicao = condicao;
    no->como.enquanto.corpo = corpo;
    return no;
}

static No *parsear_for(Parser *parser) {
    Token inicio = parser->anterior;
    bool com_parenteses = combinar(parser, TOKEN_ABRE_PAREN);
    Token variavel = consumir(parser, TOKEN_IDENTIFICADOR, "for exige uma variavel");
    consumir(parser, TOKEN_IN, "for exige 'in'");
    No *iteravel = parsear_expressao(parser);
    if (com_parenteses) {
        consumir(parser, TOKEN_FECHA_PAREN, "for exige ')'");
    }
    No *corpo = parsear_instrucao(parser);
    No *no = novo_no(NO_FOR, inicio.linha);
    no->como.para.variavel = noema_duplicar(variavel.lexema);
    no->como.para.iteravel = iteravel;
    no->como.para.corpo = corpo;
    return no;
}

static No *parsear_return(Parser *parser) {
    Token inicio = parser->anterior;
    No *no = novo_no(NO_RETURN, inicio.linha);
    if (!conferir(parser, TOKEN_PONTO_E_VIRGULA) &&
        !conferir(parser, TOKEN_FECHA_CHAVE) &&
        !conferir(parser, TOKEN_EOF) &&
        !ha_quebra_de_linha(parser)) {
        no->como.retorno.valor = parsear_expressao(parser);
    }
    consumir_terminador_instrucao(parser, "return exige terminador");
    return no;
}

static No *parsear_instrucao(Parser *parser) {
    if (conferir(parser, TOKEN_ABRE_CHAVE)) {
        return parsear_bloco(parser);
    }
    if (combinar(parser, TOKEN_IF)) {
        return parsear_if(parser, false);
    }
    if (combinar(parser, TOKEN_UNLESS)) {
        return parsear_if(parser, true);
    }
    if (combinar(parser, TOKEN_WHILE)) {
        return parsear_while(parser);
    }
    if (combinar(parser, TOKEN_FOR)) {
        return parsear_for(parser);
    }
    if (combinar(parser, TOKEN_RETURN)) {
        return parsear_return(parser);
    }
    if (combinar(parser, TOKEN_BREAK)) {
        No *no = novo_no(NO_BREAK, parser->anterior.linha);
        consumir_terminador_instrucao(parser, "break exige terminador");
        return no;
    }
    if (combinar(parser, TOKEN_CONTINUE)) {
        No *no = novo_no(NO_CONTINUE, parser->anterior.linha);
        consumir_terminador_instrucao(parser, "continue exige terminador");
        return no;
    }

    No *expressao = parsear_expressao(parser);
    consumir_terminador_instrucao(parser, "instrucao exige terminador");
    No *no = novo_no(NO_EXPR_STMT, expressao->linha);
    no->como.expressao.expressao = expressao;
    return no;
}

static No *parsear_primario(Parser *parser) {
    if (combinar(parser, TOKEN_NUMERO)) {
        No *no = novo_no(NO_LITERAL, parser->anterior.linha);
        no->como.literal.valor = valor_numero(strtod(parser->anterior.literal, NULL));
        return no;
    }
    if (combinar(parser, TOKEN_TEXTO)) {
        No *no = novo_no(NO_LITERAL, parser->anterior.linha);
        no->como.literal.valor = valor_texto(parser->anterior.literal);
        return no;
    }
    if (combinar(parser, TOKEN_TRUE)) {
        No *no = novo_no(NO_LITERAL, parser->anterior.linha);
        no->como.literal.valor = valor_bool(true);
        return no;
    }
    if (combinar(parser, TOKEN_FALSE)) {
        No *no = novo_no(NO_LITERAL, parser->anterior.linha);
        no->como.literal.valor = valor_bool(false);
        return no;
    }
    if (combinar(parser, TOKEN_NULL)) {
        No *no = novo_no(NO_LITERAL, parser->anterior.linha);
        no->como.literal.valor = valor_nulo();
        return no;
    }
    if (combinar(parser, TOKEN_FUNCTION)) {
        return parsear_funcao_anonima(parser);
    }
    if (combinar(parser, TOKEN_IDENTIFICADOR)) {
        No *no = novo_no(NO_VARIAVEL, parser->anterior.linha);
        no->como.variavel.nome = noema_duplicar(parser->anterior.lexema);
        return no;
    }
    if (combinar(parser, TOKEN_ABRE_PAREN)) {
        No *expressao = parsear_expressao(parser);
        consumir(parser, TOKEN_FECHA_PAREN, "expressao exige ')'");
        return expressao;
    }
    if (combinar(parser, TOKEN_ABRE_COLCHETE)) {
        No *lista = novo_no(NO_LISTA_LITERAL, parser->anterior.linha);
        if (!conferir(parser, TOKEN_FECHA_COLCHETE)) {
            for (;;) {
                lista_literal_adicionar(lista, parsear_expressao(parser));
                if (!combinar(parser, TOKEN_VIRGULA)) {
                    break;
                }
                if (conferir(parser, TOKEN_FECHA_COLCHETE)) {
                    break;
                }
            }
        }
        consumir(parser, TOKEN_FECHA_COLCHETE, "lista exige ']'");
        return lista;
    }
    if (combinar(parser, TOKEN_ABRE_CHAVE)) {
        No *mapa = novo_no(NO_MAPA_LITERAL, parser->anterior.linha);
        if (!conferir(parser, TOKEN_FECHA_CHAVE)) {
            for (;;) {
                Token chave = espiar(parser);
                char *nome_chave = NULL;
                if (combinar(parser, TOKEN_IDENTIFICADOR)) {
                    nome_chave = parser->anterior.lexema;
                } else if (combinar(parser, TOKEN_TEXTO)) {
                    nome_chave = parser->anterior.literal;
                } else {
                    noema_falhar("mapa exige chave como identificador ou texto");
                }
                consumir(parser, TOKEN_DOIS_PONTOS, "mapa exige ':'");
                mapa_literal_adicionar(mapa, nome_chave, parsear_expressao(parser));
                (void)chave;
                if (!combinar(parser, TOKEN_VIRGULA)) {
                    break;
                }
                if (conferir(parser, TOKEN_FECHA_CHAVE)) {
                    break;
                }
            }
        }
        consumir(parser, TOKEN_FECHA_CHAVE, "mapa exige '}'");
        return mapa;
    }

    noema_falhar("token inesperado em expressao em %s", parser->caminho);
    return NULL;
}

static No *parsear_chamada(Parser *parser) {
    No *expressao = parsear_primario(parser);

    for (;;) {
        if (combinar(parser, TOKEN_ABRE_PAREN)) {
            No *chamada = novo_no(NO_CHAMADA, expressao->linha);
            chamada->como.chamada.alvo = expressao;
            if (!conferir(parser, TOKEN_FECHA_PAREN)) {
                for (;;) {
                    chamada_adicionar_argumento(chamada, parsear_expressao(parser));
                    if (!combinar(parser, TOKEN_VIRGULA)) {
                        break;
                    }
                    if (conferir(parser, TOKEN_FECHA_PAREN)) {
                        break;
                    }
                }
            }
            consumir(parser, TOKEN_FECHA_PAREN, "chamada exige ')'");
            expressao = chamada;
            continue;
        }
        if (combinar(parser, TOKEN_PONTO)) {
            Token membro = consumir(parser, TOKEN_IDENTIFICADOR, "acesso a membro exige identificador");
            No *no = novo_no(NO_MEMBRO, membro.linha);
            no->como.membro.objeto = expressao;
            no->como.membro.nome = noema_duplicar(membro.lexema);
            expressao = no;
            continue;
        }
        if (combinar(parser, TOKEN_ABRE_COLCHETE)) {
            No *indice = novo_no(NO_INDICE, expressao->linha);
            indice->como.indice.objeto = expressao;
            indice->como.indice.indice = parsear_expressao(parser);
            consumir(parser, TOKEN_FECHA_COLCHETE, "indice exige ']'");
            expressao = indice;
            continue;
        }
        break;
    }

    return expressao;
}

static No *parsear_unario(Parser *parser) {
    if (combinar(parser, TOKEN_NOT) || combinar(parser, TOKEN_MENOS)) {
        No *no = novo_no(NO_UNARIO, parser->anterior.linha);
        no->como.unario.operador = parser->anterior.tipo;
        no->como.unario.direita = parsear_unario(parser);
        return no;
    }
    return parsear_chamada(parser);
}

static No *parsear_fator(Parser *parser) {
    No *expressao = parsear_unario(parser);
    while (combinar(parser, TOKEN_ASTERISCO) || combinar(parser, TOKEN_BARRA) || combinar(parser, TOKEN_PERCENTUAL)) {
        TipoToken operador = parser->anterior.tipo;
        No *no = novo_no(NO_BINARIO, parser->anterior.linha);
        no->como.binario.esquerda = expressao;
        no->como.binario.operador = operador;
        no->como.binario.direita = parsear_unario(parser);
        expressao = no;
    }
    return expressao;
}

static No *parsear_termo(Parser *parser) {
    No *expressao = parsear_fator(parser);
    while (combinar(parser, TOKEN_MAIS) || combinar(parser, TOKEN_MENOS)) {
        TipoToken operador = parser->anterior.tipo;
        No *no = novo_no(NO_BINARIO, parser->anterior.linha);
        no->como.binario.esquerda = expressao;
        no->como.binario.operador = operador;
        no->como.binario.direita = parsear_fator(parser);
        expressao = no;
    }
    return expressao;
}

static No *parsear_comparacao(Parser *parser) {
    No *expressao = parsear_termo(parser);
    while (combinar(parser, TOKEN_MENOR) ||
           combinar(parser, TOKEN_MENOR_IGUAL) ||
           combinar(parser, TOKEN_MAIOR) ||
           combinar(parser, TOKEN_MAIOR_IGUAL)) {
        TipoToken operador = parser->anterior.tipo;
        No *no = novo_no(NO_BINARIO, parser->anterior.linha);
        no->como.binario.esquerda = expressao;
        no->como.binario.operador = operador;
        no->como.binario.direita = parsear_termo(parser);
        expressao = no;
    }
    return expressao;
}

static No *parsear_igualdade(Parser *parser) {
    No *expressao = parsear_comparacao(parser);
    while (combinar(parser, TOKEN_IGUAL) || combinar(parser, TOKEN_DIFERENTE)) {
        TipoToken operador = parser->anterior.tipo;
        No *no = novo_no(NO_BINARIO, parser->anterior.linha);
        no->como.binario.esquerda = expressao;
        no->como.binario.operador = operador;
        no->como.binario.direita = parsear_comparacao(parser);
        expressao = no;
    }
    return expressao;
}

static No *parsear_logico_e(Parser *parser) {
    No *expressao = parsear_igualdade(parser);
    while (combinar(parser, TOKEN_AND)) {
        No *no = novo_no(NO_BINARIO, parser->anterior.linha);
        no->como.binario.esquerda = expressao;
        no->como.binario.operador = TOKEN_AND;
        no->como.binario.direita = parsear_igualdade(parser);
        expressao = no;
    }
    return expressao;
}

static No *parsear_logico_ou(Parser *parser) {
    No *expressao = parsear_logico_e(parser);
    while (combinar(parser, TOKEN_OR)) {
        No *no = novo_no(NO_BINARIO, parser->anterior.linha);
        no->como.binario.esquerda = expressao;
        no->como.binario.operador = TOKEN_OR;
        no->como.binario.direita = parsear_logico_e(parser);
        expressao = no;
    }
    return expressao;
}

static No *parsear_atribuicao(Parser *parser) {
    No *expressao = parsear_logico_ou(parser);
    if (combinar(parser, TOKEN_ATRIBUICAO)) {
        No *no = novo_no(NO_ATRIBUICAO, parser->anterior.linha);
        no->como.atribuicao.alvo = expressao;
        no->como.atribuicao.valor = parsear_atribuicao(parser);
        return no;
    }
    return expressao;
}

static No *parsear_expressao(Parser *parser) {
    return parsear_atribuicao(parser);
}

static No *parsear_declaracao(Parser *parser) {
    if (combinar(parser, TOKEN_FUNCTION)) {
        if (conferir(parser, TOKEN_IDENTIFICADOR)) {
            return parsear_declaracao_funcao(parser);
        }
        No *expressao = parsear_funcao_anonima(parser);
        consumir_terminador_instrucao(parser, "instrucao exige terminador");
        No *no = novo_no(NO_EXPR_STMT, expressao->linha);
        no->como.expressao.expressao = expressao;
        return no;
    }
    if (combinar(parser, TOKEN_PLUGIN)) {
        return parsear_declaracao_plugin(parser);
    }
    if (combinar(parser, TOKEN_LET)) {
        return parsear_declaracao_variavel(parser, NO_DECL_LET);
    }
    if (combinar(parser, TOKEN_CONST)) {
        return parsear_declaracao_variavel(parser, NO_DECL_CONST);
    }
    return parsear_instrucao(parser);
}

Programa *parser_analisar(const char *codigo, const char *caminho) {
    Parser parser = {0};
    parser.lexico = lexico_criar(codigo, caminho);
    parser.caminho = caminho;

    Programa *programa = novo_programa(caminho);
    while (!conferir(&parser, TOKEN_EOF)) {
        programa_adicionar(programa, parsear_declaracao(&parser));
    }

    return programa;
}
