/* sms_mod.h: the native mod API of the Super Mario Sunshine PC port.
 *
 * A native mod is a shared library (.so, .dll, .dylib) in its own folder
 * under mods/, beside a mod.ini that names it (docs/MODDING.md). The game
 * loads every such mod at start and calls its sms_mod_load with an SmsApi:
 * tables of functions to hear of the game's events, read and change the
 * game, draw over it, read the controllers and keyboard, keep settings, and
 * talk to other mods. A mod needs nothing but this header to build: it links
 * against nothing of the game's, so one build of a mod runs with every build
 * of the game that has the same major API version.
 *
 * Rules that keep several mods working together:
 *   - Every function is called on the game thread, inside the game's own
 *     steps (the log is the one function any thread may call). Do the slow
 *     work on threads of your own and hand results over.
 *   - Events reach each mod in priority order; an event's documentation says
 *     whether a handler may change its payload, cancel it, or consume it.
 *   - Draw only inside SMS_EV_DRAW. Take screen space from draw->hud_slot, so
 *     panels of different mods stack instead of overlapping.
 *   - Take the controllers and keyboard (input->capture) only while a menu of
 *     yours is open, and bind keys through input->bind_key, which reports
 *     clashes with the port's keys and other mods'.
 *   - Pointers the game hands you (SmsObject, payload pointers) are good for
 *     the call, and objects until the stage changes. Copy what you keep.
 *
 * Compatibility: the tables only grow at their ends, and each starts with its
 * size as the game built it, so a mod built against a newer minor version
 * checks a function is there with SMS_API_HAS before calling it. A new major
 * version is an incompatible change; the game refuses mods of another major.
 */
#ifndef SMS_MOD_H
#define SMS_MOD_H

#include <stddef.h>
#include <stdint.h>

#define SMS_MOD_API_MAJOR 1
#define SMS_MOD_API_MINOR 5
#define SMS_MOD_API_VERSION ((uint32_t)SMS_MOD_API_MAJOR << 16 | SMS_MOD_API_MINOR)

#if defined(_WIN32)
#define SMS_MOD_EXPORT __declspec(dllexport)
#else
#define SMS_MOD_EXPORT __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
#define SMS_MOD_EXTERN_C extern "C"
#else
#define SMS_MOD_EXTERN_C
#endif
#if defined(__GNUC__) || defined(__clang__)
#define SMS_MOD_PRINTF(f, a) __attribute__((format(printf, f, a)))
#else
#define SMS_MOD_PRINTF(f, a)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ basics */

typedef struct SmsMod SmsMod;     /* one loaded mod: yours is passed to sms_mod_load */
typedef struct SmsApi SmsApi;
typedef void* SmsObject;          /* a game object; good until the stage changes */
typedef uint32_t SmsImage;        /* an image loaded with draw->image_load; 0 = none */
typedef uint32_t SmsColor;        /* 0xRRGGBBAA */

#define SMS_RGBA(r, g, b, a) \
	((SmsColor)(((uint32_t)(r) & 0xFF) << 24 | ((uint32_t)(g) & 0xFF) << 16 | ((uint32_t)(b) & 0xFF) << 8 | ((uint32_t)(a) & 0xFF)))
#define SMS_RGB(r, g, b) SMS_RGBA(r, g, b, 255)

typedef struct SmsVec3 {
	float x, y, z;
} SmsVec3;

typedef struct SmsRect {
	float x, y, w, h;
} SmsRect;

/* Is `member` in this table (the game is new enough to have it)? Plain C and
   C++, no compiler extensions: the member's offset is taken from its address. */
#define SMS_API_HAS(table, member) \
	((table) && \
	 (size_t)(table)->size >= (size_t)((const char*)&(table)->member - (const char*)(table)) + sizeof((table)->member) && \
	 (table)->member)

typedef enum SmsLogLevel {
	SMS_LOG_DEBUG = 0, /* shown with SMS_NATIVE_MODS_DEBUG=1 */
	SMS_LOG_INFO  = 1,
	SMS_LOG_WARN  = 2,
	SMS_LOG_ERROR = 3
} SmsLogLevel;

/* ------------------------------------------------------------------ events */

typedef enum SmsEventType {
	/* Top of every frame of the game's loop: title, file select, stages and
	   movies alike. Payload SmsEvFrame. */
	SMS_EV_FRAME = 1,
	/* Every simulation tick of a stage, after everything moved and before
	   anything is posed for drawing: the place to move things. Not while
	   the pause menu, the map or a save screen is up. SmsEvTick. */
	SMS_EV_TICK = 2,
	/* Once a frame, after SMS_EV_FRAME: build what you draw over the game
	   (the draw table). SmsEvDraw. */
	SMS_EV_DRAW = 3,
	/* A stage's objects are loaded and set up. SmsEvStage. */
	SMS_EV_STAGE_LOADED = 4,
	/* The game asks to leave for another area (pipes, paintings, scripts).
	   Cancellable: cancel to stay. SmsEvStageExit. */
	SMS_EV_STAGE_EXIT_REQUEST = 5,
	/* The stage is about to change (every way out). The payload's area and
	   episode may be changed: the game goes there instead. SmsEvStage. */
	SMS_EV_STAGE_LEAVING = 6,
	/* Mario lost a life and has lives left; area/episode is where he goes
	   next and may be changed. SmsEvStage. */
	SMS_EV_LIFE_LOST = 7,
	/* Mario lost his last life. Cancellable: cancel to carry on as after any
	   other lost life instead of the game over screen. No payload. */
	SMS_EV_GAME_OVER = 8,
	/* Losing a life is about to reset the coins. Cancellable: cancel to keep
	   them. No payload. */
	SMS_EV_COINS_RESET = 9,
	/* A file was chosen at file select; area/episode is where it starts and
	   may be changed. SmsEvStage. */
	SMS_EV_FILE_SELECTED = 10,
	/* An item (coin, blue or red coin, 1-up, ...) was picked up.
	   SmsEvItem. */
	SMS_EV_ITEM_COLLECTED = 11,
	/* The controllers were read, before the game sees them. The pads may be
	   changed (scripted play, input remapping). SmsEvPadRead. */
	SMS_EV_PAD_READ = 12,
	/* A key went down or up in the game's window. Consumable: consume it and
	   neither later mods nor the game's controls see it. SmsEvKey. */
	SMS_EV_KEY = 13,
	/* A camera cutscene was asked for. SmsEvCutscene. */
	SMS_EV_CUTSCENE = 14,
	/* Goop is about to be cleaned at a place (water, Mario's feet).
	   SmsEvGoop. */
	SMS_EV_GOOP_CLEANED = 15,
	/* The player, or their water, hit an object (sprayed, stomped, punched,
	   ground-pounded). Cancellable: cancel and the object never hears of
	   it. SmsEvHit. */
	SMS_EV_OBJECT_HIT = 16,
	/* A save state was asked for. Cancellable: cancel to refuse; set the
	   payload's reason for the message shown. SmsEvSaveState. */
	SMS_EV_SAVE_STATE_REQUEST = 17,
	/* Mario's models are about to be drawn. Consumable: consume to leave
	   them out (draw the player your own way). SmsEvObject. */
	SMS_EV_HIDE_PLAYER_MODEL = 18,
	/* Mario's status (his action: running, jumping, swimming...) is about
	   to change. `to` and `arg` may be changed. Cancellable: cancel and he
	   stays as he is. SmsEvPlayerStatus. */
	SMS_EV_PLAYER_STATUS = 19,
	/* Mario is about to lose health (or air, under water). `amount` may be
	   changed. Cancellable: cancel and he loses nothing. SmsEvDamage. */
	SMS_EV_PLAYER_DAMAGE = 20,
	/* A Shine was collected for the first time on this file. SmsEvShine. */
	SMS_EV_SHINE_COLLECTED = 21,
	/* The game camera placed itself for this tick and is about to make its
	   matrices: position, target, field of view may be changed (a
	   first-person camera, a field of view setting). SmsEvCamera. */
	SMS_EV_CAMERA = 22,
	/* An object is about to move this tick (after = 0) or has (after = 1);
	   each "before" has its "after", nested as objects move their parts.
	   Many of them: subscribe only when you need it. SmsEvObjectMove. */
	SMS_EV_OBJECT_MOVE = 23,
	/* Another mod finished loading (or this event is replayed for each mod
	   that loaded before you subscribed). SmsEvMod. */
	SMS_EV_MOD_LOADED = 24,
	/* The game is closing. No payload. */
	SMS_EV_SHUTDOWN = 25,
	/* Multiplayer (api->net). A player joined the game (SmsEvNetPlayer).
	   Every game hears of it; a server mod greets them here. */
	SMS_EV_NET_PLAYER_JOINED = 26,
	/* A player left (SmsEvNetPlayer). */
	SMS_EV_NET_PLAYER_LEFT = 27,
	/* Host: a player arrived in the stage it plays (SmsEvNetPlayer): just
	   joined, after a lost life's reload, or following the host to a new
	   stage. Sticky commands have been replayed to them by then. */
	SMS_EV_NET_PLAYER_ARRIVED = 28,
	/* Host: it opened a new stage session, which every player follows
	   (SmsEvNetSession). Sticky commands of the last session are gone. */
	SMS_EV_NET_SESSION = 29,
	/* API 1.4. The stage's opening is over (its camera cutscene, the
	   episode's name, "Go!") and the player has the controls: the first
	   tick of play. SmsEvStage: the stage, and how it was reached (reason). */
	SMS_EV_STAGE_START = 30,
	/* API 1.5. Mario's FLUDD is about to switch nozzle, swap with its second
	   nozzle, or a nozzle's box is about to be set up (SMS_NOZZLE_*).
	   `nozzle` may be changed. Cancellable: cancel and the nozzle stays as
	   it is (for a box: it is left out). Only the player's Mario.
	   SmsEvPlayerNozzle. */
	SMS_EV_PLAYER_NOZZLE = 31,

	/* core->event_id hands out ids from here for mods' own events. */
	SMS_EV_CUSTOM_BASE = 0x10000
} SmsEventType;

