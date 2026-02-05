# Instanced Hunts - Próximos Passos

> **Última atualização:** 2026-01-28  
> **Status:** Em Desenvolvimento

---

## Resumo do Progresso

### Já Implementado ✅

| Componente | Arquivo | Descrição |
|------------|---------|-----------|
| Context ID em Creature | `creature.hpp` | `m_worldContextId`, getters/setters |
| ContextManager | `context_manager.hpp/cpp` | Singleton, lifecycle, métricas |
| Comando /context | `context.lua` | Criar/trocar contextos |
| Comando /ctxmetrics | `context_metrics.lua` | Debug de métricas |
| Visibilidade | `player.cpp::canSeeCreature` | Players não veem outros contextos |
| Colisão | `player.cpp::canWalkthrough` | Atravessa criaturas de outro contexto |
| Tile Collision | `tile.cpp::queryAdd` | Monstros ignoram criaturas de outro contexto |
| Monster Targeting | `monster.cpp::selectTarget` | Monstros não atacam outro contexto |
| Combat HP | `game.cpp::combatChangeHealth` | Sem dano entre contextos |
| Combat MP | `game.cpp::combatChangeMana` | Sem dano de mana entre contextos |
| Creature Appear | `player.cpp::sendCreatureAppear` | Não envia se contexto diferente |
| Creature Move | `player.cpp::sendCreatureMove` | Não envia movimento se contexto diferente |

---

## Opção A: Lazy Spawn (Fase 4)

> **Objetivo:** Cada player tem seus próprios monstros que só existem quando estão na área.

### Tarefas

| # | Tarefa | Arquivo | Status |
|---|--------|---------|--------|
| A1 | SpawnState por contexto | `spawn_monster.hpp/cpp` | ⬜ |
| A2 | Trigger spawn on-demand | `spawn_monster.cpp` | ⬜ |
| A3 | Monstro herda contextId do spawn trigger | `spawn_monster.cpp` | ⬜ |
| A4 | Respawn por contexto | `spawn_monster.cpp` | ⬜ |
| A5 | Loop global verificar contextos ativos | `game.cpp` | ⬜ |
| A6 | Cleanup de monstros órfãos | `context_manager.cpp` | ⬜ |
| A7 | Zona instanciada trigger | `zone.cpp` | ⬜ |

### Complexidade
- **Alta** - Requer mudanças significativas no sistema de spawn
- Estimativa: 5-7 dias

---

## Opção B: Efeitos Visuais Completos (Fase 2 - Finalização)

> **Objetivo:** Zero vazamento visual entre contextos.

### Fase B1: Magic Effects ✅

Efeitos de magia (explosões, curas, etc) só visíveis no contexto do caster.

| # | Tarefa | Arquivo | Status |
|---|--------|---------|--------|
| B1.1 | Adicionar contextId ao addMagicEffect | `game.hpp/cpp` | ✅ |
| B1.2 | Filtrar spectators por contexto | `game.cpp` | ✅ |
| B1.3 | Propagar contextId do caster | `combat.cpp` | ✅ |

### Fase B2: Distance Effects (Projectiles) ✅

Projéteis (flechas, runas) só visíveis no contexto do atirador.

| # | Tarefa | Arquivo | Status |
|---|--------|---------|--------|
| B2.1 | Adicionar contextId ao addDistanceEffect | `game.hpp/cpp` | ✅ |
| B2.2 | Filtrar spectators por contexto | `game.cpp` | ✅ |
| B2.3 | Propagar contextId do caster | `combat.cpp` | ✅ |

### Fase B3: Animated Text (Dano/Cura) ✅

Números de dano/cura só visíveis no contexto do combate.

| # | Tarefa | Arquivo | Status |
|---|--------|---------|--------|
| B3.1 | Filtrar sendMessages por contexto | `game.cpp` | ✅ |
| B3.2 | Filtrar sendEffects por contexto | `game.cpp` | ✅ |

### Fase B4: Magic Walls e Fields ⬜

Paredes mágicas e campos só bloqueiam/afetam o contexto do criador.

| # | Tarefa | Arquivo | Status |
|---|--------|---------|--------|
| B4.1 | Adicionar contextId ao Item (field) | `item.hpp` | ⬜ |
| B4.2 | Setar contextId ao criar field | `combat.cpp` | ⬜ |
| B4.3 | Verificar contextId no tile blocking | `tile.cpp` | ⬜ |
| B4.4 | Verificar contextId no field damage | `tile.cpp` | ⬜ |

### Fase B5: Chat e Comunicação ⬜

Mensagens de chat só visíveis no mesmo contexto (último).

| # | Tarefa | Arquivo | Status |
|---|--------|---------|--------|
| B5.1 | Filtrar say/yell por contexto | `game.cpp` | ⬜ |
| B5.2 | Filtrar emotes por contexto | `game.cpp` | ⬜ |
| B5.3 | Manter channels globais funcionando | `game.cpp` | ⬜ |

### Complexidade
- **Média** - Mudanças pontuais em funções existentes
- Estimativa: 2-3 dias

---

## Ordem de Implementação Atual

1. ✅ ~~Fase 1: Spike~~
2. ✅ ~~Fase 2 Parcial: Isolamento de Criaturas~~
3. ✅ ~~Fase 3: Combate~~
4. ✅ ~~Fase B1: Magic Effects~~
5. ✅ ~~Fase B2: Distance Effects~~
6. ✅ ~~Fase B3: Animated Text~~
7. 🔄 **Fase B4: Magic Walls e Fields** ← PRÓXIMA
8. ⬜ Fase B5: Chat
9. ⬜ Fase 4 (Opção A): Lazy Spawn

---

## Notas de Implementação

### Padrão para Efeitos Visuais

```cpp
// ANTES (sem contexto)
void Game::addMagicEffect(const Position &pos, uint16_t effect) {
    auto spectators = Spectators().find<Player>(pos, true);
    for (const auto &spec : spectators) {
        spec->getPlayer()->sendMagicEffect(pos, effect);
    }
}

// DEPOIS (com contexto)
void Game::addMagicEffect(const Position &pos, uint16_t effect, uint32_t contextId = 0) {
    auto spectators = Spectators().find<Player>(pos, true);
    for (const auto &spec : spectators) {
        auto player = spec->getPlayer();
        // Só envia se mesmo contexto, ou se efeito é global (contextId = 0)
        if (contextId == 0 || player->getWorldContextId() == contextId) {
            player->sendMagicEffect(pos, effect);
        }
    }
}
```

### Regra de Ouro
- `contextId = 0` significa efeito global (visível para todos)
- `contextId > 0` significa efeito privado (só para aquele contexto)
