"""Continuous real-player traversal with brief observation stops and runtime frames.
No actor teleport, Motion injection, fake impacts, or alternate cinematic camera.
"""
import unreal, pathlib
base = pathlib.Path(unreal.Paths.project_dir()) / 'Scripts/Editor/validate_ltransmit_pie.py'
exec(base.read_text().rsplit('TRANSMIT_RUN=TransmitRun()', 1)[0], globals())

class CandidateCaptureRun(TransmitRun):
 def __init__(self):
  self.frames = []
  self.warning_taken = False
  self.output = pathlib.Path(unreal.Paths.project_saved_dir()) / 'LTransmitEvidence' / ('candidate-' + str(int(time.time())))
  self.output.mkdir(parents=True, exist_ok=True)
  super().__init__()
  original = self.steps; self.steps = []
  for step in original:
   self.steps.append(step)
   if step[0] == 'reset':
    self.wait('read opening', lambda: self.now()-self.since >= .8, 2)
   if step[0] == 'walk (4770, -920)':
    self.add('look toward relay', self.look_relay)
    self.wait('read relay route', lambda: self.now()-self.since >= 1.0, 3)
   if step[0] in ('gate hit one', 'gate hit two'):
    self.wait('observe '+step[0], lambda: self.now()-self.since >= 1.6, 3)
   if step[0] == 'director complete':
    self.wait('read completion', lambda: self.now()-self.since >= .5, 2)

 def look_relay(self):
  eye, _ = self.p.get_actor_eyes_view_point()
  self.pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(eye, self.a['Route_Carrier'].get_actor_location()))
  self.frames.append((self.now()+.4, '03-chase'))
  return True

 def emit(self, event, **kwargs):
  super().emit(event, **kwargs)
  if event != 'step': return
  name = kwargs.get('name')
  views = {'reset':'00-opening', 'capture learn':'01-loaded', 'bridge spans gap':'02-bridge',
           'recapture carrier':'04-recapture', 'ram armed':'05-dock', 'capture dash one':'07-high-loaded',
           'power ram one':'08-ram-stroke', 'gate hit one':'09-first-impact', 'gate hit two':'10-final-impact',
           'director complete':'11-complete'}
  if name in views:
   self.frames.append((self.now()+(.3 if name=='reset' else .05), views[name]))
  if name in ('gate hit one', 'gate hit two'):
   for seconds, suffix in ((.45, '-045'), (1.3, '-130')):
    self.frames.append((self.now()+seconds, views[name]+suffix))

 def capture_dash(self):
  fsm = self.a['Weaponize_Charger'].state_machine
  if not self.warning_taken and fsm.get_state() == unreal.MotionChargerState.TELEGRAPH and fsm.get_elapsed_in_state() >= 1:
   self.warning_taken = True
   self.frames.append((self.now(), '06-telegraph'))
  return super().capture_dash()

 def tick(self, dt):
  for due, name in list(self.frames):
   if self.now() >= due:
    unreal.SystemLibrary.execute_console_command(self.w, 'Shot filename="'+str(self.output/(name+'.png'))+'"')
    self.frames.remove((due, name))
  super().tick(dt)

TRANSMIT_RUN = CandidateCaptureRun()