typedef struct SmsEvent {
	uint32_t type;            /* SmsEventType, or a custom id */
	uint32_t size;            /* of *data, as the game built it */
	void* data;               /* the payload for the type, or NULL */
	int cancelled;            /* an earlier handler cancelled it */
	const char* cancelled_by; /* that mod's id */
} SmsEvent;

/* A handler's answer. */
#define SMS_CONTINUE 0
#define SMS_CANCEL   1 /* cancellable events: veto; consumable events: consume */

typedef int (*SmsEventFn)(SmsMod* mod, SmsEvent* ev, void* user);

/* A script runtime's start and stop (core->host_runtime, API 1.1): `script`
   is a mod whose mod.ini says `runtime = <name>`; start runs it (its file is
   mod_dir(script) + "/" + mod_info(script, "script")) and returns 0, or
   refuses; stop ends it. Every API call made for the script passes `script`,
   so it has its own settings, options, keys and HUD slots. */
typedef int (*SmsScriptStartFn)(SmsMod* host, SmsMod* script, void* user);
typedef void (*SmsScriptStopFn)(SmsMod* host, SmsMod* script, void* user);

/* Priorities: lower runs first. Handlers of the same priority run in the
   order the mods loaded. Use MONITOR for handlers that only watch the
   outcome (they run last and must not change anything). */
#define SMS_PRIORITY_FIRST   (-1000)
#define SMS_PRIORITY_EARLY   (-100)
#define SMS_PRIORITY_NORMAL  0
#define SMS_PRIORITY_LATE    100
#define SMS_PRIORITY_MONITOR 1000

typedef enum SmsGameMode {
	SMS_MODE_BOOT = 0,     /* starting, logos */
	SMS_MODE_TITLE = 1,    /* title screen */
	SMS_MODE_MENU = 2,     /* file select and other menus */
	SMS_MODE_STAGE = 3,    /* a stage is being played */
	SMS_MODE_MOVIE = 4,    /* a full-screen movie */
	SMS_MODE_OTHER = 5
} SmsGameMode;

typedef struct SmsEvFrame {
	uint32_t frame; /* frames since start */
	int mode;       /* SmsGameMode */
} SmsEvFrame;

typedef struct SmsEvTick {
	uint32_t tick;        /* ticks since the stage loaded */
	float dt;             /* seconds per tick (1 / ticks_per_second) */
	int ticks_per_second; /* 30, 60 or 120 (the frame rate setting) */
} SmsEvTick;

typedef struct SmsEvDraw {
	float canvas_w, canvas_h; /* draw->canvas: the game picture, 480 high */
	uint32_t frame;
	int mode;                 /* SmsGameMode */
} SmsEvDraw;

typedef struct SmsEvStage {
	int area, episode; /* the stage: see docs/MODDING.md for the area numbers */
	/* API 1.4 (check ev->size): why the game leaves (SMS_EV_STAGE_LEAVING,
	   SMS_EV_LIFE_LOST, SMS_EV_FILE_SELECTED) or came (SMS_EV_STAGE_LOADED,
	   SMS_EV_STAGE_START): SMS_LEAVE_*. For SMS_LEAVE_SHINE, the Shine
	   (0..119); otherwise, or not known, -1. */
	int reason;
	int shine;
} SmsEvStage;

/* SmsEvStage.reason (API 1.4) */
typedef enum SmsLeaveReason {
	SMS_LEAVE_OTHER = 0,     /* none of these: the title, a movie's way on */
	SMS_LEAVE_WAY_OUT = 1,   /* the stage's own way out: a pipe, a painting, a door, an event */
	SMS_LEAVE_SHINE = 2,     /* a Shine ended the stage; its Shine Get and its save are over */
	SMS_LEAVE_EXIT_AREA = 3, /* the pause menu's Exit Area */
	SMS_LEAVE_LIFE_LOST = 4, /* Mario lost a life */
	SMS_LEAVE_GAME_OVER = 5, /* Mario lost his last life */
	SMS_LEAVE_WARP = 6,      /* a mod's game->warp */
	SMS_LEAVE_FILE = 7       /* a file was chosen at file select */
} SmsLeaveReason;

/* SmsEvStage.episode: no episode named; for a world's way in from the plaza
   (SMS_AREA_WORLD), its episode select chooses. */
#define SMS_EPISODE_SELECT 255

typedef struct SmsEvStageExit {
	uint32_t code;     /* the game's (area + 1) << 8 | episode request */
	int area, episode; /* decoded from code */
	SmsObject actor;   /* what asked (a pipe, a painting), or NULL */
} SmsEvStageExit;

