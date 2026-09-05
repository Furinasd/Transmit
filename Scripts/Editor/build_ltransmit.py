"""Author the isolated L_Transmit v0.1 graybox. Run inside UE Editor.
Only owns actors tagged Transmit.V01 in /Game/Transmit/Maps/L_Transmit,
and new materials under /Game/Transmit/ExperimentV01. Existing assets read-only.
"""
import unreal, math
MAP = '/Game/Transmit/Maps/L_Transmit'
ROOT = '/Game/Transmit/ExperimentV01'
TAG = 'Transmit.V01'
V = unreal.Vector
E = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

def material(name, rgb):
    path = ROOT + '/M_' + name
    m = unreal.load_asset(path)
    if m: return m
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_'+name,ROOT,unreal.Material,unreal.MaterialFactoryNew())
    c = unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector)
    c.set_editor_property('constant',unreal.LinearColor(*rgb,1))
    unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    r = unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant)
    r.set_editor_property('r',0.8)
    unreal.MaterialEditingLibrary.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

def spawn(cls,name,loc,rot=(0,0,0)):
    a=E.spawn_actor_from_class(cls,V(*loc),unreal.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
    a.set_actor_label(name);a.set_editor_property('tags',[unreal.Name(TAG)])
    a.set_folder_path(name.split('_')[0]);return a

def box(name,loc,size,mat='Chalk',yaw=0,collision=True):
    a=spawn(unreal.StaticMeshActor,name,loc,(0,yaw,0))
    c=a.static_mesh_component;c.set_static_mesh(CUBE)
    c.set_material(0,MATS[mat]);a.set_actor_scale3d(V(*(x/100 for x in size)))
    c.set_collision_profile_name('BlockAll' if collision else 'NoCollision')
    return a

def line(name,a,b,width=14,height=6,mat='Amber'):
    mid=tuple((a[i]+b[i])/2 for i in range(3));dx=b[0]-a[0];dy=b[1]-a[1]
    return box(name,mid,(math.hypot(dx,dy),width,height),mat,math.degrees(math.atan2(dy,dx)),False)

def text(name,loc,value,size=34,yaw=180):
    a=spawn(unreal.TextRenderActor,name,loc,(0,yaw,0));c=a.text_render
    c.set_text(value);c.set_world_size(size);c.set_text_render_color(unreal.Color(230,239,244,255))
    c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    return a

def marker(name,loc):return spawn(unreal.TargetPoint,name,loc)

def endpoint(name,loc,source=True):
    a=spawn(unreal.TransmitMotionEndpointActor,name,loc)
    mo=a.motion;mo.set_editor_property('participant_id',name)
    mo.set_editor_property('can_provide_motion',source);mo.set_editor_property('can_receive_motion',not source)
    mo.set_editor_property('starts_with_motion',source)
    if source:
        s=unreal.MotionState();s.set_editor_property('direction',V(1,0,0));s.set_editor_property('magnitude',600);s.set_editor_property('source_id',name)
        mo.set_editor_property('initial_motion',s)
    a.body.set_material(0,MATS['Amber']);return a

def build():
    global CUBE,MATS
    assert not L.is_in_play_in_editor(),'Stop PIE first'
    if unreal.EditorAssetLibrary.does_asset_exist(MAP):
        L.load_level(MAP)
        existing=E.get_all_level_actors()
        unknown=[a.get_actor_label() for a in existing if TAG not in [str(t) for t in a.tags] and a.get_class().get_name() not in ('WorldSettings','Brush')]
        assert not unknown, 'Refusing to overwrite unowned actors: '+str(unknown)
        for a in existing:
            if TAG in [str(t) for t in a.tags]:E.destroy_actor(a)
    else:
        assert L.new_level(MAP),'Could not create experiment map'
    CUBE=unreal.load_asset('/Engine/BasicShapes/Cube')
    MATS={k:material(k,c) for k,c in {'Slate':(.075,.11,.14),'Chalk':(.49,.54,.56),'Amber':(1,.44,.035),'Cyan':(.03,.65,.85),'Red':(.65,.055,.025),'White':(.8,.84,.8)}.items()}
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',unreal.EditorAssetLibrary.load_blueprint_class('/Game/Transmit/Blueprints/BP_TransmitGameMode'))
    # One daylight rig, authored only in this level.
    sun=spawn(unreal.DirectionalLight,'Light_Sun',(1000,-1000,2500),(-50,-25,0));sun.light_component.set_editor_property('intensity',4.0)
    sky=spawn(unreal.SkyLight,'Light_Fill',(3000,0,2000));sky.light_component.set_editor_property('source_type',unreal.SkyLightSourceType.SLS_CAPTURED_SCENE);sky.light_component.set_editor_property('intensity',1.0)
    spawn(unreal.SkyAtmosphere,'Light_Atmosphere',(0,0,-6000))
    start=spawn(unreal.PlayerStart,'Learn_PlayerStart',(0,0,100))
    # Learn: the missing 9m span exceeds an ordinary jump. Slide a 10m deck into it.
    box('Learn_Approach',(400,0,-60),(1600,1400,120),'Slate')
    box('Learn_FarBank',(2500,0,-60),(800,1400,120),'Slate')
    box('Learn_DeckStop',(2200,0,40),(80,520,80),'Chalk')
    bridge=spawn(unreal.TransmitBridgeSlab,'Learn_BridgeSlab',(680,0,40))
    bridge.motion.set_editor_property('participant_id','Learn.Bridge');bridge.body.set_material(0,MATS['Cyan'])
    endpoint('Learn_Source',(180,-290,120))
    box('Learn_SourcePlinth',(180,-290,30),(180,180,60),'Chalk')
    for y in [-650,650]:
        box('Learn_Parapet_'+str(y),(900,y,180),(2600,80,360),'Chalk')
        line('Learn_DeckRail_'+str(y),(700,y*.43,5),(2180,y*.43,5),20,10,'Cyan')
    box('Learn_BackWall',(-430,0,400),(60,1400,800),'Slate')
    text('Learn_Title',(-385,0,340),'T R A N S M I T',64,0)
    text('Learn_Controls',(-385,0,235),'E  TAKE     Q  GIVE     R  RESTART',24,0)
    # Route entry contracts from bridge to a low, visibly mechanical culvert.
    box('Route_LaunchFloor',(3100,-100,-60),(1000,1200,120),'Slate')
    endpoint('Route_Source',(2780,-320,120))
    carrier=spawn(unreal.TransmitDirectionalCarrierActor,'Route_Carrier',(3200,0,85))
    carrier.motion.set_editor_property('participant_id','Route.Carrier');carrier.set_editor_property('movement_speed',310)
    carrier.body.set_material(0,MATS['Amber'])
    box('Route_CulvertFloor',(4370,0,-60),(2420,420,120),'Chalk')
    # 150cm throat admits the carrier but excludes the standing Player capsule.
    for i,x in enumerate([3550,3970,4390,4810]):
        box('Route_LowRoof_'+str(i),(x,0,250),(380,460,200),'Slate')
        box('Route_NorthCurb_'+str(i),(x,250,220),(400,80,440),'Chalk')
        box('Route_SouthSill_'+str(i),(x,-250,48),(380,80,96),'Chalk')
    box('Route_CatchStop',(5470,0,120),(80,500,240),'Chalk')
    # Player descends into a parallel inspection gallery; Motion stays above.
    box('Route_Gallery',(4300,-900,-60),(3000,650,120),'Slate')
    box('Route_GalleryOuterWall',(4300,-1260,350),(3300,80,700),'Chalk')
    box('Route_GalleryInnerWall',(4180,-525,200),(1700,90,400),'Slate')
    box('Route_InterceptFloor',(5420,-300,-60),(980,1400,120),'Slate')
    for x in [3200,3800,4400,5000]:line('Route_Follow_'+str(x),(x,-980,5),(x+270,-980,5),16,5,'Cyan')
    line('Route_SendLine',(3100,0,3),(5400,0,3),26,6)
    # Re-route through a 30-degree throat. Existing resolver remains authoritative.
    angle=math.radians(30);dock=(6450,610,85)
    box('Route_DiagonalDeck',(5950,320,-60),(1500,540,120),'Chalk',30)
    for off in [-245,245]:
        cx=5910-math.sin(angle)*off;cy=300+math.cos(angle)*off
        box('Route_DiagonalCurb_'+str(off),(cx,cy,120),(1260,45,240),'Slate',30)
    line('Route_RerouteLine',(5380,0,6),(6450,618,6),25,8)
    box('Route_DockFloor',(6500,650,-60),(600,650,120),'Slate')
    box('Route_DockStop',(6650,725,120),(80,520,240),'Chalk',30)
    dockmark=marker('Route_DockMarker',dock)
    box('Route_DockSocket',dock,(210,210,12),'Cyan',30,False)
    # Walking route around the diagonal deck: lets the player see Ram before crossing threat.
    box('Route_ApproachArena',(6110,-440,-60),(1400,560,120),'Slate')
    box('Route_RevealPier',(6100,5,260),(140,280,520),'Chalk')
    # Arena: the same ram rail ends at the only exit. Dash cuts the route to the Ram.
    box('Weaponize_ArenaFloor',(7240,1540,-60),(2540,3320,120),'Slate')
    box('Weaponize_SouthWall',(7310,-150,350),(2180,80,700),'Chalk')
    box('Weaponize_NorthWall',(7240,3240,450),(2640,80,900),'Chalk')
    box('Weaponize_WestWall',(5940,1960,450),(80,2500,900),'Chalk')
    box('Weaponize_EastLow',(8160,1040,450),(100,2400,900),'Chalk')
    box('Weaponize_EastHigh',(8160,3030,450),(100,420,900),'Chalk')
    gate=box('Weaponize_Gate',(8160,2530,350),(120,620,700),'Red')
    gate.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    box('Weaponize_ExitFloor',(8710,2530,-60),(1000,650,120),'Slate')
    for y in [2180,2880]:box('Weaponize_ExitWall_'+str(y),(8700,y,300),(1100,70,600),'Chalk')
    box('Weaponize_EndWall',(9250,2530,320),(60,760,640),'Slate')
    text('Weaponize_EndTitle',(9205,2530,300),'MOTION\nTRANSMITTED',48,180)
    # Tall covers force a choice of lane crossing through the bait gap near x7600.
    box('Weaponize_EntryBaffle',(6770,1180,220),(1300,100,440),'Chalk')
    box('Weaponize_FarBaffle',(6770,2260,220),(1260,100,440),'Chalk')
    for y in [1510,2010]:line('Weaponize_DashEdge_'+str(y),(6220,y,7),(7940,y,7),26,10,'Red')
    line('Weaponize_DashAxis',(6250,1760,7),(7940,1760,7),14,8,'Red')
    box('Weaponize_DashStop',(7990,1760,150),(100,620,300),'Chalk')
    ram=spawn(unreal.TransmitRam,'Weaponize_Ram',(7500,2530,130))
    ram.body.set_material(0,MATS['Cyan']);ram.body.set_relative_scale3d(V(3,2.5,2.5))
    ram.motion.set_editor_property('participant_id','Weaponize.Ram')
    ram.set_editor_property('gate',gate);ram.set_editor_property('dock_marker',dockmark);ram.set_editor_property('route_carrier',carrier)
    ram.set_editor_property('dock_radius',190)
    for y in [2340,2720]:line('Weaponize_RamRail_'+str(y),(7170,y,20),(8170,y,20),32,40,'Cyan')
    line('Route_ArmLinkA',(6500,700,8),(7000,700,8),22,12,'Cyan')
    line('Route_ArmLinkB',(7000,700,8),(7000,2530,8),22,12,'Cyan')
    line('Route_ArmLinkC',(7000,2530,8),(7460,2530,8),22,12,'Cyan')
    charger=spawn(unreal.TransmitArenaCharger,'Weaponize_Charger',(6310,1760,100))
    charger.motion.set_editor_property('participant_id','Weaponize.Charger')
    charger.body.set_material(0,MATS['Red']);charger.set_editor_property('dash_speed',1100)
    fsm=charger.state_machine;fsm.set_editor_property('idle_duration_seconds',1.6);fsm.set_editor_property('telegraph_duration_seconds',1.4);fsm.set_editor_property('dash_duration_seconds',1.5);fsm.set_editor_property('recovery_duration_seconds',2.5)
    entry=marker('Weaponize_EntryMarker',(6910,700,100));exitmark=marker('Weaponize_ExitMarker',(8880,2530,100))
    director=spawn(unreal.TransmitLevelDirector,'Flow_Director',(0,0,0))
    director.set_editor_property('ram',ram);director.set_editor_property('charger',charger);director.set_editor_property('arena_entry_marker',entry);director.set_editor_property('exit_marker',exitmark)
    reset=spawn(unreal.MotionRoomResetController,'Flow_Reset',(0,0,-100));reset.set_editor_property('auto_discover_transferable_participants',True)
    # Composition: a framed destination, controlled horizons, no decoration pass.
    for x in [2900,5750]:box('Route_Portal_'+str(x),(x,-850,690),(100,820,100),'Chalk')
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(V(-500,-800,600),unreal.Rotator(pitch=-15,yaw=25,roll=0))
    assert unreal.EditorLoadingAndSavingUtils.save_map(world,MAP)
    unreal.log('TRANSMIT_BUILD_SUCCESS actors='+str(len(E.get_all_level_actors())))

build()
