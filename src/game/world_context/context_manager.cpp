/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "context_manager.hpp"
#include "game/game.hpp"
#include "game/scheduling/dispatcher.hpp"
#include "game/world_context/tile_layer_manager.hpp"
#include "map/spectators.hpp"
#include "utils/tools.hpp"

#include <algorithm>

// ============================================================================
// WorldContext
// ============================================================================

WorldContext::WorldContext(uint32_t id, const std::shared_ptr<Player> &owner) :
	m_id(id),
	m_owner(owner),
	m_creationTime(OTSYS_TIME()),
	m_lastActiveTime(OTSYS_TIME()) {
}

// ============================================================================
// ContextManager
// ============================================================================

ContextManager::ContextManager() {
	g_logger().info("[ContextManager] Initialized (max {} contexts)", MAX_ACTIVE_CONTEXTS);
}

uint32_t ContextManager::createContext(const std::shared_ptr<Player> &owner) {
	if (!m_enabled) {
		g_logger().debug("[ContextManager] Disabled - returning global context");
		return GLOBAL_CONTEXT_ID;
	}

	if (!canCreateMoreContexts()) {
		g_logger().warn("[ContextManager] Limit reached ({}) - cannot create new context", MAX_ACTIVE_CONTEXTS);
		return GLOBAL_CONTEXT_ID;
	}

	uint32_t newId = m_nextContextId.fetch_add(1);
	auto newContext = std::make_shared<WorldContext>(newId, owner);

	{
		std::unique_lock<std::shared_mutex> lock(m_contextsMutex);
		m_contexts[newId] = newContext;
	}

	m_activeCount.fetch_add(1);
	m_lifetimeCreated.fetch_add(1);

	std::string ownerName = owner ? owner->getName() : "none";
	g_logger().info("[ContextManager] Created context {} (owner: '{}', active: {})", 
		newId, ownerName, m_activeCount.load());
	
	// NOTE: Pre-warm disabled - using fallback system instead
	// The fallback in getItemsForContext shows context 0 items as base map
	// This avoids desync between cloned items and real tile items
	// if (owner) {
	// 	prewarmContext(newId, owner->getPosition());
	// }

	return newId;
}

void ContextManager::prewarmContext(uint32_t contextId, const Position &centerPos) {
	// Clone ALL items from tiles - complete map snapshot for the context
	constexpr int RADIUS = 20; // 41x41 tiles per floor (increased from 10)
	constexpr int FLOOR_RANGE = 2; // Clone 2 floors above and below
	
	g_logger().info("[ContextManager] Pre-warming context {} at ({}, {}, {}) radius {} (3 floors)",
		contextId, centerPos.x, centerPos.y, centerPos.z, RADIUS);
	
	int tilesFound = 0;
	int itemsCloned = 0;
	int floorsProcessed = 0;
	
	// Loop through 3 floors: current floor, floor below, floor above
	for (int dz = -FLOOR_RANGE; dz <= FLOOR_RANGE; ++dz) {
		int currentFloor = centerPos.z + dz;
		
		// Skip invalid floors (0-15 are valid in Tibia)
		if (currentFloor < 0 || currentFloor > 15) {
			continue;
		}
		
		floorsProcessed++;
		int tilesThisFloor = 0;
		
		for (int dx = -RADIUS; dx <= RADIUS; ++dx) {
			for (int dy = -RADIUS; dy <= RADIUS; ++dy) {
				Position pos(centerPos.x + dx, centerPos.y + dy, currentFloor);
				
				auto tile = g_game().map.getTile(pos);
				if (!tile) {
					continue;
				}
				
				tilesFound++;
				tilesThisFloor++;
				
				// Create layer for this tile
				auto layer = TileLayerManager::getInstance().getOrCreateLayer(pos, contextId);
				if (!layer) {
					continue;
				}
				
				// Clone ground
				if (auto ground = tile->getGround()) {
					auto clone = ground->clone();
					clone->setWorldContextId(contextId);
					layer->clonedItems.push_back(clone);
					itemsCloned++;
				}
				
				// Clone ALL items from the tile (complete snapshot)
				const auto& items = tile->getItemList();
				if (items) {
					for (const auto& item : *items) {
						// Clone all map items (context 0)
						if (item->getWorldContextId() == 0) {
							auto clone = item->clone();
							clone->setWorldContextId(contextId);
							layer->clonedItems.push_back(clone);
							itemsCloned++;
						}
					}
				}
			}
		}
		
		g_logger().info("[ContextManager] Floor {} (Z={}): {} tiles processed",
			dz == -1 ? "below" : (dz == 0 ? "current" : "above"), currentFloor, tilesThisFloor);
	}
	
	g_logger().info("[ContextManager] Pre-warm complete: {} floors, {} tiles found, {} items cloned for context {}",
		floorsProcessed, tilesFound, itemsCloned, contextId);
}