typedef enum SmsItemKind {
	SMS_ITEM_OTHER = 0,
	SMS_ITEM_COIN = 1,
	SMS_ITEM_RED_COIN = 2,
	SMS_ITEM_BLUE_COIN = 3,
	SMS_ITEM_ONE_UP = 4
} SmsItemKind;

typedef struct SmsEvItem {
	SmsObject item, taker;
	int kind;            /* SmsItemKind */
	uint32_t actor_type; /* the game's actor type id */
	SmsVec3 position;
} SmsEvItem;

/* The GameCube controller as the game reads it (PADStatus). */
typedef struct SmsPadStatus {
	uint16_t buttons; /* SMS_PAD_* */
	int8_t stick_x, stick_y, cstick_x, cstick_y;
	uint8_t trigger_l, trigger_r, analog_a, analog_b;
	int8_t err;       /* 0 = connected */
} SmsPadStatus;

#define SMS_PAD_LEFT  0x0001
#define SMS_PAD_RIGHT 0x0002
#define SMS_PAD_DOWN  0x0004
#define SMS_PAD_UP    0x0008
#define SMS_PAD_Z     0x0010
#define SMS_PAD_R     0x0020
#define SMS_PAD_L     0x0040
#define SMS_PAD_A     0x0100
#define SMS_PAD_B     0x0200
#define SMS_PAD_X     0x0400
#define SMS_PAD_Y     0x0800
#define SMS_PAD_START 0x1000

typedef struct SmsEvPadRead {
	SmsPadStatus* pads; /* [4], changeable */
} SmsEvPadRead;

typedef struct SmsEvKey {
	int scancode; /* SDL scancode (USB HID usage): input->key_from_name("F3") */
	int down;     /* 1 pressed, 0 released */
	int repeat;   /* held down: a key repeat */
	int modifiers;/* SMS_KEYMOD_* */
} SmsEvKey;

#define SMS_KEYMOD_SHIFT 1
#define SMS_KEYMOD_CTRL  2
#define SMS_KEYMOD_ALT   4

typedef struct SmsEvCutscene {
	const char* name; /* the camera's name */
	int has_position;
	SmsVec3 position; /* the point it centres on */
	int frames;
	SmsObject actor;
} SmsEvCutscene;

typedef struct SmsEvGoop {
	SmsVec3 position;
	float size;
} SmsEvGoop;

typedef struct SmsEvHit {
	SmsObject object, sender;
	uint32_t message; /* the game's HIT_MESSAGE_* (see docs/MODDING.md) */
} SmsEvHit;

typedef struct SmsEvSaveState {
	const char* reason; /* set when cancelling: shown to the player */
} SmsEvSaveState;

typedef struct SmsEvObject {
	SmsObject object;
} SmsEvObject;

typedef struct SmsEvPlayerStatus {
	uint32_t from;     /* the status he has */
	uint32_t to, arg;  /* the status asked for, and its argument; changeable */
	int forced;
} SmsEvPlayerStatus;

typedef struct SmsEvDamage {
	int amount;     /* changeable */
	int underwater; /* air is lost, not health */
} SmsEvDamage;

/* SmsEvPlayerNozzle.reason (API 1.5) */
typedef enum SmsNozzleReason {
	SMS_NOZZLE_SWITCH = 0, /* FLUDD switches to `nozzle` (the box, the game, set_nozzle) */
	SMS_NOZZLE_SWAP = 1,   /* the nozzle swap (the buttons): `nozzle` is the one swapped to */
	SMS_NOZZLE_BOX = 2     /* a nozzle's box in the stage: `nozzle` is the box's nozzle */
} SmsNozzleReason;

typedef struct SmsEvPlayerNozzle {
	int nozzle;  /* SmsPlayer.nozzle's numbers (docs/MODDING.md, Nozzles); changeable */
	int animate; /* the switch is shown (not for a box) */
	int reason;  /* SmsNozzleReason */
} SmsEvPlayerNozzle;

typedef struct SmsEvShine {
	int shine;      /* the Shine's number (0..119) */
	int total;      /* Shines on the file, counting this one */
} SmsEvShine;

typedef struct SmsEvCamera {
	SmsVec3 position, target; /* changeable */
	float fovy;               /* degrees, changeable */
	float aspect;
	float roll;               /* reserved: 0 */
} SmsEvCamera;

typedef struct SmsEvObjectMove {
	SmsObject object;
	int after;
} SmsEvObjectMove;

typedef struct SmsEvNetPlayer {
	int slot;          /* 0..7; the host is 0 */
	const char* name;  /* as the player set it, or their character's */
	int character;     /* SMS_NET_CHAR_* */
} SmsEvNetPlayer;

typedef struct SmsEvNetSession {
	int area, episode;
	uint32_t session;  /* its number, for api->net->session() */
} SmsEvNetSession;

typedef struct SmsEvMod {
	const char* id;
	const char* name;
	const char* version;
} SmsEvMod;

/* ------------------------------------------------------------------ tables */

typedef struct SmsCoreApi {
	uint32_t size;
	/* printf to the game's log, prefixed with the mod's id. Any thread. */
	void (*log)(SmsMod* mod, int level, const char* fmt, ...) SMS_MOD_PRINTF(3, 4);
	/* Hear of an event. Returns a handle (> 0) for unsubscribe, 0 on error. */
	int (*subscribe)(SmsMod* mod, uint32_t type, SmsEventFn fn, void* user, int priority);
	void (*unsubscribe)(SmsMod* mod, int handle);
	/* Custom events: the id for a name ("author.mod.event"); the same name
	   gives every mod the same id. */
	uint32_t (*event_id)(const char* name);
	/* Send a custom event to its subscribers; returns 1 if one cancelled. */
	int (*emit)(SmsMod* mod, uint32_t type, void* data, uint32_t size);
	/* Services: an interface (a struct of functions, say) under a name for
	   other mods. `version` is yours to bump. */
	int (*provide)(SmsMod* mod, const char* name, uint32_t version, const void* iface);
	/* The interface provided under `name` at `min_version` or later, or NULL.
	   List the providing mod in your mod.ini's `after` so it loads first. */
	const void* (*require)(SmsMod* mod, const char* name, uint32_t min_version);
	/* About mods. */
	const char* (*mod_id)(SmsMod* mod);
	const char* (*mod_dir)(SmsMod* mod);  /* the mod's folder (read only) */
	const char* (*data_dir)(SmsMod* mod); /* a folder the mod may write, made on first use */
	int (*is_loaded)(const char* id);
	/* The game build: "<commit>[-eclipse]", and pointer size (4 or 8). */
	const char* (*game_build)(void);
	int (*pointer_size)(void);
	/* Seconds since start, steady. */
	double (*time)(void);
	/* API 1.1. A value of the mod's mod.ini ("name", "version", "script",
	   or a key of the mod's own), or NULL. */
	const char* (*mod_info)(SmsMod* mod, const char* key);
	/* API 1.1. Run the script mods whose mod.ini says `runtime = <name>`
	   (the lua mod hosts "lua"). One host a runtime; script mods load after
	   every native mod, and stop before their host unloads. */
	int (*host_runtime)(SmsMod* host, const char* runtime, SmsScriptStartFn start, SmsScriptStopFn stop, void* user);
} SmsCoreApi;

