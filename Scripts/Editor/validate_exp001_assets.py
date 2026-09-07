import unreal


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def log(message):
    unreal.log(f"EXP001_VALIDATE {message}")


def find_blueprint_component(blueprint, component_class):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        associated = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if (
            isinstance(associated, unreal.ActorComponent)
            and unreal.MathLibrary.class_is_child_of(
                associated.get_class(),
                component_class,
            )
        ):
            return associated
    return None


blueprint_paths = (
    "/Game/Transmit/Blueprints/BP_LinearReceiver",
    "/Game/Transmit/Blueprints/BP_LinearSource",
    "/Game/Transmit/Blueprints/BP_TransmitCharacter",
    "/Game/Transmit/Blueprints/BP_TransmitGameMode",
    "/Game/Transmit/Blueprints/BP_TransmitPlayerController",
)

blueprints = {}
for path in blueprint_paths:
    blueprint = unreal.load_asset(path)
    require(isinstance(blueprint, unreal.Blueprint), f"Missing Blueprint: {path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    require(
        blueprint.get_editor_property("status")
        in (
            unreal.BlueprintStatus.BS_UP_TO_DATE,
            unreal.BlueprintStatus.BS_UP_TO_DATE_WITH_WARNINGS,
        ),
        f"Blueprint compile failed: {path}",
    )
    blueprints[path] = blueprint
    log(f"BLUEPRINT_OK {path}")

character = blueprints["/Game/Transmit/Blueprints/BP_TransmitCharacter"]
require(
    unreal.BlueprintEditorLibrary.get_blueprint_parent_class(character)
    == unreal.TransmitCharacter.static_class(),
    "BP_TransmitCharacter has the wrong native parent",
)
character_cdo = unreal.get_default_object(character.generated_class())
require(
    find_blueprint_component(character, unreal.MotionTransferComponent.static_class()) is not None,
    "BP_TransmitCharacter has no MotionTransferComponent",
)
require(
    find_blueprint_component(character, unreal.MotionInteractorComponent.static_class()) is not None,
    "BP_TransmitCharacter has no MotionInteractorComponent",
)
require(
    find_blueprint_component(character, unreal.MotionCarryIndicatorComponent.static_class()) is not None,
    "BP_TransmitCharacter has no owned carry indicator",
)
require(
    find_blueprint_component(character, unreal.MotionDirectionIndicatorComponent.static_class()) is not None,
    "BP_TransmitCharacter has no owned direction indicator",
)

source = blueprints["/Game/Transmit/Blueprints/BP_LinearSource"]
source_cdo = unreal.get_default_object(source.generated_class())
source_motion = source_cdo.get_editor_property("motion")
require(source_motion.get_editor_property("starts_with_motion"), "Source must start with Motion")
require(source_motion.get_editor_property("can_provide_motion"), "Source must provide Motion")
source_state = source_motion.get_editor_property("initial_motion")
require(str(source_state.get_editor_property("source_id")) == "Source.Linear.001", "SourceId drift")
require(abs(source_state.get_editor_property("magnitude") - 600.0) < 0.01, "Magnitude drift")

receiver = blueprints["/Game/Transmit/Blueprints/BP_LinearReceiver"]
receiver_cdo = unreal.get_default_object(receiver.generated_class())
receiver_motion = receiver_cdo.get_editor_property("motion")
require(receiver_motion.get_editor_property("can_receive_motion"), "Receiver must receive Motion")
require(
    receiver_motion.get_editor_property("required_canonical_direction")
    == unreal.MotionCanonicalDirection.FORWARD,
    "Receiver canonical direction requirement drift",
)
require(
    receiver_motion.get_editor_property("endpoint_mode")
    == unreal.MotionEndpointMode.CONSUME_ON_RECEIVE,
    "Receiver must consume Motion",
)

input_context = unreal.load_asset("/Game/Transmit/Input/IMC_Transmit")
require(input_context is not None, "Missing IMC_Transmit")
mapping_data = input_context.get_editor_property("default_key_mappings")
mappings = mapping_data.get_editor_property("mappings")
actual_keys = {
    str(mapping.get_editor_property("action").get_name()): str(
        mapping.get_editor_property("key").get_editor_property("key_name")
    )
    for mapping in mappings
}
require(
    actual_keys
    == {
        "IA_MotionCapture": "E",
        "IA_MotionTransfer": "Q",
        "IA_RoomReset": "R",
    },
    f"Unexpected input mappings: {actual_keys}",
)

world = unreal.EditorLoadingAndSavingUtils.load_map("/Game/Transmit/Maps/L_TestChamber")
require(world is not None, "Could not load L_TestChamber")
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actor_labels = {actor.get_actor_label() for actor in actor_subsystem.get_all_level_actors()}
required_actor_labels = (
    "PlayerStart_EXP001",
    "Source_Linear_001",
    "Receiver_Linear_001",
    "RoomReset_EXP001",
)
for expected_label in required_actor_labels:
    require(expected_label in actor_labels, f"Missing map actor: {expected_label}")

game_mode = blueprints["/Game/Transmit/Blueprints/BP_TransmitGameMode"]
require(
    world.get_world_settings().get_editor_property("default_game_mode")
    == game_mode.generated_class(),
    "L_TestChamber does not override BP_TransmitGameMode",
)

reset_actors = unreal.GameplayStatics.get_all_actors_of_class(
    world,
    unreal.MotionRoomResetController.static_class(),
)
require(len(reset_actors) == 1, "L_TestChamber must contain exactly one Room Reset controller")
require(
    reset_actors[0].get_editor_property("auto_discover_transferable_participants"),
    "Room Reset auto-discovery must be enabled in L_TestChamber",
)

log("SUCCESS")