bool ContextManager::destroyContext(uint32_t contextId) {
	if (contextId == GLOBAL_CONTEXT_ID) {
		g_logger().warn("[ContextManager] Cannot destroy global context");
		return false;
	}

	std::shared_ptr<WorldContext> context;
	{
		std::unique_lock<std::shared_mutex> lock(m_contextsMutex);
		auto it = m_contexts.find(contextId);
		if (it == m_contexts.end()) {
			g_logger().warn("[ContextManager] Context {} not found", contextId);
			return false;
		}
		context = it->second;
		m_contexts.erase(it);
	}

	m_activeCount.fetch_sub(1);
	m_lifetimeDestroyed.fetch_add(1);

	g_logger().info("[ContextManager] Destroyed context {} (active: {})", contextId, m_activeCount.load());
	return true;
}

std::shared_ptr<WorldContext> ContextManager::getContext(uint32_t contextId) {
	if (contextId == GLOBAL_CONTEXT_ID) {
		return nullptr;
	}

	std::shared_lock<std::shared_mutex> lock(m_contextsMutex);
	auto it = m_contexts.find(contextId);
	return (it != m_contexts.end()) ? it->second : nullptr;
}

bool ContextManager::contextExists(uint32_t contextId) const {
	if (contextId == GLOBAL_CONTEXT_ID) {
		return true;
	}

	std::shared_lock<std::shared_mutex> lock(m_contextsMutex);
	return m_contexts.find(contextId) != m_contexts.end();
}

