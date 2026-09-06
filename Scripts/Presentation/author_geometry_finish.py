"""One map-local simple-geometry finish. Apply unsaved, inspect, then save the map.
Only listed static surfaces receive materials; existing transforms/collision stay intact.
Cyan/amber gameplay language and all moving/functional actors are outside the scope.
"""
import unreal, math
PREFIX='Presentation_Finish_'
FLOORS=('Learn_Approach','Learn_FarBank','Route_LaunchFloor','Route_Gallery',
        'Route_InterceptFloor','Weaponize_ArenaFloor','Weaponize_ExitFloor')
# Wall, inward normal sign. Thin pieces follow authored mesh bounds.
WALLS=(('Learn_ParapetNear_-650',1),('Learn_ParapetNear_650',-1),
       ('Learn_ParapetFar_-650',1),('Learn_ParapetFar_650',-1),
       ('Route_GalleryOuterWall',1),('Weaponize_NorthWall',-1),
       ('Weaponize_WestWall',1),('Weaponize_EastLow',-1),('Weaponize_EastHigh',-1))

def apply():
 level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
 assert not level.is_in_play_in_editor()
 assert world.get_path_name().split('.')[0]=='/Game/Transmit/Maps/L_Transmit'
 editor=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 actors={a.get_actor_label():a for a in editor.get_all_level_actors()}
 mats={k:unreal.load_asset('/Game/Transmit/Presentation/Materials/M_'+k) for k in ('Ceramic','Graphite','Inlay')}
 cube=unreal.load_asset('/Engine/BasicShapes/Cube')
 assert all(mats.values()) and cube
 for label in FLOORS+tuple(w[0] for w in WALLS):
  assert isinstance(actors.get(label),unreal.StaticMeshActor),label
  assert actors[label].static_mesh_component.static_mesh==cube,label
  assert abs(actors[label].get_actor_rotation().yaw)<.01,label
 created=[]
 def box(name,center,size,material):
  name=PREFIX+name
  actor=actors.get(name)
  if actor is None:
   actor=editor.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*center))
   actor.set_actor_label(name);actors[name]=actor
  assert isinstance(actor,unreal.StaticMeshActor)
  actor.set_folder_path('Presentation/GeometryFinish')
  actor.set_editor_property('tags',[unreal.Name('Transmit.Presentation.Finish')])
  actor.set_actor_location(unreal.Vector(*center),False,False)
  actor.set_actor_rotation(unreal.Rotator(),False)
  actor.set_actor_scale3d(unreal.Vector(*(d/100 for d in size)))
  mesh=actor.static_mesh_component
  mesh.set_static_mesh(cube);mesh.set_material(0,mats[material])
  mesh.set_collision_profile_name('NoCollision')
  mesh.set_editor_property('generate_overlap_events',False)
  mesh.set_editor_property('can_ever_affect_navigation',False)
  created.append(name)
  return actor
 def bounds(actor):
  p=actor.get_actor_location();s=actor.get_actor_scale3d()
  return [p.x,p.y,p.z],[100*s.x,100*s.y,100*s.z]
 # Inset floor planes preserve the actual walking boundary and leave a dark perimeter.
 # Small gaps in the inset supply scale; no new emissive markings suggest false routes.
 covered=[]
 def subtract(rect,other):
  x0,x1,y0,y1=rect;a,b,c,d=other
  ix0,ix1=max(x0,a),min(x1,b);iy0,iy1=max(y0,c),min(y1,d)
  if ix0>=ix1 or iy0>=iy1:return [rect]
  pieces=[(x0,ix0,y0,y1),(ix1,x1,y0,y1),(ix0,ix1,y0,iy0),(ix0,ix1,iy1,y1)]
  return [r for r in pieces if r[1]-r[0]>.2 and r[3]-r[2]>.2]
 for label in FLOORS:
  actor=actors[label];center,size=bounds(actor)
  actor.static_mesh_component.set_material(0,mats['Graphite'])
  width,length=size[0]-64,size[1]-64
  nx=max(1,int(math.ceil(width/500)));ny=max(1,int(math.ceil(length/500)))
  for x in range(nx):
   for y in range(ny):
    pos=[center[0]-width/2+(x+.5)*width/nx,center[1]-length/2+(y+.5)*length/ny,center[2]+size[2]/2+.4]
    rect=(pos[0]-(width/nx-2)/2,pos[0]+(width/nx-2)/2,pos[1]-(length/ny-2)/2,pos[1]+(length/ny-2)/2)
    pieces=[rect]
    for old in covered:pieces=[part for piece in pieces for part in subtract(piece,old)]
    for k,(x0,x1,y0,y1) in enumerate(pieces):
     box(label+'_Panel_%d_%d_%d'%(x,y,k),[(x0+x1)/2,(y0+y1)/2,pos[2]],[x1-x0,y1-y0,.6],'Inlay')
  covered.append((center[0]-size[0]/2,center[0]+size[0]/2,center[1]-size[1]/2,center[1]+size[1]/2))
 # Thin base/cap pieces articulate mass, instead of filling traversal or negative space.
 for label,sign in WALLS:
  actor=actors[label];center,size=bounds(actor)
  normal=0 if size[0]<size[1] else 1
  tangent=1-normal
  face=center[normal]+sign*(size[normal]/2+1)
  base=center.copy();base[normal]=face;base[2]=center[2]-size[2]/2+14
  base_size=size.copy();base_size[normal]=3;base_size[2]=28
  box(label+'_Plinth',base,base_size,'Graphite')
  cap=center.copy();cap[2]=center[2]+size[2]/2+2
  cap_size=size.copy();cap_size[normal]+=6;cap_size[2]=4
  box(label+'_Cap',cap,cap_size,'Graphite')
  count=int(size[tangent]//550)
  for i in range(1,count+1):
   pos=center.copy();pos[normal]=face
   pos[tangent]=center[tangent]-size[tangent]/2+i*size[tangent]/(count+1)
   seam=[1.5,1.5,size[2]-56];seam[normal]=2
   box(label+'_Joint_'+str(i),pos,seam,'Graphite')
 # Navigation identity is neutral: the reserved cyan/amber are only gameplay feedback.
 signs=(('01',(-120,-606,340),90),('02',(3350,-1216,430),90),('03',(5984,1350,640),0))
 for number,pos,yaw in signs:
  label=PREFIX+'Chapter_'+number
  actor=actors.get(label)
  if actor is None:
   actor=editor.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*pos));actor.set_actor_label(label);actors[label]=actor
  assert isinstance(actor,unreal.TextRenderActor)
  actor.set_folder_path('Presentation/GeometryFinish')
  actor.set_editor_property('tags',[unreal.Name('Transmit.Presentation.Finish')])
  actor.set_actor_location(unreal.Vector(*pos),False,False)
  actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False)
  actor.text_render.set_text(number)
  actor.text_render.set_world_size(115)
  actor.text_render.set_text_render_color(unreal.Color(40,64,78,255))
  actor.text_render.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
  created.append(label)
 return dict(material_surfaces=list(FLOORS),presentation_actors=created)
