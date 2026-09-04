#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Motion/TransmitChargerActor.h"
#include "Motion/MotionCanonicalDirectionResolver.h"
#include "Motion/MotionChargerStateMachine.h"
#include "Motion/MotionGameplayTags.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/MotionTransferSettings.h"
#include "Motion/MotionTransferable.h"
#include "Motion/TransmitDirectionalCarrierActor.h"
#include "Motion/TransmitMotionEndpointActor.h"

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

    static ATransmitMotionEndpointActor* MakeEndpointActor(
        const FName ParticipantId,
        const EMotionCanonicalDirection RequiredDirection,
        const EMotionEndpointMode EndpointMode)
    {
        ATransmitMotionEndpointActor* Actor =
            NewObject<ATransmitMotionEndpointActor>(GetTransientPackage());
        Actor->AddToRoot();
        if (UMotionTransferComponent* Motion = Actor->Motion)
        {
            Motion->ConfigureForTesting(
                ParticipantId,
                /* bCanProvideMotion */ false,
                /* bCanReceiveMotion */ true,
                EndpointMode,
                TOptional<FMotionState>());
            Motion->RequiredCanonicalDirection = RequiredDirection;
        }
        return Actor;
    }

    static void ReleaseEndpointActor(ATransmitMotionEndpointActor* Actor)
    {
        if (Actor)
        {
            Actor->RemoveFromRoot();
        }
    }

    static ATransmitDirectionalCarrierActor* MakeCarrier(
        const FName ParticipantId,
        const float MovementSpeed)
    {
        ATransmitDirectionalCarrierActor* Carrier =
            NewObject<ATransmitDirectionalCarrierActor>(GetTransientPackage());
        Carrier->AddToRoot();
        Carrier->MovementSpeed = MovementSpeed;
        if (Carrier->Motion)
        {
            Carrier->Motion->ConfigureForTesting(
                ParticipantId,
                /* bCanProvideMotion */ true,
                /* bCanReceiveMotion */ true,
                EMotionEndpointMode::Store,
                TOptional<FMotionState>());
        }
        return Carrier;
    }

    static void ReleaseCarrier(ATransmitDirectionalCarrierActor* Carrier)
    {
        if (Carrier)
        {
            Carrier->RemoveFromRoot();
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

    // Camera pitch is the player's Up/Down selection: a horizontal carried
    // direction becomes Up when the camera looks up by the enter threshold.
    Resolver->ResetHysteresis();
    const FRotator CameraPitchUp(50.0f, 0.0f, 0.0f);
    const FMotionDirectionResolution UpByCamera =
        Resolver->ResolveDirection(FVector::ForwardVector, CameraPitchUp);
    TestTrue(
        TEXT("Camera pitch resolves a horizontal carried state to Up"),
        UpByCamera.bValid
            && UpByCamera.CanonicalDirection == EMotionCanonicalDirection::Up);
    TestTrue(
        TEXT("Camera pitch Up keeps world direction +Z"),
        UpByCamera.WorldDirection.Equals(FVector::UpVector, 1e-3f));

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

    // Camera-selected Up against a Forward receiver is a DirectionMismatch:
    // preview and commit reject identically and the Player keeps Motion.
    Resolver->ResetHysteresis();
    const FRotator CameraPitchUp(50.0f, 0.0f, 0.0f);
    const FMotionDirectionResolution CameraUpResolution =
        Resolver->ResolveDirection(FVector::ForwardVector, CameraPitchUp);
    TestTrue(
        TEXT("Camera Up resolution is valid"),
        CameraUpResolution.bValid
            && CameraUpResolution.CanonicalDirection == EMotionCanonicalDirection::Up);

    Player->TryGetMotionState(Carried);
    ResolvedState = Carried;
    ResolvedState.Direction = CameraUpResolution.WorldDirection;
    const FMotionCompatibilityResult ForwardMismatchPreview =
        ReceiverForward->CanReceiveState(ResolvedState, &CameraUpResolution);
    TestFalse(
        TEXT("Forward receiver preview rejects camera-selected Up"),
        ForwardMismatchPreview.bAllowed);
    TestTrue(
        TEXT("Forward receiver reports IncompatibleDirection for camera Up"),
        ForwardMismatchPreview.Rejection
            == EMotionTransferRejection::IncompatibleDirection);
    const FMotionTransferResult ForwardMismatchTransfer =
        Player->TryTransferToComponent(ReceiverForward, CameraUpResolution);
    TestFalse(
        TEXT("Camera Up transfer to Forward receiver fails"),
        ForwardMismatchTransfer.bSucceeded);
    TestTrue(
        TEXT("Player keeps motion after camera Up mismatch"),
        Player->HasMotionState());
    TestFalse(
        TEXT("Forward receiver stays empty after camera Up mismatch"),
        ReceiverForward->HasMotionState());

    ReleaseComponent(Source);
    ReleaseComponent(Player);
    ReleaseComponent(ReceiverForward);
    ReleaseComponent(ReceiverUp);
    ReleaseResolver(Resolver);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionActorPathDirectionConsistencyTest,
    "Transmit.MotionTransfer.ActorPathDirectionConsistency",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionActorPathDirectionConsistencyTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    FMotionState State = MakeState(TEXT("Source.Linear.001"), 600.0f);
    State.Direction = FVector::ForwardVector;
    UMotionTransferComponent* Source = MakeComponent(
        TEXT("Source"), true, false, EMotionEndpointMode::Store, State);
    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"), false, true, EMotionEndpointMode::Store);

    ATransmitMotionEndpointActor* ReceiverActor = MakeEndpointActor(
        TEXT("Actor.Receiver.Forward"),
        EMotionCanonicalDirection::Forward,
        EMotionEndpointMode::ConsumeOnReceive);
    TestNotNull(TEXT("Actor receiver was created"), ReceiverActor);
    if (!ReceiverActor)
    {
        ReleaseComponent(Player);
        ReleaseComponent(Source);
        return false;
    }
    UMotionTransferComponent* ReceiverMotion = ReceiverActor->Motion;
    TestNotNull(TEXT("Actor receiver owns a Motion component"), ReceiverMotion);
    if (!ReceiverMotion)
    {
        ReleaseEndpointActor(ReceiverActor);
        ReleaseComponent(Player);
        ReleaseComponent(Source);
        return false;
    }

    UMotionCanonicalDirectionResolver* Resolver = MakeResolver();
    const FRotator CameraYawZero(0.0f, 0.0f, 0.0f);
    const FMotionDirectionResolution Resolution =
        Resolver->ResolveDirection(FVector::ForwardVector, CameraYawZero);
    TestTrue(TEXT("Actor-path test resolution is valid"), Resolution.bValid);
    TestTrue(
        TEXT("Actor-path test resolution is Forward"),
        Resolution.CanonicalDirection == EMotionCanonicalDirection::Forward);
    TestTrue(
        TEXT("Actor receiver advertises the MotionTransferable interface"),
        ReceiverActor->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()));
    TestSamePtr(
        TEXT("Actor-interface helper resolves the receiver Motion component"),
        IMotionTransferable::CallGetMotionTransferComponent(ReceiverActor),
        ReceiverMotion);

    const FMotionTransferResult Capture = Player->TryCaptureFromComponent(Source);
    TestTrue(TEXT("Actor-path capture succeeds"), Capture.bSucceeded);
    FMotionState Carried;
    TestTrue(TEXT("Player owns Motion after capture"), Player->TryGetMotionState(Carried));

    FMotionTransferContext Context;
    Context.bInRange = true;
    Context.bOccluded = false;
    Context.DirectionResolution = Resolution;
    FMotionState ResolvedState = Carried;
    ResolvedState.Direction = Resolution.WorldDirection.GetSafeNormal();

    // Matching direction: the Actor-path Preview and Commit must both allow.
    const FMotionCompatibilityResult MatchPreview =
        IMotionTransferable::CallCanReceiveMotion(ReceiverActor, ResolvedState, Context);
    TestTrue(TEXT("Actor-path Preview allows a matching direction"), MatchPreview.bAllowed);
    TestEqual(
        TEXT("Actor-path Preview matching rejection is None"),
        MatchPreview.Rejection,
        EMotionTransferRejection::None);

    const FMotionTransferResult MatchTransfer =
        Player->TryTransferToActor(ReceiverActor, Context);
    TestTrue(TEXT("Actor-path Commit succeeds for a matching direction"), MatchTransfer.bSucceeded);
    TestTrue(TEXT("Actor-path Commit consumes the matching state"), MatchTransfer.bConsumed);
    TestFalse(TEXT("Player is empty after a matching Actor-path Commit"), Player->HasMotionState());
    TestFalse(TEXT("Actor receiver stores nothing after consume"), ReceiverMotion->HasMotionState());

    // Mismatching direction: Preview and Commit must reject with the same reason
    // and neither may consume Player Motion.
    Source->RestoreInitialState(false);
    Player->RestoreInitialState(false);
    const FMotionTransferResult SecondCapture = Player->TryCaptureFromComponent(Source);
    TestTrue(TEXT("Second Actor-path capture succeeds"), SecondCapture.bSucceeded);
    TestTrue(TEXT("Player owns Motion for the mismatch case"), Player->TryGetMotionState(Carried));

    ReceiverMotion->RequiredCanonicalDirection = EMotionCanonicalDirection::Up;
    ResolvedState = Carried;
    ResolvedState.Direction = Resolution.WorldDirection.GetSafeNormal();

    const FMotionCompatibilityResult MismatchPreview =
        IMotionTransferable::CallCanReceiveMotion(ReceiverActor, ResolvedState, Context);
    TestFalse(
        TEXT("Actor-path Preview rejects a mismatching direction"),
        MismatchPreview.bAllowed);
    TestEqual(
        TEXT("Actor-path Preview mismatch reason is IncompatibleDirection"),
        MismatchPreview.Rejection,
        EMotionTransferRejection::IncompatibleDirection);

    const FMotionTransferResult MismatchTransfer =
        Player->TryTransferToActor(ReceiverActor, Context);
    TestFalse(
        TEXT("Actor-path Commit rejects a mismatching direction"),
        MismatchTransfer.bSucceeded);
    TestEqual(
        TEXT("Actor-path Commit mismatch reason matches Preview"),
        MismatchTransfer.Rejection,
        MismatchPreview.Rejection);
    TestTrue(TEXT("Player keeps Motion after an Actor-path mismatch"), Player->HasMotionState());
    TestFalse(TEXT("Actor receiver stays empty after mismatch"), ReceiverMotion->HasMotionState());

    FMotionState Preserved;
    TestTrue(TEXT("Player Motion remains queryable after mismatch"), Player->TryGetMotionState(Preserved));
    TestEqual(TEXT("Mismatch preserves the carried Source identity"), Preserved.SourceId, State.SourceId);
    TestTrue(
        TEXT("Mismatch preserves the carried direction"),
        Preserved.Direction.Equals(FVector::ForwardVector, 1e-3f));

    ReleaseEndpointActor(ReceiverActor);
    ReleaseResolver(Resolver);
    ReleaseComponent(Player);
    ReleaseComponent(Source);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionDirectionPolicyOrdinaryCameraTest,
    "Transmit.MotionTransfer.DirectionPolicyOrdinaryCamera",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionDirectionPolicyOrdinaryCameraTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    FMotionState State = MakeState(TEXT("Source.Linear.Ordinary"), 600.0f);
    State.Direction = FVector::ForwardVector;
    TestEqual(
        TEXT("Ordinary Linear Motion defaults to CameraCanonical"),
        State.DirectionPolicy,
        EMotionDirectionPolicy::CameraCanonical);

    UMotionCanonicalDirectionResolver* Resolver = MakeResolver();
    const FRotator CameraYawZero(0.0f, 0.0f, 0.0f);
    const FMotionDirectionResolution Forward =
        UMotionInteractorComponent::ResolveTransferDirection(
            State,
            CameraYawZero,
            Resolver);
    TestTrue(TEXT("CameraCanonical resolves a valid direction"), Forward.bValid);
    TestTrue(
        TEXT("CameraCanonical resolves +X as Forward at yaw zero"),
        Forward.CanonicalDirection == EMotionCanonicalDirection::Forward);

    Resolver->ResetHysteresis();
    const FRotator CameraYawRight(0.0f, 90.0f, 0.0f);
    const FMotionDirectionResolution Yawed =
        UMotionInteractorComponent::ResolveTransferDirection(
            State,
            CameraYawRight,
            Resolver);
    TestTrue(TEXT("CameraCanonical stays valid after camera yaw"), Yawed.bValid);
    TestTrue(
        TEXT("CameraCanonical canonical output changes with camera yaw"),
        Yawed.CanonicalDirection == EMotionCanonicalDirection::Left
            && Yawed.CanonicalDirection != Forward.CanonicalDirection);

    Resolver->ResetHysteresis();
    const FRotator CameraPitchUp(50.0f, 0.0f, 0.0f);
    const FMotionDirectionResolution Pitched =
        UMotionInteractorComponent::ResolveTransferDirection(
            State,
            CameraPitchUp,
            Resolver);
    TestTrue(TEXT("CameraCanonical stays valid after camera pitch"), Pitched.bValid);
    TestTrue(
        TEXT("CameraCanonical canonical output changes with camera pitch"),
        Pitched.CanonicalDirection == EMotionCanonicalDirection::Up);
    TestTrue(
        TEXT("CameraCanonical world direction changes with camera pitch"),
        Pitched.WorldDirection.Equals(FVector::UpVector, 1e-3f));

    ReleaseResolver(Resolver);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionPreserveSourceDirectionPolicyTest,
    "Transmit.MotionTransfer.DirectionPolicyPreserveSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionPreserveSourceDirectionPolicyTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    FMotionState DashState = MakeState(TEXT("Boss.Dash.001"), 1200.0f);
    DashState.Direction = FVector::ForwardVector;
    DashState.DirectionPolicy = EMotionDirectionPolicy::PreserveSource;
    UMotionTransferComponent* Source = MakeComponent(
        TEXT("Boss"),
        true,
        false,
        EMotionEndpointMode::Store,
        DashState);
    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"),
        false,
        true,
        EMotionEndpointMode::Store);
    UMotionCanonicalDirectionResolver* Resolver = MakeResolver();

    const FMotionTransferResult Capture = Player->TryCaptureFromComponent(Source);
    TestTrue(TEXT("PreserveSource capture succeeds"), Capture.bSucceeded);
    FMotionState Carried;
    TestTrue(TEXT("Player owns PreserveSource Motion"), Player->TryGetMotionState(Carried));
    TestEqual(
        TEXT("Capture preserves the Dash direction policy"),
        Carried.DirectionPolicy,
        EMotionDirectionPolicy::PreserveSource);
    TestTrue(
        TEXT("Capture preserves the committed Dash world direction"),
        Carried.Direction.Equals(FVector::ForwardVector, 1e-3f));

    // Camera yaw and pitch after Capture must not re-author the preserved
    // world direction. CanonicalDirection stays None because PreserveSource is
    // not a camera-canonical result.
    const FRotator CameraYawForward(0.0f, 0.0f, 0.0f);
    const FRotator CameraYawBack(0.0f, 180.0f, 0.0f);
    const FRotator CameraYawRight(0.0f, 90.0f, 0.0f);
    const FRotator CameraYawLeft(0.0f, -90.0f, 0.0f);
    const FRotator CameraHighPitch(50.0f, 45.0f, 0.0f);
    const FRotator CameraLowPitch(-50.0f, -45.0f, 0.0f);
    const TArray<FRotator> Cameras = {
        CameraYawForward,
        CameraYawBack,
        CameraYawRight,
        CameraYawLeft,
        CameraHighPitch,
        CameraLowPitch
    };
    for (const FRotator& Camera : Cameras)
    {
        const FMotionDirectionResolution Resolution =
            UMotionInteractorComponent::ResolveTransferDirection(
                Carried,
                Camera,
                Resolver);
        TestTrue(TEXT("PreserveSource always resolves a valid direction"), Resolution.bValid);
        TestTrue(
            TEXT("PreserveSource never reports a camera-canonical direction"),
            Resolution.CanonicalDirection == EMotionCanonicalDirection::None);
        TestTrue(
            TEXT("PreserveSource Preview world direction stays the Dash direction"),
            Resolution.WorldDirection.Equals(FVector::ForwardVector, 1e-3f));
    }

    // Preview and Commit consume the same resolution result.
    ATransmitMotionEndpointActor* ReceiverActor = MakeEndpointActor(
        TEXT("Actor.Receiver.Boss"),
        EMotionCanonicalDirection::None,
        EMotionEndpointMode::ConsumeOnReceive);
    TestNotNull(TEXT("Boss receiver actor was created"), ReceiverActor);
    if (!ReceiverActor)
    {
        ReleaseResolver(Resolver);
        ReleaseComponent(Player);
        ReleaseComponent(Source);
        return false;
    }
    UMotionTransferComponent* ReceiverMotion = ReceiverActor->Motion;
    TestNotNull(TEXT("Boss receiver owns a Motion component"), ReceiverMotion);
    if (!ReceiverMotion)
    {
        ReleaseEndpointActor(ReceiverActor);
        ReleaseResolver(Resolver);
        ReleaseComponent(Player);
        ReleaseComponent(Source);
        return false;
    }

    FMotionTransferContext Context;
    Context.bInRange = true;
    Context.bOccluded = false;
    Context.DirectionResolution =
        UMotionInteractorComponent::ResolveTransferDirection(
            Carried,
            CameraYawRight,
            Resolver);
    FMotionState ResolvedState = Carried;
    ResolvedState.Direction = Context.DirectionResolution.WorldDirection.GetSafeNormal();

    const FMotionCompatibilityResult MatchPreview =
        IMotionTransferable::CallCanReceiveMotion(ReceiverActor, ResolvedState, Context);
    TestTrue(TEXT("PreserveSource Preview allows the Ram-style receiver"), MatchPreview.bAllowed);
    const FMotionTransferResult MatchTransfer =
        Player->TryTransferToActor(ReceiverActor, Context);
    TestTrue(TEXT("PreserveSource Commit succeeds"), MatchTransfer.bSucceeded);
    TestTrue(TEXT("PreserveSource Commit consumes the state"), MatchTransfer.bConsumed);
    TestTrue(
        TEXT("Commit world direction equals Preview world direction"),
        MatchTransfer.StateSnapshot.Direction.Equals(
            Context.DirectionResolution.WorldDirection,
            1e-3f));
    TestEqual(
        TEXT("Committed state keeps the PreserveSource policy"),
        MatchTransfer.StateSnapshot.DirectionPolicy,
        EMotionDirectionPolicy::PreserveSource);

    // Rejected PreserveSource transfer preserves Player ownership.
    Source->RestoreInitialState(false);
    const FMotionTransferResult SecondCapture = Player->TryCaptureFromComponent(Source);
    TestTrue(TEXT("Second PreserveSource capture succeeds"), SecondCapture.bSucceeded);
    TestTrue(TEXT("Player owns Motion for rejection case"), Player->TryGetMotionState(Carried));

    ATransmitMotionEndpointActor* UpReceiverActor = MakeEndpointActor(
        TEXT("Actor.Receiver.Up"),
        EMotionCanonicalDirection::Up,
        EMotionEndpointMode::ConsumeOnReceive);
    TestNotNull(TEXT("Up receiver actor was created"), UpReceiverActor);
    if (!UpReceiverActor)
    {
        ReleaseEndpointActor(ReceiverActor);
        ReleaseResolver(Resolver);
        ReleaseComponent(Player);
        ReleaseComponent(Source);
        return false;
    }
    UMotionTransferComponent* UpReceiverMotion = UpReceiverActor->Motion;
    TestNotNull(TEXT("Up receiver owns a Motion component"), UpReceiverMotion);
    if (!UpReceiverMotion)
    {
        ReleaseEndpointActor(UpReceiverActor);
        ReleaseEndpointActor(ReceiverActor);
        ReleaseResolver(Resolver);
        ReleaseComponent(Player);
        ReleaseComponent(Source);
        return false;
    }

    FMotionTransferContext MismatchContext;
    MismatchContext.bInRange = true;
    MismatchContext.bOccluded = false;
    MismatchContext.DirectionResolution =
        UMotionInteractorComponent::ResolveTransferDirection(
            Carried,
            CameraHighPitch,
            Resolver);
    ResolvedState = Carried;
    ResolvedState.Direction = MismatchContext.DirectionResolution.WorldDirection.GetSafeNormal();

    const FMotionCompatibilityResult MismatchPreview =
        IMotionTransferable::CallCanReceiveMotion(
            UpReceiverActor,
            ResolvedState,
            MismatchContext);
    TestFalse(
        TEXT("PreserveSource Preview rejects a RequiredCanonicalDirection target"),
        MismatchPreview.bAllowed);
    TestEqual(
        TEXT("PreserveSource Preview rejection is IncompatibleDirection"),
        MismatchPreview.Rejection,
        EMotionTransferRejection::IncompatibleDirection);
    const FMotionTransferResult MismatchTransfer =
        Player->TryTransferToActor(UpReceiverActor, MismatchContext);
    TestFalse(TEXT("PreserveSource Commit rejects the same target"), MismatchTransfer.bSucceeded);
    TestEqual(
        TEXT("PreserveSource Commit rejection matches Preview"),
        MismatchTransfer.Rejection,
        MismatchPreview.Rejection);
    TestTrue(TEXT("Player keeps Motion after rejected PreserveSource Commit"), Player->HasMotionState());
    FMotionState Preserved;
    TestTrue(TEXT("Player Motion remains queryable after rejection"), Player->TryGetMotionState(Preserved));
    TestEqual(
        TEXT("Rejection preserves the PreserveSource policy"),
        Preserved.DirectionPolicy,
        EMotionDirectionPolicy::PreserveSource);
    TestTrue(
        TEXT("Rejection preserves the Dash world direction"),
        Preserved.Direction.Equals(FVector::ForwardVector, 1e-3f));
    TestFalse(
        TEXT("Rejected target stays empty"),
        UpReceiverMotion ? UpReceiverMotion->HasMotionState() : false);

    // Reset removes stale Player policy/state and restores source snapshots.
    TestTrue(TEXT("Player Reset succeeds"), Player->RestoreInitialState(false));
    TestFalse(TEXT("Player has no Motion after Reset"), Player->HasMotionState());
    TestTrue(TEXT("Source Reset succeeds"), Source->RestoreInitialState(false));
    FMotionState RestoredSource;
    TestTrue(TEXT("Source state is restored"), Source->TryGetMotionState(RestoredSource));
    TestEqual(
        TEXT("Reset restores the snapshotted PreserveSource policy"),
        RestoredSource.DirectionPolicy,
        EMotionDirectionPolicy::PreserveSource);

    // A fresh ordinary capture after Reset must be camera-driven again: no
    // stale PreserveSource leakage from the Player reset path.
    FMotionState OrdinaryState = MakeState(TEXT("Source.Linear.AfterReset"), 600.0f);
    OrdinaryState.Direction = FVector::ForwardVector;
    UMotionTransferComponent* OrdinarySource = MakeComponent(
        TEXT("OrdinarySource"),
        true,
        false,
        EMotionEndpointMode::Store,
        OrdinaryState);
    const FMotionTransferResult OrdinaryCapture =
        Player->TryCaptureFromComponent(OrdinarySource);
    TestTrue(TEXT("Ordinary capture succeeds after Reset"), OrdinaryCapture.bSucceeded);
    FMotionState OrdinaryCarried;
    TestTrue(TEXT("Player owns ordinary Motion after Reset"), Player->TryGetMotionState(OrdinaryCarried));
    Resolver->ResetHysteresis();
    const FMotionDirectionResolution OrdinaryAfterReset =
        UMotionInteractorComponent::ResolveTransferDirection(
            OrdinaryCarried,
            CameraYawRight,
            Resolver);
    TestTrue(
        TEXT("Post-Reset ordinary Motion is camera-driven again"),
        OrdinaryAfterReset.bValid
            && OrdinaryAfterReset.CanonicalDirection
                == EMotionCanonicalDirection::Left);

    ReleaseComponent(OrdinarySource);
    ReleaseEndpointActor(UpReceiverActor);
    ReleaseEndpointActor(ReceiverActor);
    ReleaseResolver(Resolver);
    ReleaseComponent(Player);
    ReleaseComponent(Source);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionDirectionalCarrierCoreTest,
    "Transmit.MotionTransfer.DirectionalCarrierCore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionDirectionalCarrierCoreTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    FMotionState State = MakeState(TEXT("Source.Linear.001"), 600.0f);
    State.Direction = FVector::ForwardVector;
    UMotionTransferComponent* Source = MakeComponent(
        TEXT("Source"),
        true,
        false,
        EMotionEndpointMode::Store,
        State);
    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"),
        false,
        true,
        EMotionEndpointMode::Store);
    UMotionCanonicalDirectionResolver* Resolver = MakeResolver();
    ATransmitDirectionalCarrierActor* Carrier = MakeCarrier(TEXT("Carrier"), 400.0f);
    TestNotNull(TEXT("Carrier was created"), Carrier);
    if (!Carrier)
    {
        ReleaseResolver(Resolver);
        ReleaseComponent(Player);
        ReleaseComponent(Source);
        return false;
    }
    Carrier->SetActorLocation(FVector::ZeroVector);

    const FMotionTransferResult Capture = Player->TryCaptureFromComponent(Source);
    TestTrue(TEXT("Carrier setup capture succeeds"), Capture.bSucceeded);
    FMotionState Carried;
    TestTrue(TEXT("Player owns Motion before Carrier transfer"), Player->TryGetMotionState(Carried));

    // A/B: receive ordinary resolved Linear, then begin world movement along
    // the resolved direction (camera pitch selects Up).
    Resolver->ResetHysteresis();
    const FRotator CameraPitchUp(50.0f, 0.0f, 0.0f);
    const FMotionDirectionResolution Resolution =
        UMotionInteractorComponent::ResolveTransferDirection(
            Carried,
            CameraPitchUp,
            Resolver);
    TestTrue(
        TEXT("Carrier transfer resolves to Up"),
        Resolution.bValid
            && Resolution.CanonicalDirection == EMotionCanonicalDirection::Up);

    FMotionTransferContext TransferContext;
    TransferContext.bInRange = true;
    TransferContext.bOccluded = false;
    TransferContext.DirectionResolution = Resolution;
    const FMotionTransferResult Transfer =
        Player->TryTransferToActor(Carrier, TransferContext);
    TestTrue(TEXT("Carrier receives ordinary Linear Motion"), Transfer.bSucceeded);
    TestTrue(TEXT("Carrier owns Motion after receive"), Carrier->Motion->HasMotionState());
    Carrier->Tick(0.0f);
    TestTrue(TEXT("Carrier becomes movement-active after receive"), Carrier->IsMovementActive());

    FMotionState CarrierState;
    TestTrue(TEXT("Carrier state is queryable"), Carrier->Motion->TryGetMotionState(CarrierState));
    TestTrue(
        TEXT("Carrier direction follows the resolved ordinary direction"),
        CarrierState.Direction.Equals(FVector::UpVector, 1e-3f));

    const FVector LocationBefore = Carrier->GetActorLocation();
    Carrier->Tick(0.25f);
    const FVector LocationAfter = Carrier->GetActorLocation();
    TestTrue(
        TEXT("Carrier root moved in world space along the resolved direction"),
        (LocationAfter - LocationBefore).Z > 1.0f);
    TestTrue(
        TEXT("Carrier did not drift on the horizontal axes"),
        FMath::IsNearlyZero((LocationAfter - LocationBefore).X, 1e-2f)
            && FMath::IsNearlyZero((LocationAfter - LocationBefore).Y, 1e-2f));

    // D/E: capture the moving Carrier; the existing ownership transaction
    // removes the state and gameplay movement stops immediately.
    FMotionTransferContext CaptureContext;
    CaptureContext.bInRange = true;
    CaptureContext.bOccluded = false;
    const FMotionTransferResult CarrierCapture =
        Player->TryCaptureFromActor(Carrier, CaptureContext);
    TestTrue(TEXT("Player captures Motion from the Carrier"), CarrierCapture.bSucceeded);
    TestTrue(TEXT("Player owns the captured Motion"), Player->HasMotionState());
    TestFalse(TEXT("Carrier is empty after capture"), Carrier->Motion->HasMotionState());
    Carrier->Tick(0.0f);
    TestFalse(TEXT("Capture stops Carrier movement"), Carrier->IsMovementActive());
    TestFalse(TEXT("Capture leaves no collision-stop state"), Carrier->IsBlockedByCollision());

    // F: re-transfer the captured Motion to another valid target using the
    // existing rules.
    ATransmitMotionEndpointActor* ReceiverActor = MakeEndpointActor(
        TEXT("Actor.Receiver.Store"),
        EMotionCanonicalDirection::None,
        EMotionEndpointMode::Store);
    TestNotNull(TEXT("Store receiver was created"), ReceiverActor);
    if (ReceiverActor)
    {
        FMotionState Recarried;
        TestTrue(TEXT("Player carries Motion for re-transfer"), Player->TryGetMotionState(Recarried));
        FMotionTransferContext ReTransferContext;
        ReTransferContext.bInRange = true;
        ReTransferContext.bOccluded = false;
        ReTransferContext.DirectionResolution =
            UMotionInteractorComponent::ResolveTransferDirection(
                Recarried,
                CameraPitchUp,
                Resolver);
        const FMotionTransferResult ReTransfer =
            Player->TryTransferToActor(ReceiverActor, ReTransferContext);
        TestTrue(TEXT("Captured Motion re-transfers to another target"), ReTransfer.bSucceeded);
        TestTrue(TEXT("Store receiver owns the re-transferred Motion"), ReceiverActor->Motion->HasMotionState());
        ReleaseEndpointActor(ReceiverActor);
    }

    ReleaseCarrier(Carrier);
    ReleaseResolver(Resolver);
    ReleaseComponent(Player);
    ReleaseComponent(Source);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionDirectionalCarrierResetTest,
    "Transmit.MotionTransfer.DirectionalCarrierReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionDirectionalCarrierResetTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    FMotionState State = MakeState(TEXT("Source.Linear.Reset"), 600.0f);
    State.Direction = FVector::ForwardVector;
    UMotionTransferComponent* Source = MakeComponent(
        TEXT("Source"),
        true,
        false,
        EMotionEndpointMode::Store,
        State);
    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"),
        false,
        true,
        EMotionEndpointMode::Store);
    UMotionCanonicalDirectionResolver* Resolver = MakeResolver();
    ATransmitDirectionalCarrierActor* Carrier = MakeCarrier(TEXT("Carrier"), 400.0f);
    TestNotNull(TEXT("Reset Carrier was created"), Carrier);
    if (!Carrier)
    {
        ReleaseResolver(Resolver);
        ReleaseComponent(Player);
        ReleaseComponent(Source);
        return false;
    }

    const FVector InitialLocation(100.0f, 200.0f, 300.0f);
    for (int32 Cycle = 0; Cycle < 3; ++Cycle)
    {
        Carrier->SetActorLocation(InitialLocation);
        TestTrue(
            FString::Printf(TEXT("Cycle %d Carrier reset succeeds"), Cycle + 1),
            Carrier->Motion->RestoreInitialState(true));
        TestTrue(
            TEXT("Reset source state succeeds"),
            Source->RestoreInitialState(false));
        TestTrue(
            TEXT("Reset Player state succeeds"),
            Player->RestoreInitialState(false));
        TestFalse(TEXT("Carrier is empty after reset"), Carrier->Motion->HasMotionState());
        TestFalse(TEXT("Carrier movement is inactive after reset"), Carrier->IsMovementActive());
        TestFalse(TEXT("Carrier collision-stop state is clear after reset"), Carrier->IsBlockedByCollision());

        const FMotionTransferResult Capture = Player->TryCaptureFromComponent(Source);
        TestTrue(FString::Printf(TEXT("Cycle %d capture succeeds"), Cycle + 1), Capture.bSucceeded);
        FMotionState Carried;
        TestTrue(TEXT("Player carries Motion in cycle"), Player->TryGetMotionState(Carried));

        Resolver->ResetHysteresis();
        const FRotator CameraPitchUp(50.0f, 0.0f, 0.0f);
        FMotionTransferContext TransferContext;
        TransferContext.bInRange = true;
        TransferContext.bOccluded = false;
        TransferContext.DirectionResolution =
            UMotionInteractorComponent::ResolveTransferDirection(
                Carried,
                CameraPitchUp,
                Resolver);
        const FMotionTransferResult Transfer =
            Player->TryTransferToActor(Carrier, TransferContext);
        TestTrue(FString::Printf(TEXT("Cycle %d Carrier receives Motion"), Cycle + 1), Transfer.bSucceeded);
        Carrier->Tick(0.0f);
        TestTrue(TEXT("Carrier is movement-active in cycle"), Carrier->IsMovementActive());

        Carrier->Tick(0.2f);
        TestFalse(
            TEXT("Carrier moved away from the initial transform in cycle"),
            Carrier->GetActorLocation().Equals(InitialLocation, 1e-2f));

        FMotionTransferContext CaptureContext;
        CaptureContext.bInRange = true;
        CaptureContext.bOccluded = false;
        const FMotionTransferResult CarrierCapture =
            Player->TryCaptureFromActor(Carrier, CaptureContext);
        TestTrue(FString::Printf(TEXT("Cycle %d Carrier capture succeeds"), Cycle + 1), CarrierCapture.bSucceeded);
        Carrier->Tick(0.0f);
        TestFalse(TEXT("Carrier stops moving after cycle capture"), Carrier->IsMovementActive());
    }

    ReleaseCarrier(Carrier);
    ReleaseResolver(Resolver);
    ReleaseComponent(Player);
    ReleaseComponent(Source);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionDirectionalCarrierCollisionTest,
    "Transmit.MotionTransfer.DirectionalCarrierCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionDirectionalCarrierCollisionTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    TestNotNull(TEXT("Collision test world was created"), World);
    if (!World)
    {
        return false;
    }

    ATransmitDirectionalCarrierActor* Carrier =
        World->SpawnActor<ATransmitDirectionalCarrierActor>(
            FVector::ZeroVector,
            FRotator::ZeroRotator);
    TestNotNull(TEXT("World Carrier was spawned"), Carrier);
    if (!Carrier)
    {
        World->DestroyWorld(false);
        return false;
    }

    AActor* Blocker = World->SpawnActor<AActor>(
        FVector(300.0f, 0.0f, 0.0f),
        FRotator::ZeroRotator);
    TestNotNull(TEXT("Blocker was spawned"), Blocker);
    UBoxComponent* BlockerBox = NewObject<UBoxComponent>(Blocker);
    BlockerBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
    BlockerBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BlockerBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    BlockerBox->SetCollisionResponseToAllChannels(ECR_Block);
    Blocker->SetRootComponent(BlockerBox);
    BlockerBox->RegisterComponent();
    BlockerBox->SetWorldLocation(FVector(300.0f, 0.0f, 0.0f));
    Blocker->SetActorLocation(FVector(300.0f, 0.0f, 0.0f));

    Carrier->MovementSpeed = 200.0f;
    if (Carrier->Motion)
    {
        Carrier->Motion->ConfigureForTesting(
            TEXT("Carrier"),
            true,
            true,
            EMotionEndpointMode::Store,
            TOptional<FMotionState>());
    }

    FMotionState DashLike = MakeState(TEXT("Source.Linear.Collision"), 600.0f);
    DashLike.Direction = FVector::ForwardVector;
    TestTrue(
        TEXT("Collision Carrier receives seeded Motion"),
        Carrier->Motion->GrantMotionState(DashLike));
    Carrier->Tick(0.0f);
    TestTrue(TEXT("Collision Carrier is movement-active"), Carrier->IsMovementActive());

    for (int32 TickIndex = 0; TickIndex < 20; ++TickIndex)
    {
        Carrier->Tick(0.1f);
    }

    TestTrue(TEXT("Collision Carrier stops on blocking hit"), Carrier->IsBlockedByCollision());
    TestFalse(TEXT("Collision Carrier stops movement after blocking hit"), Carrier->IsMovementActive());
    TestTrue(TEXT("Collision Carrier keeps owned Motion after stop"), Carrier->Motion->HasMotionState());

    const FVector StoppedLocation = Carrier->GetActorLocation();
    for (int32 TickIndex = 0; TickIndex < 5; ++TickIndex)
    {
        Carrier->Tick(0.1f);
    }
    TestTrue(
        TEXT("Collision Carrier does not jitter or penetrate after stop"),
        Carrier->GetActorLocation().Equals(StoppedLocation, 1e-2f));

    World->DestroyWorld(false);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionChargerActorStructuralTest,
    "Transmit.MotionTransfer.ChargerActorStructure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionChargerActorStructuralTest::RunTest(const FString& Parameters)
{
    const ATransmitChargerActor* DefaultCharger = GetDefault<ATransmitChargerActor>();
    TestNotNull(TEXT("Charger default actor exists"), DefaultCharger);
    if (!DefaultCharger)
    {
        return false;
    }

    const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(DefaultCharger->GetRootComponent());
    TestNotNull(TEXT("Charger root collision is a capsule"), Capsule);
    if (Capsule)
    {
        TestTrue(
            TEXT("Charger root collision remains enabled"),
            Capsule->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
        TestEqual(
            TEXT("Charger root uses BlockAllDynamic"),
            Capsule->GetCollisionProfileName(),
            TEXT("BlockAllDynamic"));
        TestTrue(
            TEXT("Charger capsule has the authored half-height"),
            FMath::IsNearlyEqual(Capsule->GetScaledCapsuleHalfHeight(), 70.0f, 1.0f));
    }

    TestNotNull(TEXT("Charger Body presentation exists"), DefaultCharger->Body.Get());
    if (DefaultCharger->Body)
    {
        TestEqual(
            TEXT("Charger Body presentation does not compete with root collision"),
            DefaultCharger->Body->GetCollisionEnabled(),
            ECollisionEnabled::NoCollision);
    }

    TestNotNull(TEXT("Charger Motion component exists"), DefaultCharger->Motion.Get());
    if (DefaultCharger->Motion)
    {
        TestEqual(
            TEXT("Charger has a stable participant id"),
            DefaultCharger->Motion->GetParticipantId(),
            TEXT("Charger"));
        TestTrue(
            TEXT("Charger can provide captured dash Motion"),
            DefaultCharger->Motion->bCanProvideMotion);
        TestFalse(
            TEXT("Charger never stores a transferred Player state"),
            DefaultCharger->Motion->bCanReceiveMotion);
    }

    TestNotNull(
        TEXT("Charger owns a state machine instance"),
        DefaultCharger->StateMachine.Get());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMotionChargerActorCaptureGateTest,
    "Transmit.MotionTransfer.ChargerActorCaptureGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMotionChargerActorCaptureGateTest::RunTest(const FString& Parameters)
{
    using namespace MotionTransferTests;

    ATransmitChargerActor* Charger = NewObject<ATransmitChargerActor>(GetTransientPackage());
    Charger->AddToRoot();
    UMotionChargerStateMachine* Machine = Charger->StateMachine;
    UMotionTransferComponent* ChargerMotion = Charger->Motion;
    TestNotNull(TEXT("Charger instance has a state machine"), Machine);
    TestNotNull(TEXT("Charger instance has a Motion component"), ChargerMotion);
    if (!Machine || !ChargerMotion)
    {
        Charger->RemoveFromRoot();
        return false;
    }

    Machine->IdleDurationSeconds = 0.1f;
    Machine->TelegraphDurationSeconds = 0.1f;
    Machine->DashDurationSeconds = 1.0f;
    Machine->RecoveryDurationSeconds = 1.0f;
    Machine->DashCommitWindowDelaySeconds = 0.05f;

    FMotionState DashState = MakeState(TEXT("Charger.Dash.001"), 1200.0f);
    DashState.DirectionPolicy = EMotionDirectionPolicy::PreserveSource;
    DashState.Direction = FVector::ForwardVector;
    TestTrue(
        TEXT("Charger starts the Dash window with high-magnitude Motion"),
        ChargerMotion->GrantMotionState(DashState));

    FMotionTransferContext Context;
    Machine->Start();
    FMotionCompatibilityResult GateResult =
        Charger->CanCaptureMotion_Implementation(Context);
    TestFalse(TEXT("Idle rejects Charger Capture"), GateResult.bAllowed);
    TestEqual(
        TEXT("Idle rejection is TimingRejected"),
        GateResult.Rejection,
        EMotionTransferRejection::TimingRejected);

    Machine->Tick(0.2f);
    GateResult = Charger->CanCaptureMotion_Implementation(Context);
    TestFalse(TEXT("Telegraph rejects Charger Capture"), GateResult.bAllowed);
    TestEqual(
        TEXT("Telegraph rejection is TimingRejected"),
        GateResult.Rejection,
        EMotionTransferRejection::TimingRejected);

    Machine->Tick(0.2f);
    TestFalse(
        TEXT("Capture is not open before the Dash commit delay"),
        Machine->IsCaptureWindowOpen());
    Machine->Tick(0.1f);
    TestTrue(TEXT("Dash commit delay opens the Capture window"), Machine->IsCaptureWindowOpen());

    GateResult = Charger->CanCaptureMotion_Implementation(Context);
    TestTrue(TEXT("Open Dash window accepts Charger Capture"), GateResult.bAllowed);
    TestTrue(
        TEXT("Charger class advertises MotionTransferable for Actor targeting"),
        Charger->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()));
    TestNotNull(
        TEXT("Charger exposes a native MotionTransferable address"),
        Charger->GetNativeInterfaceAddress(UMotionTransferable::StaticClass()));
    const FMotionCompatibilityResult ActorInterfaceGate =
        IMotionTransferable::CallCanCaptureMotion(Charger, Context);
    TestTrue(
        TEXT("Actor-interface helper agrees with the direct Charger gate"),
        ActorInterfaceGate.bAllowed);
    TestEqual(
        TEXT("Actor-interface helper returns the same open-window result"),
        ActorInterfaceGate.Rejection,
        EMotionTransferRejection::None);
    UMotionTransferComponent* HelperMotion =
        IMotionTransferable::CallGetMotionTransferComponent(Charger);
    TestSamePtr(
        TEXT("Actor-interface helper resolves the Charger Motion component"),
        HelperMotion,
        ChargerMotion);

    UMotionTransferComponent* Player = MakeComponent(
        TEXT("Player"), false, true, EMotionEndpointMode::Store);
    const FMotionTransferResult Capture =
        Player->TryCaptureFromActor(Charger, Context);
    TestTrue(TEXT("Actor-path Capture succeeds during the open Dash window"), Capture.bSucceeded);
    TestFalse(TEXT("Charger Motion is removed after Capture"), ChargerMotion->HasMotionState());

    FMotionState CapturedState;
    TestTrue(TEXT("Player owns Motion after Charger Capture"), Player->TryGetMotionState(CapturedState));
    TestTrue(
        TEXT("Captured Charger Motion preserves direction"),
        CapturedState.Direction.Equals(FVector::ForwardVector, 1e-3f));
    TestTrue(
        TEXT("Captured Charger Motion preserves high magnitude"),
        FMath::IsNearlyEqual(CapturedState.Magnitude, 1200.0f, 1e-3f));
    TestEqual(
        TEXT("Captured Charger Motion keeps its Source identity"),
        CapturedState.SourceId,
        TEXT("Charger.Dash.001"));
    TestEqual(
        TEXT("Captured Charger Motion keeps the PreserveSource direction policy"),
        CapturedState.DirectionPolicy,
        EMotionDirectionPolicy::PreserveSource);

    Machine->ForceRecovery();
    TestTrue(TEXT("Capture stops the Dash window"), !Machine->IsCaptureWindowOpen());
    TestTrue(
        TEXT("Charger can be re-seeded after the capture stops the dash"),
        ChargerMotion->GrantMotionState(DashState));
    GateResult = Charger->CanCaptureMotion_Implementation(Context);
    TestFalse(TEXT("Recovery rejects Charger Capture"), GateResult.bAllowed);
    TestEqual(
        TEXT("Recovery rejection is TimingRejected"),
        GateResult.Rejection,
        EMotionTransferRejection::TimingRejected);

    ReleaseComponent(Player);
    Charger->RemoveFromRoot();
    return true;
}

#endif
