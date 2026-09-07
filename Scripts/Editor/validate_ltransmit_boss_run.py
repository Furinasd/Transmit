"""Run the saved L_Transmit from its real start using movement and E/Q.
Writes independent full-run, reset and visual evidence. No clean-run teleports.
"""
import unreal,pathlib,json,traceback
root=pathlib.Path(unreal.Paths.project_dir());out=root/'Saved/LTransmitEvidence/Boss';out.mkdir(parents=True,exist_ok=True)
scope={};exec((root/'Scripts/Editor/validate_ltransmit_pie.py').read_text(),scope);run=scope['TRANSMIT_RUN']
# Hold the captured resource beyond recovery to detect accidental regeneration.
def held_once():
 if run.now()-run.since<12:return False
 held=run.p.get_component_by_class(unreal.MotionTransferComponent)
 boss=run.a['Weaponize_Charger']
 assert held.has_motion_state() and not boss.motion.has_motion_state()
 assert abs(boss.get_actor_location().x-7980)<1
 return True
idx=next(i for i,s in enumerate(run.steps) if s[0]=='capture dash one')
run.steps.insert(idx+1,('single dash owner beyond recovery',held_once,15,True))
shots=set();samples=[];finished=False;reset_at=0
views={'capture practice A':'01-chinese','walk (-1500, 0)':'02-overlap-repair','ram armed':'03-dock',
 'walk (7900, -600)':'04-warm-sky','capture dash one':'05-boss-capture','power ram one':'06-rail-strike',
 'gate hit one':'07-fracture','gate hit two':'08-gate-open','director complete':'09-complete'}
def observer(dt):
 global finished,reset_at
 try:
  if not run.done:
   for row in run.rows:
    name=row.get('name')
    if name in views and name not in shots:
     unreal.SystemLibrary.execute_console_command(run.w,'Shot showui filename="'+str(out/(views[name]+'.png'))+'"',run.pc);shots.add(name)
   samples.append({'t':round(run.now()-run.start,3),'dt':dt,'boss':str(run.a['Weaponize_Charger'].get_actor_location()),'rail':str(run.a['Route_Carrier'].get_actor_location())})
  elif not finished:
   out.joinpath('full-run.json').write_text(json.dumps(run.rows,indent=2))
   out.joinpath('frame-samples.json').write_text(json.dumps(samples))
   finished=True
   if not run.rows[-1]['ok']:unreal.unregister_slate_post_tick_callback(observer_handle);return
   assert run.a['Flow_Reset'].request_room_reset();reset_at=run.now()
  elif run.now()-reset_at>1:
   r=run.a['Weaponize_Ram'];c=run.a['Route_Carrier'];b=run.a['Weaponize_Charger']
   assert not r.get_editor_property('armed') and r.hits==0
   assert c.motion.get_endpoint_mode()==unreal.MotionEndpointMode.STORE
   assert c.motion.can_provide_motion and c.motion.can_receive_motion and not c.motion.has_motion_state()
   assert (c.get_actor_location()-unreal.Vector(3200,0,85)).length()<1
   assert abs(b.get_actor_location().x-7980)<1
   assert run.a['Weaponize_Gate'].get_actor_enable_collision()
   out.joinpath('full-reset.json').write_text(json.dumps({'ok':True,'same_carrier':c.get_path_name(),'checks':['original transform','Store endpoint','provide/receive flags','empty carrier','gate collision','Boss home','unarmed and zero damage']},indent=2))
   unreal.unregister_slate_post_tick_callback(observer_handle)
   unreal.log('TRANSMIT_BOSS_FULL_RUN_AND_RESET_PASSED')
 except Exception:
  out.joinpath('run-observer-error.txt').write_text(traceback.format_exc());unreal.log_error(traceback.format_exc());unreal.unregister_slate_post_tick_callback(observer_handle)
observer_handle=unreal.register_slate_post_tick_callback(observer)
