"""Production proof: actual movement/E/Q, quiet HUD, off-axis Boss counterstroke.
No teleports or injected resources on the full run.
"""
import unreal,pathlib,json,traceback
root=pathlib.Path(unreal.Paths.project_dir());out=root/'Saved/LTransmitEvidence/Experience';out.mkdir(parents=True,exist_ok=True)
scope={};exec((root/'Scripts/Editor/validate_ltransmit_pie.py').read_text(),scope);run=scope['TRANSMIT_RUN']
def fire_off_axis():
 c=run.a['Route_Carrier'];b=run.a['Weaponize_Charger'];pv=run.aim_live(c)
 p=b.get_actor_location();pos=c.get_actor_location()
 if abs(p.x-7980)>1 or abs(p.y-2530)>1 or abs(pos.y-p.y)<320:return False
 if b.state_machine.get_state() not in [unreal.MotionChargerState.RECOVERY,unreal.MotionChargerState.IDLE] or pv.target!=c or not pv.eligible:return False
 expected=unreal.Vector(p.x-pos.x,p.y-pos.y,0);expected=expected/expected.length()
 assert (pv.projected_world_direction-expected).length()<.01,'preview must aim at Boss from off-axis rail position'
 result=run.i.request_transfer()
 if result.succeeded:run.emit('off_axis_Q',carrier=str(pos),direction=str(pv.projected_world_direction),ok=True)
 return result.succeeded
run.steps=[(n,fire_off_axis,t,r) if n in ['power ram one','power ram two'] else (n,f,t,r) for n,f,t,r in run.steps]
shots=set();done=False;reset_at=0
views={'capture practice A':'01-quiet','walk (-1500, 0)':'02-seam','send carrier':'03-launch','recapture carrier':'04-standard','walk (7900, -600)':'05-interface','capture dash one':'06-boss','gate hit one':'07-fracture','gate hit two':'08-open','director complete':'09-complete'}
def observer(dt):
 global done,reset_at
 try:
  if not run.done:
   for row in run.rows:
    n=row.get('name')
    if n in views and n not in shots:
     unreal.SystemLibrary.execute_console_command(run.w,'Shot showui filename="'+str(out/(views[n]+'.png'))+'"',run.pc);shots.add(n)
  elif not done:
   out.joinpath('full-run.json').write_text(json.dumps(run.rows,indent=2));done=True
   if not run.rows[-1]['ok']:unreal.unregister_slate_post_tick_callback(obs_handle);return
   assert run.a['Flow_Reset'].request_room_reset();reset_at=run.now()
  elif run.now()-reset_at>1:
   c=run.a['Route_Carrier'];r=run.a['Weaponize_Ram'];b=run.a['Weaponize_Charger']
   assert r.hits==0 and not r.get_editor_property('armed')
   assert c.motion.get_endpoint_mode()==unreal.MotionEndpointMode.STORE and not c.motion.has_motion_state()
   assert (c.get_actor_location()-unreal.Vector(3200,0,85)).length()<1
   assert run.a['Weaponize_Gate'].get_actor_enable_collision()
   out.joinpath('full-reset.json').write_text(json.dumps({'ok':True,'carrier':str(c.get_actor_location()),'gate':str(run.a['Weaponize_Gate'].get_actor_location())},indent=2))
   unreal.unregister_slate_post_tick_callback(obs_handle);unreal.log('EXPERIENCE_FULL_RUN_AND_RESET_PASSED')
 except Exception:
  out.joinpath('observer-error.txt').write_text(traceback.format_exc());unreal.log_error(traceback.format_exc());unreal.unregister_slate_post_tick_callback(obs_handle)
obs_handle=unreal.register_slate_post_tick_callback(observer)
