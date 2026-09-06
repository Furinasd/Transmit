# Mac Editor shutdown crash — defensive mitigation and recurrence packet

Tracking: https://github.com/Furinasd/Transmit/issues/8

Status: project-side mitigation; engine root cause and original failure resolution
remain unverified. Baseline: `60cbf00`, branch `Jason/L_Transmit_v01`.
No gameplay C++, binary assets, rendering or plugin configuration changes.

## Observed environment and classification

UE 5.8.2 CL 56702186, macOS 26.6.2 (25G83), Apple M5 Pro, 24 GB,
arm64 Editor, Metal SM6. Records span September 6–7, 2026, Asia/Shanghai.

Family A has 16 retained reports with portable stack hash
`B50BAFFC7353F261B6B34AD6D752768986469182` at inspection. This is a count
of retained reports, not a measured probability or number of failed sessions.

```text
NSApplication terminate -> exit -> __cxa_finalize_ranges
 -> FTSTicker destructor -> delegate/notification destruction
 -> SNotificationExtendable -> STextBlock -> FTextLayout
 -> FICUTextBiDi destructor -> ubidi_close_64
 -> POINTER_BEING_FREED_WAS_NOT_ALLOCATED -> malloc_report -> abort
```

This establishes an invalid free detected during process teardown. It does not
identify who first corrupted/freed the pointer. No project C++ frame appears in
this stack; that alone does not exonerate all earlier project behavior.
`bIsOOM=0` does not prove absence of all memory pressure. There is no GPU-fault
evidence in this stack. Chinese text, ICU shutdown order and outstanding async
notifications are hypotheses, not established causes. No verified Epic hotfix
or locale/unattended workaround is known from this investigation.

## Original sequence and exact evidence

Latest matching original process: PID 36952, crash directory suffix
`FA19C389644715E2FE6509ACDA9EA74C`. The user-pasted stack matches this family;
addresses vary by process. Launch, with personal path redacted:

```sh
UnrealEditor "$PROJECT/passely.uproject" \
  -ExecutePythonScript=/tmp/transmit_run_session.py -nosplash -windowed \
  -ResX=1440 -ResY=900 -abslog=/tmp/transmit-experience-final.log
```

1. Python kept the process alive and polled `/tmp/transmit-session-request.py`
   via a Slate post-tick callback. The original driver had no close operation.
2. Session ran automation/PIE, screenshots, map reload and save.
3. At **03:58:17 +08**, PIE ended. At **03:58:58**, map save completed and
   Map Check recorded 0 errors/0 warnings. These do not validate shutdown.
4. At **03:59:21.370**, the log recorded `Mac GracefulTerminationHandler`.
   Engine source installs that handler for **SIGINT, SIGTERM and SIGHUP**.
   The exact signal and sender are not retained; ordinary window-close cannot
   be inferred from this message.
5. Log closed at **03:59:24.552**; crash context recorded the invalid-free stack.
6. CrashReportClient opened at **03:59:25**. Its own shutdown at **03:59:34**
   produced a separate `EXC_BAD_ACCESS` at address `0x10` in
   `FOutputDeviceRedirector::IsRedirectingTo`, via scheduler `StopWorkers` and
   assertion logging. That secondary report is not the Editor cause.

Raw originals remain locally in
`~/Library/Application Support/Epic/UnrealEngine/5.8/Saved/Crashes/` and
`~/Library/Logs/DiagnosticReports/`. Do not upload them without redaction.

Family B is independent: PID 70597, September 6 **19:55:30 +08**, worktree
preview authoring. Generic `EditorAssetLibrary.duplicate_asset(World)` preceded
`LevelEditorSubsystem.LoadLevel`. `L_PresentationPreview` retained standalone
GC keep-flags and map loading aborted in `EditorServer.cpp:2544` with
`World Memory Leaks: 2 leaks objects and packages`. Repository handoff
`Docs/Handoff/B_Visual.md` records switching to `new_level_from_template`.
That known sequence is not evidence that all map switches are unsafe.

## Why this defensive patch

`transmit_editor_safety.py` rejects PIE/dirty-package transitions, skips reloading
the same map, checks the resulting world, and owns callback/keep-alive cleanup.
It does **not** force garbage collection: `SystemLibrary.CollectGarbage` schedules
collection and cannot prove all Python references or standalone flags are gone.
It cannot clean callbacks created by other scripts.

