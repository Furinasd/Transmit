import unreal

from toolset_registry.helpers import compile_blueprint, create_asset


ASSET_ROOT = "/Game/Transmit"
BLUEPRINT_ROOT = f"{ASSET_ROOT}/Blueprints"
INPUT_ROOT = f"{ASSET_ROOT}/Input"
MAP_ROOT = f"{ASSET_ROOT}/Maps"


def log(message):
    unreal.log(f"EXP001_GENERATE {message}")


def load_or_create_data_asset(folder, name, asset_class):
    path = f"{folder}/{name}"
    existing = unreal.load_asset(path)
    if existing:
        return existing
    return create_asset(folder, name, asset_class, unreal.DataAssetFactory())


def load_or_create_blueprint(folder, name, parent_class):
    path = f"{folder}/{name}"
    existing = unreal.load_asset(path)
    if existing:
        if not isinstance(existing, unreal.Blueprint):
            raise RuntimeError(f"{path} exists but is not a Blueprint")
        return existing

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    return create_asset(folder, name, unreal.Blueprint.static_class(), factory)


def blueprint_default_object(blueprint):
    compile_blueprint(blueprint)
    return unreal.get_default_object(blueprint.generated_class())


def find_or_add_blueprint_component(blueprint, component_class, component_name):
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
    actor_handle = None
    root_handle = None

    for handle in handles:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        associated = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
        if isinstance(associated, unreal.ActorComponent) and associated.get_name() == component_name:
            return associated
        if unreal.SubobjectDataBlueprintFunctionLibrary.is_actor(data):
            actor_handle = handle
        if unreal.SubobjectDataBlueprintFunctionLibrary.is_root_component(data):
            root_handle = handle

    is_scene_component = unreal.MathLibrary.class_is_child_of(
        component_class,
        unreal.SceneComponent.static_class(),
    )
    parent_handle = root_handle if is_scene_component else actor_handle
    if not parent_handle:
        raise RuntimeError(f"Could not resolve a component parent for {blueprint}")

    handle, failure_reason = subsystem.add_new_subobject(
        unreal.AddNewSubobjectParams(
            parent_handle=parent_handle,
            new_class=component_class,
            blueprint_context=blueprint,
        )
    )
    if not failure_reason.is_empty():
        raise RuntimeError(str(failure_reason))
    subsystem.rename_subobject(handle=handle, new_name=unreal.Text(component_name))

    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    component = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
    if not isinstance(component, unreal.ActorComponent):
        raise RuntimeError(f"Failed to add {component_name} to {blueprint}")
    return component


def configure_input_assets():
    capture = load_or_create_data_asset(INPUT_ROOT, "IA_MotionCapture", unreal.InputAction.static_class())
    transfer = load_or_create_data_asset(INPUT_ROOT, "IA_MotionTransfer", unreal.InputAction.static_class())
    reset = load_or_create_data_asset(INPUT_ROOT, "IA_RoomReset", unreal.InputAction.static_class())
    context = load_or_create_data_asset(
        INPUT_ROOT,
        "IMC_Transmit",
        unreal.InputMappingContext.static_class(),
    )

    mappings = []
    for action, key_name in ((capture, "E"), (transfer, "Q"), (reset, "R")):
        mapping = unreal.EnhancedActionKeyMapping()
        mapping.set_editor_property("action", action)
        key = unreal.Key()
        key.set_editor_property("key_name", unreal.Name(key_name))
        mapping.set_editor_property("key", key)
        mappings.append(mapping)

    mapping_data = unreal.InputMappingContextMappingData()
    mapping_data.set_editor_property("mappings", mappings)
    context.set_editor_property("default_key_mappings", mapping_data)
    return capture, transfer, reset, context


