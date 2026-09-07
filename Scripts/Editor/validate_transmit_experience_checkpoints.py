"""Production checkpoint/cover fixture: explicit setup positions, real fall and E/Q.
Use a fresh PIE or the clean-run's finished full reset. Does not save assets.
"""
import unreal,pathlib,json,traceback
V=unreal.Vector;w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
a={x.get_actor_label():x for x in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.Actor)}
p=unreal.GameplayStatics.get_player_pawn(w,0);pc=unreal.GameplayStatics.get_player_controller(w,0)
i=p.get_component_by_class(unreal.MotionInteractorComponent);m=p.get_component_by_class(unreal.MotionTransferComponent)
r=a['Weaponize_Ram'];c=a['Route_Carrier'];b=a['Weaponize_Charger'];d=a['Flow_Director']
assert a['Flow_Reset'].request_room_reset()
start=unreal.GameplayStatics.get_time_seconds(w);since=start;stage=0;rows=[];spawn=None;minimum=99999
out=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence/Experience'
def now():return unreal.GameplayStatics.get_time_seconds(w)
def advance(label):
 global stage,since
 rows.append({'label':label,'seconds':now()-start,'player':str(p.get_actor_location()),'hits':r.hits});stage+=1;since=now()
def aim(actor):
 eye,_=p.get_actor_eyes_view_point();pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(eye,actor.get_actor_location()));i.refresh_target();return i.get_current_preview()
def capture():
 pv=aim(b)
 return pv.target==b and pv.eligible and i.request_capture().succeeded
def shoot():
 pv=aim(c);bp=b.get_actor_location()
 if abs(bp.x-7980)>1 or abs(bp.y-2530)>1 or b.state_machine.get_state() not in [unreal.MotionChargerState.RECOVERY,unreal.MotionChargerState.IDLE]:return False
 return pv.target==c and pv.eligible and i.request_transfer().succeeded
def finish(ok,error=''):
 out.joinpath('checkpoints.json').write_text(json.dumps({'ok':ok,'stage':stage,'rows':rows,'cover_min_boss_x':minimum,'error':error},indent=2));unreal.log('EXPERIENCE_CHECKPOINTS '+str(ok)+' '+error);unreal.unregister_slate_post_tick_callback(cp_handle)
def tick(dt):
 global spawn,minimum
 try:
  age=now()-since
  if now()-start>100:raise AssertionError('timeout stage '+str(stage))
  if stage==0 and age>1:
   spawn=p.get_actor_location();p.set_actor_location(V(spawn.x,spawn.y,-900),False,False);advance('fall in zone 1')
  elif stage==1 and age>1:
   assert (p.get_actor_location()-spawn).length()<10
   # Fixture represents completed Learn; formal run separately proves the real solve.
   a['Learn_BridgeSlab'].set_actor_location(V(1630,0,0),False,False)
   p.set_actor_location(a['Flow_RouteEntry'].get_actor_location(),False,False);advance('zone 1 respawn; enter zone 2')
  elif stage==2 and age>1:
   p.set_actor_location(V(3500,-900,-900),False,False);advance('fall in zone 2')
  elif stage==3 and age>1:
   assert abs(p.get_actor_location().x-2440)<10
   assert a['Learn_BridgeSlab'].get_actor_location().x==1630
   assert a['Route_Source'].motion.has_motion_state() and not c.motion.has_motion_state()
   c.set_actor_location(r.get_editor_property('dock_marker').get_actor_location(),False,False)
   assert c.motion.grant_motion_state(unreal.MotionState(direction=V(0,1,0),direction_policy=unreal.MotionDirectionPolicy.CAMERA_CANONICAL,magnitude=600,source_id='Checkpoint.DockFixture'))
   advance('zone 2 respawn preserves earlier traversal; setup dock')
  elif stage==4 and age>5:
   assert r.get_editor_property('armed')
   # Dock does not prematurely establish zone 3. Service-deck retry stays here.
   p.set_actor_location(V(7000,-500,-900),False,False);advance('fall before entering zone 3')
  elif stage==5 and age>1:
   assert abs(p.get_actor_location().x-5350)<10 and r.get_editor_property('armed')
   p.set_actor_location(a['Weaponize_EntryMarker'].get_actor_location(),False,False);advance('service retry keeps dock; enter zone 3')
  elif stage==6 and age>5:
   assert r.hits==0
   p.set_actor_location(V(6800,2530,100),False,False);advance('commissioning has no High damage')
  elif stage==7:
   if capture():advance('real Boss E')
  elif stage==8:
   if shoot():advance('real rail Q')
  elif stage==9 and age>1.5:
   assert r.hits==1;p.set_actor_location(V(6900,2500,-900),False,False);advance('fall after first gate hit')
  elif stage==10 and age>1:
   assert (p.get_actor_location()-a['Weaponize_EntryMarker'].get_actor_location()).length()<15
   assert r.hits==1 and r.get_editor_property('armed') and not m.has_motion_state()
   p.set_actor_location(V(6180,1970,100),False,False);advance('zone 3 respawn preserves dock and damage; shelter behind pillar')
  elif stage==11:
   minimum=min(minimum,b.get_actor_location().x)
   if age>12:
    assert minimum>6600 and minimum<7450,(minimum,'pillar must block a real dash')
    assert abs(p.get_actor_location().x-6180)<10,'cover must prevent player impact'
    advance('pillar intercepts Boss without blocking rail fan')
    assert a['Flow_Reset'].request_room_reset()
  elif stage==12 and age>1:
   assert r.hits==0 and not r.get_editor_property('armed')
   assert a['Story_Access'].get_editor_property('hidden')
   advance('full restart clears all checkpoints and gatekeeping sign');finish(True)
 except Exception:finish(False,traceback.format_exc())
cp_handle=unreal.register_slate_post_tick_callback(tick)
