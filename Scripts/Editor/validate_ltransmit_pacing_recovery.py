"""Expanded PIE traversal with one explicitly labelled failure teleport.

Normal steps use CharacterMovement and MotionInteractor, without Motion injection.
After sending the first reuse bridge, inject a fall; verify local restoration,
walk back to the source, and complete all remaining beats. Full and repeated
room resets must restore four added bridges and exactly five unique resources.
Start PIE in the saved expanded L_Transmit before running this file.
"""
import pathlib
import unreal

base = pathlib.Path(unreal.Paths.project_dir()) / 'Scripts/Editor/validate_ltransmit_flow_recovery.py'
exec(base.read_text().rsplit('TRANSMIT_RUN=FlowRecoveryRun()', 1)[0], globals())


class PacingRecoveryRun(FlowRecoveryRun):
    BRIDGE_STARTS = {
        'Pacing_LearnBridgeA': (-7520, 0, 25),
        'Pacing_LearnBridgeB': (-5200, 880, 25),
        'Pacing_RouteBridgeA': (5480, -6200, 625),
        'Pacing_RouteBridgeB': (7900, -3520, 625),
    }
    SOURCE_NAMES = {
        'Learn_Source', 'Route_Source', 'Pacing_LearnSourceA',
        'Pacing_LearnSourceB', 'Pacing_RouteSource',
    }

    def __init__(self):
        super().__init__()
        assert all(name in self.a for name in self.BRIDGE_STARTS)
        assert all(name in self.a for name in self.SOURCE_NAMES)
        original = self.steps
        self.steps = []
        for step in original:
            self.steps.append(step)
            if step[0] == 'reset':
                self.wait('initial expanded resources and bridges', self.full_restored, 5)
            if step[0] == 'send reuse bridge':
                self.add('inject transition fall after send', self.inject_transition_fall)
                self.wait('transition retry preserves dock without skipping bridges',
                          self.transition_restored, 5)
                for pos in [(5350, -3500), (5350, -5500), (4600, -6200)]:
                    self.walk(pos)
                self.add('capture reuse resource after local retry',
                         lambda: self.verb('Pacing_RouteSource', 'capture'))
                self.add('send reuse bridge after local retry',
                         lambda: self.verb('Pacing_RouteBridgeA', 'transfer', yaw=0))
        self.add('full reset after expanded completion',
                 lambda: self.a['Flow_Reset'].request_room_reset())
        self.wait('full reset restores all expanded resources', self.full_restored, 5)
        self.add('repeat expanded full reset',
                 lambda: self.a['Flow_Reset'].request_room_reset())
        self.wait('repeated expanded reset remains unique', self.full_restored, 5)
        self.emit('pacing_recovery_begin',
                  note='Only the labelled fall is teleported; the resumed route uses real movement and targeting.')

    def near(self, actor, xyz, tolerance=2):
        p = actor.get_actor_location()
        return ((p.x - xyz[0]) ** 2 + (p.y - xyz[1]) ** 2 +
                (p.z - xyz[2]) ** 2) ** .5 <= tolerance

    def owners(self):
        owners = {}
        for actor in unreal.GameplayStatics.get_all_actors_of_class(self.w, unreal.Actor):
            for motion in actor.get_components_by_class(unreal.MotionTransferComponent):
                if not motion.has_motion_state():
                    continue
                state = motion.try_get_motion_state()
                if isinstance(state, tuple):
                    state = state[-1]
                owners.setdefault(str(state.source_id), []).append(actor.get_actor_label())
        return owners

    def inject_transition_fall(self):
        self.p.character_movement.stop_movement_immediately()
        self.p.set_actor_location(unreal.Vector(5350, -6200, -800), False, True)
        self.emit('failure_injection', kind='transition fall',
                  note='Immediately after first reuse bridge receives the resource.')
        return True

    def transition_restored(self):
        if self.now() - self.since < 1.25:
            return False
        p = self.p.get_actor_location()
        assert abs(p.x - 5350) < 100 and abs(p.y + 650) < 100 and p.z > 0, str(p)
        ram = self.a['Weaponize_Ram']
        carrier = self.a['Route_Carrier']
        dock = self.a['Route_DockMarker'].get_actor_location()
        cp = carrier.get_actor_location()
        assert ram.armed and ram.hits == 0
        assert carrier.motion.has_motion_state() and not carrier.motion.can_provide_motion
        assert not carrier.motion.can_receive_motion
        assert ((cp.x - dock.x) ** 2 + (cp.y - dock.y) ** 2) ** .5 < 145
        assert self.a['Weaponize_Gate'].get_actor_enable_collision()
        assert self.a['Weaponize_Charger'].state_machine.get_state() == unreal.MotionChargerState.IDLE
        assert not self.a['Weaponize_Charger'].motion.has_motion_state()
        assert not self.p.get_component_by_class(unreal.MotionTransferComponent).has_motion_state()
        for name in ['Pacing_RouteBridgeA', 'Pacing_RouteBridgeB']:
            assert self.near(self.a[name], self.BRIDGE_STARTS[name]), name
            assert not self.a[name].motion.has_motion_state(), name
        assert self.near(self.a['Pacing_RouteSource'], (5100, -5850, 720))
        assert self.a['Pacing_RouteSource'].motion.has_motion_state()
        owners = self.owners()
        expected = {
            'Learn_Source': ['Learn_BridgeSlab'],
            'Route_Source': ['Route_Carrier'],
            'Pacing_LearnSourceA': ['Pacing_LearnBridgeA'],
            'Pacing_LearnSourceB': ['Pacing_LearnBridgeB'],
            'Pacing_RouteSource': ['Pacing_RouteSource'],
        }
        assert owners == expected, str(owners)
        self.emit('transition_retry_verified', player=str(p), owners=owners,
                  note='Dock and completed Learn bridges retained; both reuse bridges reset; player remains before transition.')
        return True

    def full_restored(self):
        if self.now() - self.since < 1.25:
            return False
        p = self.p.get_actor_location()
        assert abs(p.x + 10000) < 100 and abs(p.y) < 100 and p.z > 0, str(p)
        for name, xyz in self.BRIDGE_STARTS.items():
            assert self.near(self.a[name], xyz), name
            assert not self.a[name].motion.has_motion_state(), name
        assert self.near(self.a['Learn_BridgeSlab'], (680, 0, 25))
        assert not self.a['Learn_BridgeSlab'].motion.has_motion_state()
        assert self.near(self.a['Route_Carrier'], (3200, 0, 85))
        assert not self.a['Route_Carrier'].motion.has_motion_state()
        assert self.a['Route_Carrier'].motion.can_provide_motion
        assert self.a['Route_Carrier'].motion.can_receive_motion
        ram = self.a['Weaponize_Ram']
        assert not ram.armed and ram.hits == 0
        assert self.a['Weaponize_Gate'].get_actor_enable_collision()
        assert abs(self.a['Weaponize_Gate'].get_actor_location().z - 350) < 1
        assert self.a['Weaponize_Charger'].state_machine.get_state() == unreal.MotionChargerState.IDLE
        assert not self.a['Flow_Director'].is_complete()
        owners = self.owners()
        assert owners == {name: [name] for name in self.SOURCE_NAMES}, str(owners)
        self.emit('expanded_full_reset_verified', owners=owners, player=str(p),
                  note='All four new bridges at authored starts; exactly five independent resources, one owner each.')
        return True


TRANSMIT_RUN = PacingRecoveryRun()
