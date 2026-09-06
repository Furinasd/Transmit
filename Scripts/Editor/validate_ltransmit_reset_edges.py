"""PIE Reset edge runs using real movement/targeting, without injected Motion.
Set TRANSMIT_RESET_EDGE to 'ram' (default) or 'external' before executing.
The reset itself calls the gameplay entry point; native key binding is a separate smoke.
"""
import unreal, pathlib
base = pathlib.Path(unreal.Paths.project_dir()) / 'Scripts/Editor/validate_ltransmit_pie.py'
exec(base.read_text().rsplit('TRANSMIT_RUN=TransmitRun()', 1)[0], globals())

class ResetEdgeRun(TransmitRun):
 def __init__(self, mode):
  super().__init__()
  self.mode = mode
  if mode == 'ram':
   end = next(i for i, s in enumerate(self.steps) if s[0] == 'power ram one')
   self.steps = self.steps[:end]
   self.add('transfer then full reset during Ram stroke', self.reset_during_stroke)
  else:
   assert mode == 'external'
   end = next(i for i, s in enumerate(self.steps) if s[0] == 'capture route')
   self.steps = self.steps[:end]
   self.add('recapture Learn from bridge', lambda: self.verb('Learn_BridgeSlab', 'capture'))
   self.walk((2750, 0))
   self.add('store Learn in route carrier', lambda: self.verb('Route_Carrier', 'transfer'))
   self.add('take route resource', lambda: self.verb('Route_Source', 'capture'))
   self.walk((2440, 0))
   self.add('store route outside retry group then retry', self.external_retry)
  self.wait('reset restores resources with no late impact', self.restored, 5)
  self.emit('reset_edge', mode=mode)

 def reset_during_stroke(self):
  result = self.verb('Weaponize_Ram', 'transfer')
  if result:
   assert self.a['Weaponize_Ram'].hits == 0
   self.emit('reset_injection', note='full Reset in same frame as real Ram transfer, before impact')
   return self.a['Flow_Reset'].request_room_reset()
  return result

 def external_retry(self):
  result = self.verb('Learn_BridgeSlab', 'transfer')
  if result:
   self.emit('reset_injection', note='local retry while Bridge owns route resource; must fall back to full Reset')
   return self.a['Flow_Director'].request_local_retry()
  return result

 def restored(self):
  if self.now() - self.since < 1.5: return False
  ram = self.a['Weaponize_Ram']; gate = self.a['Weaponize_Gate']
  player = self.p.get_actor_location()
  assert ram.hits == 0 and not ram.armed and gate.get_actor_enable_collision()
  assert abs(gate.get_actor_location().z - 350) < 1
  assert player.x < 100 and abs(player.y) < 100
  assert not self.a['Flow_Director'].is_complete()
  owners = {}
  for actor in unreal.GameplayStatics.get_all_actors_of_class(self.w, unreal.Actor):
   for motion in actor.get_components_by_class(unreal.MotionTransferComponent):
    if not motion.has_motion_state(): continue
    state = motion.try_get_motion_state()
    if isinstance(state, tuple): state = state[-1]
    owners.setdefault(str(state.source_id), []).append(actor.get_actor_label())
  self.emit('owners_after_reset', owners=owners)
  expected = {'Learn_Source', 'Route_Source'}
  if 'Pacing_LearnSourceA' in self.a:
   expected |= {'Pacing_LearnSourceA', 'Pacing_LearnSourceB', 'Pacing_RouteSource'}
   assert abs(player.x + 10000) < 100
  assert owners == {name: [name] for name in expected}
  return True

TRANSMIT_RUN = ResetEdgeRun(globals().get('TRANSMIT_RESET_EDGE', 'ram'))
