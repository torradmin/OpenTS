---
name: vs-debug-context
description: Pull live debugger state (call stack, locals, breakpoints, Debug output) from an active Visual Studio session debugging OpenTS. Use when the user has GameD.exe or Game.exe paused in devenv and wants Claude to see what's happening at the breakpoint, or asks to check/read/attach to the debugger or debug session.
---

# Visual Studio debug context

Reads live state out of a running Visual Studio (`devenv.exe`) instance
through `EnvDTE` COM automation. Use it instead of asking the user to paste
locals or a call stack by hand.

## When to use this

The user has Visual Studio open with an OpenTS debug session (`GameD.exe` or
`Game.exe`) paused at a breakpoint, and wants help understanding a crash, a
bad value, or control flow. Confirm a debug session is actually active before
running the script — if it comes back with `"mode": "run"` or `"mode":
"design"`, the process isn't stopped at a breakpoint and there's no frame or
locals to inspect.

## Running it

```bash
powershell -File .claude/tools/vs-debug-context.ps1
```

Optional parameters:

- `-ProcessId <devenv pid>` — pick a specific Visual Studio instance when more
  than one `devenv.exe` is running. Without it, the script uses whichever
  instance the Running Object Table returns first.
- `-LocalsDepth <n>` (default `2`) — how many levels of nested members to
  expand for each local/argument. Raise it for a specific variable of
  interest; keep it low for a first look, since deep object graphs get large
  fast.
- `-MaxStackFrames <n>` (default `30`) — cap on call stack frames returned.

## Output shape

JSON with:

- `mode` — `break`, `run`, or `design`. Only `break` has frame/thread data.
- `currentFrame` — function name, module, language at the breakpoint.
- `locals` / `arguments` — name, type, value, and nested `members` up to
  `-LocalsDepth`. Vtable pointers (`__vfptr`) and char-array element-by-element
  expansion are suppressed since the string value already covers them.
- `currentThread`, `callStack` — thread info and the call stack outward from
  the current frame.
- `breakpoints` — every breakpoint currently set, with file, line, enabled
  state, and hit count.
- `debugOutputTail` — last ~60 lines of the Debug output pane.

## Failure modes

- No `devenv.exe` running, or `EnumRunning`/`GetActiveObject` finds nothing —
  the script throws; tell the user no Visual Studio instance was found.
- Multiple `devenv.exe` instances — pass `-ProcessId` (get it from `tasklist`
  or the JSON's `vsProcessId` on a first unqualified run) to target the right
  one.
- `mode` isn't `break` — nothing is paused; there's no frame, locals, or call
  stack to report.
