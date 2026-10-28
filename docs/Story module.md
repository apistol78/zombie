# Story — design sketch

Native RPG systems for Traktor: items, inventories, quests, dialogs and the actors and places they
talk about. It replaces the Lua prototype in the Zombie game (`Scripts/Items/*`, `Scripts/Quests/*`).

- **Zombie** — data classes, runtime systems, entity/world components, script bindings.
- **Zombie.Editor** — pipelines, the visual dialog and quest editors, browse previews.

Written as the Traktor.Story engine module; since 2026-10-12 the code is the Zombie project's own
(`code/`, `code/Editor/`, namespace `zombie`).

This is a sketch: class shapes, ownership and data flow. Members and signatures are indicative.

---

## 1. Principles

1. **Everything a designer authors is a database instance**, referenced by its GUID through
   `resource::Id< T >`. No string names, no `Names` table: the asset *is* the name, and the
   pipeline follows the references, so whatever a story uses is built.
2. **Data and state are separate.** Assets are immutable at runtime. What changes during play —
   the pack, quest progress, flags — lives in runtime objects that serialize to a plain
   `StoryState` (an `ISerializable`), which goes into `ISaveData` as-is.
3. **The core is world-agnostic.** `Story`, `Inventory` and `QuestLog` know nothing about entities.
   A world component binds a story to whichever world is loaded, so quests span stages.
4. **Item rules are typed traits.** An item carries a small set of `IItemTrait`s (equip place,
   heal, armor, event), each a serializable class the editor shows as a property list. Game-specific
   conditions or actions go through script hooks.
5. **Scripts drive it, they don't re-implement it.** Lua keeps the HUD, the views, the stage
   glue and the pickups' behaviour (walking over one, the E prompt). It reads and drives the native
   objects through the class factory.
6. **Text is literal strings** in the assets, as in the prototype. No I18N ids.

---

## 2. Data model (Zombie, serializable, EDIT classes)

### 2.1 Overview

```
StoryData ─┬─► QuestData ─┬─► QuestStep (FindItemStep, TalkStep, ReachStep, TriggerStep)
           │              │       ├─► ItemData / ActorData / PlaceData   (what the step is about)
           │              │       ├─► IWaypoint  (EntityWaypoint, ItemWaypoint, PointWaypoint, FirstOfWaypoint)
           │              │       └─► IAction    (on completion)
           │              └─► IAction (reward)
           ├─► ItemData ─┬─► world::EntityData (dropped pickup)
           │             └─► IItemTrait (EquipTrait, HealTrait, ArmorTrait, EventTrait)
           └─► ActorData ─► DialogData ─► DialogNode ─► DialogResponse ─┬─► ICondition
                                                                        └─► IAction

Scene entities point back into the story through components:
  ActorComponentData        "this Npc is Klara"              → ActorData
  PlaceComponentData        "this is the Altar"              → PlaceData
  ItemPickupComponentData   "this lies here: 1 × Ring"       → ItemData
  ItemContainerComponentData"this chest/body holds: …"       → ItemData[]
```

### 2.2 Items

```cpp
class T_DLLCLASS ItemData : public ISerializable
{
	T_RTTI_CLASS;
public:
	virtual void serialize(ISerializer& s) override final;

	const std::wstring& getName() const;
	const std::wstring& getPluralName() const;
	const std::wstring& getInfo() const;
	const std::wstring& getIcon() const;           // icon id in the game's HUD art
	int32_t getMaxStack() const;
	bool isKeep() const;                           // the item is its entity (a relic)
	const resource::Id< world::EntityData >& getDropEntity() const;  // null = cannot be put down

	template < typename TraitType >
	const TraitType* getTrait() const;             // first trait of that type, or null

private:
	std::wstring m_name, m_pluralName, m_info, m_icon;
	int32_t m_maxStack = 1;
	bool m_quest = false, m_keep = false;
	resource::Id< world::EntityData > m_dropEntity;
	RefArray< IItemTrait > m_traits;
};
```

Traits are what an item *does*. The module ships the ones the Zombie game needs. Another game adds
its own by deriving from `IItemTrait` in its own module, and they show up in the editor's
type list like any other EDIT class.

