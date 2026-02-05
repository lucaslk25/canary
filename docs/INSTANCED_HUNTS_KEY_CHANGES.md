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
│  - Receives context switch packet (0x39)                        │
│  - Resets m_mapKnown to force position sync                     │
│  - Receives fresh MapDescription with correct position          │
│  - Only sees creatures/items from current context               │
└─────────────────────────────────────────────────────────────────┘
```

---

## Critical Changes - Server Side (Canary)

### 1. Creature Context ID (`src/creatures/creature.hpp`)

**Purpose:** Every creature has a context ID to determine visibility.

```cpp
// Member variables added to Creature class
uint32_t m_worldContextId = 0;  // 0 = global, 1+ = private instance
bool m_needsContextRefresh = false;  // True after context switch

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
        // Player's first movement - send as full map refresh
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

### 4. Context Switch Packet (`src/server/network/protocol/protocolgame.cpp`)

**Purpose:** Send new opcode 0x39 to client to signal context change.

```cpp
void ProtocolGame::sendContextSwitch(uint32_t contextId) {
    // Clear known creatures - client will receive fresh data in MapDescription
    knownCreatureSet.clear();
    
    NetworkMessage msg;
    msg.addByte(0x39);  // New opcode for context switch
    msg.add<uint32_t>(contextId);
    writeToOutputBuffer(msg);
}
```

### 5. Context Manager (`src/game/world_context/context_manager.cpp`)

**Purpose:** Manage context lifecycle and player transitions.

```cpp
void ContextManager::movePlayerToContext(const std::shared_ptr<Player> &player, uint32_t targetContextId) {
    if (!player) return;
    
    uint32_t currentContextId = player->getWorldContextId();
    if (currentContextId == targetContextId) return;
    
    // 1. Mark player as needing refresh (blocks other creature movements)
    player->setNeedsContextRefresh(true);
    
    // 2. Update player's context ID
    player->setWorldContextId(targetContextId);
    
    // 3. Send context switch packet (triggers client reset)
    player->sendContextSwitch(targetContextId);
    
    // 4. Send fresh map description with correct position
    player->sendMapDescription(player->getPosition());
}
```

### 6. Item Context Filtering (`src/game/game.cpp`, `src/map/map.cpp`)

**Purpose:** Items have context IDs and are only visible in their context.

```cpp
// Item attribute for context
ItemAttribute_t::WORLDCONTEXTID

// When adding items to tile, check context visibility
// When player picks up item, verify same context
// Loot operations respect context boundaries
```

---

## Critical Changes - Client Side (OTClient)

### 1. Context Switch Packet Handler (`src/client/protocolgameparse.cpp`)

**Purpose:** Handle new opcode 0x39 and prepare for map refresh.

```cpp
void ProtocolGame::parseContextSwitch(const InputMessagePtr& msg)
{
    const uint32_t newContextId = msg->getU32();
    const uint32_t oldContextId = g_map.getCurrentContext();
    
    g_logger.info("[ContextSwitch] {} -> {}", oldContextId, newContextId);
    
    // Update context ID in map
    g_map.setCurrentContext(newContextId);
    
    // CRITICAL: Reset m_mapKnown so MapDescription will update player position
    // Without this, client keeps old position and movements desync
    m_mapKnown = false;
    
    // Lua callback for UI updates
    g_lua.callGlobalField("g_game", "onContextSwitch", oldContextId, newContextId);
}
```

### 2. Opcode Registration (`src/client/protocolgameparse.cpp`)

**Purpose:** Register handler for new opcode.

```cpp
case 0x39:
    parseContextSwitch(msg);
    break;
```

### 3. Map Context Tracking (`src/client/map.h`, `src/client/map.cpp`)

**Purpose:** Track current context for the client.

```cpp
// Member variable
uint32_t m_currentContext = 0;

// Methods
uint32_t getCurrentContext() const { return m_currentContext; }
void setCurrentContext(uint32_t contextId) { m_currentContext = contextId; }
```

### 4. getMappedThing Fallbacks (`src/client/protocolgameparse.cpp`)

**Purpose:** Handle edge cases when creature lookup fails during transition.

```cpp
ThingPtr ProtocolGame::getMappedThing(const InputMessagePtr& msg) const {
    // ... position parsing ...
    
    // Try exact stackpos first
    if (const auto& thing = g_map.getThing(pos, stackpos)) {
        return thing;
    }
    
    // Fallback: If localPlayer is at this position, return it
    if (m_localPlayer && m_localPlayer->getPosition() == pos) {
        return m_localPlayer;
    }
    
    // Try to find any creature at this position
    if (const auto& tile = g_map.getTile(pos)) {
        if (const auto& topCreature = tile->getTopCreature()) {
            return topCreature;
        }
    }
    
    return nullptr;
}
```

---

## Solved Problems

### The Race Condition Problem

**Problem:** When switching contexts, there was a race condition where monster/NPC movements were approved before the switch but sent after, causing "creature not found" errors.

**Solution:** The `needsContextRefresh` flag blocks ALL creature movements (except player) during the transition window.

```
Before: [Monster moves] → [Approved] → [Buffer] → [Context Switch] → [Sent] → ERROR!
After:  [Monster moves] → [needsContextRefresh=true] → [BLOCKED] → OK
```

### The Position Desync Problem

**Problem:** After context switch, client had wrong player position because `m_mapKnown = true` prevented position update in MapDescription.

**Solution:** Reset `m_mapKnown = false` in `parseContextSwitch` so the next MapDescription updates the player position.

---

## File Reference

### Server (Canary)
| File | Changes |
|------|---------|
| `src/creatures/creature.hpp` | Context ID, refresh flag |
| `src/creatures/players/player.hpp` | isInSameContext method |
| `src/creatures/players/player.cpp` | canSeeCreature filter |
| `src/server/network/protocol/protocolgame.cpp` | sendMoveCreature filter, sendContextSwitch |
| `src/game/world_context/context_manager.cpp` | movePlayerToContext logic |
| `src/game/game.cpp` | Item context filtering |
| `src/map/map.cpp` | Tile context awareness |

### Client (OTClient)
| File | Changes |
|------|---------|
| `src/client/protocolgameparse.cpp` | parseContextSwitch, m_mapKnown reset, getMappedThing fallbacks |
| `src/client/protocolgame.h` | Context switch method declaration |
| `src/client/map.h` | m_currentContext member |
| `src/client/map.cpp` | setCurrentContext, getCurrentContext |

---

## Key Lessons Learned

1. **Async buffers cause race conditions** - Events queued before state change may be sent after
2. **Client map state is critical** - `m_mapKnown` controls whether position is updated
3. **Creature lookup has fallbacks** - But they must be carefully controlled
4. **Context filtering must happen at packet send** - Not just at event generation
5. **Server and client must be in sync** - Both need to agree on player position

---

*Document created: 2026-01-28*
*Last updated: 2026-01-28*
