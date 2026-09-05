#requires -version 5.1
<#
Fetches live debugger state from a running Visual Studio instance via EnvDTE
COM automation: debugger mode, current thread/stack frame, call stack,
locals, breakpoints, and the Debug output pane. Prints JSON to stdout.

Usage:
  powershell -File vs-debug-context.ps1 [-ProcessId <devenv pid>] [-LocalsDepth <n>]

If -ProcessId is omitted, the first Visual Studio instance found in the
Running Object Table is used (fine when only one devenv.exe is running).
#>
param(
    [int]$ProcessId = 0,
    [int]$LocalsDepth = 2,
    [int]$MaxStackFrames = 30
)

$ErrorActionPreference = 'Stop'

function Get-DteInstances {
    # Enumerate the Running Object Table via C# (interface-typed P/Invoke out
    # params need a compiled context to marshal correctly; PowerShell's
    # dynamic [ref] binding loses the RCW's interface identity).
    $src = @'
using System;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Collections.Generic;

namespace VsDebugTool {
    public static class RotFinder {
        [DllImport("ole32.dll")]
        static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable prot);

        [DllImport("ole32.dll")]
        static extern int CreateBindCtx(int reserved, out IBindCtx ppbc);

        public class Entry {
            public string Name;
            public int Pid;
            public object Dte;
        }

        public static List<Entry> FindDte() {
            var results = new List<Entry>();
            IRunningObjectTable rot;
            GetRunningObjectTable(0, out rot);
            IEnumMoniker enumMoniker;
            rot.EnumRunning(out enumMoniker);
            enumMoniker.Reset();
            IBindCtx bindCtx;
            CreateBindCtx(0, out bindCtx);

            IMoniker[] monikers = new IMoniker[1];
            IntPtr fetched = IntPtr.Zero;
            while (enumMoniker.Next(1, monikers, fetched) == 0) {
                string name;
                try {
                    monikers[0].GetDisplayName(bindCtx, null, out name);
                } catch { continue; }
                if (name != null && name.StartsWith("!VisualStudio.DTE")) {
                    object obj;
                    rot.GetObject(monikers[0], out obj);
                    int pid = 0;
                    int idx = name.LastIndexOf(':');
                    if (idx >= 0) int.TryParse(name.Substring(idx + 1), out pid);
                    results.Add(new Entry { Name = name, Pid = pid, Dte = obj });
                }
            }
            return results;
        }
    }
}
'@
    Add-Type -TypeDefinition $src -Language CSharp -ErrorAction Stop
    return [VsDebugTool.RotFinder]::FindDte()
}

$instances = Get-DteInstances
if (-not $instances -or $instances.Count -eq 0) {
    throw "No running Visual Studio (DTE) instance found. Is devenv.exe running?"
}

$chosen = $null
if ($ProcessId -gt 0) {
    $chosen = $instances | Where-Object { $_.Pid -eq $ProcessId } | Select-Object -First 1
    if (-not $chosen) { throw "No DTE instance found for process id $ProcessId. Found: $($instances.Name -join ', ')" }
} else {
    $chosen = $instances | Select-Object -First 1
}

$dte = $chosen.Dte
$debugger = $dte.Debugger

$modeMap = @{ 1 = 'design'; 2 = 'break'; 3 = 'run' }
$mode = $modeMap[[int]$debugger.CurrentMode]
if (-not $mode) { $mode = "unknown($([int]$debugger.CurrentMode))" }

function Convert-Locals($expressions, $depth) {
    if ($depth -le 0 -or -not $expressions) { return @() }
    $out = @()
    foreach ($e in $expressions) {
        $item = [ordered]@{
            name  = $e.Name
            type  = $e.Type
            value = $e.Value
        }
        $skipExpand = ($e.Name -eq '__vfptr') -or ($e.Type -match '^(char|wchar_t|unsigned char)\s*\[')
        if (-not $skipExpand -and $e.DataMembers -and $e.DataMembers.Count -gt 0 -and $depth -gt 1) {
            $members = @()
            $n = 0
            foreach ($m in $e.DataMembers) {
                if ($n -ge 40) { break }
                $members += $m
                $n++
            }
            $item.members = Convert-Locals $members ($depth - 1)
        }
        $out += [PSCustomObject]$item
    }
    return $out
}