```cpp
class T_DLLCLASS IItemTrait : public ISerializable { T_RTTI_CLASS; };

class EquipTrait : public IItemTrait { std::wstring m_place; };   // "weapon", "hand", "body"
class HealTrait  : public IItemTrait { float m_amount; };         // restored when used from the pack
class ArmorTrait : public IItemTrait { float m_fraction; };       // share of damage taken while worn
class EventTrait : public IItemTrait { std::wstring m_event; };   // raised when one enters the pack
```

`Inventory` reads `EquipTrait` itself, since equipping is part of the pack. The rest are read by
the game: script gets `item:getTrait(story.HealTrait)`, which returns the trait or `nil`.

Prototype → asset mapping:

| Lua prototype (`VirtualItems`)      | `ItemData`                                    |
|-------------------------------------|-----------------------------------------------|
| key `Names.RIFLE`                   | the instance GUID                             |
| `name`, `plural`, `info`, `icon`    | same                                          |
| `tune` → `stack`                    | `maxStack`                                    |
| `tune` → `heal`, `armor`            | `HealTrait`, `ArmorTrait`                     |
| `event`                             | `EventTrait`                                  |
| `equip`                             | `EquipTrait`                                  |
| `drop`                              | `dropEntity`                                  |
| `quest`                             | gone: the find steps that want it (`holdBack`) |
| `keep`                              | `keep`                                        |
| `entityId`                          | gone: an `ItemPickupComponentData` in the scene says where it lies |

### 2.3 Actors and places

```cpp
class T_DLLCLASS ActorData : public ISerializable     // a character's identity, not its body
{
	T_RTTI_CLASS;
	std::wstring m_name;                             // what the HUD calls them
	resource::Id< DialogData > m_dialog;             // null = the story's default dialog
};

class T_DLLCLASS PlaceData : public ISerializable     // somewhere quests can point at
{
	T_RTTI_CLASS;
	std::wstring m_name;
};
```

An actor is referenced by quests and dialogs, and a scene entity *is* that actor through an
`ActorComponentData`. This solves the duplicate-names problem (eight "THE HUNTSMAN") and the
entity-id-in-a-scene problem: placing Klara again keeps her identity. Places work the same way,
including the forest's cave mouth and the cave's exit.

### 2.4 Quests

```cpp
class T_DLLCLASS QuestData : public ISerializable
{
	T_RTTI_CLASS;
	std::wstring m_title, m_info;
	RefArray< QuestStep > m_steps;                   // done in order; quests do not branch
	RefArray< IAction > m_reward;
};

class T_DLLCLASS QuestStep : public ISerializable     // abstract
{
	T_RTTI_CLASS;
	Guid m_id;                                       // what conditions, actions and saves refer to
	std::wstring m_name;                             // what the editor shows; defined only here
	std::wstring m_text;                             // journal / HUD objective
	Ref< IWaypoint > m_waypoint;                     // null = derived from the step
	RefArray< IAction > m_onComplete;
};

class FindItemStep : public QuestStep { resource::Id< ItemData > m_item; int32_t m_count = 1; bool m_holdBack = false; };
class TalkStep     : public QuestStep { resource::Id< ActorData > m_actor; Guid m_line; };   // a line of the actor's dialog
class ReachStep    : public QuestStep { resource::Id< PlaceData > m_place; float m_radius = 4.0f; };
class TriggerStep  : public QuestStep { };          // completed only by an AdvanceQuestAction
```

Rules carried over from the prototype:
- A find step is done once the item has entered the pack, however it got there. The story
  remembers items found.
- Until a quest reaches a find step that holds its item back (`holdBack`), the item is held back.
  A pickup is hidden, and a container leaves it out of what it shows and gives.
- At most one quest is active, and only the active quest's current step has a waypoint.

Waypoints:

```cpp
class IWaypoint : public ISerializable;
class ActorWaypoint   : public IWaypoint { resource::Id< ActorData > m_actor; };
class PlaceWaypoint   : public IWaypoint { resource::Id< PlaceData > m_place; };
class ItemWaypoint    : public IWaypoint { resource::Id< ItemData > m_item; };   // pickup, container or carrier
class PointWaypoint   : public IWaypoint { Vector4 m_position; };
class FirstOfWaypoint : public IWaypoint { RefArray< IWaypoint > m_candidates; }; // first present in this world
```

### 2.5 Conditions and actions

