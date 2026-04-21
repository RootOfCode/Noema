#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "noema.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

typedef struct {
    char *dados;
    size_t comprimento;
    size_t capacidade;
} BufferTexto;

static void buffer_inicializar(BufferTexto *buffer) {
    buffer->capacidade = 256;
    buffer->comprimento = 0;
    buffer->dados = calloc(buffer->capacidade, 1);
    if (buffer->dados == NULL) {
        noema_falhar("memoria insuficiente");
    }
}

static void buffer_limpar(BufferTexto *buffer) {
    buffer->comprimento = 0;
    if (buffer->dados != NULL) {
        buffer->dados[0] = '\0';
    }
}

static void buffer_anexar(BufferTexto *buffer, const char *texto) {
    size_t extra = strlen(texto);
    while (buffer->comprimento + extra + 1 > buffer->capacidade) {
        buffer->capacidade *= 2;
        buffer->dados = realloc(buffer->dados, buffer->capacidade);
        if (buffer->dados == NULL) {
            noema_falhar("memoria insuficiente");
        }
    }
    memcpy(buffer->dados + buffer->comprimento, texto, extra + 1);
    buffer->comprimento += extra;
}

static const char *texto_sem_espacos_iniciais(const char *texto) {
    while (*texto != '\0' && isspace((unsigned char)*texto)) {
        texto++;
    }
    return texto;
}

static bool texto_vazio(const char *texto) {
    return *texto_sem_espacos_iniciais(texto) == '\0';
}

static bool palavra_final_eh(const char *texto, const char *palavra) {
    size_t tamanho = strlen(texto);
    while (tamanho > 0 && isspace((unsigned char)texto[tamanho - 1])) {
        tamanho--;
    }
    size_t tamanho_palavra = strlen(palavra);
    if (tamanho < tamanho_palavra) {
        return false;
    }
    size_t inicio = tamanho - tamanho_palavra;
    if (strncmp(texto + inicio, palavra, tamanho_palavra) != 0) {
        return false;
    }
    if (inicio > 0 && (isalnum((unsigned char)texto[inicio - 1]) || texto[inicio - 1] == '_')) {
        return false;
    }
    return true;
}

static bool caractere_final_indica_continuacao(const char *texto) {
    size_t tamanho = strlen(texto);
    while (tamanho > 0 && isspace((unsigned char)texto[tamanho - 1])) {
        tamanho--;
    }
    if (tamanho == 0) {
        return false;
    }
    if (tamanho >= 2 && texto[tamanho - 2] == '=' && texto[tamanho - 1] == '>') {
        return true;
    }
    if (tamanho >= 2 && texto[tamanho - 2] == '-' && texto[tamanho - 1] == '>') {
        return true;
    }
    char final = texto[tamanho - 1];
    return strchr("([{,:.+-*/%=!<>|&", final) != NULL;
}

static bool codigo_repl_precisa_mais_linhas(const char *codigo) {
    int parenteses = 0;
    int colchetes = 0;
    int chaves = 0;
    bool em_texto = false;
    char delimitador_texto = '\0';
    bool escape = false;
    bool comentario_linha = false;
    bool comentario_bloco = false;

    for (size_t i = 0; codigo[i] != '\0'; i++) {
        char c = codigo[i];
        char proximo = codigo[i + 1];

        if (comentario_linha) {
            if (c == '\n') {
                comentario_linha = false;
            }
            continue;
        }

        if (comentario_bloco) {
            if (c == '*' && proximo == '/') {
                comentario_bloco = false;
                i++;
            }
            continue;
        }

        if (em_texto) {
            if (escape) {
                escape = false;
                continue;
            }
            if (delimitador_texto != '`' && c == '\\') {
                escape = true;
                continue;
            }
            if (c == delimitador_texto) {
                em_texto = false;
                delimitador_texto = '\0';
            }
            continue;
        }

        if (c == '/' && proximo == '/') {
            comentario_linha = true;
            i++;
            continue;
        }
        if (c == '/' && proximo == '*') {
            comentario_bloco = true;
            i++;
            continue;
        }
        if (c == '"' || c == '\'' || c == '`') {
            em_texto = true;
            delimitador_texto = c;
            escape = false;
            continue;
        }

        if (c == '(') parenteses++;
        if (c == ')') parenteses--;
        if (c == '[') colchetes++;
        if (c == ']') colchetes--;
        if (c == '{') chaves++;
        if (c == '}') chaves--;
    }

    if (em_texto || comentario_bloco || parenteses > 0 || colchetes > 0 || chaves > 0) {
        return true;
    }

    if (caractere_final_indica_continuacao(codigo)) {
        return true;
    }

    return palavra_final_eh(codigo, "and") ||
           palavra_final_eh(codigo, "or") ||
           palavra_final_eh(codigo, "not") ||
           palavra_final_eh(codigo, "else") ||
           palavra_final_eh(codigo, "elif") ||
           palavra_final_eh(codigo, "in");
}

