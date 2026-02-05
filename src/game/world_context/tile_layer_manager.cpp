/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019–present OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#include "game/world_context/tile_layer_manager.hpp"

#include "game/movement/position.hpp"
#include "items/item.hpp"
#include "lib/logging/logger.hpp"

// ============================================================================
// TileLayerManager
// ============================================================================

uint64_t TileLayerManager::makeKey(const Position& pos, uint32_t contextId) {
	// Combine position hash with contextId
	uint64_t posHash = (static_cast<uint64_t>(pos.x) << 32) | 
	                   (static_cast<uint64_t>(pos.y) << 16) | 
	                   static_cast<uint64_t>(pos.z);
	return posHash ^ (static_cast<uint64_t>(contextId) << 48);
}

InstanceLayer* TileLayerManager::getOrCreateLayer(const Position& pos, uint32_t contextId) {
	uint64_t key = makeKey(pos, contextId);
	
	// Try read lock first
	{
		std::shared_lock readLock(m_mutex);
		auto it = m_layers.find(key);
		if (it != m_layers.end()) {
			return it->second.get();
		}
	}
	
	// Need to create, use write lock
	std::unique_lock writeLock(m_mutex);
	
	// Double-check (another thread might have created it)
	auto it = m_layers.find(key);
	if (it != m_layers.end()) {
		return it->second.get();
	}
	
	// Create new layer
	auto layer = std::make_unique<InstanceLayer>(contextId);
	auto* ptr = layer.get();
	m_layers[key] = std::move(layer);
	
	g_logger().trace("[TileLayerManager] Created layer for context {} at {}",
		contextId, pos.toString());
	
	return ptr;
}

InstanceLayer* TileLayerManager::getLayer(const Position& pos, uint32_t contextId) const {
	std::shared_lock lock(m_mutex);
	uint64_t key = makeKey(pos, contextId);
	auto it = m_layers.find(key);
	return (it != m_layers.end()) ? it->second.get() : nullptr;
}

bool TileLayerManager::hasLayer(const Position& pos, uint32_t contextId) const {
	std::shared_lock lock(m_mutex);
	return m_layers.contains(makeKey(pos, contextId));
}

void TileLayerManager::removeContextLayers(uint32_t contextId) {
	std::unique_lock lock(m_mutex);
	
	// Remove all layers for this context
	for (auto it = m_layers.begin(); it != m_layers.end(); ) {
		if (it->second->contextId == contextId) {
			it = m_layers.erase(it);
		} else {
			++it;
		}
	}
	
	g_logger().debug("[TileLayerManager] Removed all layers for context {}", contextId);
}

std::vector<Position> TileLayerManager::getLayerPositions(uint32_t contextId) const {
	std::shared_lock lock(m_mutex);
	
	std::vector<Position> positions;
	for (const auto& [key, layer] : m_layers) {
		if (layer->contextId == contextId) {
			// Extract position from key (reverse of makeKey)
			uint32_t x = static_cast<uint32_t>(key >> 32);
			uint32_t y = static_cast<uint32_t>((key >> 16) & 0xFFFF);
			uint32_t z = static_cast<uint32_t>(key & 0xFFFF);
			positions.emplace_back(x, y, z);
		}
	}
	
	return positions;
}

void TileLayerManager::clear() {
	std::unique_lock lock(m_mutex);
	m_layers.clear();
	g_logger().info("[TileLayerManager] Cleared all layers");
}

TileLayerManager::LayerStats TileLayerManager::getStats() const {
	std::shared_lock lock(m_mutex);
	
	LayerStats stats;
	stats.totalLayers = m_layers.size();
	stats.totalClonedItems = 0;
	
	for (const auto& [key, layer] : m_layers) {
		stats.totalClonedItems += layer->clonedItems.size();
	}
	
	// Estimate: Each layer ~100 bytes + items
	stats.memoryEstimateBytes = stats.totalLayers * 100 + 
	                           stats.totalClonedItems * 200;  // ~200 bytes per item
	
	return stats;
}