These are small serializable classes, so the editor shows them as ordinary property lists.

| Conditions (`ICondition`)                          | Actions (`IAction`)                         |
|----------------------------------------------------|---------------------------------------------|
| `HasItemCondition` (item, count, negate)           | `GiveItemAction` (item, count)              |
| `QuestCondition` (quest, NotStarted/Active/Done/AtStep, step) | `TakeItemAction` (item, count) |
| `FlagCondition` (FlagData, negate)                 | `StartQuestAction` (quest)                  |
| `AllCondition` / `AnyCondition` (children)         | `AdvanceQuestAction` (quest, step)          |
| `ScriptCondition` (ScriptConditionData → game callback) | `FinishQuestAction` (quest)            |
|                                                    | `SetFlagAction` (FlagData, value)           |
|                                                    | `EventAction` (EventData → game callback)   |

Every reference is a guid. A step is referred to by the guid of the step, flags, events, script
conditions and equip slots by the guid of a key asset (`code/Keys/`), whose instance name is its
only name. `ScriptCondition` and `EventAction` are the extension points for anything
game-specific, such as the Zombie game's trade event (opening the chest window on an NPC's pack).

### 2.6 Dialogs

The editor asset holds the graph with layout. The pipeline compiles it into a compact runtime
resource, the same split as Animation's `StateGraph` → `StateGraphCompiler` → `RtStateGraphData`.

```cpp
// Zombie.Editor
class DialogAsset : public ISerializable              // what the dialog editor edits
{
	RefArray< DialogNodeAsset > m_nodes;             // + editor position
	RefArray< DialogEdgeAsset > m_edges;             // response → node
	std::wstring m_startNode;
};

// Zombie (runtime)
class T_DLLCLASS DialogData : public ISerializable
{
	T_RTTI_CLASS;
	std::wstring m_name;                             // overrides the actor's name in the window
	AlignedVector< StartEntry > m_start;             // { node, condition }: first that holds
	RefArray< DialogNode > m_nodes;
};

class T_DLLCLASS DialogNode : public ISerializable
{
	std::wstring m_id;
	std::wstring m_text;                             // empty = the actor's greeting
	RefArray< IAction > m_onEnter;
	RefArray< DialogResponse > m_responses;
};

class T_DLLCLASS DialogResponse : public ISerializable
{
	std::wstring m_text;
	int32_t m_target = -1;                           // index into m_nodes; -1 ends the talk
	Ref< ICondition > m_condition;
	RefArray< IAction > m_actions;
	bool m_once = false;
};
```

### 2.7 The story root

```cpp
class T_DLLCLASS StoryData : public ISerializable
{
	T_RTTI_CLASS;
	RefArray< ... > m_quests;                        // resource::Id< QuestData >[]
	resource::Id< DialogData > m_defaultDialog;
	AlignedVector< ItemStack > m_startKit;           // { item, count }
};
```

`StoryData` is the one asset the game references. Its pipeline walks quests → steps → items,
actors and dialogs, so the whole story builds from one root. The runtime needs the full quest list
anyway, for example to know which items are held back.

### 2.8 Components

| Component data               | Runtime component        | Replaces in the Zombie game                     |
|------------------------------|--------------------------|-------------------------------------------------|
| `ActorComponentData`         | `ActorComponent`         | `dialog` / Names entity ids on Npc entities      |
| `PlaceComponentData`         | `PlaceComponent`         | Names place ids (`ALTAR`, `CAVE_MOUTH`)          |
| `ItemPickupComponentData`    | `ItemPickupComponent`    | the Pickup script's `item`/`count` text properties |
| `ItemContainerComponentData` | `ItemContainerComponent` | chest `item` and NPC `items` text properties     |
| `StoryWorldComponentData`    | `StoryWorldComponent`    | the waypoint, visibility and reach parts of `Questing` |

Components hold the references and describe; they don't behave. Taking a pickup is the game's:
its Lua notices the player (walk-over or E), calls `inventory:add(pickup.item, pickup.count)` and
removes the entity, and the story learns of the item through the inventory like any other. The only
thing the story does to a pickup is hide a held-back one (`entity.visible`). The game's pickup code
already ignores hidden entities, as Scripts/Pickup does today.

---

## 3. Runtime (Zombie)

### 3.1 Objects

