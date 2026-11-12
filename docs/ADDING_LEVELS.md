# Guide: Adding New Levels to Vault Heist

Vault Heist uses a data-driven level system powered by JSON (`data/levels.json`). Adding a new level requires **zero code changes** in the C++ core models or GUI logic.

## 1. Level Schema Specification

Each level entry in `data/levels.json` contains:
- `id`: Unique string identifier (e.g., `"1.6"` or `"4.6"`).
- `name`: Display title of the level.
- `type`: Operation / mechanic category (e.g., `"traversal"`, `"bst_search"`, `"avl_single_rotate"`, `"heap_build"`).
- `nodes` or `elements`: Number of nodes in the tree or elements in the heap array.
- `seed`: Random seed ensuring reproducible tree shapes and treasure values across runs.

## 2. Steps to Add a New Level

1. Open `data/levels.json`.
2. Locate the target Act (e.g., Act 1 for Binary Tree, Act 4 for Heap).
3. Add a new level object to the `levels` array of that Act:
   ```json
   {
     "id": "1.6",
     "name": "Advanced Vault Maze",
     "type": "traversal",
     "nodes": 15,
     "seed": 106
   }
   ```
4. Save the file.
5. Restart Vault Heist. The LevelManager parses `levels.json` dynamically upon startup, making the new level immediately playable in the Campaign Map.

## 3. Supported Level Types

- **Act 1:** `"traversal"`, `"reconstruction"`, `"height_diameter"`, `"lca"`, `"mirror"`
- **Act 2:** `"bst_search"`, `"bst_insert"`, `"bst_delete"`, `"bst_range"`, `"degeneration"`
- **Act 3:** `"avl_balance"`, `"avl_single_rotate"`, `"avl_double_rotate"`, `"avl_insert_predict"`, `"avl_kth_rank"`
- **Act 4:** `"heap_build"`, `"heapify"`, `"extract_max"`, `"heap_sort_steps"`, `"heap_sort_boss"`
