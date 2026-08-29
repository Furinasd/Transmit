#include "Motion/MotionTransferComponent.h"

#include "GameFramework/Actor.h"
#include "Motion/MotionTransferable.h"
#include "Motion/MotionTransferSettings.h"
#include "passely.h"

TArray<UMotionTransferComponent::FMotionPendingNotification>
    UMotionTransferComponent::GlobalPendingNotifications;
bool UMotionTransferComponent::bGlobalDispatchingNotifications = false;

UMotionTransferComponent::UMotionTransferComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMotionTransferComponent::BeginPlay()
{
    Super::BeginPlay();

    if (ParticipantId.IsNone() && GetOwner())
    {
        ParticipantId = GetOwner()->GetFName();
        UE_LOG(
            LogMotionTransfer,
            Warning,
            TEXT("%s uses its Actor name as ParticipantId. Author an explicit room-stable id."),
            *GetNameSafe(GetOwner()));
    }

    if (bStartsWithMotion)
    {
        if (InitialMotion.SourceId.IsNone())
        {
            InitialMotion.SourceId = ParticipantId;
        }

        if (InitialMotion.IsValid())
        {
            SetStateWithoutNotification(InitialMotion);
        }
        else
        {
            UE_LOG(
                LogMotionTransfer,
                Error,
                TEXT("%s has an invalid initial Motion State; it will start empty."),
                *GetNameSafe(GetOwner()));
            ClearStateWithoutNotification();
        }
    }

    CaptureInitialSnapshot();
}

bool UMotionTransferComponent::HasMotionState() const
{
    return bHasMotion;
}

bool UMotionTransferComponent::TryGetMotionState(FMotionState& OutState) const
{
    if (!bHasMotion)
    {
        return false;
    }

    OutState = CurrentMotion;
    return true;
}

FGameplayTag UMotionTransferComponent::GetMagnitudeTier() const
{
    return bHasMotion
        ? GetDefault<UMotionTransferSettings>()->ResolveMagnitudeTier(CurrentMotion.Magnitude)
        : FGameplayTag();
}

FName UMotionTransferComponent::GetParticipantId() const
{
    return ParticipantId;
}

EMotionEndpointMode UMotionTransferComponent::GetEndpointMode() const
{
    return EndpointMode;
}

FMotionCompatibilityResult UMotionTransferComponent::CanProvideMotion() const
{
    if (!bCanProvideMotion)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::InvalidSource);
    }

    if (!bHasMotion)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::SourceEmpty);
    }

    return CurrentMotion.IsValid()
        ? FMotionCompatibilityResult::Allow()
        : FMotionCompatibilityResult::Reject(EMotionTransferRejection::InvalidMotionState);
}

FMotionCompatibilityResult UMotionTransferComponent::CanReceiveState(const FMotionState& State) const
{
    if (!bCanReceiveMotion)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::InvalidTarget);
    }

    if (bHasMotion)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::CarrierOccupied);
    }

    if (!State.IsValid())
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::InvalidMotionState);
    }

    if (State.Type != EMotionType::Linear)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::IncompatibleType);
    }

    if (bRequireDirection)
    {
        const FVector NormalizedRequiredDirection = RequiredDirection.GetSafeNormal();
        if (NormalizedRequiredDirection.IsNearlyZero()
            || FVector::DotProduct(State.Direction, NormalizedRequiredDirection) < MinimumDirectionDot)
        {
            return FMotionCompatibilityResult::Reject(
                EMotionTransferRejection::IncompatibleDirection);
        }
    }

    const FGameplayTag MagnitudeTier =
        GetDefault<UMotionTransferSettings>()->ResolveMagnitudeTier(State.Magnitude);
    if (!MagnitudeTier.IsValid())
    {
        return FMotionCompatibilityResult::Reject(
            EMotionTransferRejection::IncompatibleMagnitudeTier);
    }

    if (!AcceptedMagnitudeTiers.IsEmpty())
    {
        FGameplayTagContainer MagnitudeTags;
        MagnitudeTags.AddTag(MagnitudeTier);
        if (!AcceptedMagnitudeTiers.Matches(MagnitudeTags))
        {
            return FMotionCompatibilityResult::Reject(
                EMotionTransferRejection::IncompatibleMagnitudeTier);
        }
    }

    return FMotionCompatibilityResult::Allow();
}

