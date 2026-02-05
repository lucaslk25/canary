# 🎮 Análise de Engenharia: Sistema de Instâncias em OT Servers

## 📚 Contexto: Como OT Servers (Tibia) Funcionam

### Arquitetura Fundamental

**1. Single-Threaded Game Loop**
- OT Servers rodam em **uma thread principal** para lógica de jogo
- Tick rate: ~50-100ms
- **TUDO deve ser determinístico e síncrono**

**2. Map System**
- Map é **singleton global** carregado na memória
- Tiles são **shared** entre todos os players
- Items no mapa são **persistentes e únicos**

**3. Client-Server Protocol**
- Cliente pede tiles quando player se move
- Servidor envia `GetTileDescription()` com TODOS os items do tile
- **Cliente cacheia** as descrições recebidas

---

## 🔍 Problema Fundamental Identificado

### ❌ ERRO CRÍTICO: Duplicação de Items no Mesmo Tile

**O que está acontecendo:**

```
Tile Original (Map Global):
  - Ground: Stone floor (ctx=UINT32_MAX)
  - TopItem: Wall (ctx=UINT32_MAX)
  
Player entra em context 100:
  prewarmContext() → cloneCriticalItems()
    → wall->clone()
    → clone->setWorldContextId(100)
    → tile->internalAddThing(clone)  ❌ ADICIONA ao tile existente!

Tile Após Clonagem:
  - Ground: Stone floor (ctx=UINT32_MAX)
  - TopItem: Wall (ctx=UINT32_MAX)      ← Original
  - TopItem: Wall (ctx=100)             ← Clone DUPLICADO!
```

### 🚨 Consequências

**1. "There's no enough room"**
```cpp
// tile.cpp:721
if (!tileCreature->isInGhostMode()) {
    return RETURNVALUE_NOTENOUGHROOM;  ← Tile parece "cheio"
}
```

**Causa:** Tile tem items DUPLICADOS:
- Wall original (ctx=UINT32_MAX) → Player NÃO vê
- Wall clonado (ctx=100) → Player vê
- Mas **ambos ocupam espaço** no tile
- `hasFlag(TILESTATE_BLOCKSOLID)` retorna true DUAS VEZES
- Tile fica "trancado"

**2. Items Sumem Ao Voltar**

**Cenário:**
```
Player em spawn (ctx=100):
  1. prewarmContext() clona walls/doors → Tile tem 2x items
  2. Player sai → ContextSnapshot mantém "já clonado"
  3. Player volta → isTileCloned() = true → NÃO clona novamente
  4. Cliente pede GetTileDescription()
  5. Servidor filtra: "só envie items ctx=100"
  6. ❌ MAS OS CLONES FORAM REMOVIDOS! (garbage collected?)
```

**Hipótese:** Clones são **weak referenced** ou removidos quando player sai.

---

## 🎯 Arquitetura de Instâncias em MMOs

### Como Outros MMOs Fazem

**World of Warcraft / EverQuest / etc:**

```
Instância = CÓPIA COMPLETA DO MAPA
  - Map duplicado na memória
  - Items são NOVOS objetos
  - Tiles são NOVAS estruturas
  - ZERO compartilhamento com map global
```

**Vantagens:**
- ✅ Isolamento perfeito
- ✅ Sem conflitos
- ✅ Cada instância é independente

**Desvantagens:**
- ❌ Memória: 100MB por instância
- ❌ Não escala para 1000+ instâncias

### Tentativa Atual (Snapshot/Clone)

```
Instância = MESMOS TILES + Items Clonados
  - Map é singleton (compartilhado)
  - Tiles são compartilhados
  - Items são clonados no MESMO tile
  - ❌ CONFLITO: Multiple items no mesmo slot
```

---

## 🧠 Conceitos de Game Engine Design

### 1. Entity-Component-System (ECS)

**Problema:** OT Server usa **Object-Oriented** antiga:
- `Item` é um objeto completo
- `Tile` contém lista de `Item*`
- Não há separação clara de "dados" vs "lógica"

