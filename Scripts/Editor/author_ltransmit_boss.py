"""Single L_Transmit transaction: player-targeted arena, rail, floor overlap repair.
Existing assets are edited through Unreal only. No project settings or other maps.
"""
import unreal,json,pathlib
V=unreal.Vector
E=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not L.is_in_play_in_editor()
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert L.load_level('/Game/Transmit/Maps/L_Transmit')
a={x.get_actor_label():x for x in E.get_all_level_actors()}
out=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence/Boss';out.mkdir(parents=True,exist_ok=True)
changed=[]
def move(n,p):
 a[n].set_actor_location(V(*p),False,False);changed.append(n)
def hide(n):
 a[n].set_actor_hidden_in_game(True);a[n].set_actor_enable_collision(False);changed.append(n)
# Open an arena rather than the old fixed-axis chute. Preserve the outer shell.
for n in a:
 if n in ['Weaponize_EntryBaffle','Weaponize_FarBaffle','Weaponize_DashStop'] or n.startswith(('Weaponize_DashEdge','Weaponize_DashAxis','Weaponize_RamRail','Flow_RamStance','Flow_ArmLink')):
  hide(n)
# Meet the loop foot at x=-1150 and Learn bank at x=-400; no coplanar overlap.
move('Pacing_LearnOriginalConnection',(-775,0,-60))
a['Pacing_LearnOriginalConnection'].set_actor_scale3d(V(7.5,7,1.2))
move('Weaponize_Charger',(7980,2530,100))
move('Weaponize_Ram',(7100,2530,100))
ram=a['Weaponize_Ram'];ram.set_editor_property('fixed_axis',V(1,0,0));ram.set_editor_property('impact_distance',1100)
ram.set_editor_property('rail_half_span',550);ram.set_editor_property('rail_speed',180);ram.set_editor_property('impact_radius',250)
boss=a['Weaponize_Charger'];boss.set_editor_property('dash_speed',1100)
state=boss.state_machine
for k,v in {'idle_duration_seconds':1.6,'telegraph_duration_seconds':1.2,'dash_duration_seconds':1.9,'recovery_duration_seconds':6.0,'dash_commit_window_delay_seconds':.05}.items():state.set_editor_property(k,v)
move('Flow_RamOperatingMarker',(6800,2530,100))
# Explicit map-local dynamic sun scope; existing atmosphere uses this sun.
sun=a['Light_Sun'];sun.set_editor_property('tags',list(sun.tags)+[unreal.Name('Transmit.ProgressSun')] if 'Transmit.ProgressSun' not in [str(t) for t in sun.tags] else list(sun.tags))
sun.root_component.set_mobility(unreal.ComponentMobility.MOVABLE);changed.append('Light_Sun')
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
mat=unreal.load_asset('/Game/Transmit/Presentation/Materials/M_Inlay')
def rail(n,p,size):
 actor=a.get(n)
 if actor is None:
  actor=E.spawn_actor_from_class(unreal.StaticMeshActor,V(*p));actor.set_actor_label(n);a[n]=actor
 actor.set_actor_location(V(*p),False,False);actor.set_actor_scale3d(V(*(x/100 for x in size)))
 actor.static_mesh_component.set_static_mesh(cube);actor.static_mesh_component.set_material(0,mat)
 actor.set_actor_enable_collision(False);actor.set_folder_path('Boss/Rail');changed.append(n)
for x in [7020,7180]:rail('Boss_Rail_'+str(x),(x,2530,5),(12,1180,8))
for y in [1980,2530,3080]:rail('Boss_RailStop_'+str(y),(7100,y,6),(220,14,10))
rail('Boss_HomeRingFront',(7840,2530,7),(12,360,10))
# Existing paper-thin floor finish panels were sub-centimetre coplanar surfaces.
# Lift within a tiny non-colliding finish allowance and suppress intersecting duplicate panels.
rects=[];repairs=[]
for n,actor in sorted(a.items()):
 if not (n.startswith('Presentation_Finish_') and '_Panel_' in n):continue
 p=actor.get_actor_location();sc=actor.get_actor_scale3d();rot=actor.get_actor_rotation()
 if abs(rot.pitch)>.1 or abs(rot.roll)>.1 or sc.z>.03:continue
 box=(p.x-sc.x*50,p.y-sc.y*50,p.x+sc.x*50,p.y+sc.y*50)
 duplicates=[name for name,z,r in rects if abs(z-p.z)<2 and min(box[2],r[2])-max(box[0],r[0])>2 and min(box[3],r[3])-max(box[1],r[1])>2]
 if duplicates:hide(n);repairs.append({'hidden':n,'overlap':duplicates});continue
 rects.append((n,p.z,box))
 # Absolute z from the prior finish baseline, making reauthoring idempotent.
 base=json.loads((out.parent/'Vertical/baseline.json').read_text()).get(n)
 if base:actor.set_actor_location(V(p.x,p.y,base[2]+1.6),False,False);changed.append(n)
# Retire obsolete English instruction plaques; the new Chinese HUD is authoritative.
for n,actor in a.items():
 if isinstance(actor,unreal.TextRenderActor) and (n.startswith(('Flow_','Pacing_')) or 'Instruction' in n or 'Hint' in n):hide(n)
L.save_current_level()
(out/'authoring.json').write_text(json.dumps({'map':'/Game/Transmit/Maps/L_Transmit','changed':sorted(set(changed)),'overlap_repairs':repairs},indent=2))
unreal.log('TRANSMIT_BOSS_MAP_SAVED')