```cpp
class T_DLLCLASS Story : public Object               // one per playthrough; outlives stages
{
	T_RTTI_CLASS;
public:
	static Ref< Story > create(resource::IResourceManager* resourceManager, const resource::Id< StoryData >& story);

	Inventory* getInventory() const;
	QuestLog* getQuestLog() const;

	Ref< Conversation > talk(const ActorData* actor, const std::wstring& greeting);

	bool isHeldBack(const ItemData* item) const;   // a find target no quest has reached yet
	void addListener(IStoryListener* listener);    // quest started/updated/completed, item found, event

	Ref< StoryState > saveState() const;
	bool restoreState(const StoryState* state);
};

class T_DLLCLASS Inventory : public Object
{
	int32_t add(const ItemData* item, int32_t count);      // returns how many fitted
	int32_t remove(const ItemData* item, int32_t count);
	int32_t count(const ItemData* item) const;
	int32_t room(const ItemData* item) const;
	const ItemStack* getSlot(int32_t index) const;         // { Ref< const ItemData >, count }
	bool equip(const ItemData* item);
	bool unequip(const std::wstring& place);
	const ItemData* getEquipped(const std::wstring& place) const;
	uint32_t getSerial() const;                            // views redraw on change
};

class T_DLLCLASS QuestLog : public Object
{
	bool start(const QuestData* quest);
	bool advance(const QuestData* quest, const std::wstring& stepId);
	bool finish(const QuestData* quest);
	QuestStatus getStatus(const QuestData* quest) const;   // NotStarted / Active / Done
	const QuestStep* getCurrentStep(const QuestData* quest) const;
	const QuestData* getActive() const;
	bool setActive(const QuestData* quest);
	void talkedTo(const ActorData* actor, const std::wstring& nodeId);
	bool getFlag(const std::wstring& flag) const;
	void setFlag(const std::wstring& flag, bool value);
};

class T_DLLCLASS Conversation : public Object
{
	std::wstring getSpeaker() const;
	std::wstring getLine() const;
	RefArray< const DialogResponse > getResponses() const; // conditions and "once" applied
	void choose(int32_t index);                            // runs actions, moves on
	bool hasEnded() const;
};
```

### 3.2 State, save and stages

```cpp
class T_DLLCLASS StoryState : public ISerializable
{
	AlignedVector< SlotState > m_slots;          // { Guid item, int32 count }
	SmallMap< std::wstring, Guid > m_equipped;
	AlignedVector< QuestRecord > m_quests;       // { Guid quest, int32 step, bool done, int32 order }
	Guid m_active;
	SmallSet< std::wstring > m_flags;
	SmallSet< Guid > m_found;
};
```

- **Save:** `saveData->setNow(L"story", desc, story->saveState(), true)`. A plain `ISerializable`,
  so the Lua `SaveGame` table encoding is no longer needed for this part of a save.
- **Stage switches:** the `Story` object rides in the stage parameters, the same way the Lua
  `carry` table does today: `loadStageAsync("LOADING", { target = ..., story = self._story })`,
  and `Loading` hands it on. Nothing world-bound is kept in it, and no application-level service is
  needed.
- **Per world:** `StoryWorldComponent` is created with the world. The game hands it the story and
  the player entity, and each update it:
  - indexes the world's `ActorComponent`, `PlaceComponent`, `ItemPickupComponent` and
    `ItemContainerComponent`;
  - completes reach steps from the player's position;
  - hides held-back pickups and shows them once their step is reached;
  - resolves the active step's waypoint (`getWaypoint() → optional< Vector4 >`) and talk target
    (`getTalkTarget() → Entity*`), for the compass, the minimap and the head marker;
  - completes find steps through the inventory listener, whatever the source.

### 3.3 Script exposure (StoryClassFactory)

`StoryClassFactory : IRuntimeClassFactory` registers `AutoRuntimeClass< T >` for every runtime and
data class above. Data classes get read-only properties, runtime objects methods. Script callbacks
come in through `IRuntimeDelegate`.

Zombie `Main` after the change (indicative):

