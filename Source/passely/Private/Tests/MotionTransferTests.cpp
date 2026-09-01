#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Motion/MotionCanonicalDirectionResolver.h"
#include "Motion/MotionChargerStateMachine.h"
#include "Motion/MotionGameplayTags.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/MotionTransferSettings.h"

namespace MotionTransferTests
{
    static FMotionState MakeState(
        const FName SourceId = TEXT("Source.Linear.001"),
        const float Magnitude = 600.0f)
    {
        FMotionState State;
        State.Type = EMotionType::Linear;
        State.Direction = FVector::RightVector;
        State.Magnitude = Magnitude;
        State.SourceId = SourceId;
        return State;
    }

    static UMotionTransferComponent* MakeComponent(
        const FName ParticipantId,
        const bool bCanProvide,
        const bool bCanReceive,
        const EMotionEndpointMode EndpointMode,
        const TOptional<FMotionState>& InitialState = TOptional<FMotionState>())
    {
        UMotionTransferComponent* Component =
            NewObject<UMotionTransferComponent>(GetTransientPackage());
        Component->AddToRoot();
        Component->ConfigureForTesting(
            ParticipantId,
            bCanProvide,
            bCanReceive,
            EndpointMode,
            InitialState);
        return Component;
    }

    static void ReleaseComponent(UMotionTransferComponent* Component)
    {
        if (Component)
        {
            Component->RemoveFromRoot();
        }
    }

    static UMotionCanonicalDirectionResolver* MakeResolver()
    {
        UMotionCanonicalDirectionResolver* Resolver =
            NewObject<UMotionCanonicalDirectionResolver>();
        Resolver->AddToRoot();
        Resolver->UpEnterPitchDegrees = 45.0f;
        Resolver->UpExitPitchDegrees = 35.0f;
        Resolver->HorizontalBoundaryHysteresisDegrees = 8.0f;
        return Resolver;
    }

