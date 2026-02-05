# Instanced Hunts - Technical Roadmap

> **Projeto:** Sistema de Hunts Instanciadas para Canary OT Server  
> **Versão:** 1.0.0  
> **Data:** 2026-01-28  
> **Status:** Em Desenvolvimento

---

## Índice

1. [Visão Geral do Projeto](#1-visão-geral-do-projeto)
2. [Arquitetura Técnica](#2-arquitetura-técnica)
3. [Fases de Implementação](#3-fases-de-implementação)
4. [Estratégia de Testes](#4-estratégia-de-testes)
5. [Monitoramento e Métricas](#5-monitoramento-e-métricas)
6. [Critérios de Aceitação](#6-critérios-de-aceitação)
7. [Rollback e Contingência](#7-rollback-e-contingência)
8. [Checklist de Release](#8-checklist-de-release)

---

## 1. Visão Geral do Projeto

### 1.1 Objetivo
Implementar um sistema de "hunts instanciadas" onde cada player possui seu próprio contexto de mundo (worldContext), permitindo:

- Players não verem outros players em sua instância
- Monstros independentes por player
- Respawn isolado
- Zero interação entre contextos diferentes

### 1.2 Princípios de Design

```
┌─────────────────────────────────────────────────────────────┐
│                    PRINCÍPIOS CORE                          │
├─────────────────────────────────────────────────────────────┤
│  1. Nada existe sem player                                  │
│  2. Nada roda sem necessidade                               │
│  3. Nada persiste sem dono                                  │
│  4. Fail-safe: servidor sobrevive, features degradam        │
└─────────────────────────────────────────────────────────────┘
```

### 1.3 Escopo MVP

| Incluído | Excluído |
|----------|----------|
| 1 player = 1 contexto | Party compartilhando contexto |
| Caves instanciadas específicas | PvP instanciado |
| Lazy spawn | Respawn perfeito (±2s OK) |
| Visibilidade isolada | Balanceamento fino |
| Hard caps ativos | Reuso de contexto |
| Métricas desde dia 1 | — |

---

## 2. Arquitetura Técnica

### 2.1 Novo Componente: WorldContext

```cpp
// Hierarquia de Contextos
┌─────────────────────────────────────────────────────────────┐
│                     ContextManager                          │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  contexts: HashMap<uint32_t, WorldContext>           │   │
│  │  - GLOBAL_CONTEXT (0) = mundo compartilhado          │   │
│  │  - Context 1..N = instâncias privadas                │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────┐
│                      WorldContext                           │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  id: uint32_t                                        │   │
│  │  owner: weak_ptr<Player>                             │   │
│  │  creatures: unordered_set<Creature*>                 │   │
│  │  spawnStates: map<spawnId, SpawnState>               │   │
│  │  createdAt: int64_t                                  │   │
│  │  lastActivity: int64_t                               │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Fluxo de Dados - Visibilidade

```
Player solicita visão
        │
        ▼
┌───────────────────┐
│ Spectators::find()│
└────────┬──────────┘
         │
         ▼
┌───────────────────────────────────────────────────────┐
│              getSpectators()                          │
│  ┌─────────────────────────────────────────────────┐ │
│  │  1. Calcular range de setores                   │ │
│  │  2. Para cada setor:                            │ │
│  │     - Iterar creature_list                      │ │
│  │     - NOVO: if (creature.contextId != caller)   │ │
│  │              continue; // pula                  │ │
│  │     - Verificar distância                       │ │
│  │  3. Retornar lista filtrada                     │ │
│  └─────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────┘
         │
         ▼
    CreatureVector (apenas do mesmo contexto)
```

### 2.3 Fluxo de Dados - Spawn

```
Player entra em área instanciada
        │
        ▼
┌───────────────────────────────────────────────────────┐
│              Zone::onPlayerEnter()                    │
│  ┌─────────────────────────────────────────────────┐ │
│  │  1. Verificar se zona é instanciada             │ │
│  │  2. Criar contexto se não existe                │ │
│  │  3. Atribuir contexto ao player                 │ │
│  │  4. Triggerar lazy spawn                        │ │
│  └─────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────┐
│              LazySpawnManager                         │
│  ┌─────────────────────────────────────────────────┐ │
│  │  1. Buscar spawns na área do player             │ │
│  │  2. Para cada spawn não ativo no contexto:      │ │
│  │     - Criar monstro                             │ │
│  │     - Atribuir contextId ao monstro             │ │
│  │     - Registrar no WorldContext                 │ │
│  └─────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────┘
```

---

## 3. Fases de Implementação

### Fase 0: Preparação (Atual)
- [x] Análise de código do Canary
- [x] Documentação técnica
- [ ] Setup de ambiente de testes
- [ ] Backup do código atual

### Fase 1: Spike - Prova de Conceito
**Duração estimada:** 2-3 dias

| Tarefa | Arquivos | Status |
|--------|----------|--------|
| Adicionar `worldContextId` em Creature | `creature.hpp/cpp` | ⬜ |
| Criar ContextManager básico | `context_manager.hpp/cpp` (novo) | ⬜ |
| Modificar getSpectators para filtrar | `spectators.cpp` | ⬜ |
| Comando GM `/context` para testes | `context.lua` (novo) | ⬜ |
| Testes manuais de visibilidade | — | ⬜ |

**Entregável:** Player pode alternar contextos e não ver outros players.

### Fase 2: Isolamento Completo
**Duração estimada:** 3-5 dias

| Tarefa | Arquivos | Status |
|--------|----------|--------|
| Filtrar monstros por contexto | `spectators.cpp` | ⬜ |
| Filtrar NPCs por contexto | `spectators.cpp` | ⬜ |
| Efeitos visuais por contexto | `game.cpp` | ⬜ |
| Animated text por contexto | `game.cpp` | ⬜ |
| Distance effects por contexto | `combat.cpp` | ⬜ |
| Testes automatizados básicos | `tests/` | ⬜ |

**Entregável:** Zero vazamento visual entre contextos.

### Fase 3: Combate Isolado
**Duração estimada:** 2-3 dias

| Tarefa | Arquivos | Status |
|--------|----------|--------|
| Validação de contexto em canDoCombat | `combat.cpp` | ⬜ |
| Validação de contexto em canTargetCreature | `combat.cpp` | ⬜ |
| Heals respeitam contexto | `game.cpp` | ⬜ |
| Summons herdam contexto do master | `creature.cpp` | ⬜ |
| Testes de combate cross-context | — | ⬜ |

**Entregável:** Impossível causar/receber dano entre contextos.

### Fase 4: Lazy Spawn
**Duração estimada:** 5-7 dias

| Tarefa | Arquivos | Status |
|--------|----------|--------|
| SpawnState por contexto | `spawn_monster.hpp/cpp` | ⬜ |
| Trigger de spawn on-demand | `spawn_monster.cpp` | ⬜ |
| Respawn por contexto | `spawn_monster.cpp` | ⬜ |
| Loop global de respawn | `game.cpp` | ⬜ |
| Cleanup de monstros órfãos | `context_manager.cpp` | ⬜ |
| Testes de spawn/respawn | — | ⬜ |

**Entregável:** Monstros só existem quando player está presente.

### Fase 5: Lifecycle e Cleanup
**Duração estimada:** 3-4 dias

| Tarefa | Arquivos | Status |
|--------|----------|--------|
| Criação automática de contexto | `protocolgame.cpp` | ⬜ |
| Timeout de contexto (60s) | `context_manager.cpp` | ⬜ |
| Destruição de contexto | `context_manager.cpp` | ⬜ |
| Reconexão ao mesmo contexto | `protocolgame.cpp` | ⬜ |
| Cleanup agressivo de recursos | `context_manager.cpp` | ⬜ |

**Entregável:** Contextos são gerenciados automaticamente.

### Fase 6: Hard Caps e Circuit Breakers
**Duração estimada:** 2-3 dias

| Tarefa | Arquivos | Status |
|--------|----------|--------|
| Limite máximo de contextos | `context_manager.cpp` | ⬜ |
| Limite de criaturas por contexto | `context_manager.cpp` | ⬜ |
| Limite de spawns por segundo | `spawn_monster.cpp` | ⬜ |
| Degradação graceful | `context_manager.cpp` | ⬜ |
| Alertas de saturação | `context_manager.cpp` | ⬜ |

**Entregável:** Servidor sobrevive sob carga extrema.

### Fase 7: Métricas e Instrumentação
**Duração estimada:** 2-3 dias

| Tarefa | Arquivos | Status |
|--------|----------|--------|
| Contador de contextos ativos | `metrics.cpp` | ⬜ |
| Contador de criaturas por contexto | `metrics.cpp` | ⬜ |
| Tempo médio de getSpectators | `metrics.cpp` | ⬜ |
| Respawns por segundo | `metrics.cpp` | ⬜ |
| Dashboard de métricas | `admin_panel/` | ⬜ |

**Entregável:** Visibilidade total do sistema.

### Fase 8: Otimização e Polish
**Duração estimada:** 5-7 dias

| Tarefa | Status |
|--------|--------|
| Profiling de hotspots | ⬜ |
| Otimização de cache | ⬜ |
| Otimização de memória | ⬜ |
| Testes de carga | ⬜ |
| Documentação final | ⬜ |

**Entregável:** Sistema pronto para produção.

---

## 4. Estratégia de Testes

### 4.1 Níveis de Teste

```
┌─────────────────────────────────────────────────────────────┐
│                    PIRÂMIDE DE TESTES                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│                      ┌─────────┐                            │
│                     /  E2E     \        5%                  │
│                    /  (Manual)  \                           │
│                   ───────────────                           │
│                  /   Integração  \      15%                 │
│                 /   (Automatizado)\                         │
│                ─────────────────────                        │
│               /    Testes Unitários  \   40%                │
│              /     (Automatizados)    \                     │
│             ───────────────────────────                     │
│            /     Smoke Tests (Rápidos)  \  40%              │
│           /       (Automatizados)        \                  │
│          ─────────────────────────────────                  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 Testes Manuais - Checklist por Fase

#### Fase 1: Spike
```
□ Player A em contexto 0 vê Player B em contexto 0
□ Player A em contexto 1 NÃO vê Player B em contexto 0
□ Player A em contexto 1 NÃO vê Player B em contexto 2
□ Comando /context cria novo contexto
□ Comando /context 0 retorna ao global
□ Servidor não crasha com 10 contextos simultâneos
□ Servidor não crasha com 50 contextos simultâneos
□ Log mostra criação/destruição de contextos
```

#### Fase 2: Isolamento
```
□ Monstro em contexto 0 invisível para player em contexto 1
□ Magia lançada em contexto 1 não aparece em contexto 0
□ Animated text de dano não vaza entre contextos
□ NPC em contexto 0 invisível para player em contexto 1
□ Projectile (distance effect) não vaza entre contextos
```

#### Fase 3: Combate
```
□ Player A (ctx 1) não consegue atacar Player B (ctx 0)
□ Player A (ctx 1) não consegue curar Player B (ctx 0)
□ Monstro (ctx 1) não ataca Player (ctx 0)
□ AoE spell não afeta criaturas de outro contexto
□ Summon herda contexto do master
□ Summon não ataca criaturas de outro contexto
```

#### Fase 4: Spawn
```
□ Monstro spawna quando player entra na área
□ Monstro não spawna se player não está na área
□ Respawn funciona corretamente no contexto
□ Monstro morre e respawna apenas para seu contexto
□ Dois players em contextos diferentes têm monstros separados
```

### 4.3 Testes Automatizados

#### Estrutura de Testes
```
tests/
├── unit/
│   ├── test_context_manager.cpp
│   ├── test_world_context.cpp
│   └── test_creature_context.cpp
├── integration/
│   ├── test_spectators_context.cpp
│   ├── test_combat_context.cpp
│   └── test_spawn_context.cpp
└── load/
    ├── test_many_contexts.cpp
    └── test_context_stress.cpp
```

#### Exemplo de Teste Unitário
```cpp
// tests/unit/test_context_manager.cpp
TEST(ContextManager, CreateContext) {
    auto& manager = ContextManager::getInstance();
    auto player = createMockPlayer();
    
    uint32_t contextId = manager.createContext(player);
    
    EXPECT_NE(contextId, ContextManager::GLOBAL_CONTEXT);
    EXPECT_TRUE(manager.contextExists(contextId));
}

TEST(ContextManager, DestroyContext) {
    auto& manager = ContextManager::getInstance();
    uint32_t contextId = manager.createContext(createMockPlayer());
    
    manager.destroyContext(contextId);
    
    EXPECT_FALSE(manager.contextExists(contextId));
}

TEST(ContextManager, MaxContextsLimit) {
    auto& manager = ContextManager::getInstance();
    
    for (int i = 0; i < ContextManager::MAX_CONTEXTS; i++) {
        manager.createContext(createMockPlayer());
    }
    
    // Deve falhar ou retornar GLOBAL_CONTEXT
    uint32_t overflow = manager.createContext(createMockPlayer());
    EXPECT_EQ(overflow, ContextManager::GLOBAL_CONTEXT);
}
```

#### Exemplo de Teste de Integração
```cpp
// tests/integration/test_spectators_context.cpp
TEST(SpectatorsContext, FilterByContext) {
    auto player1 = createPlayerAtPosition({100, 100, 7});
    auto player2 = createPlayerAtPosition({101, 100, 7});
    
    player1->setWorldContextId(1);
    player2->setWorldContextId(2);
    
    auto spectators = Spectators()
        .find<Player>(player1->getPosition(), false, 0, 0, 0, 0, player1->getWorldContextId());
    
    EXPECT_FALSE(spectators.contains(player2));
    EXPECT_TRUE(spectators.contains(player1));
}
```

### 4.4 Testes de Carga

#### Cenários de Stress
```
┌─────────────────────────────────────────────────────────────┐
│                 CENÁRIOS DE STRESS TEST                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Cenário 1: Muitos Contextos                                │
│  - 500 players, cada um em seu contexto                     │
│  - Medir: RAM, CPU, tempo de resposta                       │
│  - Critério: < 2GB RAM, < 50% CPU single core               │
│                                                             │
│  Cenário 2: Contexto Denso                                  │
│  - 1 contexto com 50 criaturas ativas                       │
│  - Player movendo-se rapidamente                            │
│  - Medir: getSpectators latency                             │
│  - Critério: < 1ms por chamada                              │
│                                                             │
│  Cenário 3: Spawn Storm                                     │
│  - 100 players entrando em área instanciada simultaneamente │
│  - Medir: spawns/segundo, latência                          │
│  - Critério: < 200 spawns/segundo, < 100ms latência         │
│                                                             │
│  Cenário 4: Context Churn                                   │
│  - Players entrando e saindo rapidamente                    │
│  - Medir: criação/destruição de contextos                   │
│  - Critério: Sem memory leaks após 1 hora                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Script de Load Test
```lua
-- data/scripts/testing/load_test_contexts.lua
local LoadTest = {}

function LoadTest.runContextStress(numContexts, duration)
    local startTime = os.time()
    local contexts = {}
    
    -- Criar contextos
    for i = 1, numContexts do
        local ctx = ContextManager.createContext(nil) -- mock
        table.insert(contexts, ctx)
    end
    
    print(string.format("[LoadTest] Created %d contexts", #contexts))
    
    -- Simular atividade
    while os.time() - startTime < duration do
        for _, ctx in ipairs(contexts) do
            -- Simular getSpectators
            local start = os.clock()
            Spectators():find(Position(1000, 1000, 7), false, 0, 0, 0, 0, ctx)
            local elapsed = (os.clock() - start) * 1000
            
            if elapsed > 1 then
                print(string.format("[LoadTest] WARN: getSpectators took %.2fms", elapsed))
            end
        end
    end
    
    -- Cleanup
    for _, ctx in ipairs(contexts) do
        ContextManager.destroyContext(ctx)
    end
    
    print("[LoadTest] Complete")
end

return LoadTest
```

---

## 5. Monitoramento e Métricas

### 5.1 Métricas Core

| Métrica | Descrição | Threshold Warning | Threshold Critical |
|---------|-----------|-------------------|-------------------|
| `contexts.active` | Contextos ativos | > 1500 | > 2000 |
| `contexts.created_per_min` | Taxa de criação | > 100/min | > 200/min |
| `contexts.destroyed_per_min` | Taxa de destruição | > 100/min | > 200/min |
| `creatures.per_context.avg` | Média de criaturas | > 30 | > 50 |
| `creatures.per_context.max` | Máximo de criaturas | > 50 | > 100 |
| `spectators.latency_ms.p99` | Latência P99 | > 0.5ms | > 1ms |
| `spawn.per_second` | Spawns por segundo | > 100 | > 200 |
| `memory.contexts_mb` | RAM dos contextos | > 300MB | > 500MB |

### 5.2 Visualização de Overhead - Opções

#### Opção 1: Gerenciador de Tarefas do Windows (Básico)
```
┌─────────────────────────────────────────────────────────────┐
│                 TASK MANAGER - LIMITAÇÕES                   │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ✅ O que PODE ver:                                         │
│     - CPU total do processo canary.exe                      │
│     - RAM total do processo                                 │
│     - Threads do processo                                   │
│     - I/O de disco e rede                                   │
│                                                             │
│  ❌ O que NÃO PODE ver:                                     │
│     - CPU por função específica                             │
│     - RAM por sistema (contextos vs mapa vs etc)            │
│     - Latência de funções                                   │
│     - Métricas customizadas do jogo                         │
│                                                             │
│  📊 Útil para: Visão geral de saúde do processo             │
│  📊 Não útil para: Debugging de performance específico      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Opção 2: Métricas Internas + Log (Recomendado para Dev)
```cpp
// src/utils/metrics.hpp
class InstanceMetrics {
public:
    static void recordContextCreated();
    static void recordContextDestroyed();
    static void recordSpectatorLatency(double ms);
    static void recordSpawnEvent();
    
    static void logSnapshot() {
        g_logger().info(
            "[Metrics] Contexts: {} | Creatures: {} | "
            "Spectators P99: {:.2f}ms | Spawns/s: {}",
            activeContexts.load(),
            totalCreaturesInContexts.load(),
            spectatorLatencyP99.load(),
            spawnsLastSecond.load()
        );
    }
    
private:
    static std::atomic<uint32_t> activeContexts;
    static std::atomic<uint32_t> totalCreaturesInContexts;
    static std::atomic<double> spectatorLatencyP99;
    static std::atomic<uint32_t> spawnsLastSecond;
};
```

#### Opção 3: Prometheus + Grafana (Produção)
```yaml
# docker-compose.monitoring.yml
version: '3.8'
services:
  prometheus:
    image: prom/prometheus
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
    ports:
      - "9090:9090"
      
  grafana:
    image: grafana/grafana
    ports:
      - "3000:3000"
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
```

```cpp
// Integração com Prometheus
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

class PrometheusMetrics {
public:
    void init() {
        exposer = std::make_unique<prometheus::Exposer>("0.0.0.0:8080");
        registry = std::make_shared<prometheus::Registry>();
        
        auto& context_family = prometheus::BuildGauge()
            .Name("canary_contexts_active")
            .Help("Number of active world contexts")
            .Register(*registry);
            
        contexts_gauge = &context_family.Add({});
        exposer->RegisterCollectable(registry);
    }
    
    void updateContexts(int count) {
        contexts_gauge->Set(count);
    }
    
private:
    std::unique_ptr<prometheus::Exposer> exposer;
    std::shared_ptr<prometheus::Registry> registry;
    prometheus::Gauge* contexts_gauge;
};
```

#### Opção 4: Comando GM In-Game (Dev Friendly)
```lua
-- /metrics - Mostra métricas no console do jogo
local metricsTalk = TalkAction("/metrics")

function metricsTalk.onSay(player, words, param)
    local metrics = ContextManager.getMetrics()
    
    local msg = string.format([[
========== INSTANCE METRICS ==========
Active Contexts:     %d / %d
Total Creatures:     %d
Memory Usage:        %.2f MB
Spectators Latency:  %.3f ms (P99)
Spawns/second:       %d
Context Uptime Avg:  %.1f min
======================================
    ]], 
        metrics.activeContexts,
        metrics.maxContexts,
        metrics.totalCreatures,
        metrics.memoryMB,
        metrics.spectatorLatencyP99,
        metrics.spawnsPerSecond,
        metrics.avgContextUptimeMinutes
    )
    
    player:sendTextMessage(MESSAGE_STATUS_CONSOLE_BLUE, msg)
    return true
end

metricsTalk:groupType("god")
metricsTalk:register()
```

### 5.3 Profiling Detalhado

#### Visual Studio Profiler (Windows)
```
┌─────────────────────────────────────────────────────────────┐
│              VISUAL STUDIO PERFORMANCE PROFILER             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. Debug → Performance Profiler                            │
│  2. Selecionar:                                             │
│     ☑ CPU Usage                                             │
│     ☑ Memory Usage                                          │
│     ☐ .NET Object Allocation (não aplicável)                │
│  3. Attach to Process → canary.exe                          │
│  4. Start                                                   │
│  5. Executar cenário de teste                               │
│  6. Stop Collection                                         │
│                                                             │
│  Resultado: Call tree com % de CPU por função               │
│  Foco em: getSpectators, checkCreatures, spawn functions    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### Instrumentação Manual (Lightweight)
```cpp
// src/utils/scoped_timer.hpp
class ScopedTimer {
public:
    ScopedTimer(const char* name, double thresholdMs = 1.0)
        : name(name), threshold(thresholdMs), start(std::chrono::high_resolution_clock::now()) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        if (ms > threshold) {
            g_logger().warn("[PERF] {} took {:.3f}ms (threshold: {:.1f}ms)", 
                name, ms, threshold);
        }
        
        // Atualizar métricas
        InstanceMetrics::recordLatency(name, ms);
    }
    
private:
    const char* name;
    double threshold;
    std::chrono::high_resolution_clock::time_point start;
};

// Uso
CreatureVector Spectators::getSpectators(...) {
    ScopedTimer timer("getSpectators", 0.5);  // Warn se > 0.5ms
    // ... código
}
```

### 5.4 Dashboard de Monitoramento

```
┌─────────────────────────────────────────────────────────────┐
│                    CANARY INSTANCE DASHBOARD                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐           │
│  │  CONTEXTS   │ │  CREATURES  │ │   MEMORY    │           │
│  │    127      │ │    1,847    │ │   312 MB    │           │
│  │   ▲ +3/min  │ │  avg: 14.5  │ │   ▲ +2 MB   │           │
│  └─────────────┘ └─────────────┘ └─────────────┘           │
│                                                             │
│  LATENCY (getSpectators)                                    │
│  ────────────────────────────────────────────────           │
│  P50: 0.12ms  P90: 0.34ms  P99: 0.67ms  Max: 1.2ms         │
│                                                             │
│  SPAWN RATE                                                 │
│  ────────────────────────────────────────────────           │
│  Current: 23/s  Peak: 89/s  Limit: 200/s                   │
│  ████████░░░░░░░░░░░░ 45%                                   │
│                                                             │
│  CONTEXT HEALTH                                             │
│  ────────────────────────────────────────────────           │
│  ● Healthy: 125  ⚠ Warning: 2  ● Critical: 0               │
│                                                             │
│  RECENT EVENTS                                              │
│  ────────────────────────────────────────────────           │
│  [14:32:01] Context 127 created (Player: Lucas)             │
│  [14:31:45] Context 98 destroyed (timeout)                  │
│  [14:31:22] WARN: Spectators latency spike (1.8ms)          │
│  [14:30:55] Context 125 created (Player: Maria)             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 6. Critérios de Aceitação

### 6.1 Funcionalidade

| ID | Critério | Método de Verificação |
|----|----------|----------------------|
| F1 | Players em contextos diferentes não se veem | Teste manual com 2 clients |
| F2 | Monstros são únicos por contexto | Teste manual + log |
| F3 | Combate não cruza contextos | Teste manual |
| F4 | Contexto é criado automaticamente | Log de eventos |
| F5 | Contexto é destruído após timeout | Log + verificação de RAM |
| F6 | Player reconecta ao mesmo contexto | Teste manual de reconexão |

### 6.2 Performance

| ID | Critério | Valor Aceitável | Método de Verificação |
|----|----------|-----------------|----------------------|
| P1 | getSpectators latência P99 | < 1ms | Métricas internas |
| P2 | RAM por contexto | < 200KB | Profiler |
| P3 | CPU overhead por contexto | < 0.1% | CPU profiler |
| P4 | Spawns por segundo | > 100 | Métricas internas |
| P5 | Max contextos simultâneos | > 500 | Load test |

### 6.3 Estabilidade

| ID | Critério | Método de Verificação |
|----|----------|----------------------|
| S1 | Sem crashes após 24h de operação | Teste de longa duração |
| S2 | Sem memory leaks | Valgrind / AddressSanitizer |
| S3 | Graceful degradation sob carga | Teste de stress |
| S4 | Recovery após erro de contexto | Teste de falha injetada |

---

## 7. Rollback e Contingência

### 7.1 Estratégia de Feature Flag
```cpp
// config.lua
INSTANCED_HUNTS_ENABLED = true  -- Toggle master
INSTANCED_ZONES = {"cave_of_trials", "demon_forge"}  -- Zonas instanciadas

// src/game/context/context_manager.cpp
bool ContextManager::isEnabled() {
    return g_configManager().getBoolean(INSTANCED_HUNTS_ENABLED);
}

// Em qualquer lugar que usa contextos
if (!ContextManager::isEnabled()) {
    return GLOBAL_CONTEXT;  // Fallback para comportamento original
}
```

### 7.2 Plano de Rollback

```
┌─────────────────────────────────────────────────────────────┐
│                    ROLLBACK PROCEDURE                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  NÍVEL 1: Desabilitar Feature (Imediato)                    │
│  ─────────────────────────────────────────                  │
│  1. Editar config.lua: INSTANCED_HUNTS_ENABLED = false      │
│  2. /reload config                                          │
│  3. Todos players voltam ao contexto global                 │
│  4. Tempo: < 1 minuto                                       │
│                                                             │
│  NÍVEL 2: Rollback de Código (Médio)                        │
│  ─────────────────────────────────────────                  │
│  1. git revert HEAD~N (commits da feature)                  │
│  2. Recompilar                                              │
│  3. Reiniciar servidor                                      │
│  4. Tempo: ~15-30 minutos                                   │
│                                                             │
│  NÍVEL 3: Restore de Backup (Último Recurso)                │
│  ─────────────────────────────────────────                  │
│  1. Parar servidor                                          │
│  2. Restaurar binário de backup                             │
│  3. Restaurar database se necessário                        │
│  4. Iniciar servidor                                        │
│  5. Tempo: ~1 hora                                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 8. Checklist de Release

### 8.1 Pre-Release
```
□ Todos os testes unitários passando
□ Todos os testes de integração passando
□ Teste de carga executado com sucesso
□ Teste de 24h sem memory leaks
□ Code review completo
□ Documentação atualizada
□ Feature flag configurável
□ Rollback testado
□ Backup do estado atual
□ Comunicação para players (se necessário)
```

### 8.2 Release
```
□ Deploy em ambiente de staging
□ Smoke tests em staging
□ Deploy em produção (horário de baixo tráfego)
□ Monitoramento ativo por 2 horas
□ Verificar métricas de performance
□ Verificar logs de erro
```

### 8.3 Post-Release
```
□ Monitorar por 24h
□ Coletar feedback de players
□ Documentar issues encontrados
□ Planejar hotfixes se necessário
□ Retrospectiva técnica
```

---

## Apêndice A: Comandos de Desenvolvimento

```bash
# Compilar com debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Compilar com sanitizers (detecta memory leaks)
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make -j$(nproc)

# Rodar testes
./bin/canary_tests

# Profiling com perf (Linux)
perf record -g ./bin/canary
perf report

# Profiling com valgrind (memory)
valgrind --leak-check=full ./bin/canary
```

---

## Apêndice B: Glossário

| Termo | Definição |
|-------|-----------|
| WorldContext | Instância isolada do mundo para um player/grupo |
| Context ID | Identificador único de um contexto (0 = global) |
| Spectators | Criaturas visíveis a partir de uma posição |
| Lazy Spawn | Spawn de monstros apenas quando necessário |
| Circuit Breaker | Mecanismo que desativa features sob stress |

---

*Documento gerado em 2026-01-28. Última atualização: 2026-01-28*
