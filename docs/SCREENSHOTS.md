# GUI Screenshot Evidence

These screenshots were captured from the playable demo during the manual campaign walkthrough. They are report evidence, not level instructions.

| Evidence | Screenshot | Status |
|---|---|---|
| AVL LL single rotation | [Screenshot](screenshots/level-3-2-avl-ll.jpg) | Captured |
| AVL RR single rotation | [Screenshot](screenshots/level-3-2-avl-rr.jpg) | Captured; left rotation balanced the tree |
| AVL LR double rotation | [Screenshot](screenshots/level-3-3-avl-lr.jpg) | Captured |
| AVL RL double rotation | [Screenshot](screenshots/level-3-3-avl-rl.jpg) | Captured; both rotations balanced the tree |
| Max heap construction | [Screenshot](screenshots/level-4-1-heap-build.jpg) | Captured |
| Bottom-up heapify | [Screenshot](screenshots/level-4-2-heapify.jpg) | Captured; max-heap property restored |
| Maximum extraction | [Screenshot](screenshots/level-4-3-max-extraction.jpg) | Captured |
| Heap sort result | [Screenshot](screenshots/level-4-4-heap-sort-final.jpg) | Captured |
| Final campaign level | [Screenshot](screenshots/level-4-5-final-vault-boss.jpg) | Captured |

The source changes add an RR option to Level 3.2 and an RL option to Level 3.3. These alternatives use small fixed trees and preserve the existing default LL and LR cases. The revised demo compiled successfully, and both added cases were played through in the GUI.
