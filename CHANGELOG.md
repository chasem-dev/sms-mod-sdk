# Changelog

A release's tag is the API version (`v1.1.0` is API 1.1). Within a major version the API only grows; see [Compatibility](docs/MODDING.md#compatibility).

Pre-releases (`v1.5.0-beta.1`, `-beta.2`, ...) lead to a release and are listed under it, newest first, until it comes out. What a beta added may still change before the release; see [Pre-releases](docs/MODDING.md#pre-releases).

## 1.5.0 (in beta)

FLUDD's nozzles and Mario's outfit, for mods that decide what Mario has (a randomizer's unlocks, a challenge's rules); and events for every kind of mod, in the manner of other games' modding APIs: what Mario does, what happens to the stage's actors, and what the game does around them.

### 1.5.0-beta.6

- `api->files` (`SmsFilesApi`; `SMS_API_HAS(api, files)`): a mod's own files beside the memory card, in `<card folder>-mods/<mod id>/`. Scope `SMS_FILES_MOD` is written at once; scopes 0..2 (and `SMS_FILES_CURRENT`) are save files A to C of the running game, kept as the card keeps them: written when the game saves the file being played, none for a new game, copied and erased with the file at file select.
- `SMS_EV_SAVE_FILE` (`SmsEvSaveFile`: `what`, `file`, `from`): a save file loaded, started anew, saved, copied or erased.
- `SMS_EV_NOZZLE_FIRE` (`SmsEvNozzleFire`: `nozzle`, `how`): FLUDD's trigger is about to fire, a burst (A while R is held) or the pressure (rocket, hover, turbo). Cancellable.
- `SmsEvPlayerSpawn.kind` is changeable: wait, roll or return; a coaster start changed so is a stage start like any other's, without the coaster. The coaster start is now reported as `SMS_START_COASTER` (it came as wait or roll).
- `SMS_EV_SHINE_SETUP` (`SmsEvShineSetup`: `object`, `shine`, `collected`, `model`): a Shine may take a model file of the mod's. `game->object_set_color(obj, which, colour)` (`SMS_COLOR_MATERIAL`, `SMS_COLOR_TEV0`..`2`) colours an object's model.
- `SMS_EV_YOSHI_EGG` (`SmsEvYoshiEgg`: `kind`, `present`, `object`): whether Yoshi's egg is there (a stage's egg, the plaza's egg event, that event's egg); changeable.
- `SMS_EV_EPISODE_SHINE` (`SmsEvEpisodeShine`: `area`, `episode`, `shine`): the Shine an episode stands for in the episode select and the pause menu (its name, whether it shows collected, how many episodes are offered); changeable.
- In Lua: the events `save_file`, `nozzle_fire`, `shine_setup` (`model` relative to the script mod's folder), `yoshi_egg`, `episode_shine`, `player_spawn`'s `kind` writable, `sms.files_read`/`files_write`/`files_remove`/`files_dir` and `sms.set_object_color`.
- The lua mod is version 1.5.0-beta.6.

### 1.5.0-beta.5

- Water fights for server mods (docs/MODDING.md, "Water fights"): `SMS_RULE_WATER_HITS` has FLUDD's water hit the other players. The shooter's game stops each drop at the player it hits and reports the hits to the host, which hears them as `SMS_EV_NET_PLAYER_SPLASHED` (`SmsEvNetSplash`: `slot`, `by`, `drops`, `position`).
- `SMS_CMD_SPLASH`: a player splashed as Shadow Mario is when FLUDD's water hits him, in every game: his splash and hit sound where the water hit, the player's model flushed for a few seconds, and his water meter over their head (`i[1]` of `i[2]` full). `SMS_SPLASH_DOWN` knocks them down, as Shadow Mario falls when his meter is empty; `SMS_SPLASH_QUIET` shows the meter alone.
- `net->set_collision(on)` and `net->collision()`: players collide with each other (kept out of each other, bumped back running into one, a bounce off a head). The host's setting (its settings menu, `SMS_MULTIPLAYER_COLLISION`) until a server mod sets it; -1 gives the host's setting back.
- In Lua: the `net_player_splashed` event, the `splash` command (`slot`, `meter`, `full`, `down`, `quiet`, `position`), the `water_hits` rule, `sms.net.set_collision(on)` and `sms.net.collision()`.
- The lua mod is version 1.5.0-beta.5.

### 1.5.0-beta.4

- `SMS_EV_STAGE_LOADING` (`SmsEvStageLoading`: `area`, `episode`, `reason`, `shine`, `from_area`, `from_episode`): a stage is about to load, its episode chosen. `SMS_EV_STAGE_LEAVING` comes before a world's episode select, and before the game picks the plaza's (or Hotel Delfino's, Pinna Park's, Casino Delfino's) episode; this comes after, once a load, for every stage loaded. Change `area`/`episode` and that stage loads instead (its episode recorded, a new visit's coins afresh, its own opening movie); change `from_area`/`from_episode` and Mario comes from there (the plaza starts him by that world's way in). A multiplayer host's change is everyone's.
- In Lua: the `stage_loading` event, all but `reason` and `shine` changeable.
- The lua mod is version 1.5.0-beta.4.

### 1.5.0-beta.3

- `game->set_hud_hidden(mod, mask)` and `game->hud_hidden()`: hide parts of the game's own HUD (`SMS_HUD_*` bits) in any game, single player included. Until now only a hosting game's server mod could, with `SMS_CMD_HUD`. The parts are left out of the HUD's drawing only, so the HUD's own animations, sounds and FLUDD's messages carry on as ever; the counters' sparkles go with them. Each mod's mask is its own and goes when the mod unloads.
- In Lua: `sms.set_hud_hidden{...}` and `sms.hud_hidden()`.
- The lua mod is version 1.5.0-beta.3.

### 1.5.0-beta.2

- Events for every kind of mod (docs/MODDING.md, "Events for every kind of mod"), each general enough for many mods:
  - Mario: `SMS_EV_PLAYER_SPAWN` (where the stage puts him; changeable), `SMS_EV_PLAYER_STATUS_CHANGED` (the status he took), `SMS_EV_PLAYER_JUMP`, `SMS_EV_PLAYER_LAND` (how far he fell), `SMS_EV_PLAYER_HIT` (what touched or hit him; cancellable), `SMS_EV_PLAYER_HEAL` (changeable, cancellable), `SMS_EV_PLAYER_DEATH` (its cause, `SMS_DEATH_*`, and what dealt it), `SMS_EV_PLAYER_MOUNT` (Yoshi, a Blooper), `SMS_EV_PLAYER_WATER`, `SMS_EV_YOSHI_HATCH`, `SMS_EV_YOSHI_EAT`.
  - Actors: `SMS_EV_ACTOR_SPAWN`, `SMS_EV_ACTOR_DAMAGE` (an enemy's health lost to the player's hit), `SMS_EV_ACTOR_DEATH` (defeated, or killed by the stage), `SMS_EV_SHINE_APPEAR`, `SMS_EV_SHINE_GET` (every Shine taken), `SMS_EV_MAP_EVENT` (goop cleaned sets off a building, a gate).
  - The game: `SMS_EV_FLAG_CHANGED` (any save-file flag; changeable, cancellable), `SMS_EV_SOUND` and `SMS_EV_MUSIC` (changeable, cancellable), `SMS_EV_TALK` (a talk's start, messages (changeable) and end), `SMS_EV_CUTSCENE_END`, `SMS_EV_PAUSE`, `SMS_EV_MODE_CHANGED`, `SMS_EV_SAVE_PROMPT` (cancellable), `SMS_EV_GAME_SAVED`, `SMS_EV_TIMER`, `SMS_EV_SCRIPT_CALL`.
- `SmsEvDamage.source` (check `ev->size`): the enemy or object whose hit takes Mario's health; NULL for the ground's damage. `SMS_EV_PLAYER_DAMAGE` no longer comes once Mario has lost his life (the game went on taking health he did not have).
- `game->object_type(obj)` and `game->object_health(obj, &max)`; `SMS_ACTOR_*` actor type bits.
- In Lua: the events by name (`player_jump`, `actor_death`, `flag_changed`...), with changeable fields as in C, `ev.source` on `player_damage`, `sms.object_type` and `sms.object_health`.
- The lua mod is version 1.5.0-beta.2.

### 1.5.0-beta.1

- `SMS_EV_PLAYER_NOZZLE` (`SmsEvPlayerNozzle`: `from`, `to`, `animate`, `reason`, `box`): FLUDD is about to switch nozzle (`SMS_NOZZLE_SWITCH`: a nozzle picked up, Yoshi, the game, `player->set_nozzle`), swap with its second nozzle (`SMS_NOZZLE_SWAP`), get the nozzle the file remembers as a stage starts (`SMS_NOZZLE_STAGE_START`), or a nozzle's box is about to be set up (`SMS_NOZZLE_BOX`). `to` may be changed; cancel and the nozzle stays (a box is not in the stage). Only the player's Mario.
- `player->set_nozzle(nozzle, animate)`, `player->set_fludd(has)`, `player->set_outfit(shirt, sunglasses, helmet)` (each 1, 0, or -1 as it is).
- `SmsPlayer.current_nozzle`, `.shirt`, `.sunglasses`, `.helmet` (check `size`).
- In Lua: the `player_nozzle` event, `sms.set_nozzle`, `sms.set_fludd`, `sms.set_outfit`, and `sms.mario()`'s `current_nozzle`, `shirt`, `sunglasses`, `helmet`.
- The manual's Nozzles: the diving helmet is not a nozzle (it was listed as 7).
- The lua mod is version 1.5.0-beta.1.
- Pre-releases: `SMS_MOD_API_PRERELEASE` (0 in a release, N in `-beta.N`), the `sms_mod_api_prerelease` export `SMS_MOD_DEFINE` adds, and `SmsApi.prerelease` (the game's). A game warns of a mod built against another beta. `tools/check-version.sh` checks a tag against the SDK; CI releases a beta tag as a GitHub pre-release.

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
