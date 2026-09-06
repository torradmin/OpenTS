---
title: Fix Escape not closing the game options dialog
category: fix
release: 0.2.0
targets: []
credit:
- torradmin
---

Escape now closes the game options dialog in two cases where it previously did nothing. Pressing
Escape while the dialog is already open now closes it, instead of requiring a mouse click.
Alt-tabbing away and back while the dialog is open no longer requires a mouse click either:
regaining focus used to leave keyboard input outside the dialog until a control was clicked, so
Escape had no effect until then.
