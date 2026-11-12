# Adding Levels to Vault Heist

Level definitions are stored in `data/levels.json`. The game loads this file when it starts, so close and relaunch the game after changing it.

## Add a level using an existing challenge

1. Open `data/levels.json` and find the act that should contain the level.
2. Add a level object to that act's `levels` array. Example:

```json
{ "id": "2.6", "name": "New Search Challenge", "type": "bst_search", "nodes": 9, "seed": 206 }
```

3. Give it an ID not used by any other level. Keep the existing act numbering convention (`1.x`, `2.x`, and so on).
4. Set `type` to one already used in `data/levels.json`. Existing types select the challenge behavior in the GUI.
5. Use `nodes` for tree levels or `elements` for heap levels. Choose a new integer `seed` to create a repeatable level layout.
6. Save the JSON and launch `vault_gui_demo.exe`. Use the act and level navigation buttons to open the new level.

Keep the JSON structure and field names consistent with the existing entries. Each level needs `id`, `name`, and `type`; `nodes`, `elements`, and `seed` are optional fields read by the loader, but the appropriate size and seed should be supplied for predictable gameplay.

## Add a new challenge type

Adding a new `type` string to JSON alone does not create a new challenge. The GUI currently recognizes challenge types through level-specific checks and handlers in `src/gui/main_gui.cpp`. To implement a new type:

1. Add the new level entry to `data/levels.json`.
2. Add its type check and completion condition in `src/gui/main_gui.cpp`.
3. Add setup/reset behavior, player input, on-screen instructions, and success feedback for the level.
4. Build and manually play the level from start to completion, then confirm reset and navigation work.

Keep new challenges based on the existing tree or heap classes in `src/models/`; the project requirements call for implementing these data structures from scratch.
