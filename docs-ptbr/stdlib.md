# Stdlib de Noema

A stdlib é carregada automaticamente via `stdlib/prelude.noe` no startup. Todos os namespaces abaixo ficam disponíveis sem `import` manual.

O namespace `Std` agrega todos os módulos sob `Std.Core`, `Std.IO`, etc. Os namespaces também estão disponíveis diretamente no escopo global.

Leituras relacionadas: [language.md](./language.md) · [syntax.md](./syntax.md) · [ffi.md](./ffi.md) · [files.md](./files.md)

---

## Módulos básicos

### Core — `stdlib/core.noe`

Utilitários fundamentais, tipos, conversões e helpers de memória para FFI.

**Constantes:** `Core.version` (ex.: `"Noema 0.3"`)

**Tipos e conversão:**

| Função | Descrição |
|--------|-----------|
| `Core.type(v)` / `type(v)` | nome do tipo como string |
| `Core.string(v)` | converte para string |
| `Core.number(v)` | converte para número |
| `Core.int(v)` | converte para inteiro |
| `Core.bool(v)` | converte para bool |
| `Core.parse_number(s)` | parseia string como número |
| `Core.parse_int(s)` | parseia string como inteiro |

**Predicados de tipo:**

`is_null`, `is_bool`, `is_number`, `is_string`, `is_list`, `is_map`, `is_function`, `is_pointer`

**Helpers de lógica:**

| Função | Descrição |
|--------|-----------|
| `Core.default(v, fallback)` | retorna `fallback` se `v` for null |
| `Core.defined(v)` | verdadeiro se `v != null` |
| `Core.truthy(v)` | converte para bool positivo |
| `Core.falsy(v)` | inverte truthy |
| `Core.maybe(v, fn, fallback)` | chama `fn(v)` se não null, caso contrário `fallback` |
| `Core.identity(v)` | retorna `v` sem modificar |
| `Core.noop(v)` | alias de identity |
| `Core.is_scalar(v)` | verdadeiro para null, bool, number, string |

**Coleções:**

| Função | Descrição |
|--------|-----------|
| `Core.clone_list(xs)` | cópia rasa de lista |
| `Core.repeat_value(v, n)` | lista com `n` cópias de `v` |
| `Core.times(n, fn)` | chama `fn(i)` para `i` em `0..n-1` |
| `Core.enumerate(xs)` | lista de `{index, value}` |
| `Core.range(start, end)` | lista de inteiros `[start, end)` |

**Memória / FFI:**

`memory_alloc`, `memory_free`, `memory_fill`, `memory_get_u8`, `memory_get_i32`, `memory_get_u32`, `memory_set_u8`, `memory_set_i32`, `memory_set_u32`

---

### Assert — `stdlib/assert.noe`

Asserções para testes e validações.

| Função | Descrição |
|--------|-----------|
| `Assert.assert(cond, msg?)` | lança panic se `cond` for falso |
| `Assert.eq(a, b, msg?)` | falha se `a != b` |
| `Assert.ne(a, b, msg?)` | falha se `a == b` |
| `Assert.truthy(v, msg?)` | falha se `v` for falsy |
| `Assert.is_null(v, msg?)` | falha se `v` não for null |

---

## IO, sistema e ambiente interativo

### IO — `stdlib/io.noe`

Console, prompts e entrada do usuário.

| Função | Descrição |
|--------|-----------|
| `IO.print(args...)` | imprime com newline |
| `IO.println(args...)` | alias de print |
| `IO.puts(str)` | imprime string com newline |
| `IO.write(args...)` | imprime sem newline |
| `IO.flush()` | descarrega stdout |
| `IO.show(label, value)` | imprime `label: value` formatado |
| `IO.inspect(value)` | representação de debug de um valor |
| `IO.prompt(text)` | lê linha após exibir texto |
| `IO.ask(text)` | alias de prompt |
| `IO.confirm(text)` | lê linha e interpreta como bool |
| `IO.read_line(prompt?)` | lê linha do stdin |
| `IO.read_stdin()` | lê todo o stdin |
| `IO.panic(msg)` | imprime erro e encerra o processo |

---

### FS — `stdlib/fs.noe`

Arquivos e diretórios.

