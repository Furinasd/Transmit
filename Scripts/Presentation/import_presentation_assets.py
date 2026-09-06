"""Narrow, create-only Transmit presentation import for Unreal Editor Python.

No work runs on import. Call run(category="materials") OR run(category="audio")
as separate reviewable transactions. Each call saves and reports its new assets.
Inspect the saved batch before invoking the other category. No map, world,
existing asset, or project configuration is modified.

Example via runpy in the Editor:
    scope = runpy.run_path(".../Scripts/Presentation/import_presentation_assets.py")
    report = scope["run"](category="materials")
"""

import json
from pathlib import Path
import wave

import unreal


DEFAULT_PROJECT_ROOT = "/Users/ely/.codex/worktrees/0155/passely"
DESTINATION = "/Game/Transmit/Presentation"
AUDIO_NAMES = (
    "Capture", "Transfer", "Dock", "Telegraph", "Intercept",
    "RamImpact1", "RamImpact2", "Complete",
)
MATERIALS = {
    "M_Ceramic": ((0.62, 0.65, 0.65), 0.58, 0.0, 0.0),
    "M_Graphite": ((0.045, 0.07, 0.09), 0.48, 0.35, 0.0),
    "M_Motion": ((0.025, 0.65, 0.85), 0.40, 0.0, 2.0),
    "M_Impact": ((1.0, 0.24, 0.025), 0.40, 0.0, 2.0),
    "M_Inlay": ((0.2, 0.3, 0.34), 0.50, 0.0, 0.0),
}


def _log(message):
    unreal.log("[TransmitPresentationImport] " + message)


def _existing(path, expected_class):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        return False
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not isinstance(asset, expected_class):
        raise RuntimeError("Existing asset has unexpected type; untouched: " + path)
    _log("SKIP existing (untouched): " + path)
    return True


def _expression(material, expression_class, x, y):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y)
    if expression is None:
        raise RuntimeError("Could not create expression on " + material.get_path_name())
    return expression


def _connect(node, output, property_name):
    if not unreal.MaterialEditingLibrary.connect_material_property(node, output, property_name):
        raise RuntimeError("Could not connect material property " + str(property_name))


def _create_material(name, recipe, report):
    folder = DESTINATION + "/Materials"
    path = folder + "/" + name
    if _existing(path, unreal.Material):
        report["skipped"].append(path)
        return
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError("Asset creation failed: " + path)
    report["created"].append(path)
    _log("CREATED: " + path)
    if name in ("M_Motion", "M_Impact", "M_Inlay", "M_Graphite"):
        material.set_editor_property("used_with_instanced_static_meshes", True)
    rgb, roughness, metallic, emissive = recipe
    color = _expression(material, unreal.MaterialExpressionConstant3Vector, -480, 0)
    color.set_editor_property("constant", unreal.LinearColor(*rgb, 1.0))
    _connect(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for value, property_name, y in (
        (roughness, unreal.MaterialProperty.MP_ROUGHNESS, 180),
        (metallic, unreal.MaterialProperty.MP_METALLIC, 300),
    ):
        scalar = _expression(material, unreal.MaterialExpressionConstant, -240, y)
        scalar.set_editor_property("r", value)
        _connect(scalar, "", property_name)
    if emissive:
        glow = _expression(material, unreal.MaterialExpressionConstant3Vector, -480, 420)
        glow.set_editor_property("constant", unreal.LinearColor(
            rgb[0] * emissive, rgb[1] * emissive, rgb[2] * emissive, 1.0))
        _connect(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError("Save failed; new unsaved asset retained: " + path)
    report["saved"].append(path)
    _log("SAVED: " + path)


def _import_audio(source_root, report):
    folder = DESTINATION + "/Audio"
    pending = []
    # Validate the complete requested source batch before the first binary write.
    for event in AUDIO_NAMES:
        name = "S_Transmit_" + event
        path = folder + "/" + name
        if _existing(path, unreal.SoundWave):
            report["skipped"].append(path)
            continue
        source = source_root / (name + ".wav")
        if not source.is_file():
            raise RuntimeError("Missing original source: " + str(source))
        with wave.open(str(source), "rb") as stream:
            if (stream.getnchannels(), stream.getsampwidth(), stream.getframerate()) != (1, 2, 48000):
                raise RuntimeError("Expected 48 kHz mono PCM16 source: " + str(source))
        pending.append((name, path, source))
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, path, source in pending:
        # Repeat the existence guard immediately before this import.
        if _existing(path, unreal.SoundWave):
            report["skipped"].append(path)
            continue
        task = unreal.AssetImportTask()
        for key, value in {
            "filename": str(source.resolve()), "destination_path": folder,
            "destination_name": name, "automated": True,
            "replace_existing": False, "replace_existing_settings": False,
            "save": False,
        }.items():
            task.set_editor_property(key, value)
        asset_tools.import_asset_tasks([task])
        imported = list(task.get_editor_property("imported_object_paths"))
        report["imports"].append({"source": str(source.resolve()), "objects": imported})
        _log("IMPORT: " + json.dumps(report["imports"][-1]))
        expected_object = path + "." + name
        if imported != [expected_object]:
            raise RuntimeError("Unexpected import result; inspect retained assets: " + repr(imported))
        report["created"].append(path)
        sound = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(sound, unreal.SoundWave):
            raise RuntimeError("Imported asset is not a SoundWave: " + path)
        if not unreal.EditorAssetLibrary.save_loaded_asset(sound, only_if_is_dirty=False):
            raise RuntimeError("Save failed; new unsaved sound retained: " + path)
        report["saved"].append(path)
        _log("SAVED: " + path)


def run(allowed_project_root=DEFAULT_PROJECT_ROOT, category="materials"):
    """Create one category, save, return exact writes; never touches existing assets.

    To integrate in A's checkout, explicitly pass its project root. Sources are
    resolved from this script's checkout, so import metadata records the exact
    original WAV path. Source files must travel with the integration commit.
    """
    expected = Path(allowed_project_root).expanduser().resolve()
    actual = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    if actual != expected:
        raise RuntimeError("Project guard refused writes: actual=" + str(actual)
                           + "; allowed=" + str(expected))
    if category not in ("materials", "audio"):
        raise ValueError("Choose a single transaction category: materials or audio")
    report = {"project_root": str(actual), "category": category,
              "created": [], "saved": [], "skipped": [], "imports": []}
    _log("BEGIN create-only " + category + "; protected: maps, config, existing assets")
    try:
        if category == "materials":
            # Preflight all collisions before creating any material.
            for name in MATERIALS:
                _existing(DESTINATION + "/Materials/" + name, unreal.Material)
            for name, recipe in MATERIALS.items():
                _create_material(name, recipe, report)
        else:
            source_root = Path(__file__).resolve().parents[2] / "ContentSource/Presentation/Audio"
            _import_audio(source_root, report)
    except Exception:
        unreal.log_error("[TransmitPresentationImport] PARTIAL RESULT; no cleanup attempted: "
                         + json.dumps(report, sort_keys=True))
        raise
    _log("FINISHED; stop and inspect this saved batch: " + json.dumps(report, sort_keys=True))
    return report
