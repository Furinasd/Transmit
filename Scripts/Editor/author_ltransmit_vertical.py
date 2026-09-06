"""One authorized layout + diegetic guidance transaction, only L_Transmit.

Keep all baseline actors. Reuse existing meshes/materials; no asset deletion,
project settings, core ownership or direction changes. Run outside PIE.
The baseline inventory makes subsequent runs absolute rather than additive.
"""
import json
import math
import pathlib
import unreal

ROOT = pathlib.Path(unreal.Paths.project_dir())
OUT = ROOT / 'Saved/LTransmitEvidence/Vertical'
OUT.mkdir(parents=True, exist_ok=True)
MAP = '/Game/Transmit/Maps/L_Transmit'
TAG = 'Transmit.Vertical'
V = unreal.Vector
E = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not L.is_in_play_in_editor()
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages(), 'Preserve unsaved content'
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'Preserve unsaved map'
assert L.load_level(MAP)
actors = {a.get_actor_label(): a for a in E.get_all_level_actors()}

def transform(a):
    p, r, s = a.get_actor_location(), a.get_actor_rotation(), a.get_actor_scale3d()
    return [p.x,p.y,p.z,r.pitch,r.yaw,r.roll,s.x,s.y,s.z]

baseline_path = OUT / 'baseline.json'
if not baseline_path.exists():
    assert not any(TAG in [str(t) for t in a.tags] for a in actors.values()), \
        'This map already contains the vertical pass. Preserve it; restore the original authoring baseline before reauthoring.'
    baseline_path.write_text(json.dumps({n: transform(a) for n,a in actors.items()}, indent=2))
base = json.loads(baseline_path.read_text())
assert all(n in actors for n in base), 'Missing baseline actor'
changed = set()
created = []
cube = unreal.load_asset('/Engine/BasicShapes/Cube')
cylinder = unreal.load_asset('/Engine/BasicShapes/Cylinder')
mats = {n: unreal.load_asset('/Game/Transmit/Presentation/Materials/M_'+n)
        for n in ('Ceramic','Graphite','Inlay')}
assert cube and cylinder and all(mats.values())

def get(cls, name, pos):
    a = actors.get(name)
    if a is None:
        a = E.spawn_actor_from_class(cls,V(*pos)); a.set_actor_label(name)
        a.set_editor_property('tags',[unreal.Name(TAG)])
        a.set_folder_path('Vertical/Hardware' if name.startswith('Hardware_') else 'Vertical/Layout')
        actors[name] = a; created.append(name)
    assert isinstance(a,cls), name
    if name in base: changed.add(name)
    return a

def box(name, pos, size, mat='Graphite', collision=True, yaw=0, pitch=0, mesh=None):
    a = get(unreal.StaticMeshActor,name,pos)
    a.set_actor_location(V(*pos),False,False)
    a.set_actor_rotation(unreal.Rotator(pitch=pitch,yaw=yaw,roll=0),False)
    a.set_actor_scale3d(V(*(v/100 for v in size)))
    c = a.static_mesh_component
    c.set_static_mesh(mesh or cube); c.set_material(0,mats[mat])
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    c.set_editor_property('generate_overlap_events',False)
    return a

def ramp(name, start, end, width, thickness=120, mat='Graphite', collision=True):
    dx,dy,dz = (end[i]-start[i] for i in range(3))
    run=math.hypot(dx,dy); length=math.sqrt(run*run+dz*dz)
    yaw=math.degrees(math.atan2(dy,dx)); pitch=math.degrees(math.atan2(dz,run))
    # Top surface passes exactly through both authored endpoints.
    normal=(-dz/length*dx/run,-dz/length*dy/run,run/length)
    center=[(start[i]+end[i])/2-normal[i]*thickness/2 for i in range(3)]
    return box(name,center,(length,width,thickness),mat,collision,yaw,pitch)

def move_z(name, dz):
    a=actors[name]; x,y,z=base[name][:3]
    a.set_actor_location(V(x,y,z+dz),False,False); changed.add(name)

