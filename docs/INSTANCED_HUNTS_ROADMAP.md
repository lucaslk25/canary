# Instanced Hunts - Roadmap & Next Steps

## Current Status

### What's Working
- [x] Context creation and switching
- [x] Player isolation between contexts (can't see each other)
- [x] Item context filtering (drop, pickup, loot)
- [x] Blood/effects context filtering
- [x] Monster/NPC chat context filtering
- [x] Player movement in pre-loaded area (smooth, no errors)
- [x] Position synchronization after context switch
- [x] Race condition fix (no more "creature not found" spam)

### Known Issues

| Issue | Description | Priority |
|-------|-------------|----------|
| NPCs invisible | NPCs are in context 0 (global), not visible in private contexts | High |
| Stairs/sewers broken | Action items (stairs, holes, ladders) don't work in contexts | High |
| Movement outside area | "Forced player position update" outside pre-loaded radius | Medium |
| Missing tiles | Some tiles missing in pre-loaded area | Medium |
| NPC collision | Player gets "forced update" when passing through invisible NPC | Low |

---

## Phase 1: Core Stability (Priority: Critical)

### 1.1 Fix Stairs/Sewers/Ladders

**Problem:** Action items (stairs, holes, ladders, rope spots) don't work in private contexts.

**Root Cause:** These items have `actionId` or `uniqueId` that trigger actions, but the cloned items may not preserve these IDs, or the action handlers don't account for context.

**Solution Options:**
1. **Clone action items with full attributes** - Ensure `actionId`/`uniqueId` are preserved during cloning
2. **Make action items context-agnostic** - Original items work for all contexts
3. **Register actions per context** - Clone the action registration too

**Files to Check:**
- `src/game/world_context/context_manager.cpp` - Item cloning logic
- `src/items/item.cpp` - Item attribute copying
- `src/lua/creature/actions.cpp` - Action handling

**Estimated Effort:** Medium

---

### 1.2 Fix NPC Visibility

**Problem:** NPCs are in context 0 and invisible in private contexts. Players see only the name floating.

**Solution Options:**

| Option | Pros | Cons |
|--------|------|------|
| **A) Clone NPCs per context** | Full isolation, NPCs can have different states per context | Memory overhead, complex state sync |
| **B) Make NPCs global** | Simple, low memory | NPCs shared between contexts, dialog conflicts |
| **C) Visual-only NPCs** | NPCs visible but interactions route to context 0 | Requires careful handling of NPC interactions |

**Recommended:** Option C for simplicity, with future migration to Option A.

**Implementation for Option C:**
1. In `canSeeCreature`, return `true` for NPCs regardless of context
2. In `sendMoveCreature`, allow NPC movements for all contexts
3. NPC interactions (dialog, trade) work normally since NPC is in context 0

**Files to Modify:**
- `src/creatures/players/player.cpp` - `canSeeCreature`
- `src/server/network/protocol/protocolgame.cpp` - `sendMoveCreature`

**Estimated Effort:** Low-Medium

---

### 1.3 Fix Pre-Load Area Completeness

**Problem:** Some tiles are missing in the pre-loaded area, causing visual gaps.

**Investigation Needed:**
1. Which tile types are missing? (ground, walls, items?)
2. Is the pre-warm radius sufficient?
3. Are multi-floor tiles being cloned correctly?

**Actions:**
1. Add logging to tile cloning to identify skipped tiles
2. Increase pre-warm radius from 10 to 15+
3. Audit the cloning logic for edge cases

**Files to Check:**
- `src/game/world_context/context_manager.cpp` - `preWarmContext`

**Estimated Effort:** Medium

---

## Phase 2: Polish & Edge Cases

### 2.1 Dynamic Area Loading

**Problem:** When player moves outside pre-loaded area, they get "forced position update" and movement breaks.

**Solution:** Implement dynamic tile cloning as player moves.

```cpp
// Pseudo-code
void onPlayerMove(Player* player, Position newPos) {
    if (player->isInPrivateContext()) {
        ensureTilesClonedAround(newPos, radius);
    }
}
```

