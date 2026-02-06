# Instanced Hunts - Key Changes Documentation

## Overview

This document describes the critical changes made to implement instanced hunts (world contexts) in the Canary OT Server + OTClient architecture. These changes allow players to exist in isolated "instances" where they don't see creatures, items, or effects from other contexts.

## Architecture Summary

```
┌─────────────────────────────────────────────────────────────────┐
│                         SERVER (Canary)                         │
├─────────────────────────────────────────────────────────────────┤
│  Context 0 (Global)  │  Context 1  │  Context 2  │  Context N  │
│  - All NPCs          │  - Player A │  - Player B │  - Player X │
│  - All Monsters      │  - Cloned   │  - Cloned   │  - Cloned   │
│  - Original Items    │    Items    │    Items    │    Items    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        CLIENT (OTClient)                        │
├─────────────────────────────────────────────────────────────────┤
│  - NO custom opcode needed (ghost-style mechanism)              │
│  - m_mapKnown stays TRUE (normal client state)                  │
│  - Receives standard remove/appear packets (same as /ghost)     │
│  - Only sees creatures/items from current context               │
└─────────────────────────────────────────────────────────────────┘
```

---

## Core Design Principle: Ghost-Style Context Switching

The context switch mechanism reuses the exact same packets as the `/ghost` GOD command:

1. **To disappear:** `sendRemoveTileThing` (opcode `0x6C`) — removes creature from tile
2. **To appear:** `sendCreatureAppear` (opcode `0x6A`) — adds creature to tile
3. **Map rebuild:** `sendMapDescription` (opcode `0x64`) — rebuilds visible map

**Why this works:** The `/ghost` command NEVER clears `knownCreatureSet` or resets
`m_mapKnown`. Creatures stay "known" (opcode `0x62` update), so sprites render immediately.
No custom packets, no engine changes, no client modifications needed.

**What DIDN'T work (and why):** A custom `0x39` context switch packet was tried initially.
It cleared `knownCreatureSet` and set `m_mapKnown = false`, which caused:
- Creatures sent as "new" (opcode `0x61`) instead of "update" (`0x62`)
- Sprites didn't render until player walked 1 tile
- Client entered an intermediate state that broke rendering

---

## Critical Changes - Server Side (Canary)

### 1. Creature Context ID (`src/creatures/creature.hpp`)

**Purpose:** Every creature has a context ID to determine visibility.

```cpp
// Member variables added to Creature class
uint32_t m_worldContextId = 0;  // 0 = global, 1+ = private instance
bool m_needsContextRefresh = false;  // True during context transition

// Methods added
uint32_t getWorldContextId() const { return m_worldContextId; }
void setWorldContextId(uint32_t id) { m_worldContextId = id; }
bool isInPrivateContext() const { return m_worldContextId != 0; }
bool needsContextRefresh() const { return m_needsContextRefresh; }
void setNeedsContextRefresh(bool value) { m_needsContextRefresh = value; }
```

### 2. Context Visibility Filter (`src/creatures/players/player.cpp`)

**Purpose:** Players can only see creatures in the same context.

```cpp
bool Player::canSeeCreature(const std::shared_ptr<Creature> &creature) const {
    if (creature.get() == this) {
        return true;
    }
    // ... existing checks ...
    
    // World Context System: creatures in different contexts are invisible
    if (!isInSameContext(creature)) {
        return false;
    }
    return true;
}

bool Player::isInSameContext(const std::shared_ptr<Creature> &creature) const {
    return m_worldContextId == creature->getWorldContextId();
}
```

### 3. Movement Packet Filtering (`src/server/network/protocol/protocolgame.cpp`)

**Purpose:** Prevent sending creature movements from other contexts. Handle race condition during context switch.

```cpp
void ProtocolGame::sendMoveCreature(...) {
    // CRITICAL: During context switch, block ALL creature movements except player
    // This prevents race condition where moves are queued before switch but sent after
    if (player->needsContextRefresh()) {
        if (creature != player) {
            return;  // Block all other creature movements
        }
        player->setNeedsContextRefresh(false);
        sendMapDescription(newPos);
        return;
    }
    
    // Normal context filtering (after transition complete)
    if (creature != player && creature->getWorldContextId() != player->getWorldContextId()) {
        return;
    }
    
    // ... rest of movement logic ...
}
```

### 4. Context Manager - Ghost-Style Switching (`src/game/world_context/context_manager.cpp`)

**Purpose:** Move players between contexts using the same mechanism as `/ghost`.

```cpp
void ContextManager::movePlayerToContext(player, targetContextId) {
    // PHASE 1: Collect all information BEFORE any changes
    //   - Old context spectators (need to stop seeing the player)
    //   - New context spectators (need to start seeing the player)
    //   - All creatures visible to switching player (need to be removed from client)
    
    // PHASE 2: Send REMOVE packets BEFORE context change
    //   2a. Remove all visible creatures from switching player's client
    //       (prevents cached sprite "clones" and lingering names)
    //   2b. Remove switching player from old context spectators' clients
    //       + POFF effect for spectators
    //   2c. Pre-calculate player's own stackpos (while context still matches client view)
    
    // PHASE 3: Switch context (SERVER-SIDE ONLY - no packet to client)
    //   player->setWorldContextId(targetContextId);
    //   NO sendContextSwitch! Client stays in normal state.
    
    // PHASE 4: Rebuild switching player's view (like a teleport)
    //   RemoveTileThing(self) → sendMapDescription → Avatar Appear effect
    //   Client processes this as a normal teleport (m_mapKnown stays true)
    
    // PHASE 5: Make switching player APPEAR for new context spectators
    //   EXACTLY like /ghost un-ghost: sendCreatureAppear(player, pos, true)
    //   NO forgetCreature → creature stays "known" → 0x62 update → instant render
    //   + Avatar Appear effect for spectators
}
```

