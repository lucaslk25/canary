# Canary Open Tibia Server - GM Commands Reference

Complete list of all in-game GM commands organized by access level.

---

## Table of Contents
- [Player Commands](#player-commands)
- [Gamemaster Commands](#gamemaster-commands)
- [God Commands](#god-commands)

---

## Player Commands

| Command | Description |
|---------|-------------|
| `!commands` | Shows all available commands with descriptions based on your access level |

---

## Gamemaster Commands

These commands are available to Game Masters and higher ranks (30 commands).

### Movement & Teleportation

| Command | Syntax | Description |
|---------|--------|-------------|
| `/up` | `/up` | Teleports you one floor up |
| `/down` | `/down` | Teleports you one floor down |
| `/goto` | `/goto <creatureName>` | Teleports you to a creature |
| `/active` | `/active` | Teleports you to a random active player (not AFK, not training, not ghost) |
| `/listplayers` | `/listplayers [all]` | Opens a modal window to select and teleport to a player |
| `/town` | `/town <townName/ID>` | Teleports you to a town's temple |
| `/t` | `/t [playerName]` | Teleports you or a player to their temple |
| `/c` | `/c <creatureName>` | Teleports a creature to your position |
| `/a` | `/a <steps>` | Skips forward X tiles in the direction you're facing |
| `/pos` or `!pos` | `/pos` or `/pos <x, y, z>` | Shows your current position or teleports to coordinates |
| `/teleport` or `/tp` | `/teleport <x, y, z>` | Creates a teleport item at your position with specified destination |

### Player Management

| Command | Syntax | Description |
|---------|--------|-------------|
| `/ban` | `/ban <playerName>, <days>[, reason]` | Bans a player for X days (max 350,000 days) |
| `/unban` | `/unban <playerName>` | Unbans a player and their IP address |
| `/kick` | `/kick <playerName>` | Kicks a player from the server |
| `/namelock` | `/namelock <playerName>, <reason>` | Namelocks a player (requires reason) |
| `/info` | `/info <playerName>` | Shows detailed player information (position, IP, skills, multiclient check) |

### Visibility & Status

| Command | Syntax | Description |
|---------|--------|-------------|
| `/ghost` | `/ghost` | Toggles ghost mode (invisibility) |
| `/afk` | `/afk <on/off>` | Sets AFK status with visual effect |

### Communication

| Command | Syntax | Description |
|---------|--------|-------------|
| `/b` | `/b <message>` | Broadcasts a message to all players |

### Visual Effects

| Command | Syntax | Description |
|---------|--------|-------------|
| `/effect` | `/effect <effectID>` | Sends a magic effect at your position |
| `/distanceeffect` | `/distanceeffect <effectID>` | Sends a distance effect in the direction you're facing |
| `/looktype` | `/looktype <looktypeID>` | Changes your looktype/outfit (ID 0-1468, excluding invalid types) |
| `/getlook` | `/getlook <creatureName>` | Gets the outfit details of a creature in XML format |
| `/setlight` | `/setlight <color>, [intensity]` | Sets your light (color 0-1500, intensity 1-32) |

### Server Management

| Command | Syntax | Description |
|---------|--------|-------------|
| `/clean` | `/clean` | Cleans items from the map |
| `/mc` | `/mc` | Shows multiclient check (players sharing IPs) |
| `/countmonsters` | `/countmonsters` | Counts all monsters on the server and saves to monster_count.txt |
| `/bless` | `/bless` | Shows blessing status |
| `/spy` | `/spy <playerName>` | Shows all equipment and items a player is carrying |
| `/goldrank` | `/goldrank` | Shows top 10 richest players by bank balance |

---

## God Commands

These commands are available only to administrators with God access level (70+ commands).

### Item Management

| Command | Syntax | Description |
|---------|--------|-------------|
| `/i` | `/i <itemID/name>[, count][, tier]` | Creates an item with optional count and tier |
| `/r` | `/r [all/count]` | Removes the top visible thing in front of you |
| `/attr` | `/attr <attribute>, <value>` | Sets attributes on items/creatures/players in front of you |

#### `/attr` Supported Attributes:

**Item Attributes:**
- `actionid`, `action`, `aid` - Sets action ID
- `uniqueid`, `unique`, `uid` - Sets unique ID
- `description`, `desc` - Sets description
- `name` - Sets item name
- `remove` - Removes the item
- `decay` - Triggers item decay
- `transform` - Transforms item to another ID
- `clone` - Clones the item
- `attack` - Sets attack value
- `defense` - Sets defense value
- `extradefense` - Sets extra defense
- `charge` - Sets charges
- `armor` - Sets armor value

**Creature Attributes:**
- `health` - Adds health
- `mana` - Adds mana
- `speed` - Changes speed
- `droploot` - Sets drop loot flag
- `skull` - Sets skull type
- `direction` - Sets direction
- `maxHealth` - Sets max health
- `say` - Makes creature say text

**Player Attributes:**
- `fyi` - Shows popup FYI
- `tutorial` - Sends tutorial
- `guildnick` - Sets guild nickname
- `group` - Sets group
- `vocation` - Sets vocation
- `stamina` - Sets stamina
- `town` - Sets town
- `balance` - Adds to bank balance
- `save` - Saves player
- `type` - Sets account type
- `skullTime` - Sets skull time
- `maxMana` - Sets max mana
- `maxHealth` - Sets max health
- `addItem` - Adds item to player
- `removeItem` - Removes item from player
- `premium` - Adds premium days

### Monster & NPC Management

| Command | Syntax | Description |
|---------|--------|-------------|
| `/m` | `/m <monsterName>[, count][, forge][, radius][, force]` | Creates monsters with optional forge properties |
| `/s` | `/s <monsterName>` | Summons a monster as your familiar |
| `/spawn` | `/spawn <monsterName>[, spawntime]` | Creates a monster spawn at your position |
| `/n` | `/n <npcName>[, true]` | Creates an NPC (true = saves permanently to file) |
| `/setmonstername` | `/setmonstername <newName>` | Sets the name of nearby monsters |

### Player Enhancement

| Command | Syntax | Description |
|---------|--------|-------------|
| `/addskill` | `/addskill <playerName>, <skill/level/magic>, [amount]` | Adds skills, levels, or magic levels |
| `/addmoney` | `/addmoney <playerName>, <amount>` | Adds money to a player's bank account |
| `/addaddon` | `/addaddon <playerName>, <looktype/all>, <value>` | Adds outfit addons (value: 0-3, or 'all' for all outfits) |
| `/addmount` | `/addmount <playerName>, <mountID/all>` | Adds mounts to a player (ID 1-231 or 'all') |
| `/addachievement` | `/addachievement <playerName>, <achievementID/name>` | Adds an achievement to a player |
| `/removeachievement` | `/removeachievement <playerName>, <achievementID>` | Removes an achievement from a player |
| `/checkachievements` | `/checkachievements <playerName>` | Lists all achievements a player has |
| `/addbosskill` | `/addbosskill <kills>, <monsterName>[, targetName]` | Adds bosstiary kills for a monster |
| `/addtitle` | `/addtitle <playerName>, <titleID>` | Adds a title to a player |
| `/settitle` | `/settitle <playerName>, <titleID>` | Sets a player's currently displayed title |
| `/addbadge` | `/addbadge <playerName>, <badgeID>` | Adds a badge to a player |

### Bestiary & Charms

| Command | Syntax | Description |
|---------|--------|-------------|
| `/addcharms` | `/addcharms <playerName>, <amount>` | Adds charm points to a player |
| `/addminorcharms` | `/addminorcharms <playerName>, <amount>` | Adds minor charm points to a player |
| `/resetcharms` | `/resetcharms [playerName]` | Resets charm points for you or a player |
| `/charmexpansion` | `/charmexpansion [playerName]` | Adds charm expansion for you or a player |
| `/charmrunes` | `/charmrunes [playerName]` | Unlocks all charm runes for you or a player |
| `/setbestiary` | `/setbestiary <playerName>, <monsterName/all>, <amount>` | Sets bestiary kill count (use 'all' for all monsters) |

### Forge System

| Command | Syntax | Description |
|---------|--------|-------------|
| `/adddusts` | `/adddusts <playerName>, <amount>` | Adds forge dusts to a player |
| `/removedusts` | `/removedusts <playerName>, <amount>` | Removes forge dusts from a player |
| `/getdusts` | `/getdusts <playerName>` | Gets a player's forge dust count |
| `/setdusts` | `/setdusts <playerName>, <amount>` | Sets a player's forge dust count |
| `/adddustlevel` | `/adddustlevel <playerName>, <level>` | Adds forge dust level to a player |
| `/openforge` | `/openforge` | Opens the forge window |
| `/fiendish` | `/fiendish` | Teleports to a fiendish monster |
| `/influenced` | `/influenced` | Teleports to an influenced monster |
| `/setfiendish` | `/setfiendish` | Sets a new fiendish monster |

### Storage & Flags

| Command | Syntax | Description |
|---------|--------|-------------|
| `/getstorage` | `/getstorage <playerName>, <storageKey/name>` | Gets a storage value for a player |
| `/setstorage` | `/setstorage <storageKey>, <value>[, playerName]` | Sets a storage value for a player |
| `/hasflag` | `/hasflag <playerName>, <flagNumber/name>` | Checks if a player has a specific flag |
| `/setflag` | `/setflag <playerName>, <flagNumber/name>` | Adds a flag to a player |
| `/removeflag` | `/removeflag <playerName>, <flagNumber/name>` | Removes a flag from a player |
| `/getkv` | `/getkv <key>[, playerName]` | Gets a key-value entry for you or a player |
| `/getallkv` | `/getallkv [playerName]` | Lists all key-value entries for you or a player |
| `/setkv` | `/setkv <key>, <value>[, playerName]` | Sets a key-value entry for you or a player |
| `/clearcooldown` | `/clearcooldown <boss>[, playerName]` | Clears boss cooldown for you or a player |

### House & Hireling Management

| Command | Syntax | Description |
|---------|--------|-------------|
| `/gotohouse` | `/gotohouse [playerName]` | Teleports to your house or a player's house |
| `/owner` | `/owner [playerName/none]` | Sets or removes the owner of the house you're standing in |
| `/hireling` | `/hireling [name][, sex]` | Creates a hireling lamp with optional name and sex |
| `/clearhirelingstas` | `/clearhirelingstas [playerName]` | Clears all hireling stats for you or a player |
| `/inbox` | `/inbox <playerName>, <add/remove>, <itemID>` | Manages items in a player's store inbox |

### VIP & Tutor Management

| Command | Syntax | Description |
|---------|--------|-------------|
| `/vip` | `/vip <action>, <name>[, value]` | Manages VIP status (actions: check, adddays, removedays, remove) |
| `/addtutor` | `/addtutor <playerName>` | Promotes a player to tutor rank |
| `/removetutor` | `/removetutor <playerName>` | Demotes a tutor to normal player rank |

### Server Management

| Command | Syntax | Description |
|---------|--------|-------------|
| `/save` | `/save [minutes]` | Saves the server immediately or schedules a save in X minutes |
| `/closeserver` | `/closeserver [shutdown/save/maintainance]` | Closes, shuts down, or sets server to maintenance mode |
| `/openserver` | `/openserver` | Opens the server (sets game state to normal) |
| `/reload` | `/reload <type>` | Reloads server components (types: all, config, monsters, npcs, raids, scripts, etc.) |
| `/ipban` | `/ipban <playerName>` | Bans a player's IP address for 7 days |

### Raids & Zones

| Command | Syntax | Description |
|---------|--------|-------------|
| `/raid` | `/raid <raidName>` | Starts a specific raid |
| `/simraid` | `/simraid <initialChance>, <targetChancePerDay>, <maxChancePerCheck>` | Simulates raid triggering with specified parameters |
| `/listraid` | `/listraid` | Lists all registered raids |
| `/zones` | `/zones <command>, <zoneName>` | Zone management commands |

#### `/zones` Subcommands:
- `list` - Lists all zones
- `goto` - Teleports to a zone
- `removeMonsters` - Removes all monsters in a zone
- `countMonsters` - Counts monsters in a zone
- `removeNpcs` - Removes all NPCs in a zone
- `countNpcs` - Counts NPCs in a zone
- `kickPlayers` - Kicks all players from a zone
- `listPlayers` - Lists all players in a zone
- `countPlayers` - Counts players in a zone
- `size` - Shows the size of a zone

### Testing & Effects

| Command | Syntax | Description |
|---------|--------|-------------|
| `/areasound` | `/areasound <soundID>` or `/areasound <soundID1>, <soundID2>` | Plays sound effect(s) in the area |
| `/internalsound` | `/internalsound <soundID>` or `/internalsound <soundID1>, <soundID2>` | Plays sound effect(s) only to you |
| `/globalsound` | `/globalsound <soundID>` or `/globalsound <soundID1>, <soundID2>` | Plays sound effect(s) globally to all players |
| `/testicon` | `/testicon <icon1>[, icon2, ...]` or `/testicon special, <icon>` | Tests normal or special icons |
| `/bakragoreicon` | `/bakragoreicon <iconNumber>` or `/bakragoreicon remove` | Adds or removes a Bakragore icon |
| `/playericon` | `/playericon <icon_id>, <quantity>[, direction]` | Applies a player icon with countdown (direction: up/down) |
| `/testmessage` | `/testmessage <messageType>[, textColor]` | Tests sending text messages with different types and colors |
| `/testlog` | `/testlog <level>, <message>` | Tests logging functionality (levels: info, warn, error, debug) |
| `!testcontainer` | `!testcontainer [remove]` | Tests container iterator and optionally removes items |
| `/testtaintconditions` | `/testtaintconditions` | Tests taint icon conditions |

### Development & Debugging

| Command | Syntax | Description |
|---------|--------|-------------|
| `/lua` | `/lua <code>` | Executes Lua code (e.g., `/lua player:changeSpeed(1000)`) |
| `/speed` | `/speed` | Shows current speed information |
| `/speed` | `/speed set <value>` | Sets base speed directly (e.g., `/speed set 5000`) |
| `/speed` | `/speed <value>` | Adds speed delta to current speed |

### World Context (Instanced Hunts)

| Command | Syntax | Description |
|---------|--------|-------------|
| `/context` | `/context` | Shows your current world context ID and status |
| `/context` | `/context new` | Creates a new private context (isolated instance) |
| `/context` | `/context 0` | Returns to global context (shared world) |
| `/context` | `/context <id>` | Switches to a specific context ID |
| `/context` | `/context info` | Shows debug info with nearby creatures per context |
| `/ctxmetrics` | `/ctxmetrics` | Shows context system metrics and nearby creature distribution |

---

## Notes

### General Information
- Commands are **case-sensitive** and must be typed exactly as shown
- Parameters in `<>` are **required**
- Parameters in `[]` are **optional**
- Most commands log their usage for auditing purposes
- Commands use the TalkAction system implemented in Lua scripts

### Access Levels
Access levels are hierarchical:
```
normal < gamemaster < god
```
- **Normal**: Regular players
- **Gamemaster**: Game Masters have access to all GM commands
- **God**: Administrators have access to all commands (GM + God)

### Command Aliases
Some commands support multiple aliases:
- `/teleport` = `/tp`
- `/pos` = `!pos`
- `/b` = `/broadcast`

### Skill Names for `/addskill`
- `level` or `l` - Character level
- `magic` or `m` - Magic level
- `club` - Club fighting
- `sword` - Sword fighting
- `axe` - Axe fighting
- `dist` - Distance fighting
- `shield` - Shielding
- `fish` - Fishing

### Important Warnings
- **Ban Duration**: Maximum ban duration is 350,000 days
- **Force Push**: Never use `git push --force` to main/master branches
- **Ghost Mode**: Players in ghost mode are invisible to other players
- **Server Shutdown**: Use `/closeserver shutdown` with caution - it will shut down the server immediately

### File Locations
- GM Command Scripts: `/data/scripts/talkactions/gm/`
- God Command Scripts: `/data/scripts/talkactions/god/`
- Player Command Scripts: `/data/scripts/talkactions/player/`

---

## Examples

### Teleportation Examples
```
/pos                          # Shows your current position
/pos 1000, 1000, 7           # Teleports to coordinates x=1000, y=1000, z=7
/pos {x = 1000, y = 1000, z = 7}  # Alternative format
/goto Dragon                  # Teleports to a creature named "Dragon"
/town Thais                   # Teleports to Thais temple
```

### Player Management Examples
```
/ban PlayerName, 7, Breaking rules     # Bans player for 7 days
/kick PlayerName                       # Kicks player immediately
/info PlayerName                       # Shows detailed player info
/addskill PlayerName, level, 10        # Adds 10 levels
/addmoney PlayerName, 1000000          # Adds 1,000,000 gold to bank
```

### Item Creation Examples
```
/i 2160                       # Creates 1 crystal coin
/i 2160, 100                  # Creates 100 crystal coins
/i sword, 1, 10               # Creates 1 sword with tier 10
/i golden armor               # Creates item by name
```

### Monster Creation Examples
```
/m dragon                     # Creates 1 dragon
/m demon, 5                   # Creates 5 demons
/m dragon lord, 1, 3          # Creates 1 dragon lord with forge tier 3
/s rat                        # Summons a rat as your familiar
```

### Attribute Examples
```
/attr actionid, 5000          # Sets action ID to 5000 on item in front
/attr health, 1000            # Adds 1000 health to creature in front
/attr premium, 30             # Adds 30 premium days to player in front
```

### World Context Examples (Instanced Hunts)
```
/context                      # Show current context status
/context new                  # Create a new private context
/context 0                    # Return to global (shared) context
/context 123                  # Switch to specific context ID
/context info                 # Show debug info with nearby creatures
/ctxmetrics                   # Show context system metrics
```

---

**Last Updated**: January 2026  
**Server Version**: Canary (Open Tibia Server)  
**Documentation Source**: Analyzed from `/data/scripts/talkactions/` directory
