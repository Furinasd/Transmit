"""Possessed PIE visual smoke of final HUD, same-stage R and first interaction.
Start floating PIE via the Editor tool after READY. No content writes.
"""
import pathlib,json,traceback,unreal
ROOT=pathlib.Path(unreal.Paths.project_dir())
OUT=ROOT/'Saved/LTransmitEvidence/Vertical/FinalHUD'
OUT.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Transmit/Maps/L_Transmit')
stage=0
start=None
runner=None
captured=False

def shot(w,pc,name):
 unreal.SystemLibrary.execute_console_command(w,'Shot showui filename="'+str(OUT/(name+'.png'))+'"',pc)

def tick(dt):
 global stage,start,runner,captured
 try:
  w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
  if not w:return
  pawn=unreal.GameplayStatics.get_player_pawn(w,0)
  if not pawn:return
  pc=unreal.GameplayStatics.get_player_controller(w,0)
  now=unreal.GameplayStatics.get_time_seconds(w)
  if start is None:start=now
  age=now-start
  if stage==0 and age>3:
   shot(w,pc,'01-opening');stage=1
  elif stage==1 and age>14:
   shot(w,pc,'02-guidance-faded');stage=2;start=now
  elif stage==2 and age>1:
   reset=unreal.GameplayStatics.get_actor_of_class(w,unreal.MotionRoomResetController)
   assert reset.request_room_reset()
   start=now;stage=3
  elif stage==3 and age>1:
   position=pawn.get_actor_location()
   assert abs(position.x+10000)<2 and abs(position.y)<2 and abs(position.z-392.15)<3, str(position)
   shot(w,pc,'03-same-objective-restart');stage=4;start=now
  elif stage==4 and age>1:
   scope={};exec((ROOT/'Scripts/Editor/validate_ltransmit_pie.py').read_text(),scope)
   runner=scope['TRANSMIT_RUN'];runner.steps=runner.steps[:5]
   stage=5
  elif stage==5:
   if not captured and any(r.get('name')=='capture practice A' for r in runner.rows):
    shot(w,pc,'04-loaded-action');captured=True
   if runner.done:
    assert runner.rows[-1]['ok'],str(runner.rows[-1])
    shot(w,pc,'05-crossing-ready')
    (OUT/'result.json').write_text(json.dumps({'ok':True,'capture_and_transfer':True,
     'same_objective_full_reset':True,'image_review_required':True,'tab_hold':'human check'},indent=2))
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.log('TRANSMIT_VERTICAL_UI_SUCCESS')
 except Exception:
  (OUT/'error.txt').write_text(traceback.format_exc());unreal.log_error(traceback.format_exc())
  unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
unreal.log('TRANSMIT_VERTICAL_UI_READY')
