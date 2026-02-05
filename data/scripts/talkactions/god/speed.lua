local speed = TalkAction("/speed")

function speed.onSay(player, words, param)
	-- create log
	logCommand(player, words, param)

	if param == "" then
		-- Show current speed info
		local currentSpeed = player:getSpeed()
		local baseSpeed = player:getBaseSpeed()
		local speedDelta = currentSpeed - baseSpeed
		local hasMaxSpeedFlag = player:hasFlag(24) -- SetMaxSpeed flag
		local groupId = player:getGroup():getId()
		local groupName = player:getGroup():getName()
		
		-- Calculate what getStepSpeed() SHOULD return
		local PLAYER_MAX_SPEED = 65535
		local PLAYER_MAX_STAFF_SPEED = 1500
		local PLAYER_MIN_SPEED = 10
		local maxStepSpeed = hasMaxSpeedFlag and PLAYER_MAX_STAFF_SPEED or PLAYER_MAX_SPEED
		local calculatedStepSpeed = math.max(PLAYER_MIN_SPEED, math.min(maxStepSpeed, currentSpeed))
		
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "=== Speed Info ===")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Current Speed: " .. currentSpeed)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Base Speed: " .. baseSpeed)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Speed Delta: " .. speedDelta)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Group: " .. groupName .. " (ID: " .. groupId .. ")")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Has SetMaxSpeed Flag: " .. (hasMaxSpeedFlag and "Yes (capped at 1500)" or "No"))
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Calculated StepSpeed: " .. calculatedStepSpeed)
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "=== Speed System ===")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "God group default: 1500 (set on login)")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Maximum speed: 65535 (no cap)")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Usage:")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "  /speed <value> - Add speed delta")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "  /speed set <value> - Set base speed directly")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Examples:")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "  /speed set 5000 - Set speed to 5000")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "  /speed 1000 - Add 1000 to current speed")
		return true
	end

	-- Check for "set" command
	local setMatch = param:match("^set%s+(%d+)$")
	if setMatch then
		local newBaseSpeed = tonumber(setMatch)
		if not newBaseSpeed then
			player:sendCancelMessage("Invalid speed value.")
			return true
		end
		
		-- Set base speed directly using player:setSpeed()
		-- This calls g_game().setCreatureSpeed() internally
		player:setSpeed(newBaseSpeed)
		
		-- Force a speed change to trigger client update
		player:changeSpeed(0)
		
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Base speed set to: " .. newBaseSpeed)
		return true
	end

	local speedValue = tonumber(param)
	if not speedValue then
		player:sendCancelMessage("Invalid speed value.")
		return true
	end

	-- Add speed delta
	player:changeSpeed(speedValue)
	
	-- Show updated info
	local currentSpeed = player:getSpeed()
	
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Speed changed by: " .. speedValue)
	player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "New speed: " .. currentSpeed)
	
	return true
end

speed:separator(" ")
speed:groupType("god")
speed:register()
