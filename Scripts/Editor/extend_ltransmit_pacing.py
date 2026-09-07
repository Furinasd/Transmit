"""Add gameplay practice and a resource-reuse transition to the saved candidate.

Only L_Transmit is saved. Never rebuild the existing map or change core assets.
Existing L2/Boss actors retain their configuration and transforms. The only
baseline geometry edits open the new approach and gallery departure, and move
PlayerStart to the added introduction. Run in Editor, outside PIE.
"""
import json
import pathlib
import unreal

MAP = '/Game/Transmit/Maps/L_Transmit'
TAG = 'Transmit.Pacing'
V = unreal.Vector
editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not levels.is_in_play_in_editor()
assert levels.load_level(MAP)
actors = {a.get_actor_label(): a for a in editor.get_all_level_actors()}
required = ['Learn_PlayerStart', 'Learn_BackWall', 'Route_GalleryOuterWall',
            'Learn_BridgeSlab', 'Route_Carrier', 'Weaponize_Ram', 'Weaponize_Charger']
assert all(name in actors for name in required)
evidence = pathlib.Path(unreal.Paths.project_saved_dir()) / 'LTransmitEvidence/Pacing'
evidence.mkdir(parents=True, exist_ok=True)

def snapshot():
    return {name: {'class': a.get_class().get_name(),
                   'transform': str(a.get_actor_transform())}
            for name, a in actors.items() if TAG not in [str(t) for t in a.tags]}

before = snapshot()
baseline_path = evidence / 'prewrite-actors.json'
if not baseline_path.exists():
    baseline_path.write_text(json.dumps(before, indent=2))
baseline = json.loads(baseline_path.read_text())
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
mats = {n: unreal.load_asset('/Game/Transmit/ExperimentV01/M_' + n)
        for n in ['Slate', 'Chalk', 'Amber', 'Cyan']}

def actor(cls, name, loc, yaw=0, tags=()):
    a = actors.get(name)
    if a:
        assert TAG in [str(t) for t in a.tags], 'Unowned label collision: ' + name
    else:
        a = editor.spawn_actor_from_class(cls, V(*loc))
        a.set_actor_label(name)
        actors[name] = a
    a.set_actor_location(V(*loc), False, False)
    a.set_actor_rotation(unreal.Rotator(pitch=0, yaw=yaw, roll=0), False)
    a.set_editor_property('tags', [unreal.Name(t) for t in (TAG, *tags)])
    a.set_folder_path('Pacing_Gameplay')
    return a

def box(name, loc, size, mat='Slate'):
    a = actor(unreal.StaticMeshActor, 'Pacing_' + name, loc)
    a.static_mesh_component.set_static_mesh(cube)
    a.static_mesh_component.set_material(0, mats[mat])
    a.static_mesh_component.set_collision_profile_name('BlockAll')
    a.set_actor_scale3d(V(*(v / 100 for v in size)))
    return a

def floor(name, x, y, sx, sy):
    return box(name, (x, y, -60), (sx, sy, 120))

def sign(name, loc, value, yaw=180, size=32):
    a = actor(unreal.TextRenderActor, 'Pacing_' + name, loc, yaw)
    a.text_render.set_text(value)
    a.text_render.set_world_size(size)
    a.text_render.set_text_render_color(unreal.Color(235, 240, 245, 255))
    a.text_render.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    return a

def source(name, loc, transition=False):
    a = actor(unreal.TransmitMotionEndpointActor, name, loc,
              tags=('Transmit.Pacing.Transition',) if transition else ())
    a.motion.set_editor_property('participant_id', name)
    a.motion.set_editor_property('can_provide_motion', True)
    a.motion.set_editor_property('can_receive_motion', False)
    a.motion.set_editor_property('starts_with_motion', True)
    s = unreal.MotionState()
    s.set_editor_property('direction', V(1, 0, 0))
    s.set_editor_property('magnitude', 600)
    s.set_editor_property('source_id', name)
    a.motion.set_editor_property('initial_motion', s)
    a.body.set_material(0, mats['Amber'])
    return a

def bridge(name, loc, yaw, role, transition=False):
    tags = ('Transmit.Pacing.' + role,)
    if transition:
        tags += ('Transmit.Pacing.Transition',)
    a = actor(unreal.TransmitBridgeSlab, name, loc, yaw, tags)
    a.motion.set_editor_property('participant_id', name)
    a.slab_collision.set_box_extent(V(500, 220, 15))
    a.body.set_relative_scale3d(V(10, 4.4, .3))
    a.body.set_material(0, mats['Cyan'])
    return a