**Se fosse ECS:**
```cpp
struct ItemData {
    uint16_t id;
    uint32_t contextId;
    Position pos;
};

struct Tile {
    vector<ItemData> items;  // Dados puros
};

// Renderizar apenas items do contexto
for (auto& item : tile.items) {
    if (item.contextId == playerCtx) {
        send(item);
    }
}
```

### 2. Copy-on-Write (COW)

**Conceito:** Não clonar até modificar.

```cpp
struct Tile {
    shared_ptr<ItemList> items;  // Shared entre contextos
    
    void modifyItem() {
        if (items.use_count() > 1) {
            items = make_shared<ItemList>(*items);  // Clone on write
        }
        items->modify();
    }
};
```

**Problema com OT:** Items têm **estado mutável** (decay, transform, etc).

### 3. Layered Rendering

**Conceito:** Layers sobrepostos.

```
Layer 0 (Global): Map base
Layer 1 (Context 100): Modificações do contexto
Layer 2 (Context 100): Items dinâmicos

Render = Layer 0 + Layer 1 + Layer 2
```

**Em OT:** NÃO TEM layers, tudo é flat no tile.

---

## 🔬 Root Cause Analysis

### Pergunta Fundamental

**"Por que estamos clonando items NO MESMO TILE?"**

**Resposta:** Porque não temos **tile virtualization**.

### O Que Deveria Acontecer

**Opção A: Virtual Tiles (Copy-on-Access)**

```cpp
class VirtualTile {
    Tile* baseTile;  // Tile do map global
    unordered_map<uint32_t, vector<Item*>> contextOverrides;
    
    vector<Item*> getItems(uint32_t contextId) {
        if (contextOverrides.contains(contextId)) {
            return contextOverrides[contextId];  // Items do contexto
        }
        return baseTile->getItems();  // Items globais
    }
};
```

**Vantagens:**
- ✅ Tiles separados por contexto
- ✅ Sem duplicação no mesmo tile
- ✅ Isolamento perfeito

**Desvantagens:**
- ❌ Arquitetura completamente diferente
- ❌ 1000+ linhas de código para mudar

**Opção B: Eager Loading (Clone All Upfront)**

```cpp
void createContext(contextId) {
    // Clonar TODA a área de hunt de uma vez
    for (tile in huntArea) {
        // REMOVER items originais do tile para este contexto
        for (item in tile.items) {
            if (item.isMapItem()) {
                auto clone = item.clone();
                clone->setContextId(contextId);
                // Substituir, não adicionar
            }
        }
    }
}
```

**Vantagens:**
- ✅ Sem clonagem sob demanda
- ✅ Tudo pronto ao criar contexto

**Desvantagens:**
- ❌ Muita memória upfront
- ❌ Ainda tem problema de "substituir vs adicionar"

---

## 💥 Problemas Específicos Encontrados

### 1. "There's No Enough Room"

**Causa Raiz:**
```cpp
// tile.cpp:734 - Verificação de bloqueio
if (hasFlag(TILESTATE_BLOCKSOLID)) {
    return RETURNVALUE_NOTPOSSIBLE;
}
```

**O que acontece:**
1. Tile tem wall original (ctx=UINT32_MAX, blockSolid=true)
2. Clonamos wall (ctx=100, blockSolid=true)
3. **AMBAS** as walls estão no tile
4. `hasFlag()` verifica QUALQUER item no tile
5. Encontra **2 walls** bloqueando
6. Tile fica "super bloqueado"

**Solução necessária:**
- `hasFlag()` deve filtrar por contexto
- OU walls originais devem ser "escondidas" do contexto privado

### 2. Items Sumindo

**Fluxo problemático:**