$result = [ordered]@{
    vsProcessId = $chosen.Pid
    mode        = $mode
}

if ($mode -eq 'break') {
    $frame = $debugger.CurrentStackFrame
    if ($frame) {
        $result.currentFrame = [ordered]@{
            functionName = $frame.FunctionName
            module       = $frame.Module
            language     = $frame.Language
        }
        $result.locals = Convert-Locals $frame.Locals 1
        if ($LocalsDepth -gt 1) {
            $result.locals = Convert-Locals $frame.Locals $LocalsDepth
        }
        try {
            $result.arguments = Convert-Locals $frame.Arguments 1
        } catch { }
    }

    $thread = $debugger.CurrentThread
    if ($thread) {
        $result.currentThread = [ordered]@{
            name     = $thread.Name
            id       = $thread.ID
            priority = $thread.Priority
            location = $thread.Location
        }
        $stack = @()
        $i = 0
        foreach ($sf in $thread.StackFrames) {
            if ($i -ge $MaxStackFrames) { break }
            $stack += [ordered]@{
                functionName = $sf.FunctionName
                module       = $sf.Module
            }
            $i++
        }
        $result.callStack = $stack
    }
}

$breakpoints = @()
foreach ($bp in $debugger.Breakpoints) {
    $breakpoints += [ordered]@{
        file         = $bp.File
        line         = $bp.FileLine
        functionName = $bp.FunctionName
        enabled      = $bp.Enabled
        hitCount     = $bp.CurrentHits
        condition    = $bp.Condition
    }
}
$result.breakpoints = $breakpoints

$result.debugOutputTail = $null
$result.debugOutputTailError = $null
try {
    $outputWindow = $dte.ToolWindows.OutputWindow
    $debugPane = $null
    foreach ($pane in $outputWindow.OutputWindowPanes) {
        if ($pane.Name -like '*Debug*') { $debugPane = $pane; break }
    }
    if (-not $debugPane) {
        $debugPane = $outputWindow.OutputWindowPanes.Item('Debug')
    }
    $textDoc = $debugPane.TextDocument
    $startPoint = $textDoc.StartPoint
    $editPoint = $startPoint.CreateEditPoint()
    $fullText = $editPoint.GetText($textDoc.EndPoint)
    $lines = $fullText -split "`r`n"
    $tail = $lines | Select-Object -Last 120
    $result.debugOutputTail = $tail -join "`n"
} catch {
    # Surface the failure instead of silently nulling it out, so a caller
    # can tell "no output pane" apart from "the fetch itself broke".
    $result.debugOutputTailError = $_.Exception.Message
}

# The base EnvDTE.Debugger interface has no "current exception" property, but
# the live COM object is often a more derived type (Debugger2/3/5) that does;
# PowerShell's late-bound property access reaches it without a static cast.
$result.exceptionInfo = $null
try {
    $exc = $debugger.CurrentException
    if ($exc) {
        $result.exceptionInfo = [ordered]@{
            name        = $exc.Name
            code        = $exc.Code
            description = $exc.Description
        }
    }
} catch {
    $result.exceptionInfoError = $_.Exception.Message
}

# LastBreakReason is a dbgEventReasonEnum value (5 = ExceptionThrown, 6 =
# ExceptionNotHandled, 11 = UserBreakpoint, ...). It is the closest thing to
# "why are we stopped" the base interface offers.
try {
    $result.lastBreakReason = [int]$debugger.LastBreakReason
} catch { }

$result | ConvertTo-Json -Depth 12
