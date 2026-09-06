"""Unsaved visual fixture: fly/teleport and hide player to inspect sign faces. Stop PIE afterwards."""
import unreal,pathlib,json
svw=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
sva={x.get_actor_label():x for x in unreal.GameplayStatics.get_all_actors_of_class(svw,unreal.Actor)}
svp=unreal.GameplayStatics.get_player_pawn(svw,0);svpc=unreal.GameplayStatics.get_player_controller(svw,0)
svm=svp.get_component_by_class(unreal.CharacterMovementComponent);svm.set_movement_mode(unreal.MovementMode.MOVE_FLYING)
svp.set_actor_hidden_in_game(True)
svnames=['WorkOrder','Launch','Standard','Quiet','Access','Carrier','OriginalNotice','Exit'];svindex=0;svage=0;svphase=0
svout=pathlib.Path(unreal.Paths.project_saved_dir())/'LTransmitEvidence/Experience'
def svtick(dt):
 global svindex,svage,svphase
 svage+=dt
 if svphase==0:
  actor=sva['Story_'+svnames[svindex]];actor.set_actor_hidden_in_game(False)
  target=actor.get_actor_location();normal=actor.get_actor_up_vector()
  svp.set_actor_location(target+normal*550-unreal.Vector(0,0,64),False,False)
  eye,_=svp.get_actor_eyes_view_point();svpc.set_control_rotation(unreal.MathLibrary.find_look_at_rotation(eye,target))
  svphase=1;svage=0
 elif svphase==1 and svage>1.5:
  unreal.SystemLibrary.execute_console_command(svw,'Shot showui filename="'+str(svout/('story-'+svnames[svindex]+'.png'))+'"',svpc)
  svphase=2;svage=0
 elif svphase==2 and svage>.5:
  svindex+=1
  if svindex==len(svnames):
   unreal.unregister_slate_post_tick_callback(svhandle);unreal.log('STORY_SCREENSHOTS_DONE')
  else:svphase=0
svhandle=unreal.register_slate_post_tick_callback(svtick)