```
T=0: Player entra context 100
     prewarmContext(spawnPos)
     → Clona walls no spawn
     → ContextSnapshot marca spawn como "clonado"

T=1: Player anda para área B
     queueAreaForCloning(areaB)
     → Clona walls na área B

T=2: Player volta para spawn
     queueAreaForCloning(spawn)
     → isTileCloned(spawn) = true
     → ❌ NÃO CLONA!
     
T=3: GetTileDescription(spawn)
     → Procura items com ctx=100
     → ❌ NENHUM ENCONTRADO!
     
     Por quê? Duas teorias:
     A) Clones foram garbage collected
     B) Clones nunca foram salvos corretamente
```

**Evidência:**
```cpp
// item.cpp:347 - Item::clone()
const auto &item = Item::CreateItem(id, count);
if (attributePtr) {
    item->attributePtr = std::make_unique<ItemAttribute>(*attributePtr);
}
```

**PROBLEMA:** `clone()` copia `attributePtr`, mas `WORLDCONTEXTID` é um **attribute**!

```cpp
// Depois de clonar:
clone->setWorldContextId(contextId);  // Define o attribute

// MAS: attributePtr foi copiado ANTES!
// O contextId pode estar inconsistente!
```

### 3. Bug de Animação no Telhado

**"Quando entro no templo (telhado) ele buga"**

**Teoria:**

Templos têm **múltiplos floors (Z levels)**:
```
Z=7: Telhado
Z=6: Interior
Z=5: Chão
```

**Problema:**
```cpp
// context_manager.cpp:543
for (int z = pos.z - 2; z <= pos.z + 2; z++) {
    auto tile = g_game().map.getTile(x, y, z);
    if (tile && !snapshot->isTileCloned(tile->getPosition())) {
        cloneCriticalItems(tile, contextId);
    }
}
```

**Se player está em Z=7 (telhado):**
- Loop clona Z=5, Z=6, Z=7, Z=8, Z=9
- **Z=6 (interior)** tem MUITOS items (doors, walls, furniture)
- **Z=5 (chão)** tem items do andar de baixo
- **Sobrecarga:** 3x mais items para clonar!

**Resultado:**
- Processamento > 10ms
- Timeout no `processQueuedCloning()`
- Requisições re-enfileiradas
- Fila cresce infinitamente
- Lag permanente

---

## 🎯 Soluções Reais (Game Dev Best Practices)

### Solução 1: **Tile Layering** (Recomendado para MMOs)

**Conceito:** Separar "base layer" de "instance layer"

```cpp
class LayeredTile {
    Tile* baseLayer;  // Map global (readonly)
    map<uint32_t, InstanceLayer*> instanceLayers;
    
    struct InstanceLayer {
        vector<Item*> addedItems;      // Items adicionados
        set<Item*> hiddenItems;        // Items do base escondidos
        uint32_t contextId;
    };
    
    vector<Item*> getVisibleItems(uint32_t contextId) {
        if (contextId == 0) {
            return baseLayer->getItems();  // Global: vê tudo
        }
        
        auto* layer = instanceLayers[contextId];
        vector<Item*> result;
        
        // Items do base que NÃO estão hidden
        for (auto item : baseLayer->getItems()) {
            if (!layer->hiddenItems.contains(item)) {
                result.push_back(item);
            }
        }
        
        // Items adicionados pelo contexto
        for (auto item : layer->addedItems) {
            result.push_back(item);
        }
        
        return result;
    }
};
```

**Vantagens:**
- ✅ Sem duplicação física
- ✅ Isolamento perfeito
- ✅ Base layer é readonly (thread-safe)

### Solução 2: **Spatial Hashing** (Usado em battle royales)

**Conceito:** Dividir mapa em chunks, clonar chunks

```cpp
struct Chunk {  // 32x32 tiles
    Position topLeft;
    vector<Tile> tiles;
};

struct InstancedChunk {
    Chunk* baseChunk;
    vector<Tile> clonedTiles;  // Clone completo
    uint32_t contextId;
};

// Ao criar instância: clonar apenas chunks necessários
void createInstance(contextId, huntArea) {
    for (chunk in huntArea.getChunks()) {
        auto instanceChunk = new InstancedChunk();
        instanceChunk->baseChunk = chunk;
        instanceChunk->clonedTiles = deepClone(chunk->tiles);
        
        // Modificar contextId de TODOS items nos tiles clonados
        for (tile in instanceChunk->clonedTiles) {
            for (item in tile.items) {
                item->setContextId(contextId);
            }
        }
    }
}
```