typedef struct SmsGameState {
	uint32_t size;
	int mode;             /* SmsGameMode */
	int area, episode;    /* the current stage, or -1 */
	int stage_ready;      /* a stage is loaded and Mario exists */
	uint32_t frame;       /* frames since start */
	uint32_t tick;        /* ticks since the stage loaded */
	int ticks_per_second; /* 30, 60 or 120 */
	int eclipse;          /* a Super Mario Eclipse build */
	int save_file;        /* the chosen file 0..2, or -1 */
	int paused;           /* the pause menu, map or a save screen is up (no ticks) */
} SmsGameState;

/* API 1.4: game->object_info's */
typedef struct SmsObjectInfo {
	uint32_t size;      /* sizeof, set before calling */
	uint32_t type;      /* its actor type */
	char kind[32];      /* as the stage names it ("WoodBlockCone"); "" not a map object */
	SmsVec3 position, rotation, scale; /* rotation: degrees about x, y, z */
	int alive;          /* in the stage (not killed) */
	int hidden;         /* out of the stage until shown */
	int spawned;        /* a copy SMS_CMD_SPAWN made: its id; 0 the stage's own */
} SmsObjectInfo;

/* API 1.4: game->ground's */
typedef struct SmsGround {
	uint32_t size;      /* sizeof, set before calling */
	float floor;        /* the floor's height below the point */
	uint32_t floor_type;/* its collision type (TBGCheckData::mBGType, without the camera and shadow bits) */
	int water;          /* the floor is water's surface */
	int has_water;      /* there is water's surface over or at the point: water_y */
	float water_y;
	float roof;         /* the roof's height above (a large number when there is none) */
} SmsGround;

typedef struct SmsGameApi {
	uint32_t size;
	void (*state)(SmsGameState* out); /* set out->size = sizeof *out first */
	/* The save file's flags (TFlagManager): numbers in docs/MODDING.md. */
	int32_t (*flag_get)(uint32_t id);
	void (*flag_set)(uint32_t id, int32_t value);
	/* Shorthands over the flags. */
	int (*coins)(void);
	int (*lives)(void);
	int (*shines)(void);
	int (*blue_coins)(void);
	int (*has_shine)(int shine);
	/* Leave for another stage as a pipe would; 0 when no stage is played. */
	int (*warp)(int area, int episode);
	/* Objects. find_object: by the name the stage gives it, or NULL. */
	SmsObject (*find_object)(const char* name);
	const char* (*object_name)(SmsObject obj);
	/* Objects that came with events, or from find_object, that have a place
	   in the world: 1 and the position, else 0. */
	int (*object_position)(SmsObject obj, SmsVec3* out);
	int (*object_set_position)(SmsObject obj, const SmsVec3* pos);
	/* The display name of an area ("Delfino Plaza"), or "". */
	const char* (*area_name)(int area);
	/* API 1.2: the stage's trigger volumes ("cubes", SMS_CUBE_*), as the
	   game's own scripts test them: how many a set has (0 when the stage has
	   none), and whether point p is in cube `index` of a set (1 or 0; -1:
	   no such cube). */
	int (*cube_count)(int set);
	int (*in_cube)(int set, int index, const SmsVec3* p);
	/* API 1.4: the stage's objects as this game has them (on the host, the
	   copy its server mods judge by), picked as SMS_CMD_OBJECT picks them:
	   actor types lo to hi, kind ("WoodBlockCone"; "@id" a copy SMS_CMD_SPAWN
	   made; "" any). object_count: how many; object_info: the index-th of
	   them into out (out->size set first), 1, or 0 when there is none. */
	int (*object_count)(uint32_t type_lo, uint32_t type_hi, const char* kind);
	int (*object_info)(uint32_t type_lo, uint32_t type_hi, const char* kind, int index, SmsObjectInfo* out);
	/* The ground at a point, as Mario finds it: the floor below p (from a
	   little above it), whether that is water, the water's surface over p,
	   and the roof above. 0 off the stage's map. */
	int (*ground)(const SmsVec3* p, SmsGround* out);
} SmsGameApi;

/* The game's cube sets (api->game->in_cube). */
typedef enum SmsCubeSet {
	SMS_CUBE_CAMERA = 0,  /* camera changes */
	SMS_CUBE_AREA = 1,    /* the stage's areas */
	SMS_CUBE_FAST_A = 2,  /* "fast cubes": what event scripts test (a course's start, its checkpoints) */
	SMS_CUBE_FAST_B = 3,
	SMS_CUBE_FAST_C = 4,
	SMS_CUBE_MIRROR = 5,
	SMS_CUBE_WIRE = 6,
	SMS_CUBE_STREAM = 7,  /* water currents */
	SMS_CUBE_SHADOW = 8,
	SMS_CUBE_SOUND_CHANGE = 9,
	SMS_CUBE_SOUND_EFFECT = 10
} SmsCubeSet;

typedef struct SmsPlayer {
	uint32_t size;       /* set to sizeof before calling player->get */
	int valid;           /* 0: no Mario now (menus, loading) */
	SmsVec3 position;
	SmsVec3 velocity;    /* units a tick */
	float forward_speed; /* units a tick along his facing */
	float yaw;           /* degrees: 0 faces +Z, 90 faces +X */
	int16_t face_angle[3];
	int health, max_health; /* 0..8 */
	int water, max_water;   /* FLUDD's tank */
	int has_fludd;
	int nozzle;          /* the second nozzle FLUDD has: 0 none, see docs */
	uint32_t status;     /* MARIO_STATUS_* */
	uint32_t status_timer;
	int on_ground, in_water, on_yoshi;
	SmsVec3 floor;       /* the ground below him */
	SmsObject object;    /* the game's TMario */
} SmsPlayer;

typedef struct SmsPlayerApi {
	uint32_t size;
	int (*get)(SmsPlayer* out); /* returns out->valid */
	int (*set_position)(const SmsVec3* pos);
	int (*set_velocity)(const SmsVec3* vel);
	int (*set_forward_speed)(float speed);
	int (*set_health)(int health);
	int (*set_water)(int water);
	/* Ask for a status as the game does (it may refuse). */
	int (*set_status)(uint32_t status, uint32_t arg);
	/* API 1.5. FLUDD switches to a nozzle (SmsPlayer.nozzle's numbers) as the
	   game does, `animate` showing the switch: 1, or 0 when there is no
	   Mario, no FLUDD, or an SMS_EV_PLAYER_NOZZLE handler refused. */
	int (*set_nozzle)(int nozzle, int animate);
	/* API 1.5. Mario has FLUDD (1) or has not (0); 0 when there is no Mario. */
	int (*set_fludd)(int has);
	/* API 1.5. What he wears: the aloha shirt, the sunglasses, the diving
	   helmet (1 on, 0 off); 0 when there is no Mario. */
	int (*set_outfit)(int shirt, int sunglasses, int helmet);
} SmsPlayerApi;

typedef struct SmsCamera {
	uint32_t size;
	int valid;
	SmsVec3 position, target, up;
	float fovy, aspect, near_z, far_z;
	float view[3][4]; /* world to camera, as drawn last */
	float proj[4][4];
} SmsCamera;

typedef struct SmsCameraApi {
	uint32_t size;
	int (*get)(SmsCamera* out);
	/* Where a world point is on the canvas (draw->canvas units); returns 1
	   when it is in front of the camera. depth (may be NULL): distance along
	   the view. */
	int (*world_to_canvas)(const SmsVec3* p, float* x, float* y, float* depth);
} SmsCameraApi;

