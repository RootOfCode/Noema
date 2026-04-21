#include "noema.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

typedef union {
    double numero;
    int inteiro;
    ffi_sarg inteiro_ffi;
    void *ponteiro;
    char *texto;
} SlotFfi;

static void *noema_ffi_abrir_biblioteca(const char *biblioteca) {
#ifdef _WIN32
    return (void *)LoadLibraryA(biblioteca);
#else
    return dlopen(biblioteca, RTLD_LAZY);
#endif
}

static void *noema_ffi_resolver_simbolo(void *alca, const char *simbolo) {
#ifdef _WIN32
    return (void *)GetProcAddress((HMODULE)alca, simbolo);
#else
    return dlsym(alca, simbolo);
#endif
}

static const char *noema_ffi_erro_ultimo(void) {
#ifdef _WIN32
    static char mensagem[512];
    DWORD codigo = GetLastError();
    DWORD tamanho = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   NULL,
                                   codigo,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   mensagem,
                                   (DWORD)sizeof(mensagem),
                                   NULL);
    if (tamanho == 0) {
        return "erro desconhecido do Windows";
    }
    while (tamanho > 0 && (mensagem[tamanho - 1] == '\r' || mensagem[tamanho - 1] == '\n' || mensagem[tamanho - 1] == ' ')) {
        mensagem[--tamanho] = '\0';
    }
    return mensagem;
#else
    const char *erro = dlerror();
    return erro != NULL ? erro : "erro desconhecido de dlopen";
#endif
}

static ffi_type *tipo_ffi_para_libffi(TipoFfi tipo) {
    switch (tipo) {
        case FFI_NUMERO:
            return &ffi_type_double;
        case FFI_INT:
            return &ffi_type_sint32;
        case FFI_TEXTO:
            return &ffi_type_pointer;
        case FFI_BOOL:
            return &ffi_type_sint;
        case FFI_PONTEIRO:
            return &ffi_type_pointer;
        case FFI_VAZIO:
            return &ffi_type_void;
    }
    return &ffi_type_void;
}

Valor ffi_invocar(Interpretador *interpretador, Ambiente *ambiente, int argc, Valor *argv, void *dados) {
    (void)interpretador;
    (void)ambiente;
    LigacaoPlugin *ligacao = dados;

    if (argc != ligacao->quantidade_argumentos) {
        noema_falhar("funcao ffi '%s' esperava %d argumentos e recebeu %d",
                     ligacao->nome_noema,
                     ligacao->quantidade_argumentos,
                     argc);
    }

    SlotFfi *slots = calloc((size_t)argc, sizeof(SlotFfi));
    void **valores_ffi = calloc((size_t)argc, sizeof(void *));
    if ((slots == NULL || valores_ffi == NULL) && argc > 0) {
        noema_falhar("memoria insuficiente");
    }

    for (int i = 0; i < argc; i++) {
        switch (ligacao->argumentos[i]) {
            case FFI_NUMERO:
                slots[i].numero = valor_para_numero(argv[i]);
                valores_ffi[i] = &slots[i].numero;
                break;
            case FFI_INT:
                slots[i].inteiro = (int)valor_para_numero(argv[i]);
                valores_ffi[i] = &slots[i].inteiro;
                break;
            case FFI_BOOL:
                slots[i].inteiro_ffi = valor_eh_verdadeiro(argv[i]) ? 1 : 0;
                valores_ffi[i] = &slots[i].inteiro_ffi;
                break;
            case FFI_TEXTO:
                if (argv[i].tipo == VALOR_TEXTO) {
                    slots[i].texto = argv[i].como.texto;
                } else {
                    char *texto = valor_para_texto(argv[i]);
                    slots[i].texto = texto;
                }
                valores_ffi[i] = &slots[i].texto;
                break;
            case FFI_PONTEIRO:
                if (argv[i].tipo == VALOR_PONTEIRO) {
                    slots[i].ponteiro = argv[i].como.ponteiro;
                } else if (argv[i].tipo == VALOR_TEXTO) {
                    slots[i].ponteiro = argv[i].como.texto;
                } else if (argv[i].tipo == VALOR_NULO) {
                    slots[i].ponteiro = NULL;
                } else {
                    slots[i].ponteiro = (void *)(uintptr_t)valor_para_numero(argv[i]);
                }
                valores_ffi[i] = &slots[i].ponteiro;
                break;
            case FFI_VAZIO:
                noema_falhar("tipo void nao pode ser usado como argumento ffi");
        }
    }

    SlotFfi retorno = {0};
    void *ponteiro_retorno = NULL;
    switch (ligacao->retorno) {
        case FFI_NUMERO:
            ponteiro_retorno = &retorno.numero;
            break;
        case FFI_INT:
            ponteiro_retorno = &retorno.inteiro;
            break;
        case FFI_BOOL:
            ponteiro_retorno = &retorno.inteiro_ffi;
            break;
        case FFI_TEXTO:
            ponteiro_retorno = &retorno.texto;
            break;
        case FFI_PONTEIRO:
            ponteiro_retorno = &retorno.ponteiro;
            break;
        case FFI_VAZIO:
            ponteiro_retorno = NULL;
            break;
    }

    ffi_call(&ligacao->cif, FFI_FN(ligacao->simbolo), ponteiro_retorno, valores_ffi);

    switch (ligacao->retorno) {
        case FFI_NUMERO:
            return valor_numero(retorno.numero);
        case FFI_INT:
            return valor_numero((double)retorno.inteiro);
        case FFI_BOOL:
            return valor_bool(retorno.inteiro_ffi != 0);
        case FFI_TEXTO:
            if (retorno.texto == NULL) {
                return valor_nulo();
            }
            return valor_texto(retorno.texto);
        case FFI_PONTEIRO:
            return valor_ponteiro(retorno.ponteiro);
        case FFI_VAZIO:
            return valor_nulo();
    }

    return valor_nulo();
}

