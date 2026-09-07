"""Save only BP_TransmitCharacter's existing CameraBoom shoulder composition."""
import unreal
assert not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).is_in_play_in_editor()
blueprint = unreal.load_asset('/Game/Transmit/Blueprints/BP_TransmitCharacter')
subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
arms = []
for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
 data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
 obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
 if isinstance(obj, unreal.SpringArmComponent): arms.append(obj)
assert len(arms) == 1 and abs(arms[0].target_arm_length - 400) < .01
arms[0].set_editor_property('socket_offset', unreal.Vector(0,70,50))
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
assert blueprint.get_editor_property('status') == unreal.BlueprintStatus.BS_UP_TO_DATE
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
unreal.log('TRANSMIT_CAMERA_SAVED socket=(0,70,50); existing boom length and camera FOV retained')