**Considerations:**
- Performance impact of real-time cloning
- How to handle already-visible tiles (don't re-clone)
- Pre-fetch tiles in movement direction

**Estimated Effort:** High

---

### 2.2 Context Cleanup

**Problem:** Contexts are created but never destroyed, leading to memory leaks.

**Solution:**
1. Track active players per context
2. When last player leaves, schedule context for cleanup
3. After timeout (e.g., 5 minutes), destroy context and free memory

```cpp
void ContextManager::onPlayerLeaveContext(uint32_t contextId) {
    auto& ctx = m_contexts[contextId];
    ctx.playerCount--;
    if (ctx.playerCount == 0) {
        scheduleCleanup(contextId, 5 * 60 * 1000); // 5 minutes
    }
}
```

**Estimated Effort:** Medium

---

### 2.3 Respawn System

**Problem:** Monsters in context 0 respawn, but private contexts have no monsters.

**Solution Options:**
1. **Clone monsters at context creation** - Copy monster spawns to new context
2. **Independent spawn system** - Each context has its own spawn timers
3. **On-demand spawning** - Spawn monsters when player enters area

**Recommended:** Option 2 with spawn configuration per context.

**Estimated Effort:** High

---

## Phase 3: Features

### 3.1 Party Context Sharing

**Goal:** Players in the same party share a context.

**Implementation:**
1. When party leader creates context, all members join
2. New party members auto-join leader's context
3. If leader leaves, transfer context ownership

**Estimated Effort:** Medium

---

### 3.2 Context Time Limits

**Goal:** Contexts expire after a set duration.

**Implementation:**
1. Track context creation time
2. Schedule expiration event
3. Warn players before expiration
4. Teleport players out when expired

**Estimated Effort:** Low

---

### 3.3 Context Difficulty Scaling

**Goal:** Contexts can have different monster stats.

**Implementation:**
1. Context metadata includes difficulty multiplier
2. When spawning monsters in context, apply multiplier to HP/damage
3. Scale loot/exp accordingly

**Estimated Effort:** Medium

---

## Phase 4: Performance & Optimization

### 4.1 Lazy Tile Cloning

**Current:** All tiles in radius are cloned upfront.
**Goal:** Clone tiles only when player gets close.

**Benefits:**
- Faster context creation
- Lower memory usage
- Only clone what's needed

**Estimated Effort:** High

---

### 4.2 Context Pooling

**Goal:** Reuse context structures instead of creating/destroying.

**Implementation:**
1. Maintain pool of empty context slots
2. When creating context, grab from pool
3. When destroying, reset and return to pool

**Estimated Effort:** Low

---

### 4.3 Memory Optimization

**Goal:** Reduce memory per context.

**Strategies:**
1. Share immutable data (ground tiles, walls) between contexts
2. Only clone mutable items (chests, spawned items)
3. Use copy-on-write for tile data

**Estimated Effort:** High

---

### 4.4 Benchmark & Stress Test

**Goal:** Measure performance under load.

**Metrics to Track:**
- Memory per context
- CPU usage during context switch
- Network bandwidth per player in context
- Maximum concurrent contexts

**Estimated Effort:** Medium

---

## Migration Notes

When migrating to a newer architecture, the key concepts to preserve are:

1. **Entity Context ID** - Every creature/item has a `worldContextId`
2. **Visibility Filter** - `canSeeCreature` checks context match
3. **Packet Filter** - `sendMoveCreature` blocks cross-context packets
4. **Race Condition Guard** - `needsContextRefresh` flag during transition
5. **Client Position Sync** - Reset `m_mapKnown` on context switch

These patterns are architecture-agnostic and should work in any server/client implementation.

---

## Priority Order

1. **[Critical]** Fix stairs/sewers (blocks gameplay)
2. **[Critical]** Fix NPC visibility (blocks NPCs entirely)
3. **[High]** Fix pre-load completeness (visual issues)
4. **[High]** Dynamic area loading (blocks exploration)
5. **[Medium]** Context cleanup (memory leaks)
6. **[Medium]** Respawn system (no monsters to fight)
7. **[Low]** Party sharing, time limits, difficulty
8. **[Low]** Performance optimizations

---

*Document created: 2026-01-28*
*Last updated: 2026-01-28*
