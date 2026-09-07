"""Integrate presentation-juice and author one L_Transmit wayfinding checkpoint.

Run outside PIE. Preserves every existing actor and all gameplay transforms;
text-only signs move off the walking/camera line.
Only this map is saved; no Blueprint, material asset or global setting is edited.
"""
import json
import math
import pathlib
import unreal

ROOT = pathlib.Path(unreal.Paths.project_dir())
MAP = '/Game/Transmit/Maps/L_Transmit'
TAG = 'Transmit.Wayfinding'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not levels.is_in_play_in_editor()
assert levels.load_level(MAP)
actors = {a.get_actor_label(): a for a in editor.get_all_level_actors()}
assert all(n in actors for n in ('Pacing_LearnBridgeA', 'Pacing_RouteBridgeB', 'Weaponize_Ram'))
def transform_values(a):
    p, r, s = a.get_actor_location(), a.get_actor_rotation(), a.get_actor_scale3d()
    return [round(v, 6) for v in (p.x, p.y, p.z, r.pitch, r.yaw, r.roll, s.x, s.y, s.z)]

before = {n: transform_values(a) for n, a in actors.items()}
out = ROOT / 'Saved/LTransmitEvidence/Wayfinding'
out.mkdir(parents=True, exist_ok=True)
if not (out / 'prewrite.json').exists():
    (out / 'prewrite.json').write_text(json.dumps(before, indent=2))

# Replay the other branch against the expanded map; never replace its binary.
for script in ('author_void_atmosphere.py', 'author_geometry_finish.py'):
    module = {'__name__': 'wayfinding_presentation'}
    exec((ROOT / 'Scripts/Presentation' / script).read_text(), module)
    module['apply']()

cube = unreal.load_asset('/Engine/BasicShapes/Cube')
mats = {n: unreal.load_asset('/Game/Transmit/Presentation/Materials/M_' + n)
        for n in ('Ceramic', 'Graphite')}
assert cube and all(mats.values())
created = []

def piece(name, center, size, yaw=0, collision=False, material=None):
    name = 'Wayfinding_' + name
    a = actors.get(name)
    if a:
        assert TAG in [str(t) for t in a.tags], name
    else:
        a = editor.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*center))
        a.set_actor_label(name)
        actors[name] = a
    a.set_folder_path('Wayfinding/Edges' if collision else 'Wayfinding/Route')
    a.set_editor_property('tags', [unreal.Name(TAG)])
    a.set_actor_location(unreal.Vector(*center), False, False)
    a.set_actor_rotation(unreal.Rotator(pitch=0, yaw=yaw, roll=0), False)
    a.set_actor_scale3d(unreal.Vector(*(v / 100 for v in size)))
    mesh = a.static_mesh_component
    mesh.set_static_mesh(cube)
    mesh.set_material(0, mats[material or ('Graphite' if collision else 'Ceramic')])
    mesh.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    mesh.set_editor_property('generate_overlap_events', False)
    created.append(name)

def arrow(name, x, y, yaw):
    # Neutral ground chevron: reserved Motion colours remain interaction-only.
    r = math.radians(yaw)
    for side in (-1, 1):
        cx, cy = -40, side * 32
        piece(name + str(side), (x + cx*math.cos(r)-cy*math.sin(r),
                                y + cx*math.sin(r)+cy*math.cos(r), 1.5),
              (110, 14, 2), yaw - side*38)

def route(name, points):
    # No markings across voids, on moving slabs or through blocked shortcuts.
    for i, (a, b) in enumerate(zip(points, points[1:])):
        dx, dy = b[0]-a[0], b[1]-a[1]
        count = max(1, math.ceil(math.hypot(dx, dy) / 700))
        for j in range(count):
            t = j / count
            arrow(name + '_%d_%d_' % (i, j), a[0]+t*dx, a[1]+t*dy,
                  math.degrees(math.atan2(dy, dx)))

def station(name, x, y, yaw):
    # Open frame marks the safe operating bank, not a new gameplay trigger.
    for side in (-1, 1):
        piece(name + '_Edge' + str(side), (x, y + side*130, 1.5), (240, 10, 2))
    arrow(name + '_Aim', x+60*math.cos(math.radians(yaw)),
          y+60*math.sin(math.radians(yaw)), yaw)

route('Arrival', [(-9780, 0), (-8600, 0)])
station('LearnA', -8500, 0, 0)
route('LearnTurn', [(-5700, 0), (-5200, 0), (-5200, 200)])
station('LearnB', -5200, 200, 90)
route('Return', [(-5200, 2800), (-5200, 3300), (-1500, 3300), (-1500, 0), (-550, 0)])
route('ServiceDeparture', [(5350, -700), (5350, -5500), (4600, -6000), (4600, -6200)])
station('ReuseA', 4600, -6200, 0)
# A reverse-facing mark explicitly asks the player to look back before leaving.
station('Reclaim', 7350, -6200, 180)
route('CarryNorth', [(7700, -6200), (7900, -5800), (7900, -4300)])
station('ReuseB', 7900, -4100, 90)
route('ArenaApproach', [(7900, -1700), (7900, -600), (7000, -600), (7000, 100)])

