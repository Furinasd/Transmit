"""Run in a dedicated -ExecutePythonScript Editor; never saves assets.

Exercises guarded no-op map load, owned callback cleanup and native quit.
Process exit code and absence of a new crash report must be checked externally.
"""
import pathlib
import sys
import unreal

sys.path.insert(0, str(pathlib.Path(unreal.Paths.project_dir()) / 'Scripts/Editor'))
from transmit_editor_safety import CallbackOwner, load_level_checked, require_clean_editor

world = load_level_checked('/Game/Transmit/Maps/L_TestChamber')
assert load_level_checked('/Game/Transmit/Maps/L_TestChamber') == world
world = None
owner = CallbackOwner(keep_alive=True)
frames = 0


def tick(delta):
    global frames
    frames += 1
    if frames < 5:
        return
    require_clean_editor()
    owner.close()
    owner.close()
    unreal.log('TRANSMIT_SHUTDOWN_SMOKE_CLEANUP_COMPLETE')
    # Let EditorPythonExecuter observe keep-alive=false on its next tick:
    # it destroys its async notification before deferring QUIT_EDITOR.


owner.register(tick)
