---
key: Addon
scope: client-settings
summary: An opt-in setting that auto-selects the game type dialog's answer and is kept updated with the player's last choice.
see_also: [RequiredAddOn]
when_omitted:
  kind: context-dependent
  note: the game type dialog is shown so the player can choose, unless only the base game is installed; this is also the default, since the game never adds this key on its own.
---

```ini title="sun.ini"
[Options]
Addon=1
```

`0` names the base game and `1` names Firestorm. The game never adds this key on its own; a player adds it to opt in. Once present, it resolves the game type dialog automatically instead of showing it: the multiplayer game-type dialog and the graphical main menu's game select page both skip straight to the named game the first time either is reached. The main menu's page still reappears normally if the player backs out of it, so the auto-selected type can be changed within the same session.

Both dialogs write the player's choice back to this key whenever it is picked from the page, but only for a player who already added the key. A player who never opted in keeps seeing the dialog and gets nothing written; an opted-in player gets the key updated to the game type last chosen, so that is what gets auto-selected next time.
