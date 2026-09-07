"""Saved-map sign readback and player-camera views. Visual smoke, not traversal.
Explicit viewpoint teleports are only for inspecting signs; no Motion is injected.
"""
import json
import pathlib
import traceback
import unreal

ROOT = pathlib.Path(unreal.Paths.project_dir())
OUT = ROOT / 'Saved/LTransmitEvidence/Wayfinding/FinalSigns'
OUT.mkdir(parents=True, exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Transmit/Maps/L_Transmit')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = {a.get_actor_label(): a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
signs = [a for n, a in actors.items() if n.startswith('Pacing_') and isinstance(a, unreal.TextRenderActor)]
assert len(signs) == 12
for a in signs:
    assert a.text_render.get_editor_property('vertical_alignment') == unreal.VerticalTextAligment.EVRTA_TEXT_CENTER
    back = actors['Wayfinding_SignBack_' + a.get_actor_label()]
    assert abs(back.get_actor_location().z - a.get_actor_location().z) < .01
    assert str(back.static_mesh_component.get_collision_profile_name()) == 'NoCollision'
baseline = json.loads((OUT.parent / 'prewrite.json').read_text())
moved = {'Pacing_ArrivalText','Pacing_FirstBridgeText','Pacing_OriginalLearnText',
         'Pacing_ReuseText','Pacing_ThreatPreviewText','Pacing_ArenaRetryText','Pacing_DashLessonText'}
for name, values in baseline.items():
    assert name in actors
    if name in moved:
        continue
    a = actors[name]
    p, r, s = a.get_actor_location(), a.get_actor_rotation(), a.get_actor_scale3d()
    actual = [p.x,p.y,p.z,r.pitch,r.yaw,r.roll,s.x,s.y,s.z]
    assert all(abs(x-y)<.0001 for x,y in zip(actual,values)), name
unreal.SystemLibrary.execute_console_command(world, 'MAP CHECK')
levels.editor_request_begin_play()
views = [(-10000,0,0,'opening'),(-8500,0,-30,'first-bank'),(-5200,200,155,'north-bank'),
         (5350,-5100,-90,'service'),(7350,-6200,140,'reclaim'),(7900,-1500,90,'arena-approach'),
         (7350,-600,180,'dash-lesson')]
index = 0
due = None

def tick(dt):
    global index, due
    try:
        w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not w:
            return
        pawn = unreal.GameplayStatics.get_player_pawn(w,0)
        if not pawn:
            return
        now = unreal.GameplayStatics.get_time_seconds(w)
        if index == len(views):
            (OUT / 'result.json').write_text(json.dumps({'ok':True,'retained_baseline_actors':len(baseline),
                'signs_centered':len(signs),'viewpoint_teleports':True,'traversal_claim':False},indent=2))
            unreal.unregister_slate_post_tick_callback(handle)
            levels.editor_request_end_play()
            unreal.log('TRANSMIT_SIGN_READBACK_SUCCESS')
        elif due is None:
            x,y,yaw,_ = views[index]
            pawn.set_actor_location(unreal.Vector(x,y,100),False,True)
            pawn.character_movement.stop_movement_immediately()
            unreal.GameplayStatics.get_player_controller(w,0).set_control_rotation(unreal.Rotator(pitch=-7,yaw=yaw,roll=0))
            due = now + .6
        elif now >= due:
            unreal.SystemLibrary.execute_console_command(w,'Shot filename="'+str(OUT/(views[index][3]+'.png'))+'"')
            index += 1
            due = None
    except Exception:
        (OUT / 'error.txt').write_text(traceback.format_exc())
        unreal.log_error(traceback.format_exc())
        unreal.unregister_slate_post_tick_callback(handle)
        levels.editor_request_end_play()

handle = unreal.register_slate_post_tick_callback(tick)
