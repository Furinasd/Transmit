"""Single-map background pass. No geometry, gameplay, lighting or exposure edits.
Call apply() in L_Transmit outside PIE. Caller owns review and map save.
"""
import unreal
LABEL='Presentation_VoidMist'

def apply():
 level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
 world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
 assert not level.is_in_play_in_editor()
 assert world.get_path_name().split('.')[0]=='/Game/Transmit/Maps/L_Transmit'
 actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 all_actors=actors.get_all_level_actors()
 existing=[a for a in all_actors if isinstance(a,unreal.ExponentialHeightFog)]
 assert all(a.get_actor_label()==LABEL for a in existing), 'Preserve unowned existing fog'
 assert len(existing)<=1
 fog=existing[0] if existing else actors.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,-250))
 fog.set_actor_label(LABEL)
 fog.set_folder_path('Presentation')
 fog.set_editor_property('tags',[unreal.Name('Transmit.Presentation.Void')])
 component=fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
 for key,value in {
  'fog_density':.018,
  'fog_height_falloff':.12,
  'fog_inscattering_luminance':unreal.LinearColor(.18,.27,.34,1),
  'fog_max_opacity':1.0,
  'start_distance':1200.0,
  'fog_cutoff_distance':0.0,
  'enable_volumetric_fog':False,
 }.items():component.set_editor_property(key,value)
 return fog
