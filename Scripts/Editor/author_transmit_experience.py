"""One L_Transmit transaction. Preserve floor collision union; disjoint render faces.
No renderer/project settings, input assets or other maps are modified.
"""
import unreal,pathlib,json,math
import sys
sys.path.insert(0, str(pathlib.Path(unreal.Paths.project_dir())/'Scripts/Editor'))
from transmit_editor_safety import load_level_checked
V=unreal.Vector;E=unreal.get_editor_subsystem(unreal.EditorActorSubsystem);L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not L.is_in_play_in_editor();assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages();assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
load_level_checked('/Game/Transmit/Maps/L_Transmit')
a={x.get_actor_label():x for x in E.get_all_level_actors()};out=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence/Experience';out.mkdir(parents=True,exist_ok=True)
cube=unreal.load_asset('/Engine/BasicShapes/Cube');changed=[]
def box(n,p,size,mat='/Game/Transmit/Presentation/Materials/M_Ceramic',collision=True):
 actor=a.get(n)
 if actor is None:actor=E.spawn_actor_from_class(unreal.StaticMeshActor,V(*p));actor.set_actor_label(n);a[n]=actor
 actor.set_actor_location(V(*p),False,False);actor.set_actor_scale3d(V(*(x/100 for x in size)))
 actor.static_mesh_component.set_static_mesh(cube);actor.static_mesh_component.set_material(0,unreal.load_asset(mat));actor.static_mesh_component.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
 actor.set_folder_path('Experience');changed.append(n);return actor
# Topology-safe floor repair: hidden original collision stays exactly in place.
# These are the actual flat cube surfaces identified by the prewrite overlap scan.
names=['Learn_FarBank','Route_LaunchFloor','Route_CulvertFloor','Route_Gallery','Route_InterceptFloor','Route_DiagonalDeck','Route_DockFloor','Route_ApproachArena','Route_EntryConnector','Weaponize_ArenaFloor','Weaponize_ExitFloor','Pacing_LearnFirstLanding','Pacing_LearnTurnApproach','Pacing_ReuseFirstApproach','Pacing_ReuseFirstLanding','Pacing_ReuseNorthApproach','Pacing_ArenaReturn','Vertical_ServiceFoot','Vertical_ServiceCrest']
def subtract(r,c):
 x0,y0,x1,y1=r;u0,v0,u1,v1=c;ix0=max(x0,u0);iy0=max(y0,v0);ix1=min(x1,u1);iy1=min(y1,v1)
 if ix1<=ix0 or iy1<=iy0:return [r]
 return [q for q in [(x0,y0,ix0,y1),(ix1,y0,x1,y1),(ix0,y0,ix1,iy0),(ix0,iy1,ix1,y1)] if q[2]-q[0]>.01 and q[3]-q[1]>.01]
used=[];repairs=[]
for n in names:
 actor=a[n];p=actor.get_actor_location();s=actor.get_actor_scale3d();rot=actor.get_actor_rotation()
 assert abs(rot.pitch)+abs(rot.yaw)+abs(rot.roll)<.01
 rect=(p.x-s.x*50,p.y-s.y*50,p.x+s.x*50,p.y+s.y*50);top=p.z+s.z*50;pieces=[rect]
 for z,prior in used:
  if abs(z-top)<.15:pieces=[q for r in pieces for q in subtract(r,prior)]
 used.append((top,rect))
 if pieces==[rect]:
  actor.static_mesh_component.set_visibility(True)
  redundant=a.pop('Seam_'+n+'_0',None)
  if redundant:assert E.destroy_actor(redundant)
  repairs.append({'original_collision':n,'visible_pieces':1,'original_visible':True,'area':s.x*s.y*10000})
  continue
 actor.static_mesh_component.set_visibility(False)
 for i,r in enumerate(pieces):box('Seam_'+n+'_'+str(i),((r[0]+r[2])/2,(r[1]+r[3])/2,p.z),(r[2]-r[0],r[3]-r[1],s.z*100),actor.static_mesh_component.get_material(0).get_path_name(),False)
 repairs.append({'original_collision':n,'visible_pieces':len(pieces),'area':sum((r[2]-r[0])*(r[3]-r[1]) for r in pieces)})
