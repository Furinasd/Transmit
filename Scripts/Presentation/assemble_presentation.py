"""Idempotent local assembly. Does not rebuild gameplay, move its actors, or edit config.
B: duplicate production checkpoint to a NEW preview map, then assemble there.
A: explicitly pass allow_production=True to apply to its loaded formal map.
Asset import is a separate script/transaction.
"""
import unreal

ROOT = '/Game/Transmit/Presentation'
TAG = 'Transmit.Presentation.B'
PREVIEW = ROOT + '/Maps/L_PresentationPreview'
CUES = ('Capture', 'Transfer', 'Dock', 'Telegraph', 'Intercept', 'RamImpact1', 'RamImpact2', 'Complete')


def create_preview():
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert not level.is_in_play_in_editor(), 'Stop PIE before creating preview'
    assert not unreal.EditorAssetLibrary.does_asset_exist(PREVIEW), 'Preview already exists; reopen it, never overwrite it'
    # UE loads an untitled template and saves it under the new path. Generic
    # duplicate_asset(World) retains standalone flags and is unsafe before LoadLevel.
    assert level.new_level_from_template(PREVIEW, '/Game/Transmit/Maps/L_Transmit')
    unreal.log('PRESENTATION_PREVIEW_CREATED ' + PREVIEW)


def assemble(allow_production=False):
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert not level.is_in_play_in_editor(), 'Stop PIE before assembly'
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    path = world.get_path_name().split('.')[0]
    assert path.startswith(ROOT + '/Maps/') or (allow_production and path == '/Game/Transmit/Maps/L_Transmit'), path
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    all_actors = actors.get_all_level_actors()
    by_label = {}
    for actor in all_actors:
        label = actor.get_actor_label()
        assert label not in by_label, 'Ambiguous actor label: ' + label
        by_label[label] = actor
    required = ('Weaponize_Ram', 'Weaponize_Charger', 'Weaponize_ExitMarker', 'Flow_Director')
    assert all(k in by_label for k in required), 'Missing stable gameplay anchors'
    mats = {k: unreal.load_asset(ROOT + '/Materials/M_' + k) for k in ('Ceramic', 'Graphite', 'Motion', 'Impact', 'Inlay')}
    sounds = [unreal.load_asset(ROOT + '/Audio/S_Transmit_' + k) for k in CUES]
    assert all(mats.values()) and all(sounds), 'Run separate import_presentation_assets transaction first'
    rigs = [a for a in all_actors if isinstance(a, unreal.TransmitPresentationRig)]
    assert len(rigs) <= 1, 'Multiple presentation rigs; resolve authored duplication first'
    rig = rigs[0] if rigs else actors.spawn_actor_from_class(unreal.TransmitPresentationRig, unreal.Vector())
    rig.set_actor_label('Presentation_Rig')
    rig.set_editor_property('tags', [unreal.Name(TAG)])
    rig.set_folder_path('Presentation')
    rig.set_editor_property('director', by_label['Flow_Director'])
    rig.set_editor_property('ram', by_label['Weaponize_Ram'])
    rig.set_editor_property('charger', by_label['Weaponize_Charger'])
    rig.set_editor_property('exit_anchor', by_label['Weaponize_ExitMarker'])
    rig.set_editor_property('motion_material', mats['Motion'])
    rig.set_editor_property('impact_material', mats['Impact'])
    rig.set_editor_property('structure_material', mats['Graphite'])
    rig.set_editor_property('cues', sounds)
    # A may supply cable-corner TargetPoints; preserve its references on rerun.
    corners = [a for a in all_actors if a.get_actor_label().startswith('Presentation_DockCable_')]
    if corners:
        rig.set_editor_property('dock_cable_anchors', sorted(corners, key=lambda a:a.get_actor_label()))
    changed = []
    # A's stable actor labels classify already-authored surfaces; no transforms/collision change.
    for label, actor in by_label.items():
        if not label.startswith(('Learn_', 'Route_', 'Weaponize_')):
            continue
        mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
        if not mesh:
            continue
        if any(x in label for x in ('Source', 'Carrier', 'BridgeSlab', '_Ram', 'DeckRail', 'ArmLink')):
            mat = mats['Ceramic'] if any(x in label for x in ('Source', 'Carrier', 'BridgeSlab', '_Ram')) else mats['Motion']
        elif 'Charger' in label or 'Gate' in label:
            mat = mats['Graphite']
        elif any(x in label for x in ('DashEdge', 'DashAxis')):
            mat = mats['Impact']
        elif any(x in label for x in ('Floor', 'Approach', 'FarBank')):
            mat = mats['Ceramic']
        elif any(x in label for x in ('Rail', 'Curb', 'Sill', 'BackWall', 'EndWall', 'LowRoof')):
            mat = mats['Graphite']
        else:
            mat = mats['Ceramic']
        mesh.set_material(0, mat)
        changed.append(label)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, path)
    unreal.log('PRESENTATION_ASSEMBLED map=' + path + ' surfaces=' + str(len(changed)) + ' rig=' + rig.get_path_name())
    return rig
