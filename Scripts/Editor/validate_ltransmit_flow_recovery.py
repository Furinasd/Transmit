"""Real traversal with explicit failure injection; no gameplay resource injection.
Run in PIE. Teleports only induce falls / a Charger collision. Evidence is labeled.
"""
import unreal, pathlib
base=pathlib.Path(unreal.Paths.project_dir())/'Scripts/Editor/validate_ltransmit_pie.py'
exec(base.read_text().rsplit('TRANSMIT_RUN=TransmitRun()',1)[0],globals())
class FlowRecoveryRun(TransmitRun):
 def __init__(self):
  super().__init__()
  original=self.steps;self.steps=[];skip_return_walks=False
  for step in original:
   if skip_return_walks and step[0] in ('walk (7780, 2800)', 'walk (7620, 2080)'):
    continue
   if step[0]=='capture dash two':skip_return_walks=False
   self.steps.append(step)
   if step[0]=='send carrier':
    self.add('inject route fall',lambda:self.fall())
    self.wait('route local retry preserves bridge',lambda:self.route_restored(),4)
    self.add('capture route after retry',lambda:self.verb('Route_Source','capture'))
    self.walk((2750,0))
    self.add('send carrier after retry',lambda:self.verb('Route_Carrier','transfer'))
   if step[0]=='gate hit one':
    self.add('inject first-hit dash collision',lambda:self.hit_stance())
    self.wait('arena local retry preserves dock and hit',lambda:self.arena_restored(),12)
    for pos in [(7620,700),(7620,1420)]:self.walk(pos)
    skip_return_walks=True
   if step[0]=='gate hit two':
    self.add('local retry after gate opens',lambda:self.a['Flow_Director'].request_local_retry())
    self.wait('open gate retry keeps threat ended',lambda:self.open_gate_retry_safe(),6)
    for pos in [(7620,700),(7780,2080)]:self.walk(pos)
  self.add('complete full reset',lambda:self.a['Flow_Reset'].request_room_reset())
  self.wait('full reset after completion',lambda:self.full_restored(),4)
  self.add('repeat full reset',lambda:self.a['Flow_Reset'].request_room_reset())
  self.wait('repeat reset remains restored',lambda:self.full_restored(),4)
 def fall(self):
  self.p.character_movement.stop_movement_immediately()
  self.p.set_actor_location(unreal.Vector(3000,-900,-800),False,True)
  self.emit('failure_injection',kind='route fall');return True
 def route_restored(self):
  pos=self.p.get_actor_location();c=self.a['Route_Carrier'];b=self.a['Learn_BridgeSlab']
  return abs(pos.x-2440)<100 and abs(pos.y)<100 and pos.z>0 and self.a['Route_Source'].motion.has_motion_state() and not c.motion.has_motion_state() and abs(c.get_actor_location().x-3200)<1 and b.get_actor_location().x>1600 and not self.p.get_component_by_class(unreal.MotionTransferComponent).has_motion_state()
 def hit_stance(self):
  self.p.character_movement.stop_movement_immediately()
  self.p.set_actor_location(unreal.Vector(7600,1760,100),False,True)
  self.emit('failure_injection',kind='stand in committed dash lane after hit one');return True
 def arena_restored(self):
  p=self.p.get_actor_location();r=self.a['Weaponize_Ram'];g=self.a['Weaponize_Gate'];c=self.a['Route_Carrier']
  return abs(p.x-7000)<100 and abs(p.y-700)<100 and r.armed and r.hits==1 and g.get_actor_enable_collision() and c.motion.has_motion_state() and not c.motion.can_provide_motion and not self.p.get_component_by_class(unreal.MotionTransferComponent).has_motion_state()
 def open_gate_retry_safe(self):
  if self.now()-self.since<3.2:return False
  r=self.a['Weaponize_Ram'];c=self.a['Weaponize_Charger'];g=self.a['Weaponize_Gate']
  return r.hits==2 and r.armed and not g.get_actor_enable_collision() and c.state_machine.get_state()==unreal.MotionChargerState.IDLE and not c.motion.has_motion_state()
 def full_restored(self):
  p=self.p.get_actor_location();r=self.a['Weaponize_Ram'];g=self.a['Weaponize_Gate'];c=self.a['Route_Carrier']
  return p.x<100 and abs(p.y)<100 and not r.armed and r.hits==0 and abs(g.get_actor_location().z-350)<1 and g.get_actor_enable_collision() and c.motion.can_provide_motion and not c.motion.has_motion_state() and self.a['Learn_Source'].motion.has_motion_state() and self.a['Route_Source'].motion.has_motion_state() and not self.a['Flow_Director'].is_complete()
TRANSMIT_RUN=FlowRecoveryRun()
