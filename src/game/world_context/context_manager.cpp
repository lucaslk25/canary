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
#include "utils/tools.hpp"

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
	
	// Pre-warm context with basic tiles (minimal approach)
	if (owner) {
		prewarmContext(newId, owner->getPosition());
	}

	return newId;
}

void ContextManager::prewarmContext(uint32_t contextId, const Position &centerPos) {
	// Baby step: Clone ground tiles using TileLayerManager
	// Multi-floor: Clone 3 floors (Z-1, Z, Z+1) to cover stairs/holes
	constexpr int RADIUS = 10; // Increased from 5 to 10 (21x21 tiles per floor)
	constexpr int FLOOR_RANGE = 1; // Clone 1 floor above and below
	
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
				
				// Clone ground and walls (critical items)
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
				
				// Clone walls/doors (items with BLOCKSOLID flag)
				const auto& items = tile->getItemList();
				if (items) {
					for (const auto& item : *items) {
						if (item->hasProperty(CONST_PROP_BLOCKSOLID) || 
						    item->hasProperty(CONST_PROP_BLOCKPATH)) {
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

	// 1. Mark player as needing refresh (will cause next moves to be sent as teleport)
	player->setNeedsContextRefresh(true);

	// 2. Update player's context ID
	player->setWorldContextId(targetContextId);
	
	// 3. Send context switch packet (clears client creatures)
	player->sendContextSwitch(targetContextId);
	
	// 4. Send fresh map description
	player->sendMapDescription(player->getPosition());
}