# Expand the physical aperture with the gate. Outer floor union stays unchanged.
for n,p,size in [('Weaponize_Gate',(8160,2530,450),(120,1000,900)),('Weaponize_EastLow',(8160,930,450),(100,2180,900)),('Weaponize_EastHigh',(8160,3130,450),(100,180,900))]:box(n,p,size)
for n,y in [('Weaponize_ExitWall_2180',1980),('Weaponize_ExitWall_2880',3080)]:
 p=a[n].get_actor_location();a[n].set_actor_location(V(p.x,y,p.z),False,False);changed.append(n)
# Existing trim followed old opening; suppress rather than leave floating strips.
for n,actor in a.copy().items():
 if n.startswith(('Presentation_Finish_Weaponize_EastLow','Presentation_Finish_Weaponize_EastHigh','Flow_ExitParapet_')):
  actor.set_actor_hidden_in_game(True);actor.set_actor_enable_collision(False);changed.append(n)
# Two cover pillars behind the rail. Entire rail-to-home aim fan stays clear (x >= 7020).
for i,y in enumerate([1970,2860]):
 box('Cover_Capacitor_'+str(i),(6480,y,205),(200,200,410),'/Game/Transmit/Presentation/Materials/M_Graphite')
 box('Cover_Cap_'+str(i),(6480,y,425),(224,224,30))
# Existing FMS only: 9.7s nominal cycle -> 5.65s, still enough committed warning.
fsm=a['Weaponize_Charger'].state_machine
for k,v in {'idle_duration_seconds':.65,'telegraph_duration_seconds':.95,'dash_duration_seconds':1.65,'recovery_duration_seconds':2.4,'dash_commit_window_delay_seconds':.05}.items():fsm.set_editor_property(k,v)
a['Weaponize_Charger'].set_editor_property('dash_speed',1250)
# A folded-screen silhouette belongs to the same actor, not a substitute projectile.
a['Route_Carrier'].body.set_relative_scale3d(V(.28,1.7,1.3))
# Static facility signs, intentionally readable only when looked at.
plane=unreal.load_asset('/Engine/BasicShapes/Plane')
signs=[('WorkOrder',(-9450,605,560),(520,260),180),('Launch',(3180,-560,210),(470,235),0),('Standard',(4100,-1190,350),(620,310),0),('Quiet',(8097,1760,510),(550,275),90),('Access',(8075,2530,340),(440,220),90),('Carrier',(7370,3195,270),(330,165),180),('Exit',(9065,2530,690),(520,260),90),('OriginalNotice',(8520,3040,320),(520,260),180)]
for name,pos,size,yaw in signs:
 n='Story_'+name;actor=a.get(n)
 if actor is None:actor=E.spawn_actor_from_class(unreal.StaticMeshActor,V(*pos));actor.set_actor_label(n);a[n]=actor
 actor.set_actor_location(V(*pos),False,False)
 # Local plane X/Y becomes horizontal/vertical; front points toward the route.
 actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=90),False)
 actor.set_actor_scale3d(V(size[0]/100,size[1]/100,1));actor.static_mesh_component.set_static_mesh(plane)
 actor.static_mesh_component.set_material(0,unreal.load_asset('/Game/Transmit/Presentation/Narrative/M_'+('Quiet' if name=='OriginalNotice' else name)));actor.set_actor_enable_collision(False);actor.set_folder_path('Experience/Narrative')
 if name=='Access':actor.set_editor_property('tags',[unreal.Name('Transmit.AccessSign')])
 changed.append(n)
# Reuse the existing stands. Remove duplicate English messages from the playable view.
for old,width,height in [('Hardware_MigongMotto',520,260),('Hardware_CarrierPlacard',470,235),('Hardware_Comparison',620,310)]:
 a[old].set_actor_hidden_in_game(True)
 a['Hardware_Plaque_'+old].set_actor_scale3d(V(.06,width/100,height/100))
 changed.extend([old,'Hardware_Plaque_'+old])
for old in ['Hardware_InterfaceNotice','Hardware_InterfaceQuiet']:
 for n in [old,'Hardware_Plaque_'+old,'Hardware_Post_'+old]:
  a[n].set_actor_hidden_in_game(True);a[n].set_actor_enable_collision(False);changed.append(n)
# Put the original rule behind the added gatekeeping sign, revealed after it retires.
# Quiet sign remains intact, as does the institution's interface identity.
assert L.save_current_level()
(out/'map-authoring.json').write_text(json.dumps({'map':'L_Transmit','changed':changed,'floor_repair':repairs,'actors':len(a)},indent=2))
unreal.log('EXPERIENCE_MAP_SAVED '+str(len(a)))
