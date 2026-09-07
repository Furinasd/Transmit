"""Live gameplay-camera device presentation probes in the B preview map.
Staged evidence, NOT a continuous traversal: positions are arranged for device views.
Every Capture/Transfer uses MotionInteractor; no granted state or fake impact events.
Run after PIE starts. Writes screenshots + JSON under this project's Saved only.
"""
import unreal, math, time, json
from pathlib import Path


class PresentationProbe:
    def __init__(self):
        self.world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        assert self.world and 'PresentationPreview' in self.world.get_name(), 'B preview PIE only'
        self.pawn = unreal.GameplayStatics.get_player_pawn(self.world, 0)
        self.pc = unreal.GameplayStatics.get_player_controller(self.world, 0)
        self.interactor = self.pawn.get_component_by_class(unreal.MotionInteractorComponent)
        self.motion = self.pawn.get_component_by_class(unreal.MotionTransferComponent)
        self.actors = {a.get_actor_label():a for a in unreal.GameplayStatics.get_all_actors_of_class(self.world, unreal.Actor)}
        self.rig = self.actors['Presentation_Rig']
        self.ram = self.actors['Weaponize_Ram']
        self.charger = self.actors['Weaponize_Charger']
        self.charger_home = self.charger.get_actor_location()
        self.start = time.monotonic()
        self.rows, self.frames, self.steps = [], [], []
        self.index = 0
        self.since = self.now()
        self.done = False
        self.aim_target = None
        self.output = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / 'PresentationEvidence'
        self.output.mkdir(parents=True, exist_ok=True)
        self.queue('reset', self.reset)
        self.queue('learn view', lambda:self.place('Learn_Source'))
        self.queue('before', lambda:self.shot('01-before'), delay=1.2)
        self.queue('capture', lambda:self.verb('capture'), delay=.5)
        self.queue('capture packet', lambda:self.shot('02-capture'), delay=.07)
        self.queue('loaded', lambda:self.shot('03-loaded'), delay=.6)
        self.queue('bridge view', lambda:self.place('Learn_BridgeSlab'))
        self.queue('transfer', lambda:self.verb('transfer'), delay=.8)
        self.queue('transfer packet', lambda:self.shot('04-transfer'), delay=.08)
        self.queue('rejected empty transfer', self.reject, delay=.8)
        self.queue('reset pulse cleanup', self.reset)
        self.queue('clean counters', self.check_clear, delay=.1)
        self.queue('route source view', lambda:self.place('Route_Source'))
        self.queue('capture route', lambda:self.verb('capture'), delay=.8)
        self.queue('carrier view', lambda:self.place('Route_Carrier'))
        self.queue('send carrier', lambda:self.verb('transfer'), delay=.8)
        self.queue('stage dock arrival', self.stage_dock, delay=.1)
        self.queue('dock cascade', lambda:self.shot('05-dock'), delay=.5)
        self.queue('boss view', self.boss_view, delay=1.0)
        self.queue('telegraph', self.telegraph, retry=True, timeout=12)
        self.queue('intercept', self.intercept, retry=True, timeout=12)
        self.queue('intercept picture', lambda:self.shot('07-intercept'), delay=.1)
        self.queue('ram view', lambda:self.place('Weaponize_Ram', sideways=-350))
        self.queue('power ram', lambda:self.verb('transfer'), delay=.8)
        self.queue('impact one', lambda:self.impact(1), retry=True, timeout=3, delay=.02)
        self.queue('fracture hold', lambda:self.shot('09-fracture'), delay=.5)
        self.queue('boss second view', self.boss_view)
        self.queue('intercept second', self.intercept, retry=True, timeout=14)
        self.queue('ram second view', lambda:self.place('Weaponize_Ram', sideways=-350))
        self.queue('power ram second', lambda:self.verb('transfer'), delay=.8)
        self.queue('impact two', lambda:self.impact(2), retry=True, timeout=3, delay=.02)
        self.queue('completion stage', self.complete, delay=1.3)
        self.queue('completion picture', lambda:self.shot('11-complete'), delay=1.0)
        for repeat in range(3):
            self.queue('full reset '+str(repeat), self.reset, delay=.7)
            self.queue('reset verified '+str(repeat), self.check_clear, delay=.2)
        self.handle = unreal.register_slate_post_tick_callback(self.tick)
        self.emit('begin', evidence='staged device views using actual player camera; not full traversal')

    def now(self):
        return unreal.GameplayStatics.get_time_seconds(self.world)

    def emit(self, event, **data):
        row = dict(event=event, seconds=self.now(), **data)
        self.rows.append(row)
        unreal.log('PRESENTATION_PROBE '+json.dumps(row, default=str))

    def queue(self, name, action, delay=.25, retry=False, timeout=4):
        self.steps.append((name, action, delay, retry, timeout))

    def anchor(self, actor):
        component = actor.get_component_by_class(unreal.StaticMeshComponent)
        return component.get_world_location() if component else actor.get_actor_location()

    def place(self, label, sideways=-170):
        actor = self.actors[label]
        pos = self.anchor(actor)
        # Positions relative to live anchors, never a historical map coordinate.
        self.pawn.character_movement.stop_movement_immediately()
        stance=self.actors['Learn_PlayerStart'].get_actor_location() if label.startswith('Learn_') else pos+unreal.Vector(-480, sideways, 20)
        if label == 'Weaponize_Ram':
            stance=pos+unreal.Vector(180,-80,0)
        self.pawn.set_actor_location(stance, False, True)
        self.aim_target = actor
        self.aim()
        return True

    def aim(self):
        if not self.aim_target:
            return
        camera = unreal.GameplayStatics.get_player_camera_manager(self.world, 0)
        target = self.anchor(self.aim_target)
        self.pc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_camera_location(), target))

    def shot(self, name):
        filename = str(self.output / (name+'.png'))
        unreal.SystemLibrary.execute_console_command(self.world, 'HighResShot 1280x720 filename="'+filename+'"')
        self.emit('screenshot requested', filename=filename, strokes=self.rig.visible_stroke_count, pulses=self.rig.active_pulse_count)
        return True

    def reset(self):
        self.aim_target = None
        assert self.actors['Flow_Reset'].request_room_reset()
        return True

    def verb(self, verb):
        self.interactor.clear_target()
        self.interactor.refresh_target()
        preview = self.interactor.get_current_preview()
        assert preview.target == self.aim_target and preview.eligible, 'Target preview mismatch: '+str(preview)
        result = self.interactor.request_capture() if verb=='capture' else self.interactor.request_transfer()
        self.emit(verb, succeeded=result.succeeded, target=self.aim_target.get_actor_label(), result=str(result))
        assert result.succeeded
        return True

    def reject(self):
        assert not self.motion.has_motion_state()
        before = self.rig.active_pulse_count
        result = self.interactor.request_transfer()
        assert not result.succeeded and not self.motion.has_motion_state()
        assert self.rig.active_pulse_count == before
        self.emit('rejection preserved', pulses=before)
        return True

    def check_clear(self):
        assert self.rig.active_pulse_count == 0 and self.rig.active_sound_count == 0 and not self.motion.has_motion_state()
        assert self.ram.hits == 0 and not self.ram.get_editor_property('armed')
        self.emit('reset clear', pulses=self.rig.active_pulse_count, hits=self.ram.hits)
        return True

    def stage_dock(self):
        carrier = self.actors['Route_Carrier']
        assert carrier.motion.has_motion_state()
        carrier.set_actor_location(self.actors['Route_DockMarker'].get_actor_location(), False, True)
        self.place('Weaponize_Ram', sideways=-350)
        self.emit('fixture arrangement', detail='live carrier placed at dock with legitimately transferred Motion')
        return True

    def boss_view(self):
        self.actors['Flow_Director'].request_local_retry()
        axis=self.charger.get_dash_direction()
        side=unreal.MathLibrary.cross_vector_vector(unreal.Vector(0,0,1),axis)
        self.pawn.set_actor_location(self.charger_home+axis*1300-side*340,False,True)
        self.pawn.character_movement.stop_movement_immediately()
        self.aim_target=self.charger
        self.charger.set_encounter_active(True)
        self.aim()
        return True

    def telegraph(self):
        if self.charger.state_machine.get_state() != unreal.MotionChargerState.TELEGRAPH:
            return False
        return self.shot('06-telegraph')

    def intercept(self):
        if not self.charger.state_machine.is_capture_window_open():
            return False
        self.interactor.refresh_target()
        preview=self.interactor.get_current_preview()
        if preview.target!=self.charger or not preview.eligible:
            return False
        return self.verb('capture')

    def impact(self, number):
        if self.ram.hits != number:
            return False
        return self.shot('08-impact-one' if number==1 else '10-impact-two')

    def complete(self):
        self.aim_target = None
        self.pawn.set_actor_location(self.actors['Weaponize_ExitMarker'].get_actor_location(), False, True)
        return True

    def tick(self, dt):
        if self.done:
            return
        try:
            self.frames.append(float(dt))
            self.aim()
            if self.index == len(self.steps):
                self.finish(True, 'staged device sequence and repeated full Reset passed')
                return
            name, action, delay, retry, timeout = self.steps[self.index]
            age = self.now()-self.since
            if age < delay:
                return
            if action():
                self.emit('step', name=name)
                self.index += 1
                self.since = self.now()
            elif not retry or age > timeout:
                self.finish(False, 'step failed: '+name)
            if time.monotonic()-self.start > 180:
                self.finish(False, 'wall-clock bound')
        except Exception:
            import traceback
            self.finish(False, traceback.format_exc())

    def finish(self, ok, reason):
        self.done = True
        unreal.unregister_slate_post_tick_callback(self.handle)
        self.pawn.character_movement.stop_movement_immediately()
        self.emit('finished', ok=ok, reason=reason)
        (self.output/('probe-'+str(int(time.time()))+'.json')).write_text(json.dumps(dict(rows=self.rows, slate_frame_seconds=self.frames), indent=2, default=str))


def run():
    return PresentationProbe()
