---
title: Let PlayIntro skip the startup title movie too
category: feature
release: 0.2.0
targets:
- type: key
  id: PlayIntro
  effect: changed
credit: [torradmin]
---

`PlayIntro=no` used to skip only `EVA.VQA`. It now also skips the Westwood
logo and the Tiberian Sun or Firestorm title movie played right after it at
startup. It does not affect the title movie played after choosing a game on
the game-select screen.
