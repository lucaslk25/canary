--[[
	World Context System - GM Commands for Instanced Hunts
	
	Commands:
	/context          - Show current context and create a new one
	/context 0        - Return to global context (shared world)
	/context <id>     - Switch to specific context (for testing)
	/context info     - Show detailed context information
]]

local context = TalkAction("/context")

function context.onSay(player, words, param)
	-- Log command usage for GM commands
	if player:getGroup():getId() >= 3 then
		logCommand(player, words, param)
	end

	local currentContext = player:getWorldContextId()

	-- No parameter: show info and offer to create new context
	if param == "" then
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "========== World Context ==========")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Current Context ID: " .. currentContext)
		
		if currentContext == 0 then
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Status: GLOBAL (shared world)")
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Use '/context new' to create a private context")
		else
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Status: PRIVATE (instanced)")
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Use '/context 0' to return to global")
		end
		
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "====================================")
		return true
	end

	-- "new" command: create a new private context
	if param == "new" then
		-- If already in a private context, return to global first
		if currentContext ~= 0 then
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Already in context " .. currentContext)
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Returning to global context first...")
			player:setWorldContextId(0)
			-- Small delay to let client process
			addEvent(function()
				-- Now create new context
				local newContextId = player:createWorldContext()
				
				if not newContextId then
					player:sendTextMessage(MESSAGE_FAILURE, "Failed to create context (limit reached or disabled)")
					return
				end
				
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "========== Context Created ==========")
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "New Context ID: " .. newContextId)
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Status: You are now in a PRIVATE context")
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Other players in context 0 cannot see you!")
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Use '/context 0' to return to global")
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "=====================================")
			end, 500, player:getId())
			return true
		end
		
		-- Create context via ContextManager and switch player to it
		local newContextId = player:createWorldContext()
		
		if not newContextId then
			player:sendTextMessage(MESSAGE_FAILURE, "Failed to create context (limit reached or disabled)")
			return true
		end
		
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "========== Context Created ==========")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "New Context ID: " .. newContextId)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Status: You are now in a PRIVATE context")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Other players in context 0 cannot see you!")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Use '/context 0' to return to global")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "=====================================")
		
		return true
	end

	-- "info" command: show detailed info
	if param == "info" then
		local pos = player:getPosition()
		
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "========== Context Debug Info ==========")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Player: " .. player:getName())
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Context ID: " .. currentContext)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Position: " .. pos.x .. ", " .. pos.y .. ", " .. pos.z)
		
		-- Count nearby players in same context vs different
		local spectators = Game.getSpectators(pos, false, true, 0, 0, 0, 0)
		local sameContext = 0
		local diffContext = 0
		
		for _, spec in ipairs(spectators) do
			if spec:isPlayer() then
				if spec:getWorldContextId() == currentContext then
					sameContext = sameContext + 1
				else
					diffContext = diffContext + 1
				end
			end
		end
		
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Players nearby (same context): " .. sameContext)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Players nearby (diff context): " .. diffContext)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "========================================")
		return true
	end

	-- Numeric parameter: switch to specific context
	local contextId = tonumber(param)
	if contextId then
		local oldContext = currentContext
		player:setWorldContextId(contextId)
		
		if contextId == 0 then
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Returned to GLOBAL context (shared world)")
		else
			player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Switched to context " .. contextId)
		end
		
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Previous context: " .. oldContext)
		
		-- Log for debugging
		print("[WorldContext] Player " .. player:getName() .. " switched from context " .. oldContext .. " to " .. contextId)
		return true
	end

	-- Invalid parameter
	player:sendTextMessage(MESSAGE_FAILURE, "Invalid parameter. Usage:")
	player:sendTextMessage(MESSAGE_FAILURE, "  /context       - Show current context")
	player:sendTextMessage(MESSAGE_FAILURE, "  /context new   - Create new private context")
	player:sendTextMessage(MESSAGE_FAILURE, "  /context 0     - Return to global context")
	player:sendTextMessage(MESSAGE_FAILURE, "  /context <id>  - Switch to specific context")
	player:sendTextMessage(MESSAGE_FAILURE, "  /context info  - Show debug info")
	return true
end

context:separator(" ")
context:groupType("normal")  -- Allow all players for testing
context:register()
