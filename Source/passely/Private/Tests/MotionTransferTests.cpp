#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

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

#endif
