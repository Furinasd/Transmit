"""Host-side lifecycle regression tests; not a substitute for Editor smoke."""
import importlib.util
import pathlib
import sys
import unittest
import tempfile
import json
import time
from unittest.mock import Mock, patch


class SafetyTests(unittest.TestCase):
    def setUp(self):
        self.ue = Mock()
        self.levels = Mock()
        self.editor = Mock()
        self.ue.get_editor_subsystem.side_effect = lambda kind: (
            self.levels if kind == self.ue.LevelEditorSubsystem else self.editor)
        self.levels.is_in_play_in_editor.return_value = False
        self.ue.EditorLoadingAndSavingUtils.get_dirty_map_packages.return_value = []
        self.ue.EditorLoadingAndSavingUtils.get_dirty_content_packages.return_value = []
        self.editor.get_editor_world.return_value.get_path_name.return_value = '/Game/A.A'
        spec = importlib.util.spec_from_file_location('safety_test', pathlib.Path(__file__).with_name('transmit_editor_safety.py'))
        self.safety = importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules, unreal=self.ue):
            spec.loader.exec_module(self.safety)

    def test_same_map_does_not_reload(self):
        self.safety.load_level_checked('/Game/A')
        self.levels.load_level.assert_not_called()

    def test_pie_and_dirty_assets_reject_without_loading(self):
        for source in [self.levels.is_in_play_in_editor,
                       self.ue.EditorLoadingAndSavingUtils.get_dirty_map_packages,
                       self.ue.EditorLoadingAndSavingUtils.get_dirty_content_packages]:
            source.return_value = True
            with self.assertRaises(RuntimeError):
                self.safety.load_level_checked('/Game/B')
            source.return_value = False
        self.levels.load_level.assert_not_called()

    def test_wrong_loaded_world_is_failure(self):
        with self.assertRaises(RuntimeError):
            self.safety.load_level_checked('/Game/B')

    def test_close_once_and_only_owned_handle(self):
        owner = self.safety.CallbackOwner(True)
        handle = owner.register(lambda dt: None)
        owner.close()
        owner.close()
        self.ue.unregister_slate_post_tick_callback.assert_called_once_with(handle)
        self.assertEqual([c.args[0] for c in self.ue.EditorPythonScripting.set_keep_python_script_alive.call_args_list], [True, False])

    def test_unregister_failure_retains_handle_and_keep_alive(self):
        owner = self.safety.CallbackOwner(True)
        handle = owner.register(lambda dt: None)
        self.ue.unregister_slate_post_tick_callback.side_effect = RuntimeError('failed')
        with self.assertRaises(RuntimeError):
            owner.close()
        self.assertEqual(owner.handles, [handle])
        self.assertTrue(owner.keep_alive)

    def test_destroyed_pie_persists_failed_result(self):
        import ast
        source = pathlib.Path(__file__).with_name('validate_ltransmit_pie.py')
        tree = ast.parse(source.read_text())
        tree.body = [node for node in tree.body if isinstance(node, ast.ClassDef)]
        scope = dict(unreal=self.ue, pathlib=pathlib, json=json, time=time)
        exec(compile(tree, str(source), 'exec'), scope)
        runner = scope['TransmitRun'].__new__(scope['TransmitRun'])
        runner.done = False
        runner.handle = 42
        runner.rows = []
        runner.p = Mock()
        runner.p.character_movement.stop_movement_immediately.side_effect = RuntimeError('destroyed Pawn')
        runner.now = Mock(side_effect=RuntimeError('destroyed World'))
        runner.start = 0
        runner.real_start = time.monotonic()
        with tempfile.TemporaryDirectory() as directory:
            self.ue.Paths.project_saved_dir.return_value = directory
            runner.finish(True, 'stopped')
            runner.finish(True, 'duplicate')
            result = json.loads(next(pathlib.Path(directory).rglob('run-*.json')).read_text())
        self.assertEqual(len(result), 1)
        self.assertFalse(result[0]['ok'])
        self.assertIsNone(result[0]['game_seconds'])
        self.ue.unregister_slate_post_tick_callback.assert_called_once_with(42)


if __name__ == '__main__':
    unittest.main()