# Low outer guards bound long, otherwise featureless transit. All puzzle mouths,
# source access, catching positions and Boss movement space remain open.
for name, pos, size in (
    ('ReturnNorth', (-3350, 3650, 45), (3700, 40, 90)),
    ('ReturnEastTurn', (-1150, 3290, 45), (40, 720, 90)),
    ('ReturnSouthWest', (-1850, 1525, 45), (40, 2790, 90)),
    ('ReturnSouthEast', (-1150, 1700, 45), (40, 2500, 90)),
    ('ReturnBottomTurn', (-1500, -350, 45), (700, 40, 90)),
    ('ServiceWest', (5000, -3110, 45), (40, 4580, 90)),
    ('ServiceEast', (5700, -3110, 45), (40, 4580, 90)),
    ('ServiceSouth', (5150, -6850, 45), (1700, 40, 90)),
    ('CarryEast', (8250, -4500, 45), (40, 2600, 90)),
    ('CarryWest', (7550, -4350, 45), (40, 2200, 90)),
    ('ArenaApproachEast', (8250, -1350, 45), (40, 1500, 90)),
):
    piece(name, pos, size, collision=True)

# Concise decisions where the player makes them; detailed verbs remain in HUD.
signs = {
    'Pacing_ArrivalText': '01 / LEARN\nWASD move | Mouse aim\nE take motion | Q give motion',
    'Pacing_FirstBridgeText': 'STAY ON THE BANK\nE source / Q bridge\nWait for the bridge to stop. Then cross.',
    'Pacing_TurnText': 'TURN NORTH\nUse the white operating mark.\nCheck the arrow before Q.',
    'Pacing_DockDepartureText': 'RAM CONNECTED\nWalk SOUTH along the white marks.\nThe raised cable carries power, not a path.',
    'Pacing_ReuseText': 'WEST BANK / OPERATING MARK\nTake the source, then follow the marks WEST.\nFace EAST and Q. Stay off the moving slab.',
    'Pacing_ReclaimText': 'STOP / LOOK BACK\nE the bridge you just crossed.\nCarry this motion to the next bridge.',
    'Pacing_RerouteText': 'TURN NORTH / SAME MOTION\nStand on the mark south of the bridge.\nFace north, check the arrow, then Q.',
    'Pacing_ThreatPreviewText': '03 / WEAPONIZE\nNext: capture a committed dash.\nIts direction stays locked.',
}
for name, value in signs.items():
    actors[name].text_render.set_text(value)

# The original arrival sign sat between the shoulder camera and the operating
# bank. Its new roadside placement avoids the giant text crossing the view.
sign_placements = (
    ('Pacing_ArrivalText', (-9250, -580, 230), 90),
    ('Pacing_FirstBridgeText', (-8260, -540, 220), 135),
    ('Pacing_OriginalLearnText', (-1170, 950, 230), 180),
    ('Pacing_ReuseText', (5350, -6600, 240), 90),
    ('Pacing_ThreatPreviewText', (8230, -1550, 230), 180),
    ('Pacing_DashLessonText', (6740, -400, 230), 0),
    ('Pacing_ArenaRetryText', (6740, 350, 230), 0),
)
for name, pos, yaw in sign_placements:
    actors[name].set_actor_location(unreal.Vector(*pos), False, False)
    actors[name].set_actor_rotation(unreal.Rotator(pitch=0, yaw=yaw, roll=0), False)
for name, a in list(actors.items()):
    if not name.startswith('Pacing_') or not isinstance(a, unreal.TextRenderActor):
        continue
    # GetActorBounds can still contain the previous text/alignment until the
    # render update. Explicit centering gives the backing the same anchor now.
    a.text_render.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    center = a.get_actor_location()
    size = a.text_render.get_text_local_size()
    r = math.radians(a.get_actor_rotation().yaw)
    piece('SignBack_' + name, (center.x-6*math.cos(r), center.y-6*math.sin(r), center.z),
          (6, size.y+36, size.z+24), a.get_actor_rotation().yaw, material='Graphite')

after = {a.get_actor_label(): transform_values(a) for a in editor.get_all_level_actors()}
changed = {n: {'before': transform, 'after': after.get(n)}
           for n, transform in before.items() if after.get(n) != transform}
(out / 'transform-check.json').write_text(json.dumps(changed, indent=2))
allowed = {s[0] for s in sign_placements} | {n for n in before if n.startswith('Wayfinding_SignBack_')}
assert set(changed) <= allowed, 'Protected actor moved or removed: ' + ', '.join(changed)
assert len(created) == len(set(created))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
(out / 'authoring.json').write_text(json.dumps({
    'map': MAP, 'retained_actors': len(before), 'total_actors': len(after),
    'gameplay_transforms_unchanged': True, 'moved_text_signs': [s[0] for s in sign_placements],
    'new_wayfinding_actors': created,
    'edited_signs': list(signs), 'integrated_presentation_commit': '5d6b4d9',
}, indent=2))
unreal.log('TRANSMIT_WAYFINDING_AUTHOR_SUCCESS')
