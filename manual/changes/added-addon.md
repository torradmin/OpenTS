---
title: Remember the player's game type choice across launches
category: feature
release: 0.2.0
targets:
- type: key
  id: Addon
  effect: added
credit:
- torradmin
---

A player who adds `sun.ini`'s `[Options] Addon=` key gets the game type dialog resolved automatically instead of shown, letting a Firestorm owner skip straight past the choice on every launch. The game never adds the key on its own. Both the multiplayer game-type dialog and the graphical main menu's game select page honor it the first time either is reached; the main menu page still reappears if the player backs out of it, so the auto-selected type can still be changed. Picking a type from either page writes the choice back to the key for a player who already opted in, so the last type picked is what gets auto-selected next time.
