"""Explicit guards for editor automation; no asset saves or process signals.

These mitigate lifecycle hazards, not the unproven ICU shutdown root cause.
"""
import unreal


def require_clean_editor():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if levels.is_in_play_in_editor():
        raise RuntimeError('Stop PIE before changing maps or exiting')
    saving = unreal.EditorLoadingAndSavingUtils
    if saving.get_dirty_map_packages() or saving.get_dirty_content_packages():
        raise RuntimeError('Preserve unsaved assets: save or discard explicitly in Editor')
    return levels


def load_level_checked(package):
    """Do not clear caller references or force GC inside a Slate callback."""
    levels = require_clean_editor()
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = editor.get_editor_world()
    if world and world.get_path_name().split('.')[0] == package:
        return world
    world = None
    if not levels.load_level(package):
        raise RuntimeError('Could not load ' + package)
    world = editor.get_editor_world()
    if not world or world.get_path_name().split('.')[0] != package:
        raise RuntimeError('Loaded world does not match ' + package)
    return world


class CallbackOwner:
    """Own only this suite's callbacks and optional command-line keep-alive.

    Close is idempotent. Unregistration errors propagate and retain the handle
    for diagnosis. Never unregister callbacks belonging to another session.
    """
    def __init__(self, keep_alive=False):
        self.handles = []
        self.keep_alive = keep_alive
        if keep_alive:
            unreal.EditorPythonScripting.set_keep_python_script_alive(True)

    def register(self, callback):
        handle = unreal.register_slate_post_tick_callback(callback)
        self.handles.append(handle)
        return handle

    def close(self):
        while self.handles:
            unreal.unregister_slate_post_tick_callback(self.handles[-1])
            self.handles.pop()
        if self.keep_alive:
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
            self.keep_alive = False
