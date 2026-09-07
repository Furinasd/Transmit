"""Saved-map traversal/recovery. Start a possessed floating PIE via Editor MCP.
No map writes. Clean run uses actual movement/targeting, no teleports.
"""
import json,pathlib,time,traceback,unreal
ROOT=pathlib.Path(unreal.Paths.project_dir())
OUT=ROOT/'Saved/LTransmitEvidence/Vertical'
OUT.mkdir(parents=True,exist_ok=True)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Transmit/Maps/L_Transmit')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
a={a.get_actor_label():a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
assert a['Learn_PlayerStart'].get_actor_location().z==400
assert a['Pacing_RouteBridgeA'].get_actor_location().z==625
assert a['Pacing_RouteBridgeB'].get_actor_location().z==625
assert a['Pacing_RouteSource'].get_actor_location().z==720
unreal.SystemLibrary.execute_console_command(world,'MAP CHECK')
files=['validate_ltransmit_pie.py','validate_ltransmit_pacing_recovery.py']
runner=None
index=0
results=[]
frames=[]
shots=set()
views={'capture practice A':'01-first-capture','walk (-5200, 3300)':'02-inspection-loop',
       'walk (-1500, 3300)':'03-overlook','ram armed':'04-relay',
       'walk (5350, -3500)':'05-service-climb','capture reuse resource':'06-upper-deck',
       'reclaim crossing resource':'07-reclaim','walk (7900, -600)':'08-descent',
       'gate hit one':'09-fracture','gate hit two':'10-open','director complete':'11-complete'}
started=time.monotonic()

def tick(dt):
 global runner,index
 try:
  w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
  if not w or not unreal.GameplayStatics.get_player_pawn(w,0): return
  if runner is None:
   scope={};exec((ROOT/'Scripts/Editor'/files[index]).read_text(),scope);runner=scope['TRANSMIT_RUN']
  elif runner.done:
   results.append({'script':files[index],'result':runner.rows[-1]})
   (OUT/'suite.json').write_text(json.dumps(results,indent=2))
   (OUT/'movement-samples.json').write_text(json.dumps(frames))
   index+=1
   if not results[-1]['result']['ok'] or index==len(files):
    unreal.unregister_slate_post_tick_callback(handle)
    unreal.log('TRANSMIT_VERTICAL_SUITE_FINISHED '+json.dumps(results))
   else: runner=None
  elif index==0:
   p=runner.p.get_actor_location()
   frames.append([round(runner.now(),2),round(p.x,1),round(p.y,1),round(p.z,1)])
   # Specify the PlayerController so Shot routes to the game viewport, not Editor.
   if 'opening' not in shots:
    unreal.SystemLibrary.execute_console_command(w,'Shot showui filename="'+str(OUT/'00-opening.png')+'"',runner.pc)
    shots.add('opening')
   for row in runner.rows:
    name=row.get('name')
    if name in views and name not in shots:
     unreal.SystemLibrary.execute_console_command(w,'Shot showui filename="'+str(OUT/(views[name]+'.png'))+'"',runner.pc)
     shots.add(name)
 except Exception:
  (OUT/'error.txt').write_text(traceback.format_exc());unreal.log_error(traceback.format_exc())
  unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
unreal.log('TRANSMIT_VERTICAL_READY_FOR_PIE')
