#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Transmit/TransmitLevelActors.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/MotionTransferable.h"

// Protect the content contract: neither ordinary motion nor a reversed dash can
// replace the routed, armed, fixed-axis two-impact payoff. Scene timing is PIE-tested.
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

    Player->GrantMotionState(Dash);
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
    Player->GrantMotionState(Reverse);
    TestFalse(TEXT("Opposite dash axis cannot power the Ram"),
        Player->TryTransferToActor(Ram, Context).bSucceeded);
    TestTrue(TEXT("Axis rejection retains Player ownership"), Player->HasMotionState());

    Player->RestoreInitialState(false);
    Player->GrantMotionState(Dash);
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
#endif