# L1: long enough to read the source, then apply the same verb on a perpendicular bridge.
floor('LearnArrival', -8800, 0, 3600, 1300)
floor('LearnFirstLanding', -5300, 0, 1600, 1300)
floor('LearnTurnApproach', -5200, 700, 1300, 1400)
floor('LearnTurnLanding', -5200, 3100, 1300, 1600)
floor('LearnReturnEast', -3350, 3300, 3700, 700)
floor('LearnReturnSouth', -1500, 1650, 700, 4000)
floor('LearnOriginalConnection', -900, 0, 1200, 700)
bridge('Pacing_LearnBridgeA', (-7520, 0, 25), 0, 'LearnA')
bridge('Pacing_LearnBridgeB', (-5200, 880, 25), 90, 'LearnB')
box('LearnStopA', (-6000, 0, 40), (80, 520, 80), 'Chalk')
box('LearnStopB', (-5200, 2400, 40), (520, 80, 80), 'Chalk')
source('Pacing_LearnSourceA', (-8020, -400, 120))
source('Pacing_LearnSourceB', (-5550, 350, 120))
sign('ArrivalText', (-8650, 0, 280), '01 / LEARN\nMotion already exists. You move it.\nWASD move | Mouse aim | E take | Q give', size=35)
sign('FirstBridgeText', (-7950, -580, 220), 'Take the moving amber source.\nFace across the gap, then give to the slab.\nThe source stops; the crossing moves.', 90, 27)
sign('TurnText', (-5720, 750, 220), 'TURN THE CAMERA, CHANGE THE ROUTE\nFace north across this gap.\nCheck the arrow before Q.', 0, 27)
sign('RetryText', (-3800, 3550, 220), 'A wrong direction is recoverable.\nE takes motion back from a slab.\nBACKSPACE retries this area. R restarts all.', -90, 27)
sign('OriginalLearnText', (-1400, 700, 250), 'ONE MORE CROSSING\nApply what you learned ahead.', -90, 30)

# L2 extension: retain the original low route, chase, catch, reroute and dock.
# Depart south after docking; one resource must serve two perpendicular crossings.
floor('RelayDeparture', 5350, -3450, 700, 5900)
floor('ReuseFirstApproach', 5150, -6200, 1700, 1300)
floor('ReuseFirstLanding', 7700, -6200, 1600, 1300)
floor('ReuseNorthApproach', 7900, -4500, 700, 3000)
floor('ReuseNorthLanding', 7900, -1350, 700, 1500)
floor('ArenaReturn', 7440, -600, 1620, 700)
bridge('Pacing_RouteBridgeA', (5480, -6200, 25), 0, 'RouteA', True)
bridge('Pacing_RouteBridgeB', (7900, -3520, 25), 90, 'RouteB', True)
source('Pacing_RouteSource', (5100, -5850, 120), True)
box('ReuseStopA', (7000, -6200, 40), (80, 520, 80), 'Chalk')
box('ReuseStopB', (7900, -2000, 40), (520, 80, 80), 'Chalk')
# A solid separator closes the original short connector, not the L2 puzzle itself.
box('TransitionSeparator', (5810, -1900, 90), (100, 5700, 180), 'Chalk')
box('TransitionSeparatorUpper', (5810, -1900, 520), (100, 5700, 680), 'Chalk')
sign('DockDepartureText', (5550, -700, 250), '02 / ROUTE CONNECTED\nRam is armed. Take the south maintenance route.\nThe next two crossings share ONE motion.', 180, 27)
sign('ReuseText', (5150, -5500, 240), 'SEND / CROSS / TAKE BACK\nStand WEST of the slab before sending.\nCross, then reclaim its motion from the far bank.', 90, 28)
sign('ReclaimText', (7250, -5850, 230), 'TAKE IT WITH YOU\nE on the slab you just crossed.\nThe next crossing has no source.', -90, 29)
sign('RerouteText', (8160, -4400, 230), 'SAME MOTION, NEW DIRECTION\nFace north. Give your carried motion\nto the second slab with Q.', 180, 28)

# L3: anticipation and a readable retreat before the unchanged two-hit encounter.
sign('ThreatPreviewText', (7900, -1300, 260), '03 / WEAPONIZE\nThe Ram needs a stronger motion.\nThe charge ahead is both threat and resource.', 90, 28)
sign('DashLessonText', (7300, -850, 260), 'WATCH / INTERCEPT / DELIVER\nWarning: wait. Committed dash: E to take.\nThis direction stays locked, even when you turn.', 90, 26)
sign('ArenaRetryText', (6970, 340, 250), 'Two charges. Two impacts.\nCircle to the rear of the Ram after each capture.\nBACKSPACE preserves completed impacts.', -90, 25)

actors['Learn_PlayerStart'].set_actor_location(V(-10000, 0, 100), False, False)
actors['Learn_PlayerStart'].set_actor_rotation(unreal.Rotator(), False)
actors['Learn_BackWall'].set_actor_location(V(-430, 500, 400), False, False)
actors['Learn_BackWall'].set_actor_scale3d(V(.6, 3, 8))
actors['Route_GalleryOuterWall'].set_actor_location(V(3825, -1260, 350), False, False)
actors['Route_GalleryOuterWall'].set_actor_scale3d(V(23.5, .8, 7))

after = snapshot()
allowed = {'Learn_PlayerStart', 'Learn_BackWall', 'Route_GalleryOuterWall'}
assert before.keys() == after.keys(), 'Existing actor removed'
changed = {name for name in before if before[name] != after[name]}
assert changed <= allowed, 'Protected transform changed: ' + str(changed - allowed)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
(evidence / 'authoring.json').write_text(json.dumps({
    'map': MAP, 'retained_existing_actors': len(before),
    'modified_existing': sorted(name for name in baseline if baseline[name] != after.get(name)),
    'added_actors': sorted(name for name, a in actors.items() if TAG in [str(t) for t in a.tags]),
    'target_seconds': [300, 420], 'human_playtime_verified': False,
}, indent=2))
unreal.log('TRANSMIT_PACING_AUTHOR_SUCCESS')