| Função | Descrição |
|--------|-----------|
| `FS.cwd()` | diretório atual |
| `FS.chdir(path)` | muda diretório |
| `FS.exists(path)` | verdadeiro se o caminho existe |
| `FS.is_file(path)` | verdadeiro se for arquivo |
| `FS.is_dir(path)` | verdadeiro se for diretório |
| `FS.list(path)` | lista entradas do diretório |
| `FS.mkdir(path)` | cria diretório |
| `FS.mkdir_p(path)` | cria diretório e ancestrais |
| `FS.read_text(path)` | lê arquivo como string |
| `FS.write_text(path, content)` | escreve string no arquivo |
| `FS.append_text(path, content)` | acrescenta ao arquivo |
| `FS.read_lines(path)` | lê arquivo como lista de linhas |

---

### Path — `stdlib/path.noe`

Operações de caminho de arquivo.

| Função | Descrição |
|--------|-----------|
| `Path.join(a, b)` | une dois segmentos de caminho |
| `Path.dirname(path)` | diretório pai |
| `Path.basename(path)` | nome do arquivo com extensão |
| `Path.extname(path)` | extensão (ex.: `".noe"`) |
| `Path.stem(path)` | nome sem extensão |
| `Path.parts(path)` | lista de segmentos do caminho |

---

### REPL — `stdlib/repl.noe`

Construção de REPLs customizados na própria linguagem.

| Função | Descrição |
|--------|-----------|
| `REPL.create(options)` | cria estado de REPL |
| `REPL.eval(state, line)` | avalia uma linha no contexto do REPL |
| `REPL.help(state)` | exibe ajuda |
| `REPL.start(options)` | inicia loop do REPL |

Veja [repl.md](./repl.md) para referência completa dos campos de `options`.

---

### Shell — `stdlib/shell.noe`

Construção de shells simples em Noema.

| Função | Descrição |
|--------|-----------|
| `Shell.create(options)` | cria estado de shell |
| `Shell.run(state, line)` | executa uma linha de comando |
| `Shell.start(options)` | inicia loop do shell |

---

## Matemática, aleatoriedade e strings

### Math — `stdlib/math.noe`

**Constantes:** `Math.pi`, `Math.tau`, `Math.e`

**Trigonometria:** `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`

**Numéricas:** `sqrt`, `hypot`, `abs`, `floor`, `ceil`, `round`, `trunc`, `pow`, `mod`, `sign`

**Limites e análise:** `min`, `max`, `log`, `exp`, `is_nan`, `is_finite`

**Helpers:**

| Função | Descrição |
|--------|-----------|
| `Math.clamp(v, lo, hi)` | limita `v` ao intervalo `[lo, hi]` |
| `Math.lerp(a, b, t)` | interpolação linear |
| `Math.inverse_lerp(a, b, v)` | posição relativa de `v` entre `a` e `b` |
| `Math.normalize(v, lo, hi)` | normaliza para `[0, 1]` |
| `Math.deg_to_rad(d)` | graus para radianos |
| `Math.rad_to_deg(r)` | radianos para graus |
| `Math.distance2d(x1, y1, x2, y2)` | distância euclidiana 2D |
| `Math.average(list)` | média aritmética |

---

### Random — `stdlib/random.noe`

| Função | Descrição |
|--------|-----------|
| `Random.value()` | float em `[0, 1)` |
| `Random.int(n)` | inteiro em `[0, n)` |
| `Random.between(a, b)` | float em `[a, b)` |
| `Random.int_between(a, b)` | inteiro em `[a, b)` |
| `Random.bool()` | booleano aleatório |
| `Random.sample(list)` | elemento aleatório da lista |

---

### Strings — `stdlib/strings.noe`

| Função | Descrição |
|--------|-----------|
| `Strings.lower(s)` | minúsculas |
| `Strings.upper(s)` | maiúsculas |
| `Strings.trim(s)` | remove espaços nas pontas |
| `Strings.trim_left(s)` | remove à esquerda |
| `Strings.trim_right(s)` | remove à direita |
| `Strings.normalize(s)` | minúsculas + trim |
| `Strings.split(s, sep)` | divide por separador |
| `Strings.join(list, sep)` | une com separador |
| `Strings.replace(s, from, to)` | substitui ocorrências |
| `Strings.reverse(s)` | inverte string |
| `Strings.slice(s, start, end)` | substring |
| `Strings.lines(s)` | divide por quebras de linha |
| `Strings.words(s)` | divide por espaços |
| `Strings.starts_with(s, prefix)` | predicado de prefixo |
| `Strings.ends_with(s, suffix)` | predicado de sufixo |
| `Strings.contains(s, sub)` | predicado de substring |
| `Strings.blank(s)` | verdadeiro se vazio ou só espaços |
| `Strings.word_swap(s, map)` | substitui palavras conforme mapa |
| `Strings.repeat(s, n)` | repete string `n` vezes |
| `Strings.index_of(s, sub)` | posição da primeira ocorrência |
| `Strings.last_index_of(s, sub)` | posição da última ocorrência |
| `Strings.char_at(s, i)` | caractere na posição `i` |
| `Strings.ord(s)` | código Unicode do primeiro caractere |
| `Strings.chr(n)` | caractere a partir de código Unicode |
| `Strings.pad_left(s, n, ch?)` | preenche à esquerda até largura `n` |
| `Strings.pad_right(s, n, ch?)` | preenche à direita até largura `n` |
| `Strings.title_case(s)` | primeira letra de cada palavra maiúscula |