```lua
-- Startup (new night) or a loaded save: one story for the whole playthrough
self._story = (params ~= nil and params.story) or story.Story.create(environment.resource.resourceManager, Guid("\{...StoryData...}"))

-- E at a character
local actor = npc:getComponent(story.ActorComponent).actor
local c = self._story:talk(actor, npc.sc:talk(self))
self._dialogView:showConversation(c)          -- Lua view reads c.line / c:getResponses()

-- HUD, once a frame
local wp = self._storyWorld.waypoint          -- Vector4 or nil
local who = self._storyWorld.talkTarget       -- Entity or nil

-- Game-specific hooks
self._story:addListener(function(event, ...)  -- "questStarted", "event:trade", ...
```

---

## 4. Editor (Zombie.Editor)

### 4.1 Pipelines

| Pipeline                   | Asset                         | Does                                                        |
|----------------------------|-------------------------------|-------------------------------------------------------------|
| `StoryPipeline`            | `StoryData`                   | dependencies on every quest and the start kit               |
| `QuestPipeline`            | `QuestData`                   | dependencies and validation                                 |
| `DialogPipeline`           | `DialogAsset` → `DialogData`  | compiles the graph; validation                              |
| `ItemPipeline`             | `ItemData`                    | dependency on the drop entity                               |
| `Story*ComponentPipeline`  | component data in scenes      | dependencies on the referenced assets                       |

**Validation** replaces `Journal.validate()` and fails the build with a clear message. It catches:
- dangling responses and unreachable nodes;
- a talk step waiting for a node its actor's dialog doesn't have;
- step ids that actions refer to but no step has;
- a quest finding an item that nothing in any scene holds.

All four are checked, as recorded in section 5. The last is `validateObtainable`, run when the story
(`StoryData`) is built: every item a quest finds must be held by a pickup or container in a scene
(prefabs placed in scenes included), given by an action, or in the start kit. Scenes are not
dependencies of the story, since a scene's scripts may depend on the story. So a change to a scene
alone is checked the next time the story is built.

### 4.2 Editor pages

- **Dialog editor** (`DialogEditorPage`): a `ui::GraphControl`, as in `StateGraphEditorPage`.
  - One node per line; its output pins are the responses, and edges go to the next line.
  - Selecting a node or response shows its text, condition and actions in the `PropertiesView`.
  - A test button plays the dialog against a scratch `Story`, with toggles for flags, quest
    states and pack contents.
- **Quest editor** (`QuestEditorPage`):
  - The steps run as a vertical flow of nodes. Each shows its kind, objective text, and the item,
    actor or place it's about, which opens on double-click.
  - The reward sits at the end.
  - Steps are reordered by dragging and edited in the `PropertiesView`.
  - A side panel lists everything that refers to this quest: dialogs that start or advance it, and
    conditions on it.
- **Item**: the default property page is enough. An `ItemBrowsePreview` draws the icon.
- **Scene editor plugin**:
  - Labels actor and place components in the viewport.
  - Shows each pickup's item.
  - Highlights the entities that appear as waypoints or targets in the selected quest.

### 4.3 Registration checklist

- `resources/build/{Traktor*}.xms`: projects `Zombie` and `Zombie.Editor` on every
  platform.
- `Editor.Modules` in each `Traktor.Editor.<platform>.config`.
- `Editor.BrowseTypeFilter` entries in `Traktor.Editor.config`.
- `TRAKTOR_STORY_*` captions in `Categories.dictionary`, and UI strings in
  `Zombie.dictionary`.
- `Module.cpp` with `T_FORCE_LINK_REF` for the static configurations.
- Tests in `code/Test/`. These cover what the Lua prototype tests cover today: inventory
  stacking, quest flow, conditions, the dialog runner, held-back items and save round-trips.

---

## 5. Migrating the Zombie game

Each phase leaves the game playable.

| Phase | Native                                                                                     | Removed from Lua                                    |
|-------|--------------------------------------------------------------------------------------------|-----------------------------------------------------|
| 1     | `ItemData` assets with traits (14, from `VirtualItems`), `Inventory`, `ItemPickupComponentData`, `ItemContainerComponentData`; scenes converted by a tool from the text properties | `VirtualItems`, `Items`, `Inventory`, item part of `Names`; `Pickup` keeps its behaviour but reads its item from the component |
| 2     | `ActorData`, `PlaceData`, their components; `QuestData`, `DialogAsset`, `QuestLog`, `Conversation`, `Story`, story state in saves; a one-off converter from the `Quests`/`Dialogs` tables | `Journal`, `Quests`, `Dialogs`, `Names`             |
| 3     | `StoryWorldComponent` (waypoints, visibility, reach)                                        | most of `Questing` (it keeps the windows and the HUD) |

