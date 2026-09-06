"""Dedicated Editor session launched with -ExecutePythonScript.

The log prints a unique request directory. Write request.py atomically there;
only trusted local task code belongs in this inbox. A request may call
request_shutdown() after stopping its own probes and PIE. No signal-based exit.
"""
import json
import pathlib
import sys
import tempfile
import traceback
import unreal

sys.path.insert(0, str(pathlib.Path(unreal.Paths.project_dir()) / 'Scripts/Editor'))
from transmit_editor_safety import CallbackOwner, require_clean_editor

inbox = pathlib.Path(tempfile.mkdtemp(prefix='transmit-editor-session-'))
request = inbox / 'request.py'
owner = CallbackOwner(keep_alive=True)


def request_shutdown():
    require_clean_editor()
    # Caller must finish any probes it created first. Never reach into their
    # callback registry or save/discard user assets on their behalf.
    owner.close()
    unreal.log('TRANSMIT_SESSION_SHUTDOWN_REQUESTED')


scope = dict(unreal=unreal, pathlib=pathlib, json=json,
             request_shutdown=request_shutdown)


def tick(delta):
    if not request.exists():
        return
    code = request.read_text()
    request.unlink()  # Consume exactly once, including a failed request.
    try:
        exec(compile(code, str(request), 'exec'), scope)
        result = dict(ok=True)
    except Exception:
        result = dict(ok=False, error=traceback.format_exc())
        unreal.log_error(result['error'])
    (inbox / 'result.json').write_text(json.dumps(result, indent=2))


owner.register(tick)
unreal.log('TRANSMIT_SESSION_READY inbox=' + str(inbox))