void plugin_registrar_no_ambiente(Interpretador *interpretador, Ambiente *ambiente, No *no_plugin) {
    (void)interpretador;
    void *alca = noema_ffi_abrir_biblioteca(no_plugin->como.plugin.biblioteca);
    if (alca == NULL) {
        noema_falhar("falha ao abrir biblioteca '%s': %s",
                     no_plugin->como.plugin.biblioteca,
                     noema_ffi_erro_ultimo());
    }

    Mapa *plugin = mapa_criar();
    mapa_definir(plugin, "kind", valor_texto("plugin"));
    mapa_definir(plugin, "name", valor_texto(no_plugin->como.plugin.nome));

    for (int i = 0; i < no_plugin->como.plugin.quantidade_ligacoes; i++) {
        LigacaoPluginAst *origem = &no_plugin->como.plugin.ligacoes[i];
        LigacaoPlugin *ligacao = calloc(1, sizeof(LigacaoPlugin));
        if (ligacao == NULL) {
            noema_falhar("memoria insuficiente");
        }
        ligacao->nome_noema = noema_duplicar(origem->nome_noema);
        ligacao->nome_simbolo = noema_duplicar(origem->nome_simbolo);
        ligacao->retorno = origem->retorno;
        ligacao->quantidade_argumentos = origem->quantidade_argumentos;
        ligacao->argumentos = calloc((size_t)ligacao->quantidade_argumentos, sizeof(TipoFfi));
        ligacao->tipos_argumentos_ffi = calloc((size_t)ligacao->quantidade_argumentos, sizeof(ffi_type *));
        if ((ligacao->argumentos == NULL || ligacao->tipos_argumentos_ffi == NULL) &&
            ligacao->quantidade_argumentos > 0) {
            noema_falhar("memoria insuficiente");
        }
        for (int j = 0; j < ligacao->quantidade_argumentos; j++) {
            ligacao->argumentos[j] = origem->argumentos[j];
            ligacao->tipos_argumentos_ffi[j] = tipo_ffi_para_libffi(origem->argumentos[j]);
        }
        ligacao->tipo_retorno_ffi = tipo_ffi_para_libffi(origem->retorno);
        ligacao->alca_biblioteca = alca;
        ligacao->simbolo = noema_ffi_resolver_simbolo(alca, origem->nome_simbolo);
        if (ligacao->simbolo == NULL) {
            noema_falhar("falha ao resolver simbolo '%s' no plugin '%s': %s",
                         origem->nome_simbolo,
                         no_plugin->como.plugin.nome,
                         noema_ffi_erro_ultimo());
        }

        ffi_status status = ffi_prep_cif(&ligacao->cif,
                                         FFI_DEFAULT_ABI,
                                         (unsigned int)ligacao->quantidade_argumentos,
                                         ligacao->tipo_retorno_ffi,
                                         ligacao->tipos_argumentos_ffi);
        if (status != FFI_OK) {
            noema_falhar("nao foi possivel preparar a assinatura ffi de '%s'", ligacao->nome_noema);
        }

        FuncaoNativa *funcao = calloc(1, sizeof(FuncaoNativa));
        if (funcao == NULL) {
            noema_falhar("memoria insuficiente");
        }
        funcao->nome = noema_duplicar(ligacao->nome_noema);
        funcao->funcao = ffi_invocar;
        funcao->dados = ligacao;
        mapa_definir(plugin, ligacao->nome_noema, valor_nativa(funcao));
    }

    ambiente_definir(ambiente, no_plugin->como.plugin.nome, valor_mapa(plugin), true);
}
