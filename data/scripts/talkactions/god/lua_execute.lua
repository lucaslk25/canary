local luaCommand = TalkAction("/lua")

function luaCommand.onSay(player, words, param)
	-- create log
	logCommand(player, words, param)

	if param == "" then
		player:sendCancelMessage("Command param required.")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Usage examples:")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "/lua player:changeSpeed(1000) -- Add speed delta")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "/lua player:getSpeed() -- Get current speed")
		player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "/lua player:getBaseSpeed() -- Get base speed")
		return true
	end

	-- Create environment with player variable
	local env = {
		player = player,
		-- Add other useful globals
		Game = Game,
		Position = Position,
		Creature = Creature,
		Player = Player,
		Monster = Monster,
		Npc = Npc,
		Item = Item,
		Tile = Tile,
		Town = Town,
		House = House,
		print = print,
		tostring = tostring,
		tonumber = tonumber,
		-- Add math and string libraries
		math = math,
		string = string,
		table = table,
	}
	setmetatable(env, {__index = _G})

	-- Try to load as expression first (for simple commands like player:getSpeed())
	local loadedFunction, error = load("return " .. param, "lua_command", "t", env)
	if not loadedFunction then
		-- If that fails, try to load as statement (for commands like player:changeSpeed(100))
		loadedFunction, error = load(param, "lua_command", "t", env)
	end

	if loadedFunction then
		local success, result = pcall(loadedFunction)
		if success then
			if result ~= nil then
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Result: " .. tostring(result))
			else
				player:sendTextMessage(MESSAGE_EVENT_ADVANCE, "Command executed successfully.")
			end
		else
			player:sendTextMessage(MESSAGE_ADMINISTRATOR, "Execution error: " .. tostring(result))
		end
	else
		player:sendTextMessage(MESSAGE_ADMINISTRATOR, "Syntax error: " .. tostring(error))
	end

	return true
end

luaCommand:separator(" ")
luaCommand:groupType("god")
luaCommand:register()
