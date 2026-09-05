---
key: PlayIntro
summary: Controls the one-time startup movies.
when_omitted:
  kind: value
  value: "yes"
---

When enabled, this setting selects the first-time startup path. The game writes
`PlayIntro=no` back to `sun.ini` before playing `EVA.VQA`, so a later start skips
that movie unless the setting is enabled again.

`PlayIntro` also gates the Westwood logo movie and the Tiberian Sun or Firestorm
title movie played right after it. Disabling it skips all three. It does not
gate the title movie played after selecting a game on the game-select screen.

[`FROMINSTALL`](/using/command-line/from-install/) selects the same path without
reading this setting.