---

## Coleções e programação funcional

### List — `stdlib/lists.noe`

| Função | Descrição |
|--------|-----------|
| `List.len(xs)` | comprimento |
| `List.get(xs, i)` | elemento no índice |
| `List.push(xs, v)` | adiciona ao final |
| `List.pop(xs)` | remove e retorna último |
| `List.map(xs, fn)` | transforma elementos |
| `List.filter(xs, fn)` | filtra elementos |
| `List.reduce(xs, fn, init)` | reduz a um valor |
| `List.find(xs, fn)` | primeiro que satisfaz predicado |
| `List.any(xs, fn)` | algum satisfaz? |
| `List.all(xs, fn)` | todos satisfazem? |
| `List.sum(xs)` | soma numérica |
| `List.reverse(xs)` | inverte |
| `List.slice(xs, a, b)` | sublista `[a, b)` |
| `List.first(xs)` | primeiro elemento |
| `List.second(xs)` | segundo elemento |
| `List.last(xs)` | último elemento |
| `List.take(xs, n)` | primeiros `n` elementos |
| `List.drop(xs, n)` | sem os primeiros `n` |
| `List.append(a, b)` | concatena duas listas |
| `List.prepend(xs, v)` | adiciona no início |
| `List.concat(lists)` | concatena lista de listas |
| `List.flatten(xs)` | achata um nível |
| `List.count(xs, fn)` | conta elementos que satisfazem |
| `List.contains(xs, v)` | verifica se contém valor |
| `List.unique(xs)` | remove duplicatas |
| `List.zip(a, b)` | pares de elementos |
| `List.chunk(xs, n)` | divide em grupos de tamanho `n` |
| `List.pluck(xs, key)` | extrai campo de lista de mapas |
| `List.partition(xs, fn)` | divide em `[true_items, false_items]` |
| `List.group_by(xs, fn)` | agrupa por chave |
| `List.sort_numbers(xs)` | ordena numericamente |

Callbacks recebem `(value, index)`.

---

### Maps — `stdlib/maps.noe`

| Função | Descrição |
|--------|-----------|
| `Maps.keys(m)` | lista de chaves |
| `Maps.values(m)` | lista de valores |
| `Maps.get(m, key, fallback?)` | acesso seguro |
| `Maps.has(m, key)` | verifica chave |
| `Maps.clone(m)` | cópia rasa |
| `Maps.merge(a, b)` | une dois mapas (`b` sobrescreve `a`) |
| `Maps.entries(m)` | lista de `{key, value}` |
| `Maps.from_entries(list)` | mapa a partir de `{key, value}` |
| `Maps.pick(m, keys)` | subconjunto com as chaves indicadas |
| `Maps.omit(m, keys)` | mapa sem as chaves indicadas |
| `Maps.invert(m)` | troca chaves e valores |

---

### Fn — `stdlib/functional.noe`

| Função | Descrição |
|--------|-----------|
| `Fn.pipe(value, fns)` | aplica lista de funções em sequência |
| `Fn.compose(fns)` | retorna função composta |
| `Fn.tap(v, fn)` | chama `fn(v)` e retorna `v` |
| `Fn.once(fn)` | retorna versão que executa apenas uma vez |
| `Fn.memoize(fn)` | retorna versão com cache |
| `Fn.constantly(v)` | retorna função que sempre retorna `v` |

---

## Texto, diálogo e regras

### Patterns — `stdlib/patterns.noe`

Matching simbólico, wildcards e templates — base para chatbots e IA simbólica.

