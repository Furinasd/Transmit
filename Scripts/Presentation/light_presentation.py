"""Level-local lighting only. Separate from material/audio/map assembly transactions."""
import unreal


def run(allow_production=False):
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert not level.is_in_play_in_editor()
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    path = world.get_path_name().split('.')[0]
    assert path.startswith('/Game/Transmit/Presentation/Maps/') or (allow_production and path == '/Game/Transmit/Maps/L_Transmit'), path
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = subsystem.get_all_level_actors()
    by_label = {a.get_actor_label(): a for a in actors}
    sun, fill = by_label.get('Light_Sun'), by_label.get('Light_Fill')
    assert isinstance(sun, unreal.DirectionalLight) and isinstance(fill, unreal.SkyLight), 'Expected authored daylight rig'
    # Preserve the direction's lateral rake while lowering it for readable slab shadows.
    sun.set_actor_rotation(unreal.Rotator(pitch=-50, yaw=-28, roll=0), False)
    sun.light_component.set_editor_property('intensity', 5.5)
    sun.light_component.set_editor_property('light_color', unreal.Color(255, 242, 223, 255))
    fill.light_component.set_editor_property('intensity', 1.2)
    # Fixed EV avoids a dark machine blooming into an unreadable cyan sheet on camera cuts.
    post = by_label.get('Presentation_Exposure')
    if post is None:
        post = subsystem.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector())
        post.set_actor_label('Presentation_Exposure')
        post.set_editor_property('tags', [unreal.Name('Transmit.Presentation.B')])
        post.set_folder_path('Presentation')
    assert isinstance(post, unreal.PostProcessVolume)
    post.set_editor_property('unbound', True)
    post.set_editor_property('priority', 10)
    settings = post.get_editor_property('settings')
    for prop, value in {
        'override_auto_exposure_min_brightness': True,
        'override_auto_exposure_max_brightness': True,
        'auto_exposure_min_brightness': -1.5,
        'auto_exposure_max_brightness': -1.5,
        'override_auto_exposure_bias': True,
        'auto_exposure_bias': 0.0,
        'override_bloom_intensity': True,
        'bloom_intensity': 0.22,
        'override_vignette_intensity': True,
        'vignette_intensity': 0.18,
        'override_motion_blur_amount': True,
        'motion_blur_amount': 0.0,
    }.items():
        settings.set_editor_property(prop, value)
    post.set_editor_property('settings', settings)
    assert unreal.EditorLoadingAndSavingUtils.save_map(world, path)
    unreal.log('PRESENTATION_LIGHTING_SAVED ' + path + ' sun/fill + Presentation_Exposure; project config untouched')
