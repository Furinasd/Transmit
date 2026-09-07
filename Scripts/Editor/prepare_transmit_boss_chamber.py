"""Prepare unsaved L_TestChamber fixtures; never save this test layout.
Then start PIE and run validate_transmit_boss_chamber.py. Stop PIE and reload
L_TestChamber without saving to discard only these task-created fixtures.
"""
import unreal
E=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
L=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert not L.is_in_play_in_editor()
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'Preserve existing edits'
assert L.load_level('/Game/Transmit/Maps/L_TestChamber')
V=unreal.Vector
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
def spawn(cls,name,pos):
 a=E.spawn_actor_from_class(cls,V(*pos));a.set_actor_label(name);return a
floor=spawn(unreal.StaticMeshActor,'BossTest_Floor',(0,3500,-60))
floor.static_mesh_component.set_static_mesh(cube);floor.set_actor_scale3d(V(45,35,1.2))
carrier=spawn(unreal.TransmitDirectionalCarrierActor,'BossTest_Carrier',(-900,3500,100))
boss=spawn(unreal.TransmitArenaCharger,'BossTest_Boss',(800,3500,100))
for k,v in {'idle_duration_seconds':1.6,'telegraph_duration_seconds':1.2,'dash_duration_seconds':1.9,'recovery_duration_seconds':6.0,'dash_commit_window_delay_seconds':.05}.items():boss.state_machine.set_editor_property(k,v)
boss.set_editor_property('dash_speed',1100)
gate=spawn(unreal.StaticMeshActor,'BossTest_Gate',(980,3500,350))
gate.static_mesh_component.set_static_mesh(cube);gate.set_actor_scale3d(V(1.2,6.2,7));gate.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
ram=spawn(unreal.TransmitRam,'BossTest_Ram',(-100,3500,100))
ram.set_editor_property('gate',gate);ram.set_editor_property('route_carrier',carrier);ram.set_editor_property('dock_marker',carrier);ram.set_editor_property('impact_distance',1100)