    static void ReleaseResolver(UMotionCanonicalDirectionResolver* Resolver)
    {
        if (Resolver)
        {
            Resolver->RemoveFromRoot();
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionMagnitudePolicyTest,
    "Transmit.MotionTransfer.MagnitudePolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionMagnitudePolicyTest::RunTest(const FString& Parameters)
{
    const UMotionTransferSettings* Settings = GetDefault<UMotionTransferSettings>();
    FText ValidationError;
    TestTrue(
        TEXT("Default magnitude bands are valid"),
        UMotionTransferSettings::ValidateMagnitudeBands(
            Settings->MagnitudeBands,
            ValidationError));

    TestEqual(
        TEXT("399.99 resolves to Weak"),
        Settings->ResolveMagnitudeTier(399.99f),
        MotionGameplayTags::Motion_Tier_Weak.GetTag());
    TestEqual(
        TEXT("400 resolves to Medium"),
        Settings->ResolveMagnitudeTier(400.0f),
        MotionGameplayTags::Motion_Tier_Medium.GetTag());
    TestEqual(
        TEXT("800 resolves to Strong"),
        Settings->ResolveMagnitudeTier(800.0f),
        MotionGameplayTags::Motion_Tier_Strong.GetTag());

    TArray<FMotionMagnitudeBand> InvalidBands = Settings->MagnitudeBands;
    InvalidBands[1].MinInclusive = 399.0f;
    TestFalse(
        TEXT("Overlapping magnitude bands are rejected"),
        UMotionTransferSettings::ValidateMagnitudeBands(InvalidBands, ValidationError));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionOwnershipCycleTest,
    "Transmit.MotionTransfer.OwnershipCycles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionOwnershipCycleTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    const FMotionState State = MakeState();
    UMotionTransferComponent* Source = MakeComponent(
        TEXT("Source"), true, false, EMotionEndpointMode::Store, State);
    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"), false, true, EMotionEndpointMode::Store);
    UMotionTransferComponent* Receiver = MakeComponent(
        TEXT("Receiver"), false, true, EMotionEndpointMode::ConsumeOnReceive);

    for (int32 Cycle = 0; Cycle < 20; ++Cycle)
    {
        const FMotionTransferResult Capture = Player->TryCaptureFromComponent(Source);
        TestTrue(FString::Printf(TEXT("Cycle %d capture succeeds"), Cycle + 1), Capture.bSucceeded);
        TestFalse(TEXT("Source is empty after capture"), Source->HasMotionState());
        TestTrue(TEXT("Player owns Motion after capture"), Player->HasMotionState());

        FMotionState CarriedState;
        TestTrue(TEXT("Player state can be queried"), Player->TryGetMotionState(CarriedState));
        TestEqual(TEXT("SourceId is preserved"), CarriedState.SourceId, State.SourceId);
        TestTrue(TEXT("Direction is preserved"), CarriedState.Direction.Equals(State.Direction));
        TestTrue(
            TEXT("Magnitude is preserved"),
            FMath::IsNearlyEqual(CarriedState.Magnitude, State.Magnitude));

        const FMotionTransferResult Transfer = Player->TryTransferToComponent(Receiver);
        TestTrue(FString::Printf(TEXT("Cycle %d transfer succeeds"), Cycle + 1), Transfer.bSucceeded);
        TestTrue(TEXT("Receiver consumes the Motion"), Transfer.bConsumed);
        TestFalse(TEXT("Player is empty after consume"), Player->HasMotionState());
        TestFalse(TEXT("Consumer stores no transferable state"), Receiver->HasMotionState());

        TestTrue(TEXT("Source reset succeeds"), Source->RestoreInitialState(false));
        TestTrue(TEXT("Player reset succeeds"), Player->RestoreInitialState(false));
        TestTrue(TEXT("Receiver reset succeeds"), Receiver->RestoreInitialState(false));
        TestTrue(TEXT("Source restored for next cycle"), Source->HasMotionState());
        TestFalse(TEXT("Player remains empty after reset"), Player->HasMotionState());
    }

    ReleaseComponent(Source);
    ReleaseComponent(Player);
    ReleaseComponent(Receiver);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionRejectedCapturePreservesOwnerTest,
    "Transmit.MotionTransfer.RejectionPreservesOwner",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionRejectedCapturePreservesOwnerTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    const FMotionState SourceState = MakeState(TEXT("Source.A"), 600.0f);
    const FMotionState ExistingPlayerState = MakeState(TEXT("Source.B"), 900.0f);
    UMotionTransferComponent* Source = MakeComponent(
        TEXT("Source"), true, false, EMotionEndpointMode::Store, SourceState);
    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"), false, true, EMotionEndpointMode::Store, ExistingPlayerState);

    const FMotionTransferResult Result = Player->TryCaptureFromComponent(Source);
    TestFalse(TEXT("Capture into occupied Player is rejected"), Result.bSucceeded);
    TestEqual(
        TEXT("Rejection identifies occupied carrier"),
        Result.Rejection,
        EMotionTransferRejection::CarrierOccupied);
    TestTrue(TEXT("Source keeps its Motion"), Source->HasMotionState());
    TestTrue(TEXT("Player keeps its existing Motion"), Player->HasMotionState());

    FMotionState ActualSourceState;
    FMotionState ActualPlayerState;
    Source->TryGetMotionState(ActualSourceState);
    Player->TryGetMotionState(ActualPlayerState);
    TestEqual(TEXT("Source identity is unchanged"), ActualSourceState.SourceId, SourceState.SourceId);
    TestEqual(
        TEXT("Player identity is unchanged"),
        ActualPlayerState.SourceId,
        ExistingPlayerState.SourceId);

    ReleaseComponent(Source);
    ReleaseComponent(Player);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionNotificationReentrancyTest,
    "Transmit.MotionTransfer.NotificationReentrancy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionNotificationReentrancyTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    UMotionTransferComponent* Source = MakeComponent(
        TEXT("Source"), true, false, EMotionEndpointMode::Store, MakeState());
    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"), false, true, EMotionEndpointMode::Store);
    UMotionTransferComponent* Receiver = MakeComponent(
        TEXT("Receiver"), false, true, EMotionEndpointMode::ConsumeOnReceive);

    TArray<EMotionTransferVerb> NotificationOrder;
    bool bLockWasClearDuringBroadcast = true;
    Player->OnMotionTransactionNative().AddLambda(
        [&](const FMotionTransferResult& Result)
        {
            bLockWasClearDuringBroadcast &= !Player->IsTransactionInProgress();
            NotificationOrder.Add(Result.Verb);
            if (Result.bSucceeded && Result.Verb == EMotionTransferVerb::Capture)
            {
                Player->TryTransferToComponent(Receiver);
            }
        });

    const FMotionTransferResult Capture = Player->TryCaptureFromComponent(Source);
    TestTrue(TEXT("Initial capture succeeds"), Capture.bSucceeded);
    TestTrue(TEXT("Transaction lock is clear during every broadcast"), bLockWasClearDuringBroadcast);
    TestEqual(TEXT("Two transaction notifications are delivered"), NotificationOrder.Num(), 2);
    if (NotificationOrder.Num() == 2)
    {
        TestEqual(TEXT("Capture notification is first"), NotificationOrder[0], EMotionTransferVerb::Capture);
        TestEqual(TEXT("Re-entrant Transfer notification is second"), NotificationOrder[1], EMotionTransferVerb::Transfer);
    }
    TestFalse(TEXT("Player is empty after re-entrant consume"), Player->HasMotionState());
    TestFalse(TEXT("Receiver stores no state after consume"), Receiver->HasMotionState());

    ReleaseComponent(Source);
    ReleaseComponent(Player);
    ReleaseComponent(Receiver);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionStickyScoreTest,
    "Transmit.MotionTransfer.StickyScoring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionStickyScoreTest::RunTest(const FString& Parameters)
{
    const float CurrentScore = UMotionInteractorComponent::CalculateRawScore(
        0.95f, 0.4f, 1.0f, 0.25f);
    TestFalse(
        TEXT("A merely higher raw score does not beat stickiness"),
        UMotionInteractorComponent::ShouldSwitchTarget(
            CurrentScore,
            CurrentScore + 0.1f,
            0.15f));
    TestTrue(
        TEXT("A score above current plus StickyBonus switches"),
        UMotionInteractorComponent::ShouldSwitchTarget(
            CurrentScore,
            CurrentScore + 0.151f,
            0.15f));
    TestFalse(
        TEXT("The exact threshold does not switch"),
        UMotionInteractorComponent::ShouldSwitchTarget(
            CurrentScore,
            CurrentScore + 0.15f,
            0.15f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionCanonicalResolverTest,
    "Transmit.MotionTransfer.CanonicalResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionCanonicalResolverTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    UMotionCanonicalDirectionResolver* Resolver = MakeResolver();
    const FRotator CameraYawZero(0.0f, 0.0f, 0.0f);
    const FRotator CameraYawRight(0.0f, 90.0f, 0.0f);

    const FMotionDirectionResolution Forward =
        Resolver->ResolveDirection(FVector::ForwardVector, CameraYawZero);
    TestTrue(TEXT("Forward resolution is valid"), Forward.bValid);
    TestTrue(
        TEXT("Forward resolves to Forward"),
        Forward.CanonicalDirection == EMotionCanonicalDirection::Forward);
    TestTrue(
        TEXT("Forward world direction is +X"),
        Forward.WorldDirection.Equals(FVector::ForwardVector, 1e-3f));

    const FMotionDirectionResolution Back =
        Resolver->ResolveDirection(FVector::BackwardVector, CameraYawZero);
    TestTrue(
        TEXT("Back resolves to Back"),
        Back.bValid && Back.CanonicalDirection == EMotionCanonicalDirection::Back);

    const FMotionDirectionResolution Right =
        Resolver->ResolveDirection(FVector::RightVector, CameraYawZero);
    TestTrue(
        TEXT("Right resolves to Right"),
        Right.bValid && Right.CanonicalDirection == EMotionCanonicalDirection::Right);
    TestTrue(
        TEXT("Right world direction is +Y"),
        Right.WorldDirection.Equals(FVector::RightVector, 1e-3f));

    const FMotionDirectionResolution Left =
        Resolver->ResolveDirection(FVector::LeftVector, CameraYawZero);
    TestTrue(
        TEXT("Left resolves to Left"),
        Left.bValid && Left.CanonicalDirection == EMotionCanonicalDirection::Left);

    const FMotionDirectionResolution Up =
        Resolver->ResolveDirection(FVector::UpVector, CameraYawZero);
    TestTrue(
        TEXT("Up resolves to Up"),
        Up.bValid && Up.CanonicalDirection == EMotionCanonicalDirection::Up);
    TestTrue(
        TEXT("Up world direction is +Z"),
        Up.WorldDirection.Equals(FVector::UpVector, 1e-3f));

    const FMotionDirectionResolution Down =
        Resolver->ResolveDirection(FVector::DownVector, CameraYawZero);
    TestTrue(
        TEXT("Down resolves to Down"),
        Down.bValid && Down.CanonicalDirection == EMotionCanonicalDirection::Down);

    // Camera yaw changes the horizontal basis: +X becomes Left when the camera faces +Y.
    const FMotionDirectionResolution Yawed =
        Resolver->ResolveDirection(FVector::ForwardVector, CameraYawRight);
    TestTrue(
        TEXT("Camera yaw maps +X to Left"),
        Yawed.bValid && Yawed.CanonicalDirection == EMotionCanonicalDirection::Left);
    TestTrue(
        TEXT("Camera yaw keeps world direction +X"),
        Yawed.WorldDirection.Equals(FVector::ForwardVector, 1e-3f));

    // Identical pose and input must resolve identically.
    Resolver->ResetHysteresis();
    const FMotionDirectionResolution DeterministicA =
        Resolver->ResolveDirection(FVector::ForwardVector, CameraYawZero);
    const FMotionDirectionResolution DeterministicB =
        Resolver->ResolveDirection(FVector::ForwardVector, CameraYawZero);
    TestTrue(
        TEXT("Resolver is deterministic"),
        DeterministicA.CanonicalDirection == DeterministicB.CanonicalDirection
            && DeterministicA.WorldDirection.Equals(DeterministicB.WorldDirection, 1e-3f));

    // Pitch hysteresis: entering Up requires 45 degrees; leaving Up requires < 35.
    Resolver->ResetHysteresis();
    Resolver->ResolveDirection(FVector::UpVector, CameraYawZero);
    const FVector Dir40 = FVector(
        FMath::Cos(FMath::DegreesToRadians(40.0f)),
        0.0f,
        FMath::Sin(FMath::DegreesToRadians(40.0f)));
    const FMotionDirectionResolution StayUp =
        Resolver->ResolveDirection(Dir40, CameraYawZero);
    TestTrue(
        TEXT("Up is kept above the exit threshold"),
        StayUp.bValid && StayUp.CanonicalDirection == EMotionCanonicalDirection::Up);
    const FVector Dir30 = FVector(
        FMath::Cos(FMath::DegreesToRadians(30.0f)),
        0.0f,
        FMath::Sin(FMath::DegreesToRadians(30.0f)));
    const FMotionDirectionResolution ExitUp =
        Resolver->ResolveDirection(Dir30, CameraYawZero);
    TestTrue(
        TEXT("Up exits below the exit threshold"),
        ExitUp.bValid && ExitUp.CanonicalDirection == EMotionCanonicalDirection::Forward);

    Resolver->ResetHysteresis();
    const FMotionDirectionResolution BelowEnter =
        Resolver->ResolveDirection(Dir40, CameraYawZero);
    TestTrue(
        TEXT("Horizontal is kept below the enter threshold"),
        BelowEnter.bValid
            && BelowEnter.CanonicalDirection == EMotionCanonicalDirection::Forward);
    const FVector Dir50 = FVector(
        FMath::Cos(FMath::DegreesToRadians(50.0f)),
        0.0f,
        FMath::Sin(FMath::DegreesToRadians(50.0f)));
    const FMotionDirectionResolution EnterUp =
        Resolver->ResolveDirection(Dir50, CameraYawZero);
    TestTrue(
        TEXT("Up is entered above the enter threshold"),
        EnterUp.bValid && EnterUp.CanonicalDirection == EMotionCanonicalDirection::Up);

    // Horizontal hysteresis keeps the previous sector inside the boundary band.
    Resolver->ResetHysteresis();
    Resolver->ResolveDirection(FVector::RightVector, CameraYawZero);
    const FVector Dir40TowardForward = FVector(
        FMath::Cos(FMath::DegreesToRadians(40.0f)),
        FMath::Sin(FMath::DegreesToRadians(40.0f)),
        0.0f);
    const FMotionDirectionResolution KeepRight =
        Resolver->ResolveDirection(Dir40TowardForward, CameraYawZero);
    TestTrue(
        TEXT("Right is kept inside the horizontal hysteresis band"),
        KeepRight.bValid && KeepRight.CanonicalDirection == EMotionCanonicalDirection::Right);

    Resolver->ResetHysteresis();
    Resolver->ResolveDirection(FVector::ForwardVector, CameraYawZero);
    const FVector Dir50TowardRight = FVector(
        FMath::Cos(FMath::DegreesToRadians(50.0f)),
        FMath::Sin(FMath::DegreesToRadians(50.0f)),
        0.0f);
    const FMotionDirectionResolution KeepForward =
        Resolver->ResolveDirection(Dir50TowardRight, CameraYawZero);
    TestTrue(
        TEXT("Forward is kept inside the horizontal hysteresis band"),
        KeepForward.bValid && KeepForward.CanonicalDirection == EMotionCanonicalDirection::Forward);

    ReleaseResolver(Resolver);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionResolvedTransferTest,
    "Transmit.MotionTransfer.ResolvedTransfer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionResolvedTransferTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    FMotionState State = MakeState(TEXT("Source.Linear.001"), 600.0f);
    State.Direction = FVector::ForwardVector;
    UMotionTransferComponent* Source = MakeComponent(
        TEXT("Source"), true, false, EMotionEndpointMode::Store, State);
    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"), false, true, EMotionEndpointMode::Store);
    UMotionTransferComponent* ReceiverForward = MakeComponent(
        TEXT("Receiver.Forward"), false, true, EMotionEndpointMode::ConsumeOnReceive);
    ReceiverForward->RequiredCanonicalDirection = EMotionCanonicalDirection::Forward;
    UMotionTransferComponent* ReceiverUp = MakeComponent(
        TEXT("Receiver.Up"), false, true, EMotionEndpointMode::ConsumeOnReceive);
    ReceiverUp->RequiredCanonicalDirection = EMotionCanonicalDirection::Up;

    UMotionCanonicalDirectionResolver* Resolver = MakeResolver();
    const FRotator CameraYawZero(0.0f, 0.0f, 0.0f);
    const FMotionDirectionResolution Resolution =
        Resolver->ResolveDirection(FVector::ForwardVector, CameraYawZero);
    TestTrue(TEXT("Transfer resolution is valid"), Resolution.bValid);
    TestTrue(
        TEXT("Transfer resolution is Forward"),
        Resolution.CanonicalDirection == EMotionCanonicalDirection::Forward);

    const FMotionTransferResult Capture = Player->TryCaptureFromComponent(Source);
    TestTrue(TEXT("Capture succeeds before resolved transfer"), Capture.bSucceeded);
    FMotionState Carried;
    Player->TryGetMotionState(Carried);
    TestTrue(
        TEXT("Capture preserves source direction"),
        Carried.Direction.Equals(FVector::ForwardVector, 1e-3f));

    // Preview and Commit consume the same resolution result.
    FMotionState ResolvedState = Carried;
    ResolvedState.Direction = Resolution.WorldDirection;
    const FMotionCompatibilityResult Preview =
        ReceiverForward->CanReceiveState(ResolvedState, &Resolution);
    TestTrue(TEXT("Forward receiver preview allows"), Preview.bAllowed);
    const FMotionTransferResult Transfer =
        Player->TryTransferToComponent(ReceiverForward, Resolution);
    TestTrue(TEXT("Resolved transfer succeeds"), Transfer.bSucceeded);
    TestTrue(TEXT("Resolved transfer consumes"), Transfer.bConsumed);
    TestFalse(TEXT("Player is empty after resolved transfer"), Player->HasMotionState());
    TestFalse(TEXT("Consumer stores nothing"), ReceiverForward->HasMotionState());

    // Direction mismatch: preview and commit reject with the same reason, ownership preserved.
    Source->RestoreInitialState(false);
    Player->RestoreInitialState(false);
    const FMotionTransferResult SecondCapture = Player->TryCaptureFromComponent(Source);
    TestTrue(TEXT("Second capture succeeds"), SecondCapture.bSucceeded);
    Player->TryGetMotionState(Carried);
    ResolvedState = Carried;
    ResolvedState.Direction = Resolution.WorldDirection;
    const FMotionCompatibilityResult MismatchPreview =
        ReceiverUp->CanReceiveState(ResolvedState, &Resolution);
    TestFalse(TEXT("Up receiver preview rejects"), MismatchPreview.bAllowed);
    TestTrue(
        TEXT("Up receiver preview reports IncompatibleDirection"),
        MismatchPreview.Rejection == EMotionTransferRejection::IncompatibleDirection);
    const FMotionTransferResult MismatchTransfer =
        Player->TryTransferToComponent(ReceiverUp, Resolution);
    TestFalse(TEXT("Mismatch transfer fails"), MismatchTransfer.bSucceeded);
    TestTrue(
        TEXT("Mismatch transfer reports IncompatibleDirection"),
        MismatchTransfer.Rejection == EMotionTransferRejection::IncompatibleDirection);
    TestTrue(TEXT("Player keeps motion after mismatch"), Player->HasMotionState());
    TestFalse(TEXT("Up receiver stays empty after mismatch"), ReceiverUp->HasMotionState());

    ReleaseComponent(Source);
    ReleaseComponent(Player);
    ReleaseComponent(ReceiverForward);
    ReleaseComponent(ReceiverUp);
    ReleaseResolver(Resolver);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionChargerStateMachineTest,
    "Transmit.MotionTransfer.ChargerStateMachine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionChargerStateMachineTest::RunTest(const FString& Parameters)
{
    UMotionChargerStateMachine* Machine = NewObject<UMotionChargerStateMachine>();
    Machine->AddToRoot();
    Machine->IdleDurationSeconds = 0.0f;
    Machine->TelegraphDurationSeconds = 0.5f;
    Machine->DashDurationSeconds = 0.6f;
    Machine->RecoveryDurationSeconds = 1.0f;
    Machine->DashCommitWindowDelaySeconds = 0.1f;
    Machine->bLoopAfterRecovery = true;

    Machine->Start();
    TestTrue(
        TEXT("Charger starts in Idle"),
        Machine->GetState() == EMotionChargerState::Idle);
    TestFalse(TEXT("Idle capture window is closed"), Machine->IsCaptureWindowOpen());

    Machine->Tick(0.01f);
    TestTrue(
        TEXT("Idle advances to Telegraph"),
        Machine->GetState() == EMotionChargerState::Telegraph);
    TestFalse(TEXT("Telegraph capture window is closed"), Machine->IsCaptureWindowOpen());

    Machine->Tick(0.5f);
    TestTrue(
        TEXT("Telegraph advances to Dash"),
        Machine->GetState() == EMotionChargerState::Dash);
    TestTrue(TEXT("Dash is committed"), Machine->IsDashCommitted());
    TestFalse(
        TEXT("Dash window is closed before commit delay"),
        Machine->IsCaptureWindowOpen());

    Machine->Tick(0.05f);
    TestFalse(
        TEXT("Dash window is still closed at 0.05s"),
        Machine->IsCaptureWindowOpen());
    Machine->Tick(0.06f);
    TestTrue(
        TEXT("Dash window opens after commit delay"),
        Machine->IsCaptureWindowOpen());

    Machine->Tick(0.49f);
    TestTrue(
        TEXT("Dash advances to Recovery"),
        Machine->GetState() == EMotionChargerState::Recovery);
    TestFalse(TEXT("Recovery capture window is closed"), Machine->IsCaptureWindowOpen());
    TestFalse(TEXT("Charger is not dashing in Recovery"), Machine->IsDashCommitted());

    Machine->Tick(1.0f);
    TestTrue(
        TEXT("Recovery loops back to Idle"),
        Machine->GetState() == EMotionChargerState::Idle);

    // Capture interrupt: Dash -> ForceRecovery closes the window immediately.
    Machine->Tick(0.01f);
    TestTrue(
        TEXT("Cycle reaches Telegraph again"),
        Machine->GetState() == EMotionChargerState::Telegraph);
    Machine->Tick(0.5f);
    TestTrue(
        TEXT("Cycle reaches Dash again"),
        Machine->GetState() == EMotionChargerState::Dash);
    Machine->ForceRecovery();
    TestTrue(
        TEXT("ForceRecovery enters Recovery"),
        Machine->GetState() == EMotionChargerState::Recovery);
    TestFalse(
        TEXT("Window is closed after ForceRecovery"),
        Machine->IsCaptureWindowOpen());

    Machine->Reset();
    TestTrue(
        TEXT("Reset returns to Idle"),
        Machine->GetState() == EMotionChargerState::Idle);
    TestFalse(TEXT("Reset closes the capture window"), Machine->IsCaptureWindowOpen());

    Machine->RemoveFromRoot();
    return true;
}

#endif