typedef enum SmsAnchor {
	SMS_ANCHOR_TOP_LEFT = 0,
	SMS_ANCHOR_TOP = 1,
	SMS_ANCHOR_TOP_RIGHT = 2,
	SMS_ANCHOR_LEFT = 3,
	SMS_ANCHOR_CENTER = 4,
	SMS_ANCHOR_RIGHT = 5,
	SMS_ANCHOR_BOTTOM_LEFT = 6,
	SMS_ANCHOR_BOTTOM = 7,
	SMS_ANCHOR_BOTTOM_RIGHT = 8
} SmsAnchor;

/* text flags */
#define SMS_TEXT_SHADOW  1
#define SMS_TEXT_OUTLINE 2
#define SMS_TEXT_CENTER  4  /* x is the middle */
#define SMS_TEXT_RIGHT   8  /* x is the right end */

/* The canvas is the game's picture, 480 units high and as wide as the
   picture's aspect (640 at 4:3, about 853 at 16:9), from its top left. It is
   drawn at the window's resolution over the game and under the port's own
   menus. Text is ASCII. */
typedef struct SmsDrawApi {
	uint32_t size;
	void (*canvas)(float* w, float* h);
	void (*rect)(float x, float y, float w, float h, SmsColor c);
	void (*rect_outline)(float x, float y, float w, float h, float thickness, SmsColor c);
	void (*rect_gradient)(float x, float y, float w, float h, SmsColor top, SmsColor bottom);
	void (*line)(float x0, float y0, float x1, float y1, float thickness, SmsColor c);
	void (*triangle)(float x0, float y0, float x1, float y1, float x2, float y2, SmsColor c);
	void (*circle)(float cx, float cy, float radius, SmsColor c);
	void (*circle_outline)(float cx, float cy, float radius, float thickness, SmsColor c);
	/* `size` is the line height; returns the width drawn. */
	float (*text)(float x, float y, float size, SmsColor c, int flags, const char* text);
	float (*text_width)(float size, const char* text);
	/* Images: PNG, JPEG, TGA or BMP, from a path in the mod's folder. */
	SmsImage (*image_load)(SmsMod* mod, const char* path);
	/* An image from pixels: RGBA8, w x h, row 0 at the top (copied). */
	SmsImage (*image_create)(SmsMod* mod, const uint8_t* rgba, int w, int h);
	void (*image_free)(SmsMod* mod, SmsImage image);
	int (*image_size)(SmsImage image, int* w, int* h);
	void (*image)(SmsImage image, float x, float y, float w, float h, SmsColor tint);
	void (*image_region)(SmsImage image, float x, float y, float w, float h, float u0, float v0, float u1, float v1,
	                     SmsColor tint);
	/* Only draw inside a rectangle until the matching pop (nest freely). */
	void (*clip_push)(float x, float y, float w, float h);
	void (*clip_pop)(void);
	/* The world, through the game camera, drawn over the scene (not hidden
	   behind walls). */
	void (*world_line)(const SmsVec3* a, const SmsVec3* b, float thickness, SmsColor c);
	void (*world_box)(const SmsVec3* center, const SmsVec3* half_size, float thickness, SmsColor c);
	void (*world_ring)(const SmsVec3* center, float radius, float thickness, SmsColor c); /* level */
	/* Text whose middle bottom is at a world point; size shrinks with
	   distance when `scaled`. Returns 1 if drawn. */
	int (*world_text)(const SmsVec3* p, float size, int scaled, SmsColor c, int flags, const char* text);
	/* A w x h rectangle at an anchor of the canvas for this frame, below (or
	   above, at the bottom anchors) the ones other mods took there this
	   frame, so panels never overlap. Returns 0 if it does not fit. */
	int (*hud_slot)(SmsMod* mod, int anchor, float w, float h, SmsRect* out);
} SmsDrawApi;

typedef struct SmsPad {
	uint32_t size;
	int connected;
	uint16_t held, pressed, released; /* SMS_PAD_*; pressed/released: this frame */
	float stick_x, stick_y;           /* -1..1, up is +y */
	float cstick_x, cstick_y;
	float trigger_l, trigger_r;       /* 0..1 */
} SmsPad;

typedef struct SmsInputApi {
	uint32_t size;
	/* Controller `port` (0..3) as read this frame, before any mod changed it
	   and before a capture blanked it. */
	int (*pad)(int port, SmsPad* out);
	/* Keyboard, by scancode (key_from_name): down now; went down this frame. */
	int (*key_down)(int scancode);
	int (*key_pressed)(int scancode);
	int (*key_from_name)(const char* name); /* SDL key names: "F3", "Space", "A"; -1 if unknown */
	const char* (*key_name)(int scancode);
	/* A key for a named action of the mod: `default_key` unless the player
	   set another in the mod's settings (key.<action>). Warns when the key
	   is the port's or another mod's. Returns the scancode or -1. */
	int (*bind_key)(SmsMod* mod, const char* action, const char* default_key);
	/* Take the controllers and keyboard from the game while a menu is open:
	   the game sees nothing pressed and keys reach only you. Returns 0 if
	   another mod has them. Release with on = 0. */
	int (*capture)(SmsMod* mod, int on);
	/* The id of the mod that has them, or NULL. */
	const char* (*captured_by)(void);
} SmsInputApi;

typedef enum SmsOptionType {
	SMS_OPTION_BOOL = 0,
	SMS_OPTION_INT = 1,
	SMS_OPTION_FLOAT = 2,
	SMS_OPTION_CHOICE = 3, /* an int index into `choices` */
	SMS_OPTION_ACTION = 4  /* a button: on_change is called when chosen */
} SmsOptionType;

/* A setting the player can change in the Mods menu (F1). Its value is kept
   in the mod's settings under `key`, read with config->get_*. */
typedef struct SmsOption {
	uint32_t size; /* sizeof(SmsOption) */
	const char* key;
	const char* label;
	int type;      /* SmsOptionType */
	float def, min, max, step;
	const char* const* choices;
	int choice_count;
	void (*on_change)(SmsMod* mod, const char* key, void* user);
	void* user;
} SmsOption;

/* Settings, kept per mod in <data_dir>/settings.ini (key = value). */
typedef struct SmsConfigApi {
	uint32_t size;
	int (*get_int)(SmsMod* mod, const char* key, int def);
	float (*get_float)(SmsMod* mod, const char* key, float def);
	/* The returned string is good until the key is set again. */
	const char* (*get_string)(SmsMod* mod, const char* key, const char* def);
	void (*set_int)(SmsMod* mod, const char* key, int value);
	void (*set_float)(SmsMod* mod, const char* key, float value);
	void (*set_string)(SmsMod* mod, const char* key, const char* value);
	/* Written at once and at exit; call after a batch of sets if you like. */
	void (*save)(SmsMod* mod);
	int (*add_option)(SmsMod* mod, const SmsOption* option);
} SmsConfigApi;

/* Some of the game's system sounds, for audio->play_system_sound. */
#define SMS_SOUND_HEAL   0x4801 /* health back */
#define SMS_SOUND_APPEAR 0x480D /* something appears */
#define SMS_SOUND_CURSOR 0x480F /* menu cursor */
#define SMS_SOUND_DECIDE 0x4810 /* menu choice */
#define SMS_SOUND_COIN   0x4811
#define SMS_SOUND_NEWS   0x4812 /* a message arrives */
#define SMS_SOUND_SHINE  0x4814 /* Shine get */
#define SMS_SOUND_CANCEL 0x481D /* menu back */
#define SMS_SOUND_DAMAGE 0x4823