def configure_character_blueprint():
    path = f"{BLUEPRINT_ROOT}/BP_TransmitCharacter"
    blueprint = unreal.load_asset(path)
    if not blueprint:
        blueprint = unreal.EditorAssetLibrary.duplicate_asset(
            "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter",
            path,
        )
    if not isinstance(blueprint, unreal.Blueprint):
        raise RuntimeError("Failed to duplicate BP_ThirdPersonCharacter")

    unreal.BlueprintEditorLibrary.reparent_blueprint(
        blueprint,
        unreal.TransmitCharacter.static_class(),
    )
    compile_blueprint(blueprint)

    motion = find_or_add_blueprint_component(
        blueprint,
        unreal.MotionTransferComponent.static_class(),
        "Motion",
    )
    motion.set_editor_property("participant_id", unreal.Name("Player"))
    motion.set_editor_property("starts_with_motion", False)
    motion.set_editor_property("can_provide_motion", False)
    motion.set_editor_property("can_receive_motion", True)
    motion.set_editor_property("endpoint_mode", unreal.MotionEndpointMode.STORE)

    find_or_add_blueprint_component(
        blueprint,
        unreal.MotionInteractorComponent.static_class(),
        "MotionInteractor",
    )
    carry_indicator = find_or_add_blueprint_component(
        blueprint,
        unreal.MotionCarryIndicatorComponent.static_class(),
        "MotionCarryIndicator",
    )
    carry_indicator.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, 90.0))

    compile_blueprint(blueprint)
    return blueprint


def configure_endpoint_blueprints():
    source = load_or_create_blueprint(
        BLUEPRINT_ROOT,
        "BP_LinearSource",
        unreal.TransmitMotionEndpointActor.static_class(),
    )
    source_cdo = blueprint_default_object(source)
    source_motion = source_cdo.get_editor_property("motion")
    source_motion.set_editor_property("participant_id", unreal.Name("Source.Linear.001"))
    source_motion.set_editor_property("starts_with_motion", True)
    source_motion.set_editor_property("can_provide_motion", True)
    source_motion.set_editor_property("can_receive_motion", False)
    source_motion.set_editor_property("endpoint_mode", unreal.MotionEndpointMode.STORE)
    source_state = unreal.MotionState()
    source_state.set_editor_property("direction", unreal.Vector(1.0, 0.0, 0.0))
    source_state.set_editor_property("magnitude", 600.0)
    source_state.set_editor_property("source_id", unreal.Name("Source.Linear.001"))
    source_motion.set_editor_property("initial_motion", source_state)
    source_cdo.get_editor_property("motion_indicator").set_editor_property(
        "light_color",
        unreal.Color(0, 220, 255, 255),
    )
    compile_blueprint(source)

    receiver = load_or_create_blueprint(
        BLUEPRINT_ROOT,
        "BP_LinearReceiver",
        unreal.TransmitMotionEndpointActor.static_class(),
    )
    receiver_cdo = blueprint_default_object(receiver)
    receiver_motion = receiver_cdo.get_editor_property("motion")
    receiver_motion.set_editor_property("participant_id", unreal.Name("Receiver.Linear.001"))
    receiver_motion.set_editor_property("starts_with_motion", False)
    receiver_motion.set_editor_property("can_provide_motion", False)
    receiver_motion.set_editor_property("can_receive_motion", True)
    receiver_motion.set_editor_property(
        "endpoint_mode",
        unreal.MotionEndpointMode.CONSUME_ON_RECEIVE,
    )
    receiver_cdo.get_editor_property("motion_indicator").set_editor_property(
        "light_color",
        unreal.Color(80, 255, 80, 255),
    )
    compile_blueprint(receiver)
    return source, receiver