### 5. Item Context Filtering (`src/game/game.cpp`, `src/map/map.cpp`)

**Purpose:** Items have context IDs and are only visible in their context.

```cpp
// Item attribute for context
ItemAttribute_t::WORLDCONTEXTID

// When adding items to tile, check context visibility
// When player picks up item, verify same context
// Loot operations respect context boundaries
```

---

## Client Side (OTClient)

### No Custom Packets Needed

The client requires **zero protocol changes** for context switching. All visibility
is handled server-side using standard game packets:

- `0x6C` (RemoveTileThing) — creature disappears
- `0x6A` (AddTileThing / CreatureAppear) — creature appears
- `0x64` (MapDescription) — full map rebuild

### Context-Aware Cache System (Optional Infrastructure)

The client has a context-aware tile caching system for potential future use:

```cpp
// map.h - Context cache infrastructure
uint32_t m_currentContextId = 0;
std::unordered_map<uint32_t, std::unique_ptr<ContextCache>> m_contextCaches;

void setCurrentContext(uint32_t contextId);
uint32_t getCurrentContext() const;
ContextCache* getActiveCache();
```

This infrastructure exists but is **not required** for the current ghost-style
context switching. It may be useful for future features like pre-caching
context tile data on the client side.

---

## Visual Effects

| Event | Who Sees | Effect |
|-------|----------|--------|
| Player enters context | Spectators in new context | `CONST_ME_AVATAR_APPEAR` (244) |
| Player enters context | Switching player | `CONST_ME_AVATAR_APPEAR` (244) |
| Player leaves context | Spectators in old context | `CONST_ME_POFF` (3) |

---

## Solved Problems

### The Race Condition Problem

**Problem:** When switching contexts, monster/NPC movements were approved before the switch but sent after, causing "creature not found" errors.

**Solution:** The `needsContextRefresh` flag blocks ALL creature movements (except player) during the transition window.

```
Before: [Monster moves] → [Approved] → [Buffer] → [Context Switch] → [Sent] → ERROR!
After:  [Monster moves] → [needsContextRefresh=true] → [BLOCKED] → OK
```

### The Phantom Player / Clone Problem

**Problem:** When switching contexts near other players, "static clones" of creatures appeared and lingered until walking out of view and back.

**Solution:** Phase 2a explicitly sends `sendRemoveTileThing` for ALL creatures visible to the switching player before changing context. This clears the client's cached sprites.

### The Sprite Not Rendering Problem

**Problem:** After context switch, player sprites appeared as names only — the sprite didn't render until walking 1 tile.

**Root Cause:** `sendContextSwitch(0x39)` cleared `knownCreatureSet` and set `m_mapKnown=false`. Creatures were sent as "new" (opcode `0x61`) instead of "update" (`0x62`), which didn't trigger immediate sprite rendering.

**Solution:** Removed `sendContextSwitch` entirely. Context change is server-side only. Creatures stay "known" and the client stays in normal state (`m_mapKnown=true`). This is the exact same mechanism used by the `/ghost` GOD command, which has always worked perfectly.

---

## File Reference

### Server (Canary)
| File | Changes |
|------|---------|
| `src/creatures/creature.hpp` | Context ID, refresh flag |
| `src/creatures/players/player.hpp` | isInSameContext method |
| `src/creatures/players/player.cpp` | canSeeCreature filter |
| `src/server/network/protocol/protocolgame.cpp` | sendMoveCreature context filter |
| `src/game/world_context/context_manager.cpp` | movePlayerToContext (ghost-style) |
| `src/game/game.cpp` | Item context filtering |
| `src/map/map.cpp` | Tile context awareness |
| `src/items/tile.cpp` | getItemsForContext, getStackposOfCreature |

### Client (OTClient)
| File | Changes |
|------|---------|
| `src/client/map.h` | Context cache infrastructure (optional) |
| `src/client/map.cpp` | setCurrentContext, getActiveCache (optional) |

---

## Key Lessons Learned

1. **Reuse existing game mechanics** — The `/ghost` command already solved creature appear/disappear perfectly. Custom packets were unnecessary and harmful.
2. **Don't clear knownCreatureSet** — Keeping creatures "known" ensures the client uses opcode `0x62` (update) which renders sprites immediately, unlike `0x61` (new) which has rendering delays.
3. **Don't touch m_mapKnown** — Setting `m_mapKnown=false` puts the client in an intermediate state that breaks creature rendering. Let the client stay in normal operating mode.
4. **Server-side only context changes** — The client doesn't need to know which context it's in. All filtering happens on the server.
5. **Async buffers cause race conditions** — Events queued before state change may be sent after. Use the `needsContextRefresh` flag to block them.
6. **Context filtering must happen at packet send** — Not just at event generation.

---

*Document created: 2026-01-28*
*Last updated: 2026-01-28*