FMotionTransferResult UMotionTransferComponent::TryCaptureFromActor(
    AActor* SourceActor,
    const FMotionTransferContext& Context)
{
    if (!IsValid(SourceActor)
        || !SourceActor->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()))
    {
        return NotifyRejectedRequest(
            EMotionTransferVerb::Capture,
            EMotionTransferRejection::InvalidSource);
    }

    const FMotionCompatibilityResult Compatibility =
        IMotionTransferable::Execute_CanCaptureMotion(SourceActor, Context);
    if (!Compatibility.bAllowed)
    {
        return NotifyRejectedRequest(EMotionTransferVerb::Capture, Compatibility.Rejection);
    }

    UMotionTransferComponent* SourceComponent =
        IMotionTransferable::Execute_GetMotionTransferComponent(SourceActor);
    return TryMoveBetween(SourceComponent, this, EMotionTransferVerb::Capture, true);
}

FMotionTransferResult UMotionTransferComponent::TryTransferToActor(
    AActor* TargetActor,
    const FMotionTransferContext& Context)
{
    if (!IsValid(TargetActor)
        || !TargetActor->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()))
    {
        return NotifyRejectedRequest(
            EMotionTransferVerb::Transfer,
            EMotionTransferRejection::InvalidTarget);
    }

    if (!bHasMotion)
    {
        return NotifyRejectedRequest(
            EMotionTransferVerb::Transfer,
            EMotionTransferRejection::SourceEmpty);
    }

    const FMotionCompatibilityResult Compatibility =
        IMotionTransferable::Execute_CanReceiveMotion(TargetActor, CurrentMotion, Context);
    if (!Compatibility.bAllowed)
    {
        return NotifyRejectedRequest(EMotionTransferVerb::Transfer, Compatibility.Rejection);
    }

    UMotionTransferComponent* TargetComponent =
        IMotionTransferable::Execute_GetMotionTransferComponent(TargetActor);
    return TryMoveBetween(this, TargetComponent, EMotionTransferVerb::Transfer, false);
}

FMotionTransferResult UMotionTransferComponent::TryCaptureFromComponent(
    UMotionTransferComponent* SourceComponent)
{
    return TryMoveBetween(SourceComponent, this, EMotionTransferVerb::Capture, true);
}

FMotionTransferResult UMotionTransferComponent::TryTransferToComponent(
    UMotionTransferComponent* TargetComponent)
{
    return TryMoveBetween(this, TargetComponent, EMotionTransferVerb::Transfer, false);
}