def configure_controller_blueprint(capture, transfer, reset, transmit_context):
    controller = load_or_create_blueprint(
        BLUEPRINT_ROOT,
        "BP_TransmitPlayerController",
        unreal.TransmitPlayerController.static_class(),
    )
    cdo = blueprint_default_object(controller)
    cdo.set_editor_property(
        "mapping_contexts",
        [
            unreal.load_asset("/Game/Input/IMC_Default"),
            unreal.load_asset("/Game/Input/IMC_MouseLook"),
            transmit_context,
        ],
    )
    cdo.set_editor_property("capture_action", capture)
    cdo.set_editor_property("transfer_action", transfer)
    cdo.set_editor_property("reset_action", reset)
    compile_blueprint(controller)
    return controller


def configure_game_mode_blueprint(character, controller):
    game_mode = load_or_create_blueprint(
        BLUEPRINT_ROOT,
        "BP_TransmitGameMode",
        unreal.GameModeBase.static_class(),
    )
    cdo = blueprint_default_object(game_mode)
    cdo.set_editor_property("default_pawn_class", character.generated_class())
    cdo.set_editor_property("player_controller_class", controller.generated_class())
    compile_blueprint(game_mode)
    return game_mode


def save_assets(assets):
    for asset in assets:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError(f"Failed to save {asset.get_path_name()}")


def spawn_static_mesh(actor_subsystem, mesh, label, location, scale):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor,
        location,
        unreal.Rotator(),
    )
    actor.set_actor_label(label)
    actor.set_actor_scale3d(scale)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(mesh)
    return actor


def create_test_map(source, receiver, game_mode):
    map_path = f"{MAP_ROOT}/L_TestChamber"
    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        log(f"Map already exists; preserved {map_path}")
        return

    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(True)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube")

    spawn_static_mesh(
        actor_subsystem,
        cube,
        "TestChamber_Floor",
        unreal.Vector(400.0, 0.0, -50.0),
        unreal.Vector(18.0, 12.0, 0.5),
    )
    spawn_static_mesh(
        actor_subsystem,
        cube,
        "Occlusion_TestWall",
        unreal.Vector(400.0, -500.0, 150.0),
        unreal.Vector(0.5, 2.0, 2.0),
    )

    player_start = actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(-650.0, 0.0, 100.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    player_start.set_actor_label("PlayerStart_EXP001")

    source_actor = actor_subsystem.spawn_actor_from_class(
        source.generated_class(),
        unreal.Vector(0.0, 0.0, 100.0),
        unreal.Rotator(),
    )
    source_actor.set_actor_label("Source_Linear_001")

    receiver_actor = actor_subsystem.spawn_actor_from_class(
        receiver.generated_class(),
        unreal.Vector(900.0, 250.0, 100.0),
        unreal.Rotator(),
    )
    receiver_actor.set_actor_label("Receiver_Linear_001")

    reset_actor = actor_subsystem.spawn_actor_from_class(
        unreal.MotionRoomResetController,
        unreal.Vector(400.0, 0.0, 0.0),
        unreal.Rotator(),
    )
    reset_actor.set_actor_label("RoomReset_EXP001")
    reset_actor.set_editor_property("auto_discover_transferable_participants", True)

    directional_light = actor_subsystem.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 600.0),
        unreal.Rotator(-45.0, -30.0, 0.0),
    )
    directional_light.set_actor_label("KeyLight_EXP001")
    sky_light = actor_subsystem.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(),
    )
    sky_light.set_actor_label("SkyLight_EXP001")

    world.get_world_settings().set_editor_property(
        "default_game_mode",
        game_mode.generated_class(),
    )
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, map_path):
        raise RuntimeError(f"Failed to save {map_path}")


def main():
    capture, transfer, reset, transmit_context = configure_input_assets()
    character = configure_character_blueprint()
    source, receiver = configure_endpoint_blueprints()
    controller = configure_controller_blueprint(capture, transfer, reset, transmit_context)
    game_mode = configure_game_mode_blueprint(character, controller)

    save_assets(
        [
            capture,
            transfer,
            reset,
            transmit_context,
            character,
            source,
            receiver,
            controller,
            game_mode,
        ]
    )
    create_test_map(source, receiver, game_mode)
    log("SUCCESS")


main()
