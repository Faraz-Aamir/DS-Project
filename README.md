# Vault Heist

Vault Heist is a small data-structures game. Each vault room is a tree node, and levels turn tree and heap operations into short challenges.

## Run the demo

1. Open the project folder.
2. Build the demo using the command below.
3. Start at the main menu. Choose **Campaign** to open the four-act map, or try Endless and Sandbox.

## Build

The project uses GCC from the Raylib w64devkit and Raylib installed under `C:\raylib`. Run commands from the project root in PowerShell. If Raylib is installed in a different directory, update the include and library paths.

Build the GUI demo:

```powershell
C:\raylib\w64devkit\bin\g++.exe -std=c++17 -O2 -Wall -Wextra -IC:\raylib\raylib\src -o vault_gui_demo.exe src\gui\main_gui.cpp src\models\BinaryTree.cpp -LC:\raylib\raylib\src -lraylib -lopengl32 -lgdi32 -lwinmm
```

Run the built demo from the project root:

```powershell
.\vault_gui_demo.exe
```

It loads `data\levels.json` beside the executable.

The shared `Validator` checks tree links/counts, BST ordering, AVL heights/sizes/balance, and heap order. The unit tests include `lcaBST` and valid/invalid invariant cases. The benchmark accepts `--disable-events` to suppress returned model event records during measurements.

Build and run the model unit tests:

```powershell
C:\raylib\w64devkit\bin\g++.exe -std=c++17 -O2 -Wall -Wextra -Isrc -o unit_tests.exe tests\unit_test.cpp src\models\BinaryTree.cpp
.\unit_tests.exe
```

Build and run the benchmarks:

```powershell
C:\raylib\w64devkit\bin\g++.exe -std=c++17 -O2 -Wall -Wextra -Isrc -o benchmark.exe src\benchmark\benchmark.cpp src\models\BinaryTree.cpp
.\benchmark.exe --disable-events | Set-Content -Encoding utf8 benchmark_results.csv
```

## Controls

- Click rooms to answer traversal, search, range, balance, LCA, and mirror challenges.
- Type a number into the visible input box for root, height, diameter, insert, and delete questions. Press **Enter** or use the on-screen button.
- Use the level's action buttons for AVL rotations and heap operations.
- Press **Z** to undo the last tree, heap, or guided-level action. **Replay**, **Play/Pause**, the timeline, and the arrow keys step through recorded action states. **H** gives a hint and caps the level at two stars. Click **Reset** to restart a level. Use the mouse wheel to zoom, drag empty canvas to pan, and hover over a room to see its key, depth, height, and balance factor.
- The title bar counts completed levels. The HUD shows alarm steps against the level capacity, score, and earned stars. Score follows the spec formula: 1000 − 10 × alarm steps − penalties, floored at 0. Completed level scores and stars are saved in `data/progress.txt`; the current level tree or heap is saved in `data/current_state.txt` using preorder null markers or the heap array.

## Demo route

Show one example from each act:

1. **Act 1, Level 1.1:** click the rooms in preorder.
2. **Act 2, Level 2.3:** delete the highlighted node and show the inorder-successor replacement.
3. **Act 3, Level 3.3:** apply the left rotation, then the right rotation.
4. **Act 4, Level 4.5:** advance Heap Sort one step at a time and show the sorted region.

For a shorter demo, show Levels **1.1**, **2.1**, and **2.2**.

## Campaign

The four acts cover binary tree traversal and reconstruction, BST operations, AVL balancing and order statistics, and Max Heap operations. Level names, types, sizes, and random seeds are stored in `data/levels.json`.

See [LEVELS_GUIDE.md](LEVELS_GUIDE.md) for instructions on adding levels and challenge types.

## Other modes and event playback

- **Endless** gives a timed stream of BST insert/delete operations with three lives and records local high scores.
- **Sandbox** is a free-form BST and heap editor for demonstrating operations.
- **Leaderboard** shows the ten highest local Endless scores. **Settings** switches the theme and frame limit.
- The campaign map gates levels using saved completion progress and opens a briefing and result screen for each level.
- GUI actions publish structured events through the event bus. The event log keeps the tree state after each event; playback and the scrubber display those saved states. Undo is available for tree edits, rotations, heap actions, and guided challenge selections/answers.

## Project evidence

- [Project report (DOCX)](docs/Vault_Heist_Project_Report.docx)
- [Unit test report](TEST_REPORT.md)
- [Benchmark results and charts](docs/BENCHMARK_RESULTS.md)
- [Sorting comparison chart](docs/sorting_comparison.svg)
- [GUI screenshot index](docs/SCREENSHOTS.md)
- [Viva preparation](docs/VIVA_PREP.md)
- Raw benchmark output: `benchmark_results.csv`
