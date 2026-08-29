import unreal


def report(name, value):
    unreal.log(f"EXP001_INSPECT {name}: {value}")


for class_name in (
    "MotionTransferComponent",
    "MotionInteractorComponent",
    "MotionCarryIndicatorComponent",
    "TransmitPlayerController",
    "TransmitMotionEndpointActor",
    "MotionRoomResetController",
    "BlueprintFactory",
    "InputActionFactory",
    "InputMappingContext",
    "EnhancedActionKeyMapping",
    "InputMappingContextMappingData",
):
    report(class_name, getattr(unreal, class_name, None))

for api_name in (
    "EditorAssetLibrary",
    "AssetToolsHelpers",
    "BlueprintEditorLibrary",
    "SubobjectDataSubsystem",
    "EditorLevelLibrary",
    "EditorLoadingAndSavingUtils",
    "LevelEditorSubsystem",
):
    api = getattr(unreal, api_name, None)
    report(api_name, api)
    if api:
        methods = [
            entry
            for entry in dir(api)
            if any(token in entry.lower() for token in ("level", "map", "subobject", "save", "blueprint"))
        ]
        report(f"{api_name}.methods", methods)

for asset_path in (
    "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter",
    "/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController",
    "/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode",
    "/Game/Input/IMC_Default",
):
    asset = unreal.load_asset(asset_path)
    report(asset_path, asset)
    if isinstance(asset, unreal.Blueprint):
        generated_class = asset.generated_class()
        report(f"{asset_path}.generated_class", generated_class)
        report(f"{asset_path}.cdo", unreal.get_default_object(generated_class))

report(
    "EditorActorSubsystem.methods",
    [
        entry
        for entry in dir(unreal.EditorActorSubsystem)
        if any(token in entry.lower() for token in ("spawn", "actor", "destroy"))
    ],
)

for documented in (
    unreal.Key,
    unreal.EnhancedActionKeyMapping,
    unreal.InputMappingContextMappingData,
    unreal.EditorLoadingAndSavingUtils.new_blank_map,
    unreal.EditorLoadingAndSavingUtils.save_map,
    unreal.EditorActorSubsystem.spawn_actor_from_class,
):
    report(f"doc.{documented}", documented.__doc__)

report("Key.fields", dir(unreal.Key()))
report("EnhancedActionKeyMapping.fields", dir(unreal.EnhancedActionKeyMapping()))
report("InputMappingContextMappingData.fields", dir(unreal.InputMappingContextMappingData()))

for asset_path in (
    "/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode",
    "/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController",
):
    blueprint = unreal.load_asset(asset_path)
    cdo = unreal.get_default_object(blueprint.generated_class())
    for property_name in (
        "default_pawn_class",
        "player_controller_class",
        "default_mapping_contexts",
        "mobile_excluded_mapping_contexts",
    ):
        try:
            report(f"{asset_path}.{property_name}", cdo.get_editor_property(property_name))
        except Exception as error:
            report(f"{asset_path}.{property_name}.error", error)