def sign(name,pos,text,yaw=180,size=26):
    a=get(unreal.TextRenderActor,name,pos)
    a.set_actor_location(V(*pos),False,False)
    a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False)
    c=a.text_render; c.set_text(text); c.set_world_size(size)
    c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    c.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    c.set_text_render_color(unreal.Color(224,233,232,255))
    # Backing width based on authored text, not stale render bounds after edit.
    lines=text.split('\n'); width=max(len(line) for line in lines)*size*.62+40
    height=len(lines)*size*1.25+26
    rad=math.radians(yaw)
    back='Wayfinding_SignBack_'+name if 'Wayfinding_SignBack_'+name in actors else 'Hardware_Plaque_'+name
    box(back,(pos[0]-5*math.cos(rad),pos[1]-5*math.sin(rad),pos[2]),(6,width,height),collision=False,yaw=yaw)
    # A grounded mounting stem replaces floating tutorial billboards.
    floor=height_at(pos[0],pos[1])
    if pos[2]-height/2 > floor:
        box('Hardware_Post_'+name,(pos[0]-8*math.cos(rad),pos[1]-8*math.sin(rad),(pos[2]-height/2+floor)/2),
            (10,10,pos[2]-height/2-floor),'Graphite',False)
    return a

# Height profiles used by connected floors, white marks and protective railings.
def arrival_height(x): return 300 if x<=-9300 else max(0,300*(-8500-x)/800)
def loop_east_height(x): return max(0,min(480,480*(x+4850)/2750))
def loop_south_height(y): return max(0,min(480,480*(y-450)/2450))
def service_height(y): return max(0,min(600,600*(-1200-y)/4300))
def north_height(y): return max(0,min(600,600*(-600-y)/1300))
def height_at(x,y):
    if x<-7000 and abs(y)<800: return arrival_height(x)
    if -5300<x<-1800 and y>2850: return loop_east_height(x)
    if -2000<x<-1000 and y>350: return loop_south_height(y)
    if 4250<x<5900 and y<-650: return service_height(y)
    if x>6000 and y<-1900: return 600
    if x>7500 and y<-600: return north_height(y)
    return 0

# Arrival: immediate player control, raised view of the first source + missing span.
box('Pacing_LearnArrival',(-7750,0,-60),(1500,1300,120))
box('Vertical_ArrivalLookout',(-9950,0,240),(1300,1300,120))
ramp('Vertical_ArrivalDescent',(-9300,0,300),(-8500,0,0),1300)
move_z('Learn_PlayerStart',300)
# First-bank landing occupies the existing final 15m of the arrival floor.

# Return loop: the same route now rises to a chip-top overlook, then descends.
ramp('Pacing_LearnReturnEast',(-5200,3300,0),(-1800,3300,480),700)
# Use exact ramp slope for this portion of the profile.
def loop_east_height(x): return max(0,min(480,480*(x+5200)/3400))
box('Vertical_LoopCorner',(-1575,3300,420),(650,700,120))
ramp('Pacing_LearnReturnSouth',(-1500,2950,480),(-1500,450,0),700)
def loop_south_height(y): return max(0,min(480,480*(y-450)/2500))
box('Vertical_LoopFoot',(-1500,50,-60),(700,800,120))
# No vertical jump at either end of a ramp, and all railings follow its slope.
ramp('Wayfinding_ReturnNorth',(-5200,3650,90),(-1800,3650,570),40,90)
box('Wayfinding_ReturnEastTurn',(-1150,3290,525),(40,720,90))
ramp('Wayfinding_ReturnSouthWest',(-1850,2950,570),(-1850,450,90),40,90)
ramp('Wayfinding_ReturnSouthEast',(-1150,2950,570),(-1150,450,90),40,90)

# Service deck: both existing reuse bridges and their sole Source share +6m.
for n in ('Pacing_ReuseFirstApproach','Pacing_ReuseFirstLanding','Pacing_ReuseNorthApproach',
          'Pacing_RouteBridgeA','Pacing_RouteBridgeB','Pacing_RouteSource',
          'Pacing_ReuseStopA','Pacing_ReuseStopB',
          'Wayfinding_ServiceSouth','Wayfinding_CarryEast','Wayfinding_CarryWest'):
    move_z(n,600)