FMotionTransferResult UMotionTransferComponent::TryMoveBetween(
    UMotionTransferComponent* SourceComponent,
    UMotionTransferComponent* TargetComponent,
    const EMotionTransferVerb Verb,
    const bool bRequireProviderFlag)
{
    if (bTransactionInProgress)
    {
        return NotifyRejectedRequest(Verb, EMotionTransferRejection::TransactionBusy);
    }

    if (!IsValid(SourceComponent) || SourceComponent == TargetComponent)
    {
        return NotifyRejectedRequest(Verb, EMotionTransferRejection::InvalidSource);
    }

    if (!IsValid(TargetComponent))
    {
        return NotifyRejectedRequest(Verb, EMotionTransferRejection::InvalidTarget);
    }

    if (bRequireProviderFlag)
    {
        const FMotionCompatibilityResult ProviderResult = SourceComponent->CanProvideMotion();
        if (!ProviderResult.bAllowed)
        {
            return NotifyRejectedRequest(Verb, ProviderResult.Rejection);
        }
    }
    else if (!SourceComponent->bHasMotion)
    {
        return NotifyRejectedRequest(Verb, EMotionTransferRejection::SourceEmpty);
    }

    const FMotionState MovedState = SourceComponent->CurrentMotion;
    const FMotionCompatibilityResult ReceiverResult = TargetComponent->CanReceiveState(MovedState);
    if (!ReceiverResult.bAllowed)
    {
        return NotifyRejectedRequest(Verb, ReceiverResult.Rejection);
    }

    bTransactionInProgress = true;

    SourceComponent->ClearStateWithoutNotification();
    const bool bConsumed = TargetComponent->EndpointMode == EMotionEndpointMode::ConsumeOnReceive;
    if (!bConsumed)
    {
        TargetComponent->SetStateWithoutNotification(MovedState);
    }

    const bool bPostconditionValid = !SourceComponent->bHasMotion
        && (bConsumed
            ? !TargetComponent->bHasMotion
            : TargetComponent->bHasMotion
                && TargetComponent->CurrentMotion.SourceId == MovedState.SourceId
                && TargetComponent->CurrentMotion.Direction.Equals(MovedState.Direction)
                && FMath::IsNearlyEqual(TargetComponent->CurrentMotion.Magnitude, MovedState.Magnitude));

    if (!ensureAlwaysMsgf(
            bPostconditionValid,
            TEXT("Motion transaction postcondition failed for %s -> %s."),
            *SourceComponent->ParticipantId.ToString(),
            *TargetComponent->ParticipantId.ToString()))
    {
        SourceComponent->SetStateWithoutNotification(MovedState);
        TargetComponent->ClearStateWithoutNotification();
        bTransactionInProgress = false;
        return NotifyRejectedRequest(Verb, EMotionTransferRejection::InvalidMotionState);
    }

    FMotionTransferResult Result;
    Result.bSucceeded = true;
    Result.Verb = Verb;
    Result.Rejection = EMotionTransferRejection::None;
    Result.StateSnapshot = MovedState;
    Result.FromParticipantId = SourceComponent->ParticipantId;
    Result.ToParticipantId = TargetComponent->ParticipantId;
    Result.bConsumed = bConsumed;

    FMotionPendingNotification Notification;
    Notification.PreviousOwner = SourceComponent;
    Notification.NewOwner = bConsumed ? nullptr : TargetComponent;
    Notification.Consumer = bConsumed ? TargetComponent : nullptr;
    Notification.Result = Result;

    bTransactionInProgress = false;
    EnqueueNotification(MoveTemp(Notification));

    return Result;
}

FMotionTransferResult UMotionTransferComponent::NotifyRejectedRequest(
    const EMotionTransferVerb Verb,
    const EMotionTransferRejection Rejection)
{
    FMotionPendingNotification Notification;
    Notification.Result = MakeRejectedResult(Verb, Rejection);
    const FMotionTransferResult Result = Notification.Result;
    EnqueueNotification(MoveTemp(Notification));
    return Result;
}

FMotionTransferResult UMotionTransferComponent::MakeRejectedResult(
    const EMotionTransferVerb Verb,
    const EMotionTransferRejection Rejection) const
{
    FMotionTransferResult Result;
    Result.bSucceeded = false;
    Result.Verb = Verb;
    Result.Rejection = Rejection;
    Result.FromParticipantId = ParticipantId;
    if (bHasMotion)
    {
        Result.StateSnapshot = CurrentMotion;
    }
    return Result;
}

void UMotionTransferComponent::EnqueueNotification(FMotionPendingNotification&& Notification)
{
    Notification.Dispatcher = this;
    GlobalPendingNotifications.Add(MoveTemp(Notification));
    if (!bGlobalDispatchingNotifications && !bTransactionInProgress)
    {
        FlushNotifications();
    }
}

void UMotionTransferComponent::FlushNotifications()
{
    if (bGlobalDispatchingNotifications)
    {
        return;
    }

    check(!bTransactionInProgress);
    bGlobalDispatchingNotifications = true;

    while (!GlobalPendingNotifications.IsEmpty())
    {
        FMotionPendingNotification Notification = MoveTemp(GlobalPendingNotifications[0]);
        GlobalPendingNotifications.RemoveAt(0, 1, EAllowShrinking::No);

        UMotionTransferComponent* Dispatcher = Notification.Dispatcher.Get();
        if (!IsValid(Dispatcher))
        {
            continue;
        }

        check(!Dispatcher->bTransactionInProgress);

        if (UMotionTransferComponent* PreviousOwner = Notification.PreviousOwner.Get())
        {
            PreviousOwner->OnMotionStateChanged.Broadcast(Notification.Result);
        }

        if (UMotionTransferComponent* NewOwner = Notification.NewOwner.Get())
        {
            NewOwner->OnMotionStateChanged.Broadcast(Notification.Result);
        }

        if (UMotionTransferComponent* Consumer = Notification.Consumer.Get())
        {
            Consumer->OnMotionConsumed.Broadcast(Notification.Result);
        }

        if (UMotionTransferComponent* ResetOwner = Notification.ResetOwner.Get())
        {
            ResetOwner->OnMotionStateChanged.Broadcast(Notification.Result);
        }

        Dispatcher->MotionTransactionNative.Broadcast(Notification.Result);
        Dispatcher->OnMotionTransaction.Broadcast(Notification.Result);

        UE_LOG(
            LogMotionTransfer,
            Log,
            TEXT("Motion %s: %s -> %s, success=%s, rejection=%s, source=%s"),
            *UEnum::GetValueAsString(Notification.Result.Verb),
            *Notification.Result.FromParticipantId.ToString(),
            *Notification.Result.ToParticipantId.ToString(),
            Notification.Result.bSucceeded ? TEXT("true") : TEXT("false"),
            *UEnum::GetValueAsString(Notification.Result.Rejection),
            *Notification.Result.StateSnapshot.SourceId.ToString());
    }

    bGlobalDispatchingNotifications = false;
}

