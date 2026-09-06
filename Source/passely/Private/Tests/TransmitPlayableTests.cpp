#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Transmit/TransmitLevelActors.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/MotionTransferable.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// A routed rail converts captured High energy to its authored stroke.
// Rejections retain ownership; accepted energy is consumed exactly once.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransmitRamSignatureTest,
    "Transmit.Playable.RamSignatureAndRejectionOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTransmitRamSignatureTest::RunTest(const FString& Parameters)
{
    ATransmitRam* Ram = NewObject<ATransmitRam>(GetTransientPackage());
    Ram->AddToRoot();
    Ram->Motion->ConfigureForTesting(TEXT("Ram"), false, true,
        EMotionEndpointMode::ConsumeOnReceive, TOptional<FMotionState>());
    UMotionTransferComponent* Player = NewObject<UMotionTransferComponent>();
    Player->AddToRoot();
    Player->ConfigureForTesting(TEXT("Player"), false, true,
        EMotionEndpointMode::Store, TOptional<FMotionState>());

    FMotionState Dash;
    Dash.Direction = FVector::ForwardVector;
    Dash.DirectionPolicy = EMotionDirectionPolicy::PreserveSource;
    Dash.Magnitude = 1200.0f;
    Dash.SourceId = TEXT("Test.Dash");
    FMotionTransferContext Context;
    Context.DirectionResolution = FMotionDirectionResolution::PreserveSource(Dash.Direction);

    Player->ConfigureForTesting(TEXT("Player"), false, true,
        EMotionEndpointMode::Store, TOptional<FMotionState>(Dash));
    TestFalse(TEXT("An undelivered carrier leaves Ram locked"),
        Player->TryTransferToActor(Ram, Context).bSucceeded);
    TestTrue(TEXT("Unarmed rejection retains Player ownership"), Player->HasMotionState());

    Ram->bArmed = true;
    FMotionState Ordinary = Dash;
    Ordinary.DirectionPolicy = EMotionDirectionPolicy::CameraCanonical;
    Ordinary.Magnitude = 600.0f;
    TestFalse(TEXT("Ordinary motion cannot substitute for Charger Dash"),
        IMotionTransferable::CallCanReceiveMotion(Ram, Ordinary, Context).bAllowed);
    FMotionState WeakDash = Dash;
    WeakDash.Magnitude = 600.0f;
    TestFalse(TEXT("PreserveSource alone does not bypass magnitude"),
        IMotionTransferable::CallCanReceiveMotion(Ram, WeakDash, Context).bAllowed);
    FMotionState Reverse = Dash;
    Reverse.Direction = -FVector::ForwardVector;
    Context.DirectionResolution = FMotionDirectionResolution::PreserveSource(Reverse.Direction);
    Player->RestoreInitialState(false);
    Player->ConfigureForTesting(TEXT("Player"), false, true,
        EMotionEndpointMode::Store, TOptional<FMotionState>(Reverse));
    TestTrue(TEXT("Rail converts a player-directed dash independently of its input axis"),
        Player->TryTransferToActor(Ram, Context).bSucceeded);
    TestFalse(TEXT("Converted reverse dash is consumed once"), Player->HasMotionState());

    Player->RestoreInitialState(false);
    Player->ConfigureForTesting(TEXT("Player"), false, true,
        EMotionEndpointMode::Store, TOptional<FMotionState>(Dash));
    Context.DirectionResolution = FMotionDirectionResolution::PreserveSource(Dash.Direction);
    TestTrue(TEXT("Armed Ram accepts correct preserved direction in Preview"),
        IMotionTransferable::CallCanReceiveMotion(Ram, Dash, Context).bAllowed);
    const FMotionTransferResult Result = Player->TryTransferToActor(Ram, Context);
    TestTrue(TEXT("Commit matches Preview"), Result.bSucceeded);
    TestTrue(TEXT("Ram consumes the single motion resource"), Result.bConsumed);
    TestFalse(TEXT("Player no longer owns consumed dash"), Player->HasMotionState());
    TestFalse(TEXT("Consumed dash is not stored or duplicated in Ram"), Ram->Motion->HasMotionState());
    Ram->Hits = 2;
    TestFalse(TEXT("A broken gate cannot request further resources"),
        IMotionTransferable::CallCanReceiveMotion(Ram, Dash, Context).bAllowed);
    Player->RemoveFromRoot();
    Ram->RemoveFromRoot();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransmitBossReaimTest,
    "Transmit.Playable.BossReaimReturnAndSingleResource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTransmitBossReaimTest::RunTest(const FString&)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Player = World->SpawnActor<APawn>();
    // A transform-bearing pawn is needed for the real PlayerController lookup.
    auto* Root = NewObject<USceneComponent>(Player);
    Player->SetRootComponent(Root); Root->RegisterComponent();
    auto* Controller = World->SpawnActor<APlayerController>();
    World->AddController(Controller);
    Controller->Possess(Player);
    TestEqual(TEXT("Fixture player is discoverable through the production lookup"), UGameplayStatics::GetPlayerPawn(World, 0), Player);
    auto* Held = NewObject<UMotionTransferComponent>(Player);
    Held->RegisterComponent();
    Held->ConfigureForTesting(TEXT("Player"),false,true,EMotionEndpointMode::Store,{});
    auto* Boss = World->SpawnActor<ATransmitArenaCharger>(FVector(0,0,100),FRotator::ZeroRotator);
    Boss->DispatchBeginPlay();
    auto* FSM = Boss->StateMachine.Get();
    FSM->IdleDurationSeconds=.05f; FSM->TelegraphDurationSeconds=.05f;
    FSM->DashDurationSeconds=.3f; FSM->RecoveryDurationSeconds=2.0f;
    FSM->DashCommitWindowDelaySeconds=0;
    Player->SetActorLocation(FVector(-2000,0,100));
    Boss->SetEncounterActive(true); Boss->Tick(.06f); Boss->Tick(.06f);
    FMotionState First;
    TestTrue(TEXT("First dash grants energy"),Boss->Motion->TryGetMotionState(First));
    TestTrue(TEXT("First commitment aims at the actual player"),First.Direction.Equals(-FVector::ForwardVector,.01f));
    FSM->ForceRecovery(); Boss->Tick(.05f);
    TestFalse(TEXT("Missed energy retires in recovery"),Boss->Motion->HasMotionState());
    for(int32 I=0;I<50 && FSM->GetState()!=EMotionChargerState::Idle;++I) Boss->Tick(.05f);
    TestTrue(TEXT("Miss returns to its authored home"),Boss->GetActorLocation().Equals(FVector(0,0,100),.01f));
    Player->SetActorLocation(FVector(0,2000,100));
    Boss->Tick(.06f); Boss->Tick(.06f);
    FMotionState Second;
    TestTrue(TEXT("Next dash grants fresh energy"),Boss->Motion->TryGetMotionState(Second));
    TestTrue(TEXT("Next dash does not reuse the previous direction"),Second.Direction.Equals(FVector::RightVector,.01f));
    FMotionTransferContext Context;
    TestTrue(TEXT("Real actor capture succeeds during committed dash"),Held->TryCaptureFromActor(Boss,Context).bSucceeded);
    for(int32 I=0;I<250;++I) Boss->Tick(.05f);
    TestTrue(TEXT("Capture also returns home"),Boss->GetActorLocation().Equals(FVector(0,0,100),.01f));
    TestTrue(TEXT("The player's captured resource remains owned"),Held->HasMotionState());
    TestFalse(TEXT("Boss never regenerates a second copy while held"),Boss->Motion->HasMotionState());
    TestEqual(TEXT("Boss waits at home until that resource is used"),FSM->GetState(),EMotionChargerState::Idle);
    World->DestroyWorld(false);
    return true;
}
#endif
