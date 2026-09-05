"""Live-frame integration run. Uses actual CharacterMovement and MotionInteractor.
No ownership injection, direct component transfers, or actor teleport in the clean run.
Start PIE in L_Transmit first, then exec this file. Results are local Saved evidence.
"""
import unreal,math,json,time,pathlib
if globals().get('TRANSMIT_RUN') and not TRANSMIT_RUN.done:TRANSMIT_RUN.finish(False,'replaced')
class TransmitRun:
 def __init__(self):
  self.w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
  assert self.w and 'L_Transmit' in self.w.get_name()
  self.p=unreal.GameplayStatics.get_player_pawn(self.w,0);self.pc=unreal.GameplayStatics.get_player_controller(self.w,0)
  self.i=self.p.get_component_by_class(unreal.MotionInteractorComponent)
  self.a={a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(self.w,unreal.Actor)}
  self.rows=[];self.done=False;self.steps=[];self.index=0;self.since=self.now();self.wall=time.monotonic();self.start=self.now()
  self.add('reset',lambda:self.a['Flow_Reset'].request_room_reset())
  self.add('capture learn',lambda:self.verb('Learn_Source','capture'))
  self.add('send bridge',lambda:self.verb('Learn_BridgeSlab','transfer'))
  self.wait('bridge spans gap',lambda:self.a['Learn_BridgeSlab'].is_blocked_by_collision() and self.a['Learn_BridgeSlab'].get_actor_location().x>1600,12)
  for pos in [(600,0),(1150,0),(1950,0),(2440,0)]:self.walk(pos)
  self.add('capture route',lambda:self.verb('Route_Source','capture'))
  self.walk((2750,0))
  self.add('send carrier',lambda:self.verb('Route_Carrier','transfer'))
  for pos in [(3100,0),(3100,-920),(3750,-920),(4770,-920),(5150,-700)]:self.walk(pos)
  self.wait('carrier catch',lambda:self.a['Route_Carrier'].is_blocked_by_collision(),12)
  self.walk((5100,-100))
  self.add('recapture carrier',lambda:self.verb('Route_Carrier','capture'))
  self.walk((4900,-290))
  self.add('reroute carrier',lambda:self.verb('Route_Carrier','transfer',yaw=30))
  self.wait('ram armed',lambda:self.a['Weaponize_Ram'].get_editor_property('armed'),15)
  for pos in [(5200,-750),(5850,-650),(7000,-500),(7000,700),(7620,700),(7620,1420)]:self.walk(pos)
  self.wait('capture dash one',lambda:self.capture_dash(),18)
  self.walk((7680,2450))
  self.add('power ram one',lambda:self.verb('Weaponize_Ram','transfer'))
  self.wait('gate hit one',lambda:self.a['Weaponize_Ram'].hits==1,4)
  self.walk((7620,2080))
  self.wait('capture dash two',lambda:self.capture_dash(),18)
  self.walk((7680,2450))
  self.add('power ram two',lambda:self.verb('Weaponize_Ram','transfer'))
  self.wait('gate hit two',lambda:self.a['Weaponize_Ram'].hits==2,4)
  for pos in [(7930,2530),(8460,2530),(8880,2530)]:self.walk(pos)
  self.handle=unreal.register_slate_post_tick_callback(self.tick)
  self.emit('begin',note='continuous scripted movement, real targeting, no injected Motion; human acceptance separate')
 def now(self):return unreal.GameplayStatics.get_time_seconds(self.w)
 def emit(self,event,**kwargs):
  row={'event':event,'game_seconds':round(self.now()-self.start,3),**kwargs};self.rows.append(row);unreal.log('TRANSMIT_RUN '+json.dumps(row,default=str))
 def add(self,n,f):self.steps.append((n,f,3,False))
 def wait(self,n,f,t):self.steps.append((n,f,t,True))
 def walk(self,pos):
  def action():
   loc=self.p.get_actor_location();d=unreal.Vector(pos[0]-loc.x,pos[1]-loc.y,0);length=math.hypot(d.x,d.y)
   if length<65:self.p.character_movement.stop_movement_immediately();return True
   self.p.add_movement_input(d/length,1,True)
   self.pc.set_control_rotation(unreal.Rotator(pitch=-7,yaw=math.degrees(math.atan2(d.y,d.x)),roll=0))
   return False
  self.wait('walk '+str(pos),action,20)
 def verb(self,label,verb,yaw=None):
  a=self.a[label];eye,_=self.p.get_actor_eyes_view_point();r=unreal.MathLibrary.find_look_at_rotation(eye,a.get_actor_location())
  if yaw is not None:r.yaw=yaw
  self.pc.set_control_rotation(r);self.i.clear_target();self.i.refresh_target();preview=self.i.get_current_preview()
  if preview.target!=a or not preview.eligible:
   self.emit('target failure',expected=label,selected=preview.target.get_actor_label() if preview.target else None,preview=str(preview),player=str(self.p.get_actor_location()));return False
  result=self.i.request_capture() if verb=='capture' else self.i.request_transfer()
  self.emit(verb,target=label,ok=result.succeeded,result=str(result),preview=str(preview));return result.succeeded
 def capture_dash(self):
  ch=self.a['Weaponize_Charger']
  if not ch.state_machine.is_capture_window_open():return False
  return self.verb('Weaponize_Charger','capture')
 def tick(self,dt):
  if self.done:return
  try:
   if self.index>=len(self.steps):self.finish(True,'exit reached');return
   n,f,timeout,retry=self.steps[self.index];age=self.now()-self.since
   if age<.25:return
   ok=f()
   if ok:
    self.emit('step',name=n,player=str(self.p.get_actor_location()));self.index+=1;self.since=self.now();self.wall=time.monotonic()
   elif not retry or age>timeout or time.monotonic()-self.wall>90:self.finish(False,'step failed: '+n)
  except Exception as e:
   import traceback
   self.finish(False,traceback.format_exc())
 def finish(self,ok,reason):
  self.done=True;unreal.unregister_slate_post_tick_callback(self.handle)
  self.p.character_movement.stop_movement_immediately()
  self.emit('complete',ok=ok,reason=reason,player=str(self.p.get_actor_location()))
  dest=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence';dest.mkdir(parents=True,exist_ok=True)
  (dest/('run-'+str(int(time.time()))+'.json')).write_text(json.dumps(self.rows,indent=2,default=str))
TRANSMIT_RUN=TransmitRun()