The Lua views (`DialogView`, `JournalView`, `InventoryView`, `ChestView`) and the HUD widgets stay.
They read native objects instead of Lua tables.

**Phase 1, as done (2026-10-10).** The items are `data/Source/Items` (since 2026-10-11 `data/Source/Story/Items`) in the Zombie database, made by
`tools/gen_story_items.py` with the guids `Names` already had, so nothing naming an item changed.
`tools/convert_story_items.py` moved the scenes' text properties into components (pickup prefabs,
the spent relic on each ward, and every placed chest and character). `VirtualItems`, the Lua
`Inventory` and the item numbers in `Config` are gone; the pack is a `zombie.Inventory`.
Two things stayed in Lua that the table above has going:

- **`Items`**, cut down to a bridge. Lua still keeps an item by a key -- the guid as text -- wherever
  it is plain data or a table key (a chest's contents, a character's loot, the world books), since a
  `Guid` is neither. `Items` turns guids into keys and reads an `ItemData` (through the resource
  manager) into a flat table with the traits unpacked. It goes when the books do, in phase 2.
- **The item part of `Names`**, as `Names.ITEMS`, with escaped guids. The escape is what takes every
  item into the build, including those only ever handed over in a dialog; it also tells `Items` an
  item from a person, without a failed database look-up. Phase 2's `StoryData` will list the items
  instead.

Script reaches the native side with either form: every story binding that takes an item accepts a
`Guid` or its text. Slots count from 0 natively; the Lua views keep their 1-based cells and convert.

**Phase 2, as done (2026-10-10).** `data/Source/Story` holds the story: `Story` (the `StoryData` the
game binds), four quests, six dialogs (one the default), ten actors and three places, written once
by `tools/convert_story_quests.py` from the Lua tables (dumped with `tools/story_dump.lua`). The same
tool put an `ActorComponentData` on each person and a `PlaceComponentData` on each place in the
scenes. `Journal`, `Quests`, `Dialogs` and `Names` are gone from Lua; `Questing` drives the native
`Story` and still resolves waypoints, hides held-back pickups and hangs the head marker (phase 3).
Where it differs from the sketch above:

- **Saved state is a `PropertyGroup`** (`Story::saveState`), not a `StoryState` class. The game's
  `SaveGame` already stores property groups, and stage switches carry the `Story` object itself.
- **The game's hooks are three replaceable handlers** -- events, script conditions, distances --
  set by each stage, rather than a listener list that would keep the closures of stages long left.
  For the same reason the inventory gained `removeAllListeners`, which each stage calls on arrival.
- **`FindItemStep` has no count**: an item is found once it has entered the pack, as before.
- **`QuestCondition` has `Started`/`NotDone`** besides the listed states, which the Lua dialogs used.
- **`EventAction` can end the conversation** (`endConversation`); "trade" does. The conversation
  remembers the events it raised (`Conversation::hasEvent`).
- **Dialog assets are edited with the property editor** for now. They are compiled and checked by
  `DialogPipeline` (unknown target or start nodes fail the build; unreachable nodes warn), and
  `StoryPipeline` checks quests. The graph editor pages of 4.2 came after phase 3 (below).
- **`Items` keeps the item constants** (`Items.RIFLE`, ...) the game code refers to, and lists them
  as the known items, since `StoryData` does not list items.

**Phase 3, as done (2026-10-10).** `StoryWorldComponentData` is in both scenes' world components, and
`Questing` hands the created `StoryWorldComponent` the story and the player. The component:

- indexes the world's actors, places, pickups and containers every `refreshInterval` (1 s), or at
  once on `refresh()`; an entity no longer in the world is skipped;
- completes reach steps from the player's position, so the story's distance handler is no longer
  used by the game;
- shows or hides the pickups of items a quest holds back (`Story::isHeldBack`), whenever the quest log
  moves or the world is indexed again;
- resolves the followed step's `waypoint` and `talkTarget` each update: the step's own waypoint,
  else the actor, the place, or the nearest visible pickup or container holding the item.

