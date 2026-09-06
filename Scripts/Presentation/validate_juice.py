"""Run in a fresh Editor using -ExecutePythonScript; never saves or authors assets.
Reuses the production traversal and records actual camera FOV through committed events.
"""
import unreal, pathlib, json, time, traceback
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
ROOT = pathlib.Path(unreal.Paths.project_dir())
OUT = ROOT / 'Saved/JuiceEvidence'
OUT.mkdir(parents=True, exist_ok=True)
exec((ROOT/'Scripts/Editor/capture_ltransmit_candidate.py').read_text().rsplit('TRANSMIT_RUN = CandidateCaptureRun()', 1)[0], globals())

class JuiceRun(CandidateCaptureRun):
 def __init__(self):
  self.camera_samples=[]
  super().__init__()
  self.camera=unreal.GameplayStatics.get_player_camera_manager(self.w,0)
  self.base_fov=self.p.get_component_by_class(unreal.CameraComponent).field_of_view
  self.pending_cancel=None
  self.cancellations=[]
  original=self.steps; self.steps=[]
  for step in original:
   if step[0]=='capture route':
    self.walk((2550,0))
    self.add('capture for local retry',lambda:self.capture_cancel('Route_Source',True))
    self.wait('local retry settled',lambda:self.pending_cancel is None and self.now()-self.since>.2,2)
    self.add('verify local retry FOV',self.verify_clear)
   self.steps.append(step)
  self.add('reset after completion',lambda:self.a['Flow_Reset'].request_room_reset())
  for repeat in range(3):
   self.add('capture for active reset '+str(repeat),lambda:self.capture_cancel('Learn_Source',False))
   self.wait('reset settled '+str(repeat),lambda:self.pending_cancel is None and self.now()-self.since>.2,2)
   self.add('verify reset FOV '+str(repeat),self.verify_clear)
 def capture_cancel(self,label,local):
  ok=self.verb(label,'capture')
  if ok:self.pending_cancel=(self.now(),local)
  return ok
 def verify_clear(self):
  rig=self.a['Presentation_Rig']
  assert abs(self.camera.get_fov_angle()-self.base_fov)<.01
  assert rig.active_pulse_count==0 and rig.active_sound_count==0
  return True
 def tick(self,dt):
  if hasattr(self,'camera') and not self.done:
   self.camera_samples.append(dict(t=self.now(),fov=self.camera.get_fov_angle(),step=self.steps[min(self.index,len(self.steps)-1)][0]))
  if self.pending_cancel and self.now()-self.pending_cancel[0]>.04:
   try:
    before=self.camera.get_fov_angle()
    assert abs(before-self.base_fov)>.05, 'Cancellation must occur during visible impulse'
    local=self.pending_cancel[1]
    assert (self.a['Flow_Director'].request_local_retry() if local else self.a['Flow_Reset'].request_room_reset())
    if local:
     assert self.a['Learn_BridgeSlab'].get_actor_location().x>1500, 'Local retry must preserve completed bridge'
     assert self.p.get_actor_location().x>=2440, 'Must exercise Route checkpoint, not full reset'
    self.cancellations.append(dict(local=local,before_fov=before))
    self.pending_cancel=None
   except Exception:
    self.finish(False,traceback.format_exc());return
  super().tick(dt)
 def finish(self,ok,reason):
  fovs=[r['fov'] for r in self.camera_samples] or [self.base_fov]
  observed=min(fovs)<self.base_fov-.3 and max(fovs)>self.base_fov+.3
  ok=ok and observed and len(self.cancellations)==4
  if not ok and reason=='exit reached':reason='Camera observation or cancellation coverage failed'
  result=dict(ok=ok,reason=reason,base_fov=self.base_fov,min_fov=min(fovs),max_fov=max(fovs),cancellations=self.cancellations,samples=self.camera_samples)
  super().finish(ok,reason)
  (OUT/'runtime.json').write_text(json.dumps(result,indent=2))
  unreal.log('JUICE_RESULT '+json.dumps({k:v for k,v in result.items() if k!='samples'}))
  unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
  unreal.EditorPythonScripting.set_keep_python_script_alive(False)

started=time.monotonic()
phase=0
runner=None
def bootstrap(dt):
 global phase,runner
 try:
  editor=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
  if phase==0 and time.monotonic()-started>3:
   assert 'L_Transmit' in unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name()
   editor.editor_request_begin_play();phase=1
  elif phase==1:
   world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
   if world and unreal.GameplayStatics.get_player_pawn(world,0):
    runner=JuiceRun();phase=2
    unreal.unregister_slate_post_tick_callback(bootstrap_handle)
  if time.monotonic()-started>90:raise RuntimeError('PIE startup timeout')
 except Exception:
  error=traceback.format_exc()
  (OUT/'bootstrap-error.txt').write_text(error)
  (OUT/'runtime.json').write_text(json.dumps(dict(ok=False,reason=error)))
  unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
  unreal.EditorPythonScripting.set_keep_python_script_alive(False)
  unreal.unregister_slate_post_tick_callback(bootstrap_handle)
bootstrap_handle=unreal.register_slate_post_tick_callback(bootstrap)