Wayfinding now releases its owned keep-alive/callback on completion/failure;
experience authoring uses the guarded loader. The base traversal finalizer
records failure even when the PIE Pawn/World has already been destroyed, retaining
the normal timing fields and marking unavailable game time as null.

Engine `EditorPythonExecuter.cpp` creates an `FAsyncTaskNotification` for
`-ExecutePythonScript`. When keep-alive becomes false, its next tick calls
`DestroyNotification()` then defers `QUIT_EDITOR`. This provides a concrete
reason to release keep-alive and let the executor finish before terminating the
process. It does not prove the observed invalid allocation originated there.

## Validation and controlled reproduction

Host regression command: `python3 -B Scripts/Editor/test_editor_safety.py`.
Six passing cases cover no-op reload, PIE/dirty refusal, mismatched world,
idempotent owned cleanup, retained handle/keep-alive on unregistration failure,
and persisted failed results after Pawn/World destruction.

Dedicated, read-only Editor smoke:

```sh
"$UE/Engine/Binaries/Mac/UnrealEditor" "$PROJECT/passely.uproject" \
  -ExecutePythonScript="$PROJECT/Scripts/Editor/validate_editor_shutdown.py" \
  -unattended -nosplash -ModelContextProtocolPort=18793 \
  -abslog=/tmp/transmit-shutdown-smoke-default.log
```

Record the actual exit status, cleanup marker, close reason, and newly created
crash/ensure directories. The script opens TestChamber without saving, checks
same-map loading and closes its callback twice. This is a narrow lifecycle probe,
not a gameplay or original full-session rerun. Never count its marker alone as
success. A ten-session clean result is the issue's future closure gate, not a
claim made by this patch.

Initial smoke with `-DisablePlugins=ModelContextProtocol,AllToolsets` reached
cleanup and native CloseEditor, but exited **1** after a startup ensure
`AssetBaseClassLoaded` / missing `/Script/GameFeatures.GameFeatureData`.
This is not a passing run; do not prescribe plugin disabling as a fix.

Default-plugin control (PID 45028) cleaned callbacks then immediately called
`SystemLibrary.quit_editor()`: **exit 1**, new crash
`3E58725F7044A8927DB55BACA7F8A612`, exact same `B50BAFFC...` stack. Thus
callback cleanup plus immediate native quit is insufficient; unattended mode
does not prevent it. The final smoke removes only that immediate quit call,
allowing the executor's next tick to destroy its notification and defer quit.

Final deferred-exit smoke (PID 45277, `/tmp/transmit-shutdown-smoke-deferred.log`)
completed at **04:32:31 +08**, **exit 0**. This is one successful controlled run,
not a ten-session reliability claim or an engine-wide fix. Immediate native quit
versus executor-deferred quit is the discriminating change; ports were distinct
to avoid interfering with another Editor. Full original automation was not rerun.

`Scripts/Editor/transmit_editor_session.py` replaces the ephemeral shared
`/tmp/transmit_run_session.py` workflow for future runs. It logs a unique inbox,
consumes each trusted request once, retains an explicit error result, and exposes
`request_shutdown()` which refuses PIE/dirty state, releases its own callback and
keep-alive, then lets the Python executor finish. Start with this tracked script
as `-ExecutePythonScript`. Finish probes and PIE, then submit `request_shutdown()`
in that session's `request.py`; inspect both result.json and process exit. This
does not retrofit an already-running old session. Its orchestration has static
validation only; the owned-cleanup mechanism was exercised by the smoke above.

For further isolation compare fresh Editor without Python, the narrow script,
and original workload under the same engine/plugins/locale, changing one variable
at a time. Test signal exit only in a dedicated process with no unsaved assets;
record the sender and signal. Do not signal a shared user Editor.

## Recurrence / Epic feedback packet

Record commit, dirty status, exact launch and exit method, timestamp/timezone,
PID, map, Editor/PIE/packaged mode, script/request revision, owned callback state,
keep-alive state, first actionable error, complete stack/hash, and exit code.
Preserve matching CrashContext, log, stderr and dump together before restarting.
Record secondary reporter failures separately. Remove LoginId, EpicAccountId,
AUTH_PASSWORD/tokens, machine identifiers and personal paths from public text.
Share dumps only through an explicitly approved private reporting channel.

Open gates: original signal-exit reproduction, sustained clean shutdowns,
controlled cross-map test, full gameplay suite after automation edits, and
upstream allocation ownership diagnosis. Do not close issue #8 merely because
the mitigation was pushed.