static bool erro_repl_parece_incompleto(const char *erro) {
    return strstr(erro, "instrucao exige terminador") != NULL ||
           strstr(erro, "declaracao exige terminador") != NULL ||
           strstr(erro, "token inesperado em expressao") != NULL ||
           strstr(erro, "expressao exige ')'") != NULL ||
           strstr(erro, "lista exige ']'") != NULL ||
           strstr(erro, "mapa exige '}'") != NULL ||
           strstr(erro, "bloco exige '}'") != NULL ||
           strstr(erro, "funcao exige ')'") != NULL ||
           strstr(erro, "funcao exige '{'") != NULL ||
           strstr(erro, "chamada exige ')'") != NULL ||
           strstr(erro, "indice exige ']'") != NULL;
}

static bool stdin_tem_dados_pendentes(void) {
#ifdef _WIN32
    int descritor = _fileno(stdin);
    if (descritor < 0 || !_isatty(descritor)) {
        return false;
    }
    return _kbhit() != 0;
#else
    int descritor = fileno(stdin);
    if (descritor < 0 || !isatty(descritor)) {
        return false;
    }
    fd_set leitura;
    FD_ZERO(&leitura);
    FD_SET(descritor, &leitura);
    struct timeval timeout = {0, 0};
    int pronto = select(descritor + 1, &leitura, NULL, NULL, &timeout);
    return pronto > 0 && FD_ISSET(descritor, &leitura);
#endif
}

static bool texto_comeca_com_palavra(const char *texto, const char *palavra) {
    size_t tamanho = strlen(palavra);
    if (strncmp(texto, palavra, tamanho) != 0) {
        return false;
    }
    char proximo = texto[tamanho];
    return proximo == '\0' || isspace((unsigned char)proximo) || proximo == '(';
}

static bool codigo_repl_suprime_resultado(const char *codigo) {
    const char *texto = texto_sem_espacos_iniciais(codigo);
    if (texto_comeca_com_palavra(texto, "let") ||
        texto_comeca_com_palavra(texto, "var") ||
        texto_comeca_com_palavra(texto, "const") ||
        texto_comeca_com_palavra(texto, "function")) {
        return true;
    }
    if (strncmp(texto, "fn", 2) == 0 && isspace((unsigned char)texto[2])) {
        const char *cursor = texto_sem_espacos_iniciais(texto + 2);
        if ((isalpha((unsigned char)*cursor) || *cursor == '_')) {
            while (isalnum((unsigned char)*cursor) || *cursor == '_') {
                cursor++;
            }
            return *cursor == '(' || isspace((unsigned char)*cursor);
        }
    }
    return false;
}

static const char *repl_extrair_topico_ajuda(const char *texto) {
    if (strncmp(texto, ":help", 5) == 0 &&
        (texto[5] == '\0' || isspace((unsigned char)texto[5]))) {
        return texto_sem_espacos_iniciais(texto + 5);
    }
    if (strncmp(texto, "help", 4) == 0 &&
        (texto[4] == '\0' || isspace((unsigned char)texto[4]))) {
        return texto_sem_espacos_iniciais(texto + 4);
    }
    return NULL;
}

static bool repl_linha_eh_comando(const char *texto) {
    return texto[0] == ':';
}

static void repl_mostrar_ajuda_geral(void) {
    printf("Noema REPL\n");
    printf("Comandos:\n");
    printf("  :help                 mostra esta ajuda\n");
    printf("  :help syntax          resumo da sintaxe da linguagem\n");
    printf("  :help stdlib          namespaces da stdlib embarcada\n");
    printf("  :help ffi             plugins e FFI\n");
    printf("  :quit, :exit, quit    encerra o REPL\n");
    printf("  :reset                descarta o bloco multiline pendente\n");
    printf("Dicas:\n");
    printf("  blocos multiline usam o prompt ....>\n");
    printf("  a stdlib ja carrega Core, IO, FS, Path, REPL, Shell, Math,\n");
    printf("  Random, Strings, List, Maps, Fn, Assert, Patterns, Dialogue e Rules\n");
    printf("  digite :help syntax, :help stdlib ou :help ffi\n");
}

