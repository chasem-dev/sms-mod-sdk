# Changelog

A release's tag is the API version (`v1.1.0` is API 1.1). Within a major version the API only grows; see [Compatibility](docs/MODDING.md#compatibility).

## 1.5.0

Nozzles and Mario's outfit, for mods that decide what Mario has (a randomizer's unlocks, a challenge's rules).

- `SMS_EV_PLAYER_NOZZLE` (`SmsEvPlayerNozzle`: `nozzle`, `animate`, `reason`): FLUDD is about to switch nozzle (`SMS_NOZZLE_SWITCH`), swap with its second nozzle (`SWAP`), or a nozzle's box is about to be set up (`BOX`). `nozzle` may be changed; cancel and the nozzle stays (a box is left out).
- `player->set_nozzle(nozzle, animate)`, `player->set_fludd(has)`: FLUDD's nozzle, and whether Mario has FLUDD, as the game would set them.
- `player->set_outfit(shirt, sunglasses, helmet)`: the aloha shirt, the sunglasses and the diving helmet.

## 1.4.0

Server mods change and add to the stage, and run the players' games further (docs/MODDING.md, "Server mods"); and for any mod, the game's worlds, episodes and cutscenes, and why a stage is left ("world").

- `SMS_CMD_OBJECT`: one of the stage's objects (by actor types, its kind as the stage names it, and which of them), or each of a kind: placed and turned (`SMS_OBJECT_PLACE`), a spinner or a lift on its rail at a speed of its own (`SPEED`), moved on from how the stage loaded it to a moment of its own, the same in every game (`PHASE`), shown or hidden, its collision with it (`SHOW`, `HIDE`), sized (`SCALE`), turned every way (`TURN`), gliding to a place (`MOVE`), or gone (`KILL`). Sticky per object and what it does.
- `SMS_CMD_SPAWN`: a copy of any object the stage loaded (made again from its record in the stage's file, at a new place), or an item by name (coins, and what the stage has the model of), with the mod's own id; `"@<id>"` then names it to `SMS_CMD_OBJECT` and `game->object_info`.
- `SMS_CMD_PLAYER` (the game's own miss, a launch, off a Blooper or Yoshi, health, water, nozzle), `SMS_CMD_RULES` (no FLUDD, no damage, no pause, no pickups), `SMS_CMD_BOARD` (eight rows of standings at the left), `SMS_CMD_HUD` (its parts hidden), `SMS_CMD_MARKER` (text or an arrow over a place or a player), `SMS_CMD_SCREEN` (fade out, fade in, flash), `SMS_CMD_SHAKE` and `SMS_CMD_RUMBLE`; `SMS_CMD_SOUND` at a place in the world.
- `SmsCmd.f2`: four more numbers (after `text`, taken when `size` covers them), and `SMS_CMD_UNSTICK`: a sticky command forgotten. A session keeps up to 192 sticky commands (64 before).
- `net->set_look` and `net->look`: a player's Customize Mario and FLUDD colours forced in every game (their own Mario repainted at once); `net->set_respawn`: where a player comes back as they arrive. `SmsNetPlayer.pad`: what each player presses.
- `game->object_count`, `game->object_info` and `game->ground`: the stage's objects (types, kinds, places, sizes, alive or hidden) and the ground, water and roof at a point, for any mod.
- In Lua: every command by name (`sms.net.send(to, { type = "spawn", kind = "WoodBlockCone", index = 0, id = 1, position = ... })`), `sms.net.set_look`, `look`, `set_respawn`, `sms.net.player(slot).pad`, `sms.object_count`, `sms.object_info`, `sms.ground`.
- `api->world` (`SmsWorldApi`): what an area is (`SMS_AREA_*`: the Airstrip, the plaza, a world's way in, another part of a world, a secret course, Corona Mountain, the title), the world it belongs to, a world's episodes (how many, each one's Shine and name, the one the episode select offers next), the episode a Shine is for, and the stage's camera cutscenes (`has_cutscene`, `play_cutscene`, `cutscene_playing`). In Lua, `sms.world`.
- `SmsEvStage.reason` and `.shine` (check `ev->size`): why the game leaves a stage (`SMS_LEAVE_SHINE`, `EXIT_AREA`, `LIFE_LOST`, `GAME_OVER`, `WAY_OUT`, `WARP`, `FILE`) or came to this one, and the Shine that ended it. In Lua, `ev.reason` (`"shine"`, ...) and `ev.shine`.
- `SMS_EV_STAGE_LEAVING` to a world's way in with an episode named goes straight to that episode, as if it had been chosen at the episode select; `SMS_EPISODE_SELECT` (255) shows the select. On a multiplayer host the players follow the stage a mod chose.
- `SMS_EV_STAGE_START`: the stage's opening is over and the player has the controls (Lua `stage_start`).
- The lua mod is version 1.4.0.

## 1.3.0

Versions in multiplayer: a host may take players on other builds of the game, and say which versions it needs (docs/MODDING.md, "Versions").

- `SmsNetPlayer.game_version` and `api_version`: what each player's game said when it joined (its version, the launcher's "2026.10.18.1", and the mod API it carries), so a server mod knows which commands it understands. In Lua, `sms.net.player(slot).game_version` and `.api_version` (`"1.2"`).
- `mod.ini`'s `client_api = 1.2`: what a server mod needs of the players' games; the host refuses older ones, saying why.

## 1.2.0

Multiplayer, and server mods: mods that run only on the host of a multiplayer game, a dedicated server as a rule, while the players join with the plain game (docs/MODDING.md, "Server mods").

- `api->net` (`SmsNetApi`): the game's role (host, client), whether the host is a dedicated server, the players (`SmsNetPlayer`: name, character, colour, where they are, whether they are in the stage), and on the host: commands to the players' games (`net->send`, `SmsCmd`: banner, notice, standing line, the HUD's clock, "Ready... Go!", sound, music, warp, hold, onto a Blooper, watch a player, the stage's actors shown, hidden or killed, an event script stopped, staying in the stage; each with a delay lined up across the games, and sticky for the session), a stage afresh for everyone (`net->load_stage`), and the game mode's name and state for the launcher's list.
- Events `SMS_EV_NET_PLAYER_JOINED`, `SMS_EV_NET_PLAYER_LEFT`, `SMS_EV_NET_PLAYER_ARRIVED`, `SMS_EV_NET_SESSION`.
- `mod.ini`'s `side = server`: a mod that loads only in a hosting game.
- `game->cube_count`, `game->in_cube`: the stage's trigger volumes (camera, area and the event scripts' "fast cubes"), as the game's scripts test them.

## 1.1.1

- The lua mod's Linux libraries load on glibc 2.35 and newer (Ubuntu 22.04, Debian 12, SteamOS); 1.1.0's needed 2.38. CI builds Linux mods on Ubuntu 22.04, and the templates' workflows do too.

## 1.1.0

The SDK as its own repository, split from the port's `sdk/` folder with its history.

- `core->mod_info(mod, key)`: a value from a mod's `mod.ini`.
- `core->host_runtime(host, runtime, start, stop, user)`: a native mod runs script mods (`runtime = <name>` in their `mod.ini`). The lua mod uses it.
- The **lua** mod (`lua/`), released as an optional mod with a library for every platform.
- `SMS_API_HAS` is plain C and C++ (it used `__typeof__`, which MSVC's C compiler lacks).
- `SmsMod.cmake` works as a FetchContent dependency (the target `sms::mod_sdk`, `-m32`/`-m64` per target), names macOS libraries `.dylib` as the game looks for them, and links MSVC's C runtime statically.
- The examples moved to [sms-mod-templates](https://github.com/chasem-dev/sms-mod-templates).

## 1.0.0

The first API: events with priorities, cancelling and consuming; the `core`, `game`, `player`, `camera`, `draw`, `input`, `config`, `audio` and `native` tables; HUD slots; the Mods menu's options; key bindings; services and custom events between mods.