typedef struct SmsAudioApi {
	uint32_t size;
	/* One of the game's system sound effects (MSD_SE_SY_*: menu blips,
	   coin, 1-up...), not placed in the world. */
	void (*play_system_sound)(uint32_t id);
} SmsAudioApi;

/* The game's own objects, for mods built against the decomp's headers with
   the same compiler and pointer size as the game. Layouts change between
   game builds: check native->build() against what you were built for. */
typedef struct SmsNativeApi {
	uint32_t size;
	const char* (*build)(void);
	/* "gpMarioOriginal", "gpMarDirector", "gpApplication", "gpCamera",
	   "gpItemManager", "gpMap", "TFlagManager": the object, or NULL. */
	void* (*get)(const char* name);
} SmsNativeApi;

/* ------------------------------------------------------------------ multiplayer (API 1.2) */

/* api->net: the multiplayer game this one is in, and, on the host, the word
 * it sends players' games. A server mod (`side = server` in its mod.ini)
 * runs on the host only, and players join with the plain game: everything it
 * shows or does in their games goes through these commands, which every
 * build of the game carries out (docs/MODDING.md, "Server mods"). */

typedef enum SmsNetRole {
	SMS_NET_OFF = 0,    /* not in a multiplayer game */
	SMS_NET_HOST = 1,   /* hosting it (a dedicated server too) */
	SMS_NET_CLIENT = 2  /* joined someone's */
} SmsNetRole;

#define SMS_NET_MAX_PLAYERS 8
#define SMS_NET_SLOT(n) (1u << (n)) /* one player, for `to` */
#define SMS_NET_ALL 0xFFu           /* every player (the host's own game too) */

typedef enum SmsNetCharacter {
	SMS_NET_CHAR_MARIO = 0, SMS_NET_CHAR_LUIGI, SMS_NET_CHAR_WARIO, SMS_NET_CHAR_METAL_MARIO,
	SMS_NET_CHAR_FIRE_MARIO, SMS_NET_CHAR_PURPLE_MARIO, SMS_NET_CHAR_ORANGE_MARIO, SMS_NET_CHAR_ICE_MARIO
} SmsNetCharacter;

/* SmsNetPlayer.flags */
#define SMS_NETP_VISIBLE    1u   /* in play and drawn */
#define SMS_NETP_SURFING    2u   /* riding a Blooper */
#define SMS_NETP_YOSHI      4u   /* riding Yoshi */
#define SMS_NETP_SPECTATING 8u   /* out of lives, watching */
#define SMS_NETP_PAUSED     16u  /* their game is paused */
#define SMS_NETP_CUTSCENE   32u  /* in a cutscene */

typedef struct SmsNetPlayer {
	uint32_t size;     /* set to sizeof before calling net->player */
	int slot;
	int server;        /* the dedicated server itself: no player (slot 0) */
	int character;     /* SMS_NET_CHAR_* */
	char name[24];
	SmsColor color;    /* their character's colour, for text about them */
	int in_stage;      /* host: arrived in the stage session it plays */
	SmsVec3 position;  /* where they are, as their last update said */
	float yaw;         /* their facing, degrees as SmsPlayer's */
	uint32_t flags;    /* SMS_NETP_* */
	int ping;          /* round trip to them (host) or the host (client), ms; -1 unknown */
	/* API 1.3 (check out->size): as their game said when it joined (host;
	   any game knows its own): its version ("2026.10.18.1", the launcher's;
	   "" unknown) and the mod API it carries ((major << 16) | minor; 0
	   unknown), so a server mod knows which commands it understands. */
	char game_version[32];
	uint32_t api_version;
	/* API 1.4 (check out->size): what they press, as their last update
	   said (their own controller before the game sees it; held players'
	   too): SMS_PAD_* buttons, the sticks (-128..127) and triggers (0..255). */
	SmsPadStatus pad;
} SmsNetPlayer;

/* Commands: what the host's server mod has players' games do. Each fills
 * the fields its type names below; the rest are ignored.
 *   to:        the players (SMS_NET_ALL, SMS_NET_SLOT(n), or bits ORed);
 *              the host's own game carries out its own bit (slot 0) too.
 *   delay_ms:  carried out this long after it was sent, at the same moment
 *              in every game (each takes off half its round trip): a start
 *              lined up across the players.
 *   flags:     SMS_CMD_STICKY: kept for the stage session, and carried out
 *              again for each player who arrives in it later (a newcomer, a
 *              player back after falling in). A sticky command replaces the
 *              last one of its kind (its type; for ACTORS, its type range;
 *              for STOP_SCRIPT, its script). */
typedef enum SmsCmdType {
	/* Big text in the middle of the screen, in color (i[0], 0xRRGGBBAA);
	   text "" hides it. */
	SMS_CMD_BANNER = 1,
	/* A line of news at the top for a few seconds: text. */
	SMS_CMD_NOTICE = 2,
	/* A standing line at the top, shown when there is no news: text ("": none). */
	SMS_CMD_STATUS = 3,
	/* The HUD's clock: i[0] SMS_TIMER_*, i[1] its time in hundredths of a
	   second, from which it counts on by itself. */
	SMS_CMD_TIMER = 4,
	/* The game's "Ready..." then "Go!", "Go!" i[0] ms after the command is
	   carried out (each game starts "Ready..." as long before as its frame
	   rate needs: about 2 s at 30 fps, 1 s at 60). */
	SMS_CMD_READY_GO = 5,
	/* A sound: i[0] a system sound (audio->play_system_sound's numbers,
	   SMS_SOUND_*; an unsigned number stored in the int). */
	SMS_CMD_SOUND = 6, /* API 1.4: i[1] 1 plays it at f[0..2] in the world (older games: as a system sound) */
	/* Music: i[0] the game's background music id (MSD_BGM_*, an unsigned
	   number stored in the int: SMS_MUSIC_*); 0 stops the music a command started. */
	SMS_CMD_MUSIC = 7,
	/* The player to f[0..2] facing f[3] degrees (as a warp pipe puts them). */
	SMS_CMD_WARP = 8,
	/* i[0] 1: the player's controls do nothing (and a Blooper they ride stops); 0: theirs again. */
	SMS_CMD_HOLD = 9,
	/* The player onto a Blooper of their own game, at f[0..2] facing f[3]:
	   the first of actor types i[0] to i[1] that is free (0x400000C5 to
	   0x400000C7: red, yellow, green), brought out if hidden. */
	SMS_CMD_MOUNT = 10,
	/* i[0] a slot: the player's camera rides with that player, theirs out of
	   play, as a player out of lives watches; -1: back in play where they are. */
	SMS_CMD_WATCH = 11,
	/* The stage's actors of types i[0] to i[1] (TLiveActor::getActorType;
	   Shines are 0x20000013): i[2] SMS_ACTORS_*. */
	SMS_CMD_ACTORS = 12,
	/* Stop the stage's event script loaded from text (its resource path,
	   "/scene/map/sp/gessoride.sb"): the time trial of a course, a story event. */
	SMS_CMD_STOP_SCRIPT = 13,
	/* i[0] 1: the player stays in this stage: their own ways out (pipes,
	   doors, Exit Area) lead nowhere, and only the host moves them on; 0: free. */
	SMS_CMD_STAY = 14,
	/* API 1.4: one of the stage's objects: of actor types i[0] to i[1] and of
	   the kind text, as the stage names it ("WoodBlockCone", "normallift",
	   "rollblocky"; "" any), the i[2]th (0 first, in the order every game
	   loads them; -1: each of them). i[3] SMS_OBJECT_* says what, with f[]. */
	SMS_CMD_OBJECT = 15,
	/* API 1.4. A copy of one of the stage's objects (picked as for
	   SMS_CMD_OBJECT: i[0]..i[1] types, text its kind, i[2] which), or with
	   types 0 and 0 an item by its name in text (SMS_SPAWN_*: "coin",
	   "coin_red", "1up", "FruitBanana"...), at f[0..2] turned f[3]
	   degrees. i[3] is its id (1 or more, the mod's own): SMS_CMD_OBJECT and
	   game->object_info reach it as kind "@<id>". Sticky per id, as every
	   copy should be: a player arriving later has the same. */
	SMS_CMD_SPAWN = 16,
	/* The player: i[0] SMS_PLAYER_*. */
	SMS_CMD_PLAYER = 17,
	/* What the player may do: i[0] SMS_RULE_* bits (0: all of it). */
	SMS_CMD_RULES = 18,
	/* A board at the top right: row i[0] (0 to 7; -1 every row) is text, in
	   i[1]'s colour (0xRRGGBBAA; 0 white); "" clears the row. Sticky per row. */
	SMS_CMD_BOARD = 19,
	/* The HUD's parts hidden: i[0] SMS_HUD_* bits (0 all shown again). */
	SMS_CMD_HUD = 20,
	/* Marker i[0] (0 to 7): text (an arrow when "") in i[2]'s colour over
	   f[0..2] in the world, or over player i[1] (-1: the place); i[3] 1 takes
	   it away. Sticky per marker. */
	SMS_CMD_MARKER = 21,
	/* The screen: i[0] SMS_SCREEN_*, in i[1]'s colour (0xRRGGBBAA), over f[0] seconds. */
	SMS_CMD_SCREEN = 22,
	/* The camera shakes, f[0] strong (1 a hard hit), for f[1] seconds. */
	SMS_CMD_SHAKE = 23,
	/* The controller rumbles: i[0] 0 weak, 1 hard, i[1] times (1 or more). */
	SMS_CMD_RUMBLE = 24
} SmsCmdType;