**Vantagens:**
- ✅ Isolamento total
- ✅ Memória controlada (só chunks necessários)
- ✅ Sem lazy loading complexo

### Solução 3: **Proxy Pattern** (Mais simples para OT)

**Conceito:** Wrapper inteligente que filtra items

```cpp
class ContextAwareTile {
    Tile* realTile;
    uint32_t playerContext;
    
    vector<Item*> getItems() {
        vector<Item*> filtered;
        for (auto item : realTile->getItems()) {
            if (shouldShowItem(item, playerContext)) {
                filtered.push_back(item);
            }
        }
        return filtered;
    }
    
    bool shouldShowItem(Item* item, uint32_t ctx) {
        if (ctx == 0) {
            // Global: vê items com ctx=0 ou ctx=UINT32_MAX
            return item->getContextId() == 0 || 
                   item->isVisibleToAllContexts();
        } else {
            // Private: vê APENAS items do próprio contexto
            return item->getContextId() == ctx;
        }
    }
    
    bool hasFlag(TileFlag flag) {
        // ❌ ERRO ATUAL: verifica TODOS items
        // ✅ CORRETO: verifica apenas items VISÍVEIS
        for (auto item : getItems()) {  // Filtrados!
            if (item->hasFlag(flag)) {
                return true;
            }
        }
        return false;
    }
};
```

---

## 📋 Recomendações Finais

### Curto Prazo (Fix Imediato)

**1. NÃO clonar items - usar FILTERING apenas**

Remover toda lógica de clonagem e usar approach diferente:

```cpp
// Em vez de clonar items, apenas MARCAR o contexto como "ativo"
// Todos veem os items do mapa, MAS:
// - Collision só com items/creatures do mesmo contexto
// - Visuals filtrados por contexto no protocolo

void createContext(contextId) {
    // NÃO clonar nada!
    // Apenas registrar que contexto existe
}

// No protocolo:
GetTileDescription() {
    // Enviar TODOS items, mas com flag de contexto
    // Cliente decide o que renderizar? NÃO, servidor filtra
    
    // Enviar apenas items "públicos" (mapas)
    // Items dinâmicos (corpses, drops) já têm contextId
}
```

**2. Fazer items do mapa serem "transparentes" para contextos**

```cpp
// Items com ctx=UINT32_MAX não bloqueiam movimento em contextos privados
bool Tile::hasFlag(TileFlag flag, uint32_t contextId) {
    for (auto item : items) {
        // Pular items do mapa se estamos em contexto privado
        if (contextId != 0 && item->isVisibleToAllContexts()) {
            continue;  // Não conta para blocking
        }
        
        if (item->hasFlag(flag)) {
            return true;
        }
    }
    return false;
}
```

### Médio Prazo (Refactor Arquitetural)

**Implementar Tile Layering (Solução 1)**

- 2-3 dias de trabalho
- Mudanças em `Tile`, `Map`, `ProtocolGame`
- Backward compatible

### Longo Prazo (Redesign)

**Chunk-based Instance System (Solução 2)**

- 1-2 semanas
- Reescrever map loading
- Melhor performance em geral

---

## 🎬 Conclusão

**O problema fundamental:**
- Estamos **adicionando** items clonados AO INVÉS de **substituir** os originais
- Tiles ficam com items DUPLICADOS
- Sistema de blocking/collision não filtra por contexto
- Cliente não recebe atualizações quando volta para área

**Por que não funciona:**
- OT Server não foi projetado para instâncias
- Arquitetura assume tiles únicos e compartilhados
- Clonagem causa state inconsistente

**Próximo passo:**
- Decidir entre "fix rápido" (filtering) ou "fix correto" (layering)