ramp('Pacing_RelayDeparture',(5350,-1200,0),(5350,-5500,600),700)
box('Vertical_ServiceFoot',(5350,-850,-60),(700,700,120))
box('Vertical_ServiceCrest',(5350,-5625,540),(700,250,120))
ramp('Wayfinding_ServiceWest',(5000,-1200,90),(5000,-5500,690),40,90)
ramp('Wayfinding_ServiceEast',(5700,-1200,90),(5700,-5500,690),40,90)
box('Vertical_InterfaceLanding',(7900,-2000,540),(700,200,120))
ramp('Pacing_ReuseNorthLanding',(7900,-1900,600),(7900,-600,0),700)
ramp('Wayfinding_ArenaApproachEast',(8250,-1900,690),(8250,-600,90),40,90)
# Prevent the elevated path from turning the previous separator into a shortcut.
box('Pacing_TransitionSeparatorUpper',(5810,-1900,820),(100,5700,1280),'Ceramic')

# Reproject pre-existing noncolliding ground marks to the new top surfaces.
# Horizontal strips are short; each is tilted to follow its local plane.
for n,values in base.items():
    if not n.startswith('Wayfinding_') or n.startswith('Wayfinding_SignBack_') or n in changed: continue
    a=actors[n]
    if not isinstance(a,unreal.StaticMeshActor): continue
    if str(a.static_mesh_component.get_collision_profile_name())!='NoCollision': continue
    x,y,z,pitch,yaw,roll,sx,sy,sz=values
    if z>10: continue
    h=height_at(x,y)
    if h<=0: continue
    dx=(height_at(x+1,y)-height_at(x-1,y))/2
    dy=(height_at(x,y+1)-height_at(x,y-1))/2
    r=math.radians(yaw)
    along=dx*math.cos(r)+dy*math.sin(r)
    across=-dx*math.sin(r)+dy*math.cos(r)
    a.set_actor_location(V(x,y,z+h),False,False)
    a.set_actor_rotation(unreal.Rotator(pitch=math.degrees(math.atan(along)),yaw=yaw,roll=-math.degrees(math.atan(across))),False)
    changed.add(n)

# Twelve long tutorial boards become small maintenance placards; HUD holds the verbs.
for name,pos,value,yaw,size in (
 ('Pacing_ArrivalText',(-9490,-580,510),'MIGONG / BOARD ASSEMBLY\nOPERATOR: TEMPORARY',90,27),
 ('Pacing_FirstBridgeText',(-8260,-540,150),'01A / BRIDGE CONTACT\nOPERATE FROM BANK',135,22),
 ('Pacing_TurnText',(-5720,750,180),'01B / NORTH CONTACT',0,24),
 ('Pacing_RetryText',(-3350,3550,470),'INSPECTION LOOP\nBOARD 01 > BUS RELAY',-90,24),
 ('Pacing_OriginalLearnText',(-1170,950,310),'01C / BUS CONNECTION',180,24),
 ('Pacing_DockDepartureText',(5550,-700,210),'SERVICE DECK / +6m\nFOLLOW WHITE MARKS',180,24),
 ('Pacing_ReuseText',(5350,-6600,790),'SERVICE 01 / SEND',90,24),
 ('Pacing_ReclaimText',(7250,-5850,790),'LOOK BACK / RECLAIM',-90,24),
 ('Pacing_RerouteText',(8160,-4400,790),'SERVICE 02 / REUSE',180,24),
 ('Pacing_ThreatPreviewText',(8230,-1550,620),'BAIGUO / UPPER INTERFACE',180,23),
 ('Pacing_DashLessonText',(6740,-400,190),'INSPECTION CHECKPOINT',0,24),
 ('Pacing_ArenaRetryText',(6740,350,190),'TWO IMPACTS / ONE PASSAGE',0,22),
): sign(name,pos,value,yaw,size)

# Original tutorial lettering also becomes concise facility identity.
for n,a in list(actors.items()):
    if not isinstance(a,unreal.TextRenderActor) or n.startswith(('Pacing_','Hardware_')): continue
    if n=='Learn_Controls': a.text_render.set_text('BUS 01 / CONTACT TEST'); changed.add(n)

# A giant host, visible from arrival: board substrate, packages, solder pads,
# screw heads and heatsink skyline. These sit outside every traversable surface.
box('Hardware_Mainboard',(-1700,700,-1450),(23000,16000,180),'Graphite',False)
for i,(x,y,sx,sy,z) in enumerate(((-7400,1850,1000,1000,1000),(-3300,1750,1700,1200,700),
                               (1500,3600,2300,2100,1500),(5900,-8200,3500,1400,800))):
    box('Hardware_Package_%d'%i,(x,y,z/2-400),(sx,sy,z),'Graphite',False)
    box('Hardware_PackageCap_%d'%i,(x,y,z-396),(sx-60,sy-60,8),'Ceramic',False)
    for side in (-1,1):
        for pin in range(8):
            box('Hardware_Pin_%d_%d_%d'%(i,side,pin),(x-sx*.4+pin*sx*.8/7,y+side*(sy/2+80),-200),
                (70,180,45),'Ceramic',False)