#define SMS_CMD_STICKY 1u
/* API 1.4: send nothing, and forget the sticky command of this kind (as
   sticky commands replace each other: the same type, to the same players,
   and for those keyed so, the same object, id, row or marker). */
#define SMS_CMD_UNSTICK 2u

/* Some of the game's music (SMS_CMD_MUSIC) */
#define SMS_MUSIC_EVENT        ((int32_t)0x8001001Fu) /* a race's, a timed event's */
#define SMS_MUSIC_FANFARE_RACE ((int32_t)0x80010026u) /* a race won */

/* SMS_CMD_TIMER's i[0] */
#define SMS_TIMER_HIDE  0
#define SMS_TIMER_SHOW  1  /* stopped at i[1] */
#define SMS_TIMER_UP    2  /* counting up from i[1] */
#define SMS_TIMER_DOWN  3  /* counting down from i[1] to 0 */

/* SMS_CMD_ACTORS's i[2] */
#define SMS_ACTORS_SHOW 0  /* as the stage has them */
#define SMS_ACTORS_HIDE 1  /* out of the stage (unseen, touching nothing), until shown */
#define SMS_ACTORS_KILL 2  /* gone for this stage (a Shine nobody should get) */

/* SMS_CMD_OBJECT's i[3] (API 1.4). Sticky, one of each per object (SHOW and
   HIDE are one: the last says which). */
#define SMS_OBJECT_PLACE 0 /* to f[0..2], turned f[3] degrees about the vertical; where it goes back to as well
                              (not one on a rail: its rail says where it goes) */
#define SMS_OBJECT_SPEED 1 /* f[0] times its own speed (up to 20): a spinner's turning, the other way round
                              when negative; a lift's or a platform's along its rail (0.05 at least) */
#define SMS_OBJECT_PHASE 2 /* back to how the stage loaded it, then f[0] seconds (up to 60) of its own
                              movement at once: a lift or spinner where it would be by then, the same in
                              every game (send it with a delay_ms for the moment it should be so) */
#define SMS_OBJECT_SHOW  3 /* back */
#define SMS_OBJECT_HIDE  4 /* out of the stage: unseen, nothing to touch or stand on, until shown */
#define SMS_OBJECT_SCALE 5 /* f[0..2] its size about x, y, z (1 as made) */
#define SMS_OBJECT_TURN  6 /* f[0..2] its turn about x, y, z, degrees (not one on a rail) */
#define SMS_OBJECT_MOVE  7 /* glides to f[0..2] turned f[3] degrees about the vertical, over f[4] seconds
                              (SmsCmd.f2[0]); where it goes back to as well (not one on a rail) */
#define SMS_OBJECT_KILL  8 /* gone for this stage */

/* SMS_CMD_SPAWN's items (text, with types 0 and 0); the stage must have the
   model of one that is not a coin (one of its kind there, or in its archive) */
#define SMS_SPAWN_COIN     "coin"
#define SMS_SPAWN_RED_COIN "coin_red"
#define SMS_SPAWN_1UP      "1up"

/* SMS_CMD_PLAYER's i[0] */
#define SMS_PLAYER_LOSE_LIFE 0 /* the game's own miss, now */
#define SMS_PLAYER_LAUNCH    1 /* thrown along f[0..2] (a direction) at f[3] units a frame (40 a big push) */
#define SMS_PLAYER_DISMOUNT  2 /* off a Blooper or Yoshi */
#define SMS_PLAYER_HEALTH    3 /* i[1]: 0 to 8 (0 does not lose a life) */
#define SMS_PLAYER_WATER     4 /* i[1]: FLUDD's tank, 0 to 100 percent */
#define SMS_PLAYER_NOZZLE    5 /* i[1]: 0 spray, 1 rocket, 2 underwater, 4 hover, 5 turbo */

/* SMS_CMD_RULES's bits */
#define SMS_RULE_NO_FLUDD   1u /* FLUDD sprays nothing */
#define SMS_RULE_NO_DAMAGE  2u /* no health lost (falls and drops still cost a life) */
#define SMS_RULE_NO_PAUSE   4u /* no pause menu or map */
#define SMS_RULE_NO_PICKUPS 8u /* coins and items stay where they are */

/* SMS_CMD_HUD's bits */
#define SMS_HUD_COINS      1u
#define SMS_HUD_SHINES     2u
#define SMS_HUD_BLUE_COINS 4u
#define SMS_HUD_HEALTH     8u
#define SMS_HUD_WATER      16u
#define SMS_HUD_LIVES      32u
#define SMS_HUD_RED_COINS  64u
#define SMS_HUD_ALL        0xFFFFu

/* SMS_CMD_SCREEN's i[0] */
#define SMS_SCREEN_CLEAR    0 /* the picture back at once */
#define SMS_SCREEN_FADE_OUT 1 /* to the colour */
#define SMS_SCREEN_FADE_IN  2 /* from the colour */
#define SMS_SCREEN_FLASH    3 /* the colour, fading away */