static void repl_mostrar_ajuda_sintaxe(void) {
    printf("Sintaxe basica de Noema\n");
    printf("  var answer = 42\n");
    printf("  fn square(x) => x * x\n");
    printf("  when answer > 0 { print(\"ok\") }\n");
    printf("  each item in [1, 2, 3] { print(item) }\n");
    printf("  macro unless_do(cond, body) { return syntax if not $cond { $body } }\n");
    printf("Recursos:\n");
    printf("  funcoes, loops, listas, mapas, strings com #{...}, macros AST, syntax e eval\n");
    printf("  semicolon e opcional quando a linha ja termina a instrucao\n");
}

static void repl_mostrar_ajuda_stdlib(void) {
    printf("Stdlib carregada automaticamente\n");
    printf("  Core      tipos, conversoes, defaults, enumerate, times\n");
    printf("  IO        print, read_line, ask, confirm, panic\n");
    printf("  FS        arquivos e diretorios\n");
    printf("  Path      join, dirname, basename, extname, stem\n");
    printf("  REPL      REPLs escritos em Noema\n");
    printf("  Shell     shells simples escritos em Noema\n");
    printf("  Math      matematica e utilitarios numericos\n");
    printf("  Random    aleatoriedade e amostragem\n");
    printf("  Strings   normalizacao, split, join, replace, title_case\n");
    printf("  List      map, filter, reduce, partition, group_by\n");
    printf("  Maps      clone, merge, entries, pick, omit\n");
    printf("  Fn        pipe, compose, memoize, once\n");
    printf("  Assert    assercoes\n");
    printf("  Patterns  matching, templates e reflexao textual\n");
    printf("  Dialogue  memoria e REPLs conversacionais\n");
    printf("  Rules     motores de regras e fatores de certeza\n");
}

static void repl_mostrar_ajuda_ffi(void) {
    printf("Plugins e FFI\n");
    printf("  plugins ficam separados da stdlib\n");
    printf("  use plugin Nome { library \"a\", \"b\" bind simbolo(...) -> tipo }\n");
    printf("  o runtime tenta as bibliotecas na ordem declarada\n");
    printf("  'as \"...\"' em bind e opcional quando o simbolo nativo tem o mesmo nome\n");
    printf("  tipos suportados: number, int, bool, string, pointer, void\n");
    printf("  Linux costuma usar .so; Windows costuma usar .dll\n");
}

static void repl_mostrar_ajuda(const char *topico) {
    if (topico == NULL || *topico == '\0') {
        repl_mostrar_ajuda_geral();
        return;
    }

    if (strcmp(topico, "syntax") == 0 || strcmp(topico, "lang") == 0) {
        repl_mostrar_ajuda_sintaxe();
        return;
    }
    if (strcmp(topico, "stdlib") == 0 || strcmp(topico, "library") == 0) {
        repl_mostrar_ajuda_stdlib();
        return;
    }
    if (strcmp(topico, "ffi") == 0 || strcmp(topico, "plugin") == 0 || strcmp(topico, "plugins") == 0) {
        repl_mostrar_ajuda_ffi();
        return;
    }

    printf("topico de ajuda desconhecido: %s\n", topico);
    printf("use :help, :help syntax, :help stdlib ou :help ffi\n");
}

static bool repl_executar_comando(const char *conteudo,
                                  bool tem_pendente,
                                  BufferTexto *acumulado,
                                  bool *deve_encerrar) {
    if (!tem_pendente && strcmp(conteudo, "quit") == 0) {
        *deve_encerrar = true;
        return true;
    }

    if (strcmp(conteudo, ":exit") == 0 || strcmp(conteudo, ":quit") == 0) {
        *deve_encerrar = true;
        return true;
    }

    if (strcmp(conteudo, ":reset") == 0) {
        buffer_limpar(acumulado);
        return true;
    }

    if (!tem_pendente) {
        const char *topico_ajuda = repl_extrair_topico_ajuda(conteudo);
        if (topico_ajuda != NULL) {
            repl_mostrar_ajuda(*topico_ajuda == '\0' ? NULL : topico_ajuda);
            return true;
        }
    } else if (strncmp(conteudo, ":help", 5) == 0 &&
               (conteudo[5] == '\0' || isspace((unsigned char)conteudo[5]))) {
        const char *topico_ajuda = texto_sem_espacos_iniciais(conteudo + 5);
        repl_mostrar_ajuda(*topico_ajuda == '\0' ? NULL : topico_ajuda);
        return true;
    }

    if (repl_linha_eh_comando(conteudo)) {
        fprintf(stderr, "erro noema: comando do repl desconhecido: %s\n", conteudo);
        return true;
    }

    return false;
}