bool UMotionTransferComponent::RestoreInitialState(const bool bBroadcastStateChange)
{
    if (bTransactionInProgress || bGlobalDispatchingNotifications)
    {
        return false;
    }

    bTransactionInProgress = true;
    if (bInitialHasMotion)
    {
        SetStateWithoutNotification(InitialMotionSnapshot);
    }
    else
    {
        ClearStateWithoutNotification();
    }

    FMotionPendingNotification Notification;
    Notification.ResetOwner = this;
    Notification.Result.bSucceeded = true;
    Notification.Result.Verb = EMotionTransferVerb::Reset;
    Notification.Result.ToParticipantId = ParticipantId;
    if (bInitialHasMotion)
    {
        Notification.Result.StateSnapshot = InitialMotionSnapshot;
    }

    bTransactionInProgress = false;
    if (bBroadcastStateChange)
    {
        EnqueueNotification(MoveTemp(Notification));
    }

    return true;
}

void UMotionTransferComponent::CaptureInitialSnapshot()
{
    bInitialHasMotion = bHasMotion;
    InitialMotionSnapshot = CurrentMotion;
}

void UMotionTransferComponent::SetStateWithoutNotification(const FMotionState& State)
{
    bHasMotion = true;
    CurrentMotion = State;
}

void UMotionTransferComponent::ClearStateWithoutNotification()
{
    bHasMotion = false;
    CurrentMotion = FMotionState();
}

bool UMotionTransferComponent::IsTransactionInProgress() const
{
    return bTransactionInProgress;
}

bool UMotionTransferComponent::IsDispatchingNotifications() const
{
    return bGlobalDispatchingNotifications;
}

FMotionTransactionNativeSignature& UMotionTransferComponent::OnMotionTransactionNative()
{
    return MotionTransactionNative;
}

#if WITH_DEV_AUTOMATION_TESTS
void UMotionTransferComponent::HandleTestingStateChanged(
    const FMotionTransferResult& Result)
{
    if (!Result.bSucceeded || Result.Verb != EMotionTransferVerb::Capture)
    {
        return;
    }

    UMotionTransferComponent* SourceComponent = TestingTransactionSource.Get();
    UMotionTransferComponent* TargetComponent = TestingTransactionTarget.Get();
    if (IsValid(SourceComponent) && IsValid(TargetComponent))
    {
        TargetComponent->TryCaptureFromComponent(SourceComponent);
    }
}

void UMotionTransferComponent::ConfigureForTesting(
    const FName InParticipantId,
    const bool bInCanProvideMotion,
    const bool bInCanReceiveMotion,
    const EMotionEndpointMode InEndpointMode,
    const TOptional<FMotionState>& InInitialState)
{
    ParticipantId = InParticipantId;
    bCanProvideMotion = bInCanProvideMotion;
    bCanReceiveMotion = bInCanReceiveMotion;
    EndpointMode = InEndpointMode;
    bStartsWithMotion = InInitialState.IsSet();

    if (InInitialState.IsSet())
    {
        InitialMotion = InInitialState.GetValue();
        SetStateWithoutNotification(InitialMotion);
    }
    else
    {
        ClearStateWithoutNotification();
    }

    CaptureInitialSnapshot();
}
#endif
