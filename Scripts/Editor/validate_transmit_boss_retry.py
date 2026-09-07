"""Production PIE retry fixture. Explicit dock setup; all Boss E/Q actions are real.
Run after validate_ltransmit_boss_run.py has completed its full reset.
"""
import unreal,pathlib,json,traceback
assert run.done and run.rows[-1]['ok']
R=run.a['Weaponize_Ram'];C=run.a['Route_Carrier'];B=run.a['Weaponize_Charger'];D=run.a['Flow_Director']
M=run.p.get_component_by_class(unreal.MotionTransferComponent)
C.set_actor_location(R.get_editor_property('dock_marker').get_actor_location(),False,False)
assert C.motion.grant_motion_state(unreal.MotionState(direction=unreal.Vector(0,1,0),direction_policy=unreal.MotionDirectionPolicy.CAMERA_CANONICAL,magnitude=600,source_id='RetryFixture.Route'))
run.p.set_actor_location(D.get_editor_property('arena_entry_marker').get_actor_location(),False,False)
start=run.now();since=start;stage=0;checks=[]
def advance(label):
 global stage,since
 checks.append(label);stage+=1;since=run.now()
def retry_tick(dt):
 global stage
 try:
  age=run.now()-since
  if run.now()-start>70:raise AssertionError('retry fixture timeout '+str(stage))
  if stage==0:
   if age>5:
    assert R.get_editor_property('armed')
    run.p.set_actor_location(unreal.Vector(6800,2530,100),False,False)
    advance('same carrier docked')
  elif stage in [1,4,8]:
   if run.capture_dash():advance('real E capture')
  elif stage in [2,5,9]:
   if run.fire_rail():advance('real Q stroke')
  elif stage==3:
   if age>1.5:assert R.hits==1;advance('one actual gate hit committed')
  elif stage==6:
   assert R.hits==1 # callback occurs before the next full stroke can resolve
   assert D.request_local_retry();advance('local retry during second stroke')
  elif stage==7:
   if age>2:
    assert R.hits==1 and R.get_editor_property('armed') and not M.has_motion_state()
    assert abs(C.get_actor_location().x-7100)<1 and abs(B.get_actor_location().x-7980)<1
    run.p.set_actor_location(unreal.Vector(6800,2530,100),False,False)
    advance('retry preserved dock and committed damage; cancelled pending damage')
  elif stage==10:
   assert run.a['Flow_Reset'].request_room_reset();advance('full reset during stroke')
  elif stage==11 and age>2:
   assert R.hits==0 and not R.get_editor_property('armed')
   assert (C.get_actor_location()-unreal.Vector(3200,0,85)).length()<1
   assert C.motion.get_endpoint_mode()==unreal.MotionEndpointMode.STORE
   assert not C.motion.has_motion_state() and not M.has_motion_state()
   assert run.a['Weaponize_Gate'].get_actor_enable_collision()
   checks.append('full reset restored carrier endpoint, resources and gate')
   save(True);unreal.unregister_slate_post_tick_callback(retry_handle)
 except Exception:
  save(False,traceback.format_exc());unreal.unregister_slate_post_tick_callback(retry_handle)
def save(ok,error=''):
 dest=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence/Boss/retry.json'
 dest.write_text(json.dumps({'ok':ok,'seconds':run.now()-start,'checks':checks,'error':error},indent=2));unreal.log('BOSS_RETRY '+dest.read_text())
retry_handle=unreal.register_slate_post_tick_callback(retry_tick)