static void configurar_contexto_cli(Interpretador *interpretador,
                                    const char *script,
                                    const char *diretorio_stdlib,
                                    int argc,
                                    char **argv,
                                    int inicio_args) {
    Lista *argumentos = lista_criar();
    for (int i = inicio_args; i < argc; i++) {
        lista_adicionar(argumentos, valor_texto(argv[i]));
    }
    ambiente_definir(interpretador->global, "ARGV", valor_lista(argumentos), true);
    ambiente_definir(interpretador->global, "SCRIPT", valor_texto(script != NULL ? script : "<repl>"), true);
    ambiente_definir(interpretador->global, "NOEMA_STDLIB", valor_texto(diretorio_stdlib), true);
}

static int executar_arquivo(Interpretador *interpretador, const char *caminho) {
    FonteProcessada fonte = preprocessar_arquivo(caminho, &interpretador->macros, &interpretador->importados);
    Programa *programa = parser_analisar(fonte.codigo, fonte.caminho);
    interpretador_executar_programa(interpretador, programa, interpretador->global);
    return 0;
}

static int executar_repl(Interpretador *interpretador) {
    char buffer[4096];
    BufferTexto acumulado;
    bool mostrar_prompt = true;
    buffer_inicializar(&acumulado);
    printf("Noema REPL. Use :help ou help, :exit ou :quit.\n");
    for (;;) {
        bool tem_pendente = acumulado.comprimento > 0;
        if (mostrar_prompt) {
            printf("%s", tem_pendente ? "....> " : "noema> ");
            fflush(stdout);
        }
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("\n");
            break;
        }
        size_t tamanho = strlen(buffer);
        while (tamanho > 0 && (buffer[tamanho - 1] == '\n' || buffer[tamanho - 1] == '\r')) {
            buffer[--tamanho] = '\0';
        }
        const char *conteudo = texto_sem_espacos_iniciais(buffer);
        bool linha_vazia = texto_vazio(buffer);
        bool deve_encerrar = false;

        if (!tem_pendente && linha_vazia) {
            mostrar_prompt = !stdin_tem_dados_pendentes();
            continue;
        }

        if (repl_executar_comando(conteudo, tem_pendente, &acumulado, &deve_encerrar)) {
            if (deve_encerrar) {
                break;
            }
            mostrar_prompt = !stdin_tem_dados_pendentes();
            continue;
        }

        buffer_anexar(&acumulado, buffer);
        buffer_anexar(&acumulado, "\n");

        bool forcar_avaliacao = tem_pendente && linha_vazia;
        if (!forcar_avaliacao && codigo_repl_precisa_mais_linhas(acumulado.dados)) {
            mostrar_prompt = !stdin_tem_dados_pendentes();
            continue;
        }

        Valor valor = valor_nulo();
        char *erro = NULL;
        if (!interpretador_tentar_executar_codigo(interpretador, interpretador->global, acumulado.dados, "<repl>", &valor, &erro)) {
            if (!forcar_avaliacao && erro_repl_parece_incompleto(erro)) {
                free(erro);
                mostrar_prompt = !stdin_tem_dados_pendentes();
                continue;
            }
            fprintf(stderr, "erro noema: %s\n", erro);
            free(erro);
            buffer_limpar(&acumulado);
            mostrar_prompt = !stdin_tem_dados_pendentes();
            continue;
        }
        if (valor.tipo != VALOR_NULO && !codigo_repl_suprime_resultado(acumulado.dados)) {
            char *texto = valor_para_texto(valor);
            printf("%s\n", texto);
        }
        buffer_limpar(&acumulado);
        mostrar_prompt = !stdin_tem_dados_pendentes();
    }
    return 0;
}

int main(int argc, char **argv) {
    char *diretorio_stdlib = noema_descobrir_diretorio_stdlib(argv[0]);
    noema_configurar_diretorio_stdlib(diretorio_stdlib);

    Interpretador *interpretador = interpretador_criar();
    int codigo_saida = 0;

    if (argc < 2) {
        configurar_contexto_cli(interpretador, NULL, diretorio_stdlib, argc, argv, 1);
        codigo_saida = executar_repl(interpretador);
    } else if (strcmp(argv[1], "--repl") == 0 || strcmp(argv[1], "-i") == 0) {
        const char *script = argc >= 3 ? argv[2] : NULL;
        int inicio_args = script != NULL ? 3 : 2;
        configurar_contexto_cli(interpretador, script, diretorio_stdlib, argc, argv, inicio_args);
        if (script != NULL) {
            codigo_saida = executar_arquivo(interpretador, script);
            if (codigo_saida == 0) {
                codigo_saida = executar_repl(interpretador);
            }
        } else {
            codigo_saida = executar_repl(interpretador);
        }
    } else {
        configurar_contexto_cli(interpretador, argv[1], diretorio_stdlib, argc, argv, 2);
        codigo_saida = executar_arquivo(interpretador, argv[1]);
    }

    interpretador_destruir(interpretador);
    return codigo_saida;
}
