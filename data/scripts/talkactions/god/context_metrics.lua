--[[
	World Context Metrics - GM Command for monitoring instanced hunts performance
	
	Commands:
	/ctxmetrics     - Show context system metrics
]]

local contextMetrics = TalkAction("/ctxmetrics")

function contextMetrics.onSay(player, words, param)
	logCommand(player, words, param)

	local pos = player:getPosition()
	
	-- Collect metrics from nearby area
	local spectators = Game.getSpectators(pos, true, false, 20, 20, 20, 20)
	
	local contextCounts = {}
	local playerCount = 0
	local monsterCount = 0
	local npcCount = 0
	
	for _, spec in ipairs(spectators) do
		local ctxId = spec:getWorldContextId()
		contextCounts[ctxId] = (contextCounts[ctxId] or 0) + 1
		
		if spec:isPlayer() then
			playerCount = playerCount + 1
		elseif spec:isMonster() then
			monsterCount = monsterCount + 1
		elseif spec:isNpc() then
			npcCount = npcCount + 1
		end
	end
	
	-- Count unique contexts
	local uniqueContexts = 0
	for _ in pairs(contextCounts) do
		uniqueContexts = uniqueContexts + 1
	end
	
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "========== Context Metrics ==========")
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "")
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "=== Nearby Creatures (40x40 area) ===")
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Total Creatures: " .. #spectators)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "  Players: " .. playerCount)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "  Monsters: " .. monsterCount)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "  NPCs: " .. npcCount)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "")
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "=== Context Distribution ===")
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Unique Contexts: " .. uniqueContexts)
	
	-- Show top contexts by creature count
	local sortedContexts = {}
	for ctxId, count in pairs(contextCounts) do
		table.insert(sortedContexts, {id = ctxId, count = count})
	end
	table.sort(sortedContexts, function(a, b) return a.count > b.count end)
	
	local shown = 0
	for _, ctx in ipairs(sortedContexts) do
		if shown >= 5 then break end
		local label = ctx.id == 0 and "GLOBAL" or tostring(ctx.id)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "  Context " .. label .. ": " .. ctx.count .. " creatures")
		shown = shown + 1
	end
	
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "")
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "=== Your Status ===")
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Your Context: " .. player:getWorldContextId())
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "=====================================")
	
	return true
end

contextMetrics:separator(" ")
contextMetrics:groupType("god")
contextMetrics:register()