`Questing` keeps the windows, the HUD markers and the chest-window filtering of held-back items,
which reads the game's own record of what is left in each chest. The component reads containers'
contents as the scene placed them; a quest item taken from one is found, which moves the step on.

**Editor pages, as done (2026-10-10).** Both graph pages of 4.2 exist. The scene editor plugin came
on 2026-10-11 (below). The item preview does not exist: an item's `icon` is the name of a picture
in the game's own HUD art, which the engine cannot draw.

- **Dialog editor.** Lines are linked by guid (`DialogAsset` version 2), set as pins are connected;
  a line's name is only what talk steps refer to. Older assets, linked by name, are converted as
  they are read. The guids and graph positions are hidden from the properties view.
  - The test panel ("Dialog") plays the compiled dialog as a chat (`ui::ChatView`).
  - There is no scratch `Story`. Items that conditions check are listed as checkboxes, and
    give/take actions tick and untick them. Quest, flag and script conditions are taken to hold.
    Once-responses drop out until a restart.
- **Quest editor.** `QuestData` is edited directly; it keeps no editor positions, since the steps
  are laid out as one column in order.
  - Steps are reordered by dragging or with move up/down, and can be cut, copied and pasted.
  - Double-clicking a step opens its item, actor or place.
  - The quest panel lists the quest's problems: missing targets, duplicate step guids, and a talk
    step at a line its actor's dialog lacks. Such steps are drawn as errors.
  - The panel also lists every dialog, quest, item, actor or story that starts, advances,
    finishes or checks the quest, flagging any that name a step the quest does not have.

**Scene editor plugin, as done (2026-10-11).** `StorySceneEditorPlugin` labels every story component
in the viewport. Labels face the camera, sit above the entity, and are colored by kind; the guide
toggle is "Story.Labels". They show:

- the actor's or place's name;
- the item and count a pickup holds;
- the list a container holds.

A quest picked in the scene toolbar ("(no quest)" by default) highlights in red, through walls,
every actor, place and item its steps or their waypoints are about. Each highlight has its step
above it ("2. find") and a line down to the entity, and a reach step's radius is drawn around its
place. The pick is per scene editor and follows edits to the quest.

**References by guid, as done (2026-10-11).** No asset refers to anything by a string any more.

- **Steps and lines.** Quest steps have a guid (made with the step) and a name the quest defines.
  Conditions, actions and saves use the guid. Talk steps wait for a dialog line by its guid
  (`TalkStep::m_line`). Responses have a guid too; a once-response is remembered per actor and
  response guid.
- **Keys.** Flags, events, script conditions and equip slots are key assets: `FlagData`,
  `EventData`, `ScriptConditionData` and `EquipSlotData`. They have no data, only a guid and the
  instance's name. The game is told the guid (the `event` notification, the condition handler,
  the inventory's slots). Zombie keeps them as keys in Lua (`Items.WEAPON`, `Items.EVENTS`,
  Questing's `TRADE`).
- **Validation.** `validateStoryAsset` (`Editor/StoryValidation.h`) runs in every story
  pipeline: dialogs, quests, actors, places, the story, items, keys, and the components in scenes.
  - Every typed reference must resolve to an instance of the right type. A null reference is an
    error except where null means something: an actor's dialog, a talk step's line, the step of
    an `AdvanceQuestAction`, and the step of a `QuestCondition` that is not `AtStep`.
  - A step reference must name a step of the quest beside it. A line reference must name a line
    of the talking actor's own dialog.
  - Lists may not hold empty entries.
  - Step, line and response guids must be unique.
  - References to steps and lines are not build dependencies; the quest or actor beside them is.
- **Editor.** `editor::IReferenceResolver` (in `Traktor.Editor`) lets the properties view name,
  browse and open a reference to a part of an instance. `StoryReferenceResolver` does this for
  steps ("The Burial Ledger > 2. find") and lines ("Klara > thanks"). The MCP bridge accepts such
  references without looking them up as instances; the pipeline checks them.
- **Data.** Zombie's data was converted once by `tools/convert_story_guids.py`. Saves moved to
  version 5. The story now saves each quest's step guid as well as its index, so a save survives
  steps being reordered.

