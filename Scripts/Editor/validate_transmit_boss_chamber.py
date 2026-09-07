"""PIE integration in L_TestChamber, unsaved editor fixtures only.
Teleports are explicit fixture setup; capture/transfer use the real interactor.
"""
import unreal,pathlib,json,math,traceback
W=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
A={x.get_actor_label():x for x in unreal.GameplayStatics.get_all_actors_of_class(W,unreal.Actor)}
B=A['BossTest_Boss'];C=A['BossTest_Carrier'];R=A['BossTest_Ram'];P=unreal.GameplayStatics.get_player_pawn(W,0);PC=unreal.GameplayStatics.get_player_controller(W,0)
I=P.get_component_by_class(unreal.MotionInteractorComponent);M=P.get_component_by_class(unreal.MotionTransferComponent)
V=unreal.Vector
P.set_actor_location(V(-400,3500,100),False,False)
state=unreal.MotionState(direction=V(0,1,0),direction_policy=unreal.MotionDirectionPolicy.CAMERA_CANONICAL,magnitude=600,source_id='Chamber.Route')
assert C.motion.grant_motion_state(state)
B.set_encounter_active(True)
start=unreal.GameplayStatics.get_time_seconds(W);stage=0;rows=[];aim=0;rail=[];since=start
out=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence/Experience';out.mkdir(parents=True,exist_ok=True)
def now():return unreal.GameplayStatics.get_time_seconds(W)
def emit(n,**extra):
 rows.append({'name':n,'seconds':round(now()-start,3),'boss':str(B.get_actor_location()),'carrier':str(C.get_actor_location()),'hits':R.hits,**extra});unreal.log('BOSS_CHAMBER '+json.dumps(rows[-1]))
def step(n):
 global stage,since,aim
 emit(n);stage+=1;since=now();aim=0

def target(actor):
 eye,_=P.get_actor_eyes_view_point();PC.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(eye,actor.get_actor_location()));I.refresh_target()
 return I.get_current_preview()
def tick(dt):
 global stage,aim
 try:
  age=now()-since;s=B.state_machine.get_state();bp=B.get_actor_location();cp=C.get_actor_location()
  if now()-start>90:raise AssertionError('timeout at '+str(stage))
  if stage==0:
   if s==unreal.MotionChargerState.DASH:
    P.set_actor_location(V(-400,4600,100),False,False);step('dash aims at player; evade after commit')
  elif stage==1:
   if s==unreal.MotionChargerState.RECOVERY and age>3.0 and (bp-V(800,3500,100)).length()<1:
    assert R.get_editor_property('armed');assert not C.motion.has_motion_state();step('miss returns home; routing energy consumed once')
  elif stage==2:
   rail.append([now(),cp.y])
   if age>13:
    dy=[(rail[j][1]-rail[j-1][1])/(rail[j][0]-rail[j-1][0]) for j in range(1,len(rail)) if rail[j][0]>rail[j-1][0]]
    assert any(x>150 for x in dy) and any(x<-150 for x in dy)
    step('same carrier reverses at constant rail speed')
    P.set_actor_location(V(-400,3500,100),False,False)
  elif stage in [3,6,9]:
   target(B)
   if s==unreal.MotionChargerState.DASH and B.state_machine.is_capture_window_open() and (bp-P.get_actor_location()).length()<650:
    preview=I.get_current_preview()
    if preview.target==B and preview.eligible:
     result=I.request_capture()
     if result.succeeded:step('actual E captures player-directed dash')
  elif stage in [4,7,10]:
   target(C)
   ready=s in [unreal.MotionChargerState.RECOVERY,unreal.MotionChargerState.IDLE] and (bp-V(800,3500,100)).length()<1
   aligned=abs(cp.y-3500)<70 if stage!=4 else abs(cp.y-3500)>400
   preview=I.get_current_preview()
   if ready and aligned and preview.target==C and preview.eligible:
    result=I.request_transfer()
    if result.succeeded:
     assert not M.has_motion_state()
     if stage==4:
      B.set_encounter_active(False);B.set_actor_location(V(800,4700,100),False,False)
     step('actual Q locks Boss position '+('then Boss evades' if stage==4 else 'for hit'))
  elif stage==5:
   if age>1.6:
    assert R.hits==0;B.set_actor_location(V(800,3500,100),False,False);B.set_encounter_active(True);step('committed stroke does not home after Boss evades; no free gate damage')
  elif stage==8:
   if age>1.6:
    assert R.hits==1;step('real circular hit fractures gate once')
  elif stage==11:
   if age>1.6:
    assert R.hits==2 and not A['BossTest_Gate'].get_actor_enable_collision();step('second real hit opens gate')
  elif stage==12:
   B.set_encounter_active(True);P.set_actor_location(V(350,3500,100),False,False);step('setup contact return')
  elif stage==13:
   if age>7 and (bp-V(800,3500,100)).length()<1:
    step('player contact also returns home');out.joinpath('chamber.json').write_text(json.dumps({'ok':True,'rows':rows},indent=2));unreal.unregister_slate_post_tick_callback(handle)
 except Exception:
  out.joinpath('chamber.json').write_text(json.dumps({'ok':False,'stage':stage,'error':traceback.format_exc(),'rows':rows},indent=2));unreal.log_error(traceback.format_exc());unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)