void ContextManager::movePlayerToContext(const std::shared_ptr<Player> &player, uint32_t targetContextId) {
	if (!player) {
		return;
	}

	uint32_t currentContextId = player->getWorldContextId();
	if (currentContextId == targetContextId) {
		return;
	}

	g_logger().info("[Context] Player '{}': {} -> {}", player->getName(), currentContextId, targetContextId);

	Position originalPos = player->getPosition();
	auto playerTile = player->getTile();
	if (!playerTile) {
		return;
	}

	// ========================================================================
	// PHASE 1: Collect all information BEFORE any changes
	// ========================================================================

	// 1a. Find players in old context (they need to stop seeing the switching player)
	//     and players in new context (they need to start seeing the switching player)
	struct SpectatorRemoveInfo {
		std::shared_ptr<Player> spectator;
		int32_t stackpos;
	};
	std::vector<SpectatorRemoveInfo> oldContextPlayerRemoves;
	std::vector<std::shared_ptr<Player>> newContextSpectators;

	auto allPlayerSpectators = Spectators().find<Player>(originalPos, true);
	for (const auto &spectator : allPlayerSpectators) {
		if (spectator == player) continue;
		auto specPlayer = spectator->getPlayer();
		if (!specPlayer) continue;

		if (specPlayer->getWorldContextId() == currentContextId) {
			int32_t stackpos = playerTile->getStackposOfCreature(specPlayer, player);
			oldContextPlayerRemoves.push_back({specPlayer, stackpos});
			g_logger().info("[Context] Old spectator {} sees {} at stackpos {}",
				specPlayer->getName(), player->getName(), stackpos);
		} else if (specPlayer->getWorldContextId() == targetContextId) {
			newContextSpectators.push_back(specPlayer);
		}
	}

	// 1b. Find ALL creatures visible to the switching player in their current context
	//     These need to be explicitly removed from the player's CLIENT before switching
	//     Without this, the client keeps old creature sprites cached (causing "clones")
	struct CreatureOnTile {
		Position pos;
		int32_t stackpos;
	};
	std::vector<CreatureOnTile> creaturesToRemoveFromPlayer;

	auto allCreatures = Spectators().find<Creature>(originalPos, true);
	for (const auto &creature : allCreatures) {
		if (creature == player) continue;
		if (!player->canSeeCreature(creature)) continue;

		auto creatureTile = creature->getTile();
		if (!creatureTile) continue;

		int32_t stackpos = creatureTile->getStackposOfCreature(player, creature);
		if (stackpos >= 0) {
			creaturesToRemoveFromPlayer.push_back({creature->getPosition(), stackpos});
			g_logger().info("[Context] Will remove creature at pos={} stackpos={} from {}'s client",
				creature->getPosition().toString(), stackpos, player->getName());
		}
	}

	// Sort: same position -> remove highest stackpos first (so lower stackpos stays valid)
	std::sort(creaturesToRemoveFromPlayer.begin(), creaturesToRemoveFromPlayer.end(),
		[](const CreatureOnTile &a, const CreatureOnTile &b) {
			if (a.pos.x != b.pos.x) return a.pos.x < b.pos.x;
			if (a.pos.y != b.pos.y) return a.pos.y < b.pos.y;
			if (a.pos.z != b.pos.z) return a.pos.z < b.pos.z;
			return a.stackpos > b.stackpos; // Same pos: highest stackpos first
		});

	g_logger().info("[Context] Removing {} creatures from {}'s client, {} old spectators, {} new spectators",
		creaturesToRemoveFromPlayer.size(), player->getName(),
		oldContextPlayerRemoves.size(), newContextSpectators.size());

	// ========================================================================
	// PHASE 2: Send REMOVE packets BEFORE context change
	// ========================================================================

	// 2a. Remove all visible creatures from the SWITCHING PLAYER's client
	//     This is the key fix: prevents cached creature sprites ("clones" and "lingering names")
	for (const auto &info : creaturesToRemoveFromPlayer) {
		player->sendRemoveTileThing(info.pos, info.stackpos);
	}

	// 2b. Remove switching player from OLD context spectators' clients (with poff effect)
	for (const auto &info : oldContextPlayerRemoves) {
		if (info.stackpos >= 0) {
			info.spectator->sendMagicEffect(originalPos, CONST_ME_POFF);
			info.spectator->sendRemoveTileThing(originalPos, info.stackpos);
		}
	}

	// ========================================================================
	// PHASE 3: Switch context
	// ========================================================================
	player->setWorldContextId(targetContextId);

	// ========================================================================
	// PHASE 4: Rebuild switching player's view
	// ========================================================================

	// 4a. Send context switch packet (clears server-side knownCreatureSet)
	player->sendContextSwitch(targetContextId);

	// 4b. Send full map description (opcode 0x64) to rebuild the entire client map
	//     This replaces the old 1-tile teleport which only sent partial map updates
	//     Combined with m_mapKnown=false from sendContextSwitch, this forces full rebuild
	player->sendMapDescription(originalPos);

	// 4c. Avatar appear effect for the switching player (arriving in new context)
	player->sendMagicEffect(originalPos, CONST_ME_AVATAR_APPEAR);

	// ========================================================================
	// PHASE 5: Make switching player APPEAR for new context spectators
	// ========================================================================
	for (const auto &specPlayer : newContextSpectators) {
		g_logger().info("[Context] Sending creature appear to {} for {}",
			specPlayer->getName(), player->getName());
		// Must forget first so server sends full creature data (opcode 0x61), not update (0x62)
		specPlayer->forgetCreature(player);
		specPlayer->sendCreatureAppear(player, originalPos, false);
		specPlayer->sendMagicEffect(originalPos, CONST_ME_AVATAR_APPEAR);
	}
}
