"""A-owned formal-map composition over B2; run outside PIE in A's Editor.
Existing actor identities/resources/collision are preserved. Saves L_Transmit only.
"""
import unreal, pathlib, math
ROOT = pathlib.Path(unreal.Paths.project_dir()).resolve()
assert ROOT == pathlib.Path('/Users/ely/Documents/Unreal Projects/passely')
L = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
E = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not L.is_in_play_in_editor()
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().split('.')[0] == '/Game/Transmit/Maps/L_Transmit'
a = {x.get_actor_label(): x for x in E.get_all_level_actors()}
V = unreal.Vector
m = {k: unreal.load_asset('/Game/Transmit/Presentation/Materials/M_' + k)
     for k in ('Ceramic', 'Graphite', 'Inlay', 'Motion', 'Impact')}
assert all(m.values()) and 'Presentation_Rig' in a

# Fixed walking surfaces contrast with white moving devices and architectural walls.
floors = ('Learn_Approach', 'Learn_FarBank', 'Route_LaunchFloor', 'Route_Gallery',
          'Route_InterceptFloor', 'Route_ApproachArena', 'Route_EntryConnector',
          'Weaponize_ArenaFloor', 'Weaponize_ExitFloor', 'Route_DockFloor')
for label in floors: a[label].static_mesh_component.set_material(0, m['Inlay'])
for label, actor in a.items():
 if label.startswith(('Route_Follow_', 'Route_InterceptGuide', 'Route_EntryGuide', 'Flow_RelayStance')):
  actor.static_mesh_component.set_material(0, m['Graphite'])
 if label.startswith(('Route_SendLine', 'Route_RerouteLine')):
  actor.static_mesh_component.set_material(0, m['Motion'])
 if label.startswith(('Flow_ExitFrame', 'Flow_ExitParapet')):
  actor.static_mesh_component.set_material(0, m['Ceramic'])
a['Learn_SourcePlinth'].static_mesh_component.set_material(0, m['Graphite'])
a['Learn_PlayerStart'].set_actor_rotation(unreal.Rotator(pitch=0, yaw=-28, roll=0), False)

def point(label, pos):
 actor = a.get(label)
 if actor is None:
  actor = E.spawn_actor_from_class(unreal.TargetPoint, V(*pos)); actor.set_actor_label(label)
  actor.set_editor_property('tags', [unreal.Name('Transmit.Flow')]); actor.set_folder_path('Flow_Architecture')
  a[label] = actor
 actor.set_actor_location(V(*pos), False, False)
 return actor

def strip(label, start, end, width, material):
 actor = a.get(label)
 if actor is None:
  actor = E.spawn_actor_from_class(unreal.StaticMeshActor, V()); actor.set_actor_label(label)
  actor.set_editor_property('tags', [unreal.Name('Transmit.Flow')]); actor.set_folder_path('Flow_Architecture')
  actor.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
  a[label] = actor
 va, vb = V(*start), V(*end)
 actor.set_actor_location((va + vb) * .5, False, False)
 actor.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(va, vb), False)
 actor.set_actor_scale3d(V((vb - va).length() / 100, width / 100, .05))
 actor.static_mesh_component.set_material(0, m[material])
 actor.static_mesh_component.set_collision_profile_name('NoCollision')

# Thin visible conduit clears the Dock curb and enters through the real arena opening.
# The last floor segment also exposes the Ram's rear operating side to the player.
corners = [(5550,950,280), (5770,950,280), (5770,500,280), (7000,500,280),
           (7780,500,8), (7780,2800,8), (7150,2800,8), (7150,2530,8)]
anchors = [point('Presentation_DockCable_%02d' % i, p) for i,p in enumerate(corners)]
a['Presentation_Rig'].set_editor_property('dock_cable_anchors', anchors)
for label in ('Route_ArmLinkA', 'Route_ArmLinkB', 'Route_ArmLinkC', 'Flow_ArmLinkRam'):
 a[label].set_actor_hidden_in_game(True)
# Physical support only; cyan energizing remains exclusively driven by the real Rig.
points = [(5350,950,85)] + corners + [(7500,2530,130)]
for i in range(len(points)-1):
 start, end = points[i], points[i+1]
 strip('Flow_Conduit_%02d' % i, (start[0],start[1],start[2]-4),
       (end[0],end[1],end[2]-4), 8, 'Graphite')
point('Flow_RamOperatingMarker', (7150,2750,100))
for y in (2670,2830):
 strip('Flow_RamStance_%d' % y, (7050,y,7), (7250,y,7), 10, 'Graphite')
strip('Flow_RamStanceBack', (7050,2670,7), (7050,2830,7), 10, 'Graphite')
assert unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Transmit/Maps/L_Transmit')
unreal.log('A_FORMAL_PRESENTATION_COMPOSITION_SAVED')
