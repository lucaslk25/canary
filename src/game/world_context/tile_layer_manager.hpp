/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019–present OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include "game/movement/position.hpp"

class Item;

/**
 * @brief Instance layer for a tile in a specific world context.
 * 
 * Represents modifications to a base tile for a specific instance:
 * - clonedItems: Items cloned from the base tile (with modified contextId)
 * - hiddenBaseItems: Set of item IDs from base tile to hide
 * 
 * This allows multiple instances to coexist without duplicating items
 * in the physical tile structure.
 */
struct InstanceLayer {
	uint32_t contextId;
	std::vector<std::shared_ptr<Item>> clonedItems;
	std::unordered_set<uint16_t> hiddenBaseItemIds;  // Item IDs to hide from base
	
	InstanceLayer(uint32_t ctx) : contextId(ctx) {}
	
	// Non-copyable
	InstanceLayer(const InstanceLayer&) = delete;
	InstanceLayer& operator=(const InstanceLayer&) = delete;
	
	// Movable
	InstanceLayer(InstanceLayer&&) = default;
	InstanceLayer& operator=(InstanceLayer&&) = default;
};

/**
 * @brief Manager for tile instance layers across all contexts.
 * 
 * Stores layers separately from tiles to avoid modifying Tile class extensively.
 * Uses Position as key to find layers for a specific tile + context combination.
 */
class TileLayerManager {
public:
	static TileLayerManager& getInstance() {
		static TileLayerManager instance;
		return instance;
	}
	
	~TileLayerManager() = default;
	
	// Non-copyable
	TileLayerManager(const TileLayerManager&) = delete;
	TileLayerManager& operator=(const TileLayerManager&) = delete;
	
	/**
	 * @brief Get or create a layer for a specific tile + context.
	 */
	InstanceLayer* getOrCreateLayer(const Position& pos, uint32_t contextId);
	
	/**
	 * @brief Get existing layer for a tile + context (null if doesn't exist).
	 */
	InstanceLayer* getLayer(const Position& pos, uint32_t contextId) const;
	
	/**
	 * @brief Check if a layer exists for a tile + context.
	 */
	bool hasLayer(const Position& pos, uint32_t contextId) const;
	
	/**
	 * @brief Remove all layers for a specific context.
	 * Called when context is destroyed.
	 */
	void removeContextLayers(uint32_t contextId);
	
	/**
	 * @brief Get all positions that have layers for a specific context.
	 * Used for cleanup when context is destroyed.
	 */
	std::vector<Position> getLayerPositions(uint32_t contextId) const;
	
	/**
	 * @brief Clear all layers (for testing/cleanup).
	 */
	void clear();
	
	/**
	 * @brief Get statistics about layer usage.
	 */
	struct LayerStats {
		size_t totalLayers;
		size_t totalClonedItems;
		size_t memoryEstimateBytes;
	};
	LayerStats getStats() const;

private:
	TileLayerManager() = default;
	
	// Key: hash(position, contextId)
	std::unordered_map<uint64_t, std::unique_ptr<InstanceLayer>> m_layers;
	mutable std::shared_mutex m_mutex;
	
	// Helper to create unique key from position + contextId
	static uint64_t makeKey(const class Position& pos, uint32_t contextId);
};

// Global accessor
inline TileLayerManager& g_tileLayerManager() {
	return TileLayerManager::getInstance();
}
