/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2024 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.com/
 */

#pragma once

#include "creatures/players/player.hpp"
#include "game/movement/position.hpp"
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

// Global context ID (shared world)
constexpr uint32_t GLOBAL_CONTEXT_ID = 0;

// Forward declarations
class Player;
class Creature;

/**
 * WorldContext - Represents an isolated world instance
 */
class WorldContext {
public:
	explicit WorldContext(uint32_t id, const std::shared_ptr<Player> &owner = nullptr);
	~WorldContext() = default;

	uint32_t getId() const { return m_id; }
	std::shared_ptr<Player> getOwner() const { return m_owner.lock(); }
	
private:
	uint32_t m_id;
	std::weak_ptr<Player> m_owner;
	int64_t m_creationTime;
	int64_t m_lastActiveTime;
};

/**
 * ContextManager - Manages all world contexts (Singleton)
 */
class ContextManager {
public:
	ContextManager();
	~ContextManager() = default;

	// Core operations
	uint32_t createContext(const std::shared_ptr<Player> &owner = nullptr);
	bool destroyContext(uint32_t contextId);
	std::shared_ptr<WorldContext> getContext(uint32_t contextId);
	bool contextExists(uint32_t contextId) const;
	
	// Player management
	void movePlayerToContext(const std::shared_ptr<Player> &player, uint32_t targetContextId);

	// Prewarm
	void prewarmContext(uint32_t contextId, const Position &centerPos);

	// System info
	bool isEnabled() const { return m_enabled; }
	void setEnabled(bool enabled) { m_enabled = enabled; }
	size_t getActiveContextCount() const { return m_activeCount.load(); }
	bool canCreateMoreContexts() const { return m_activeCount.load() < MAX_ACTIVE_CONTEXTS; }

private:
	static constexpr size_t MAX_ACTIVE_CONTEXTS = 2200;
	
	bool m_enabled = true;
	std::atomic<uint32_t> m_nextContextId { 1 };
	std::atomic<size_t> m_activeCount { 0 };
	std::atomic<size_t> m_lifetimeCreated { 0 };
	std::atomic<size_t> m_lifetimeDestroyed { 0 };

	mutable std::shared_mutex m_contextsMutex;
	std::unordered_map<uint32_t, std::shared_ptr<WorldContext>> m_contexts;
};

// Global singleton
inline ContextManager& g_contextManager() {
	static ContextManager instance;
	return instance;
}