| Função | Descrição |
|--------|-----------|
| `Patterns.default_reflections()` | mapa padrão de reflexões (I→you, etc.) |
| `Patterns.normalize(text)` | normaliza texto para matching |
| `Patterns.match(text, pattern)` | retorna mapa de capturas ou null |
| `Patterns.fill(template, vars)` | substitui `$var` no template |
| `Patterns.captures(text, pattern)` | lista de grupos capturados |
| `Patterns.reflect(text, map)` | aplica reflexão pronominal |
| `Patterns.sort_rules(rules)` | ordena por especificidade |
| `Patterns.first_match(text, rules)` | primeira regra que casa |

Padrões usam `*` como wildcard. Capturas são acessadas pelo nome do wildcard.

**Exemplo:**

```noema
var m = Patterns.match("my name is Alice", "my name is *")
IO.puts(m["0"])   // "Alice"
```

---

### Dialogue — `stdlib/dialogue.noe`

Sessões conversacionais com memória, tópico e histórico.

| Função | Descrição |
|--------|-----------|
| `Dialogue.create(options?)` | cria sessão conversacional |
| `Dialogue.remember(session, key, value)` | armazena valor na memória |
| `Dialogue.recall(session, key, fallback?)` | recupera valor da memória |
| `Dialogue.push_memory(session, key, value)` | empilha valor na memória |
| `Dialogue.pop_memory(session, key)` | desempilha valor da memória |
| `Dialogue.set_topic(session, topic)` | define tópico atual |
| `Dialogue.record(session, speaker, text)` | registra fala no histórico |
| `Dialogue.last_message(session, speaker?)` | última mensagem (do speaker, se informado) |
| `Dialogue.rule(session, pattern, response)` | adiciona regra de resposta |
| `Dialogue.context(session)` | retorna contexto atual |
| `Dialogue.apply(session, input)` | aplica regras ao input e retorna resposta |
| `Dialogue.start(session, options?)` | inicia loop conversacional |

**Campos de `Dialogue.create(options)`:**

| Campo | Padrão | Descrição |
|-------|--------|-----------|
| `name` | `"agent"` | nome do agente |
| `topic` | `null` | tópico inicial |
| `memory` | `{}` | memória inicial |
| `fallback` | `null` | resposta padrão quando nenhuma regra casa |

---

### Rules — `stdlib/rules.noe`

Motor de fatos, inferência e certeza — base para sistemas especialistas.

| Função | Descrição |
|--------|-----------|
| `Rules.create(name)` | cria base de conhecimento |
| `Rules.set(kb, name, value)` | define fato |
| `Rules.get(kb, name, fallback?)` | recupera fato |
| `Rules.known(kb, name)` | verdadeiro se fato está definido |
| `Rules.clear(kb, names)` | remove fatos |
| `Rules.certainty(value)` | normaliza para `[-1, 1]` |
| `Rules.combine(a, b)` | combina dois fatores de certeza |
| `Rules.update(kb, name, value)` | atualiza combinando com certeza existente |
| `Rules.present(kb, name, threshold?)` | fato acima do limiar positivo |
| `Rules.absent(kb, name, threshold?)` | fato abaixo do limiar negativo |
| `Rules.rule(kb, name, cond, action)` | adiciona regra de inferência |
| `Rules.conclude(name, cf)` | ação de conclusão padrão |
| `Rules.reset(kb)` | limpa todos os fatos |
| `Rules.infer(kb)` | executa ciclo de inferência |
| `Rules.trace(kb)` | executa inferência com log |
| `Rules.rank(kb, names)` | ordena fatos por certeza |
| `Rules.parse_certainty(text)` | parseia descrição verbal de certeza |
| `Rules.ask_certainty(prompt)` | pergunta certeza ao usuário |

**Fatores de certeza** são valores em `[-1, 1]`:

- `1.0` — certeza absoluta de que o fato é verdadeiro
- `0.0` — desconhecido
- `-1.0` — certeza absoluta de que o fato é falso

**Exemplo mínimo:**

```noema
var kb = Rules.create("diagnostico")

Rules.set(kb, "febre", 0.8)
Rules.set(kb, "tosse", 0.6)

Rules.rule(kb, "gripe",
    fn (kb, _) => Rules.present(kb, "febre") and Rules.present(kb, "tosse"),
    Rules.conclude("gripe", 0.7)
)

Rules.infer(kb)
IO.show("gripe", Rules.get(kb, "gripe"))
```

---

## Ordem recomendada de leitura

1. `stdlib/prelude.noe`
2. `stdlib/core.noe`
3. `stdlib/io.noe`
4. `stdlib/lists.noe`
5. `stdlib/maps.noe`
6. `stdlib/patterns.noe`
7. `stdlib/dialogue.noe`
8. `stdlib/rules.noe`