for i,(x,y) in enumerate(((-8450,-1800),(-6500,-2300),(-3900,5100),(6400,-8100))):
    box('Hardware_Capacitor_%d'%i,(x,y,550),(620,620,1600),'Graphite',False,mesh=cylinder)
    box('Hardware_CapacitorTop_%d'%i,(x,y,1360),(590,590,26),'Ceramic',False,mesh=cylinder)
    box('Hardware_CapacitorVent_%d'%i,(x,y,1375),(420,28,6),'Graphite',False,yaw=25)
    box('Hardware_CapacitorVentCross_%d'%i,(x,y,1375),(28,420,6),'Graphite',False,yaw=25)
for i,(x,y,z) in enumerate(((-9300,970,60),(-6500,-1150,-50),(-2100,4100,0),(6900,-7200,360))):
    box('Hardware_SolderPad_%d'%i,(x,y,z-50),(650,650,40),'Inlay',False,mesh=cylinder)
    box('Hardware_Screw_%d'%i,(x,y,z),(390,390,80),'Ceramic',False,mesh=cylinder)
    for yaw in (0,90): box('Hardware_ScrewSlot_%d_%d'%(i,yaw),(x,y,z+42),(240,40,6),'Graphite',False,yaw=yaw)
# Heatsink fins give the long elevated passages a stable distant landmark.
box('Hardware_HeatsinkBase',(2300,6700,-100),(5000,1600,350),'Graphite',False)
for i in range(13):
    box('Hardware_HeatsinkFin_%02d'%i,(-100+400*i,6700,1500),(100,1500,3000),'Ceramic',False)
# Interface silhouette beyond the gate is a local work destination, not the world's end.
box('Hardware_InterfaceShell',(10800,2400,1100),(900,2800,2800),'Ceramic',False)
box('Hardware_InterfaceSocket',(10340,2400,1100),(28,2050,1300),'Graphite',False)
for i in range(8): box('Hardware_InterfacePin_%d'%i,(10310,1580+i*235,780),(45,90,420),'Inlay',False)
# Side markings establish maintenance identity without occupying Motion colours.
sign('Hardware_MigongMotto',(-9450,605,560),'MIGONG WORKS\nBUILT FOR EVERYONE.',-90,28)
sign('Hardware_CarrierPlacard',(3180,-560,210),'HUAXU STANDARDS\nC-01 / UNIVERSAL CARRIER',90,25)
sign('Hardware_Comparison',(4100,-1190,350),'50% AHEAD*\n*DISPLAY HEIGHT: 3m vs 2m',90,25)
sign('Hardware_InterfaceNotice',(8350,2950,200),'BAIGUO INTERFACE\nPASS AFTER MAINTENANCE',-90,26)
sign('Hardware_InterfaceQuiet',(7650,3270,370),'Only apple can do.',-90,24)

# All unscoped actors, including Boss, Ram, original L2 and lighting, are protected.
after={a.get_actor_label():transform(a) for a in E.get_all_level_actors()}
assert all(n in after for n in base)
actual={n for n in base if any(abs(x-y)>.001 for x,y in zip(base[n],after[n]))}
assert actual <= changed, str(actual-changed)
for n in ('Weaponize_Ram','Weaponize_Charger','Weaponize_Gate','Route_Carrier','Route_Source',
          'Route_DockMarker','Flow_Director','Flow_Reset','Learn_Source','Learn_BridgeSlab'):
    assert after[n]==base[n], 'Protected gameplay actor: '+n
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world,'MAP CHECK')
assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
(OUT/'authoring.json').write_text(json.dumps({'map':MAP,'retained':len(base),'total':len(after),
    'modified_existing':sorted(changed),'moved_existing':sorted(actual),'created':sorted(n for n in after if n not in base),
    'heights_cm':{'arrival':300,'inspection_loop':480,'service_deck':600},
    'protected_core_transforms':True,'only_binary':'Content/Transmit/Maps/L_Transmit.umap'},indent=2))
unreal.log('TRANSMIT_VERTICAL_AUTHOR_SUCCESS')
