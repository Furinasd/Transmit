"""Fresh-editor saved-map traversal and recovery gate; no content writes.

Launch with -ExecutePythonScript. Screenshots observe the actual player camera.
No teleport or Motion injection in the clean run. Recovery only injects the
explicit failures already defined by validate_ltransmit_pacing_recovery.py.
"""
import json
import pathlib
import time
import traceback
import unreal

ROOT = pathlib.Path(unreal.Paths.project_dir())
OUT = ROOT / 'Saved/LTransmitEvidence/Wayfinding'
OUT.mkdir(parents=True, exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert LEVELS.load_level('/Game/Transmit/Maps/L_Transmit')
WORLD = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
editor_actors = {a.get_actor_label(): a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
assert len([n for n in editor_actors if n.startswith('Wayfinding_')]) > 50
assert 'Presentation_VoidMist' in editor_actors and 'Presentation_Finish_Chapter_03' in editor_actors
assert editor_actors['Learn_PlayerStart'].get_actor_location().x == -10000
unreal.SystemLibrary.execute_console_command(WORLD, 'MAP CHECK')
LEVELS.editor_request_begin_play()
FILES = ['validate_ltransmit_pie.py', 'validate_ltransmit_pacing_recovery.py']
index = 0
runner = None
results = []
started = time.monotonic()
captured = set()
views = {'capture practice A': '01-first-operation', 'capture practice B': '02-turn-north',
         'walk (-5200, 3300)': '03-return-turn', 'ram armed': '04-docked',
         'walk (5350, -3500)': '05-service-route', 'capture reuse resource': '06-west-bank',
         'reclaim crossing resource': '07-look-back', 'walk (7900, -4100)': '08-reuse-north',
         'walk (7000, -600)': '09-arena-entry', 'gate hit two': '10-complete-impacts'}

def tick(dt):
    global index, runner
    try:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not world or not unreal.GameplayStatics.get_player_pawn(world, 0):
            assert time.monotonic()-started < 90, 'PIE startup timeout'
            return
        if runner is None:
            scope = {}
            exec((ROOT / 'Scripts/Editor' / FILES[index]).read_text(), scope)
            runner = scope['TRANSMIT_RUN']
        elif runner.done:
            result = runner.rows[-1]
            results.append({'script': FILES[index], 'result': result})
            index += 1
            (OUT / 'suite.json').write_text(json.dumps(results, indent=2))
            if not result['ok'] or index == len(FILES):
                unreal.unregister_slate_post_tick_callback(handle)
                LEVELS.editor_request_end_play()
                unreal.log('TRANSMIT_WAYFINDING_SUITE_FINISHED ' + json.dumps(results))
            else:
                runner = None
        elif index == 0:
            for row in runner.rows:
                name = row.get('name')
                if name in views and name not in captured:
                    unreal.SystemLibrary.execute_console_command(world, 'Shot filename="'+str(OUT / (views[name]+'.png'))+'"')
                    captured.add(name)
    except Exception:
        (OUT / 'error.txt').write_text(traceback.format_exc())
        unreal.log_error(traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(handle)
        LEVELS.editor_request_end_play()

handle = unreal.register_slate_post_tick_callback(tick)
