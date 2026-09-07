"""Narrow narrative-sign asset transaction: 7 textures + 7 static materials."""
import unreal,pathlib,json
root=pathlib.Path(unreal.Paths.project_dir());dest='/Game/Transmit/Presentation/Narrative'
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
assert not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
tools=unreal.AssetToolsHelpers.get_asset_tools();changed=[]
for png in sorted((root/'Saved/NarrativeSigns').glob('*.png')):
 task=unreal.AssetImportTask();task.filename=str(png);task.destination_path=dest;task.destination_name='T_'+png.stem;task.automated=True;task.save=False;task.replace_existing=False
 tex=unreal.load_asset(dest+'/T_'+png.stem)
 if not tex:
  tools.import_asset_tasks([task]);tex=unreal.load_asset(dest+'/T_'+png.stem)
 assert tex
 tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
 path=dest+'/M_'+png.stem;mat=unreal.load_asset(path)
 if not mat:
  mat=tools.create_asset('M_'+png.stem,dest,unreal.Material,unreal.MaterialFactoryNew())
  mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
  sample=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionTextureSample,0,0);sample.texture=tex
  unreal.MaterialEditingLibrary.connect_material_property(sample,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
  unreal.MaterialEditingLibrary.recompile_material(mat)
 unreal.EditorAssetLibrary.save_loaded_asset(tex);unreal.EditorAssetLibrary.save_loaded_asset(mat)
 changed.extend([tex.get_path_name(),mat.get_path_name()])
out=root/'Saved/LTransmitEvidence/Experience';out.mkdir(parents=True,exist_ok=True)
(out/'sign-assets.json').write_text(json.dumps(changed,indent=2))
unreal.log('EXPERIENCE_SIGN_ASSETS_SAVED '+str(len(changed)))