/* net->set_respawn's how */
#define SMS_RESPAWN_NONE    0
#define SMS_RESPAWN_WARP    1 /* there, as a warp puts them */
#define SMS_RESPAWN_BLOOPER 2 /* onto a Blooper there */

typedef struct SmsCmd {
	uint32_t size;     /* sizeof(SmsCmd) */
	uint32_t type;     /* SMS_CMD_* */
	uint32_t flags;    /* SMS_CMD_STICKY */
	uint32_t delay_ms;
	int32_t i[4];
	float f[4];
	const char* text;  /* copied; at most 159 bytes are sent */
	float f2[4];       /* API 1.4 (taken when size covers it): the commands' f[4..7] */
} SmsCmd;

typedef struct SmsNetApi {
	uint32_t size;
	int (*role)(void);              /* SMS_NET_* */
	int (*dedicated)(void);         /* the host is a server, not a player */
	int (*local_slot)(void);        /* this game's player; -1 before a client has one */
	uint32_t (*players)(void);      /* bit n: slot n is in the game (a server's slot 0 too) */
	int (*player)(int slot, SmsNetPlayer* out); /* 1 when that slot is in the game */
	uint32_t (*session)(void);      /* the stage session played (0: none) */
	/* Host only (0 elsewhere): commands to players' games. Returns the
	   players it went to. */
	uint32_t (*send)(uint32_t to, const SmsCmd* cmd);
	/* Host: everyone to that stage, afresh (a new session, even for the
	   stage played now). A dedicated server waiting at the title goes there. */
	int (*load_stage)(int area, int episode);
	/* Host: what the launcher's list of games shows: the game mode's name
	   ("squid-race", at most 15 bytes) and a line of its state. */
	void (*set_mode)(const char* name);
	void (*set_status)(const char* text);
	/* API 1.4, host only. A player's Customize Mario look, Mario's and
	   FLUDD's colours ("hat=2fa846;shirt=2fa846;fluddBody=..."), forced: in
	   their own game and every other, newcomers too, until set again; NULL
	   gives them their own back. look(): theirs as it is now, into out. */
	int (*set_look)(int slot, const char* look);
	int (*look)(int slot, char* out, int size);
	/* Where a player comes back each time they arrive in the session (just
	   joined, back after a lost life): SMS_RESPAWN_*, at p facing yaw
	   degrees, after the session's sticky commands. NONE: the stage's own. */
	int (*set_respawn)(int slot, int how, const SmsVec3* p, float yaw);
} SmsNetApi;

/* ------------------------------------------------------------------ worlds and episodes (API 1.4) */

/* api->world: the game's map of its stages, as its own menus see it. An area
 * is one stage (SmsEvStage.area); a world is a place the plaza leads to, with
 * its areas (Sirena Beach and Hotel Delfino), its secret courses and its
 * eight episodes, each with its Shine. Worlds are named by the area the
 * plaza leads into (Bianco Hills: 2). Super Mario Eclipse numbers its stages
 * its own way: in its builds every area is SMS_AREA_NONE. */

typedef enum SmsAreaKind {
	SMS_AREA_NONE = 0,       /* no such area (or an Eclipse build) */
	SMS_AREA_AIRSTRIP = 1,   /* Delfino Airstrip, where the story starts */
	SMS_AREA_PLAZA = 2,      /* Delfino Plaza, and its parts */
	SMS_AREA_WORLD = 3,      /* a world's way in from the plaza, with its episode select */
	SMS_AREA_WORLD_PART = 4, /* another part of a world (Hotel Delfino, the park of Pinna Park) */
	SMS_AREA_SECRET = 5,     /* a secret course */
	SMS_AREA_FINAL = 6,      /* Corona Mountain */
	SMS_AREA_TITLE = 7       /* the title screen's plaza (15) */
} SmsAreaKind;

typedef struct SmsWorldApi {
	uint32_t size;
	/* What an area is (SMS_AREA_*), and the world it belongs to: the area
	   the plaza leads into (2 for every area of Bianco Hills, its secret
	   courses too; 1 for the plaza's own; 0 the Airstrip; 52 Corona
	   Mountain), or -1. */
	int (*area_kind)(int area);
	int (*world_of)(int area);
	/* A world's episodes (any of its areas names it): how many (8 for the
	   seven worlds, else 0); the Shine an episode is for (0..119, or -1);
	   and its name as the game shows it, copied into out (one line, ASCII;
	   returns its length, 0 when there is none). Episodes count from 0. */
	int (*episode_count)(int area);
	int (*episode_shine)(int area, int episode);
	int (*episode_name)(int area, int episode, char* out, int size);
	/* The episode the world's episode select offers first: the one after
	   the newest whose Shine the file has (the first with none; the last
	   when that is had). -1: no episodes. */
	int (*next_episode)(int area);
	/* The episode a Shine is the story Shine of, with its world in *area
	   (may be NULL); -1 for the others (100 coins, blue coins, the plaza's). */
	int (*shine_episode)(int shine, int* area);
	/* The stage's camera cutscenes, by the names SMS_EV_CUTSCENE gives
	   ("camera_sirena_gate_in"): whether the stage has one by that name; and
	   play one as the game plays its own (the player waits, the HUD hides),
	   centred on focus (NULL: as it was made), for `frames` (-1: as long as
	   it is). Returns 1 when it is on its way: in a stage that is played,
	   with no cutscene, talk or menu on. */
	int (*has_cutscene)(const char* name);
	int (*play_cutscene)(const char* name, const SmsVec3* focus, int frames);
	/* A cutscene (the game's or a mod's) or a talk is on now. */
	int (*cutscene_playing)(void);
} SmsWorldApi;

struct SmsApi {
	uint32_t size;
	uint16_t major, minor;
	const SmsCoreApi* core;
	const SmsGameApi* game;
	const SmsPlayerApi* player;
	const SmsCameraApi* camera;
	const SmsDrawApi* draw;
	const SmsInputApi* input;
	const SmsConfigApi* config;
	const SmsAudioApi* audio;
	const SmsNativeApi* native;
	const SmsNetApi* net;           /* API 1.2 (SMS_API_HAS(api, net)) */
	const SmsWorldApi* world;       /* API 1.4 (SMS_API_HAS(api, world)) */
};

/* ------------------------------------------------------------------ the mod's side */

/* Every mod defines these with SMS_MOD_DEFINE() and sms_mod_load:
 *
 *   SMS_MOD_DEFINE()
 *   SMS_MOD_EXTERN_C SMS_MOD_EXPORT int sms_mod_load(SmsMod* mod, const SmsApi* api) { ...; return 0; }
 *
 * sms_mod_load returns 0 when loaded; anything else refuses (the game logs it
 * and goes on without the mod). sms_mod_unload, if defined, is called at
 * exit and before a hot reload; subscriptions, images, options and
 * services are dropped for you. */
typedef int (*SmsModLoadFn)(SmsMod* mod, const SmsApi* api);
typedef void (*SmsModUnloadFn)(SmsMod* mod);
typedef uint32_t (*SmsModVersionFn)(void);

#define SMS_MOD_DEFINE() \
	SMS_MOD_EXTERN_C SMS_MOD_EXPORT uint32_t sms_mod_api_version(void) { return SMS_MOD_API_VERSION; }

#ifdef __cplusplus
}
#endif

#endif
