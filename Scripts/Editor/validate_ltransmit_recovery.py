"""Destructive-path PIE probes. Teleports only the live pawn to induce failures.
Run after the continuous traversal test, with PIE active. Never saves assets.
"""
import unreal,time,json,pathlib
if globals().get('TRANSMIT_RECOVERY') and not TRANSMIT_RECOVERY.done:TRANSMIT_RECOVERY.finish(False,'replaced')
class RecoveryProbe:
 def __init__(self):
  self.w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
  assert self.w and 'L_Transmit' in self.w.get_name()
  self.p=unreal.GameplayStatics.get_player_pawn(self.w,0)
  self.a={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(self.w,unreal.Actor)}
  self.rows=[];self.done=False;self.phase=0;self.since=self.now();self.waiting=False
  self.a['Flow_Reset'].request_room_reset()
  self.handle=unreal.register_slate_post_tick_callback(self.tick)
 def now(self):return unreal.GameplayStatics.get_time_seconds(self.w)
 def emit(self,event,**data):
  r={'event':event,**data};self.rows.append(r);unreal.log('TRANSMIT_RECOVERY '+json.dumps(r,default=str))
 def check(self):
  ram=self.a['Weaponize_Ram'];carrier=self.a['Route_Carrier'];bridge=self.a['Learn_BridgeSlab'];gate=self.a['Weaponize_Gate']
  checks={'player_at_start':self.p.get_actor_location().x<100 and abs(self.p.get_actor_location().y)<100,
   'player_empty':not self.p.get_component_by_class(unreal.MotionTransferComponent).has_motion_state(),
   'learn_source_restored':self.a['Learn_Source'].motion.has_motion_state(),
   'route_source_restored':self.a['Route_Source'].motion.has_motion_state(),
   'bridge_restored':abs(bridge.get_actor_location().x-680)<1 and not bridge.motion.has_motion_state(),
   'carrier_restored':abs(carrier.get_actor_location().x-3200)<1 and not carrier.motion.has_motion_state() and carrier.motion.can_provide_motion,
   'ram_reset':ram.hits==0 and not ram.get_editor_property('armed'),
   'gate_reset':abs(gate.get_actor_location().z-350)<1 and gate.get_actor_enable_collision()}
  self.emit('snapshot',phase=self.phase,checks=checks)
  return all(checks.values())
 def tick(self,dt):
  try:
   age=self.now()-self.since
   if self.phase==0 and age>.6:
    if not self.check():self.finish(False,'completed-gate reset failed');return
    # Physical throat admits a carrier, but a standing character sweep hits roof.
    start=unreal.Vector(3300,0,92);end=unreal.Vector(3750,0,92)
    h=unreal.SystemLibrary.capsule_trace_single(self.w,start,end,35,90,unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[self.p],unreal.DrawDebugTrace.NONE,True)
    h2=unreal.SystemLibrary.sphere_trace_single(self.w,unreal.Vector(3300,0,85),unreal.Vector(3750,0,85),50,unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[self.p,self.a['Route_Carrier']],unreal.DrawDebugTrace.NONE,True)
    self.emit('throat',player_blocker=h.to_tuple()[9].get_actor_label() if h else None,carrier_blocker=h2.to_tuple()[9].get_actor_label() if h2 else None)
    if not h or h2:self.finish(False,'throat clearance invalid');return
    self.phase=1;self.since=self.now()
   elif 1<=self.phase<=3:
    if not self.waiting:
     self.p.set_actor_location(unreal.Vector(0,0,-700),False,True);self.waiting=True;self.since=self.now()
    elif age>.6:
     if not self.check():self.finish(False,'repeated fall reset failed');return
     self.phase+=1;self.waiting=False;self.since=self.now()
   elif 4<=self.phase<=5:
    if not self.waiting:
     self.p.character_movement.stop_movement_immediately()
     self.p.set_actor_location(unreal.Vector(7580,1760,100),False,True);self.waiting=True;self.since=self.now()
    elif age>1 and self.p.get_actor_location().x<100:
     if not self.check():self.finish(False,'dash-hit reset incomplete');return
     self.emit('dash_hit_recovery',seconds=age);self.phase+=1;self.waiting=False;self.since=self.now()
    elif age>12:self.finish(False,'Charger did not reset Player on collision')
   elif self.phase>5:self.finish(True,'three falls and two dash collisions restored clean state')
  except:
   import traceback
   self.finish(False,traceback.format_exc())
 def finish(self,ok,why):
  self.done=True;unreal.unregister_slate_post_tick_callback(self.handle);self.emit('complete',ok=ok,reason=why)
  d=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence';d.mkdir(parents=True,exist_ok=True)
  (d/('recovery-'+str(int(time.time()))+'.json')).write_text(json.dumps(self.rows,indent=2,default=str))
TRANSMIT_RECOVERY=RecoveryProbe()