**Actors hold who a character is (2026-10-11).** `ActorData` has a property group for the game
(`properties`), which the story itself does not read. `ActorComponent` binds its actor when it is
created (`StoryEntityFactory` takes the resource manager), so script reads `name`, `properties` and
`data` from it. In Zombie, an NPC reads its name from its actor and every other setting from its
entity's script properties first, then its actor's.

- Who a character is (class, age, courage, greeting, faction, health, mood) moved from the scene
  entities' script properties into the actors, with `tools/move_npc_properties.py`.
- Only `route` stays on the entity, since it belongs to where the character is placed.
- The generic characters got one actor per kind ("A Cutthroat", "The Huntsman", "The Trapper"),
  which many entities share.
- Renaming an actor renames the character everywhere: nameplates, the dialog's speaker and the
  scene editor's labels.

**Deaths and failing quests (2026-10-11).** The game tells the quest log when an actor dies
(`QuestLog::died`). The log remembers it in saves, and `ActorCondition` checks an actor alive or
dead. A quest fails when a step's `failWhen` condition holds while the quest is at that step.
Steps are version 2; a version 1 step has no `failWhen`.

A quest also fails on its own, with no `failWhen`, once an actor that the current step or any
later step talks to has died (`QuestData::getNeededActors`, checked as `QuestLog` settles a quest).
That covers a quest given after the death too. The quest editor marks those steps "can fail" as
well (2026-10-28).

A failed quest is neither under way nor done; `QuestEntry::isFailed` tells which. The game is told `questFailed` (quest, the step it failed at).
A failed quest cannot be given again, and the items its later find steps want stay held back.

In Zombie, the stage tells the story when a character with an actor dies, including a character
restored dead from a save (`Main:_actorDied`). Greta raises the locket only while Tomas lives,
and "The Lost Locket" fails if Tomas dies before he is asked, or Greta before the locket is back.
No quest sets a `failWhen` for this; each one relies on the talk steps. The journal shows failed quests as
ended, marked FAILED, and the HUD notes "QUEST FAILED".

**Source layout (2026-10-11).** The code follows Traktor's CLAUDE.md: one class per file, and the
implementations of an interface in a folder of their own: `Actions/`, `Conditions/`, `Keys/`,
`Steps/`, `Traits/` and `Waypoints/` (`QuestStep`, `IAction`, `ICondition`, `IItemTrait` and
`IWaypoint` stay at the top). The type names, and so the data, did not change.

**Greetings are dialog lines (2026-10-11).** What an actor says is in its dialog only. The sketch's
greeting, which the game passed in to fill lines with no text, is gone:

- `Story::talk(actor)` takes only the actor. It returns null when the actor has no dialog, and
  such an actor cannot be talked to.
- The speaker is the dialog's name, else the actor's.
- `StoryData` has no default dialog (version 1). `DialogPipeline` fails a line with no text.
- In Zombie, `tools/move_greetings.py` wrote each actor's `greeting` property into its dialog's
  empty line. Actors without a dialog got their own, copied from the old default dialog: the
  greeting and the trade response. The stock lines by mood went too.
- `Main:_nearestTalker` only offers characters whose actor has a dialog.

**Quest items are the steps' (2026-10-23).** `ItemData` has no `quest` flag (version 1), so a find
step can want any item, a generic one included:

- The story notes an item as found when any find step looks for it (`Story::update`).
- `FindItemStep::holdBack` (version 3) holds its item back until a quest reaches the step. It is
  off by default, and on in Zombie's five quests.
- `Story::isWanted` says whether a quest under way has a find step for the item. The inventory
  shows its hint from that, and allows a drop when the item has a drop entity or is kept.

---

## 6. Decisions

| Question                               | Decision                                                                 |
|----------------------------------------|--------------------------------------------------------------------------|
| Text                                   | Literal strings in the assets.                                           |
| References                             | Guids only, never names: a name is defined once, by the instance or part it names, and only shown. Every reference is checked by the pipelines, which fail the build on any that dangles. |
| Where the `Story` lives across stages  | Carried in the stage parameters; no application-level service.           |
| Item rules                             | Typed `IItemTrait`s: `EquipTrait`, `HealTrait`, `ArmorTrait`, `EventTrait`; a game adds its own in its own module. |
| Quest shape                            | Linear steps. No branching.                                              |
| Pickups                                | Left to the game. `ItemPickupComponentData` only says what lies there; the story hides held-back ones. |
