"""Narrow, idempotent production delta over the bf1d0aa formal map.
Run in A's Editor after reviewing baseline. Only formal L_Transmit is saved.
Existing gameplay identities and unrelated actors are retained.
"""
import unreal, pathlib, json, math
assert str(pathlib.Path(unreal.Paths.project_dir()).resolve()) == '/Users/ely/Documents/Unreal Projects/passely'
L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
E=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not L.is_in_play_in_editor()
MAP='/Game/Transmit/Maps/L_Transmit'
assert L.load_level(MAP)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors={a.get_actor_label():a for a in E.get_all_level_actors()}
required=['Learn_Source','Learn_BridgeSlab','Route_Source','Route_Carrier','Route_DockMarker','Weaponize_Ram','Weaponize_Charger','Flow_Director','Flow_Reset']
assert all(k in actors for k in required), 'Missing baseline actors'
evidence=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence';evidence.mkdir(parents=True,exist_ok=True)
(evidence/'flow-prewrite-actors.json').write_text(json.dumps({k:{'class':a.get_class().get_name(),'location':str(a.get_actor_location()),'rotation':str(a.get_actor_rotation())} for k,a in actors.items()},indent=2))
V=unreal.Vector
mats={n:unreal.load_asset('/Game/Transmit/ExperimentV01/M_'+n) for n in ['Chalk','Slate','Cyan','Amber','Red']}
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
def box(name,loc,size,mat='Chalk',yaw=0,collision=True):
 a=actors.get(name)
 if a is None:
  a=E.spawn_actor_from_class(unreal.StaticMeshActor,V(*loc));a.set_actor_label(name)
  a.set_editor_property('tags',[unreal.Name('Transmit.Flow')]);a.set_folder_path('Flow_Architecture');actors[name]=a
 assert isinstance(a,unreal.StaticMeshActor)
 a.set_actor_location(V(*loc),False,False);a.set_actor_rotation(unreal.Rotator(0,yaw,0),False)
 a.set_actor_scale3d(V(*(x/100 for x in size)))
 a.static_mesh_component.set_static_mesh(cube);a.static_mesh_component.set_material(0,mats[mat])
 a.static_mesh_component.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
 return a

def marker(name,loc,yaw=0):
 a=actors.get(name)
 if a is None:
  a=E.spawn_actor_from_class(unreal.TargetPoint,V(*loc));a.set_actor_label(name)
  a.set_editor_property('tags',[unreal.Name('Transmit.Flow')]);actors[name]=a
 a.set_actor_location(V(*loc),False,False);a.set_actor_rotation(unreal.Rotator(0,yaw,0),False);return a

def line(name,a,b,mat='Cyan'):
 dx=b[0]-a[0];dy=b[1]-a[1]
 return box(name,tuple((a[i]+b[i])/2 for i in range(3)),(math.hypot(dx,dy),22,10),mat,math.degrees(math.atan2(dy,dx)),False)

# Keep the low route and the outer gallery; replace the obsolete diagonal catch leg.
box('Route_CatchStop',(5440,0,120),(80,500,240),'Chalk')
box('Route_DiagonalDeck',(5350,500,-60),(560,1200,120),'Chalk')
box('Route_DiagonalCurb_-245',(5055,650,120),(45,900,240),'Slate')
box('Route_DiagonalCurb_245',(5645,550,120),(45,1100,240),'Slate')
box('Route_DockFloor',(5350,1000,-60),(650,650,120),'Slate')
box('Route_DockStop',(5350,1100,120),(520,80,240),'Chalk')
box('Route_DockSocket',(5350,950,85),(240,240,12),'Cyan',collision=False)
dock=marker('Route_DockMarker',(5350,950,85))
line('Route_RerouteLine',(5350,0,6),(5350,1010,6),'Amber')
# The live link crosses along the distant edge and visibly feeds the existing Ram rail.
line('Route_ArmLinkA',(5350,950,8),(5350,1120,8))
line('Route_ArmLinkB',(5350,1120,8),(7000,1120,8))
line('Route_ArmLinkC',(7000,1120,8),(7000,2530,8))
line('Flow_ArmLinkRam',(7000,2530,8),(7460,2530,8))
# Make the player and motion paths visible to one another through the low passage.
for i,x in enumerate([3550,3970,4390,4810]):
 box('Route_LowRoof_'+str(i),(x,0,230),(120,460,160),'Slate')
box('Route_GalleryInnerWall',(4180,-525,65),(1700,90,130),'Slate')
# Stand south of the relay and face +Y: selection and output direction now agree.
line('Route_InterceptGuide',(5100,-950,6),(5350,-320,6))
line('Flow_RelayStance',(5250,-330,7),(5450,-330,7))
route_entry=marker('Flow_RouteEntry',(2440,0,100),0)
catch=marker('Flow_CatchMarker',(5350,0,85),90)
entry=marker('Weaponize_EntryMarker',(7000,700,100),90)
d=actors['Flow_Director'];d.set_editor_property('bridge',actors['Learn_BridgeSlab'])
d.set_editor_property('route_source',actors['Route_Source']);d.set_editor_property('route_entry_marker',route_entry);d.set_editor_property('catch_marker',catch)
d.set_editor_property('arena_entry_marker',entry)
r=actors['Weaponize_Ram'];r.set_editor_property('dock_marker',dock);r.set_editor_property('dock_radius',145)
# Longer anticipation makes the first committed dash legible; capture timing remains FSM-owned.
fsm=actors['Weaponize_Charger'].state_machine
fsm.set_editor_property('idle_duration_seconds',1.8)
fsm.set_editor_property('telegraph_duration_seconds',1.7)
fsm.set_editor_property('recovery_duration_seconds',2.8)
# The restored connection opens to sky, with a safe low parapet and a framed horizon.
box('Weaponize_ExitFloor',(8750,2530,-60),(1100,1000,120),'Slate')
box('Weaponize_EndWall',(9320,2530,65),(70,1060,130),'Chalk')
for y in [2000,3060]:
 box('Flow_ExitParapet_'+str(y),(8850,y,65),(1000,70,130),'Chalk')
box('Flow_ExitFrameNorth',(9140,3090,430),(130,130,860),'Chalk')
box('Flow_ExitFrameSouth',(9140,1970,430),(130,130,860),'Chalk')
box('Flow_ExitFrameLintel',(9140,2530,850),(130,1250,120),'Chalk')
actors['Weaponize_EndTitle'].set_actor_location(V(9190,2530,550),False,False)
assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
unreal.log('TRANSMIT_FLOW_AUTHOR_SUCCESS actors='+str(len(E.get_all_level_actors())))
