#include "Motion/MotionInteractorComponent.h"

#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Motion/MotionTransferable.h"
#include "Motion/MotionTransferComponent.h"

UMotionInteractorComponent::UMotionInteractorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    DirectionResolver = CreateDefaultSubobject<UMotionCanonicalDirectionResolver>(
        TEXT("DirectionResolver"));
}

void UMotionInteractorComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RefreshTarget();
}

void UMotionInteractorComponent::RefreshTarget()
{
    UMotionTransferComponent* PlayerMotion = ResolvePlayerMotionComponent();
    if (!PlayerMotion)
    {
        ClearTarget();
        return;
    }

    TArray<FCandidateEvaluation> Candidates;
    GatherCandidates(Candidates);

    const FCandidateEvaluation* CurrentEvaluation = nullptr;
    const FCandidateEvaluation* BestLegal = nullptr;
    const FCandidateEvaluation* BestInvalid = nullptr;

    for (const FCandidateEvaluation& Candidate : Candidates)
    {
        if (Candidate.Actor == CurrentTarget)
        {
            CurrentEvaluation = &Candidate;
        }

        const FCandidateEvaluation*& Best = Candidate.bEligible ? BestLegal : BestInvalid;
        if (!Best || IsBetterCandidate(Candidate, *Best))
        {
            Best = &Candidate;
        }
    }

    const FCandidateEvaluation* Selection = nullptr;
    if (CurrentEvaluation && CurrentEvaluation->bEligible)
    {
        if (!BestLegal
            || BestLegal == CurrentEvaluation
            || !ShouldSwitchTarget(
                CurrentEvaluation->RawScore,
                BestLegal->RawScore,
                StickyBonus))
        {
            Selection = CurrentEvaluation;
        }
        else
        {
            Selection = BestLegal;
        }
    }
    else
    {
        Selection = BestLegal ? BestLegal : BestInvalid;
    }

    const EMotionTransferVerb Verb = PlayerMotion->HasMotionState()
        ? EMotionTransferVerb::Transfer
        : EMotionTransferVerb::Capture;
    ApplySelectedCandidate(Selection, Verb);
}

void UMotionInteractorComponent::ClearTarget()
{
    const bool bHadTarget = CurrentPreview.Target != nullptr;
    CurrentTarget.Reset();
    CurrentPreview = FMotionInteractionPreview();
    CurrentTargetContext = FMotionTransferContext();
    if (bHadTarget)
    {
        OnPreviewChanged.Broadcast(CurrentPreview);
    }
}

FMotionTransferResult UMotionInteractorComponent::RequestCapture()
{
    UMotionTransferComponent* PlayerMotion = ResolvePlayerMotionComponent();
    if (!PlayerMotion)
    {
        return RejectRequest(
            EMotionTransferVerb::Capture,
            EMotionTransferRejection::MissingMotionComponent);
    }

    if (bRequestsBlocked)
    {
        return PlayerMotion->NotifyRejectedRequest(
            EMotionTransferVerb::Capture,
            EMotionTransferRejection::RequestsBlocked);
    }

    if (IsRequestOnCooldown())
    {
        return PlayerMotion->NotifyRejectedRequest(
            EMotionTransferVerb::Capture,
            EMotionTransferRejection::CooldownActive);
    }

    RefreshTarget();
    if (PlayerMotion->HasMotionState())
    {
        return PlayerMotion->NotifyRejectedRequest(
            EMotionTransferVerb::Capture,
            EMotionTransferRejection::CarrierOccupied);
    }

    if (!CurrentPreview.Target || !CurrentPreview.bEligible)
    {
        return PlayerMotion->NotifyRejectedRequest(
            EMotionTransferVerb::Capture,
            CurrentPreview.Target
                ? CurrentPreview.Rejection
                : EMotionTransferRejection::InvalidSource);
    }

    LastRequestTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    return PlayerMotion->TryCaptureFromActor(
        CurrentPreview.Target,
        CurrentTargetContext);
}

FMotionTransferResult UMotionInteractorComponent::RequestTransfer()
{
    UMotionTransferComponent* PlayerMotion = ResolvePlayerMotionComponent();
    if (!PlayerMotion)
    {
        return RejectRequest(
            EMotionTransferVerb::Transfer,
            EMotionTransferRejection::MissingMotionComponent);
    }

    if (bRequestsBlocked)
    {
        return PlayerMotion->NotifyRejectedRequest(
            EMotionTransferVerb::Transfer,
            EMotionTransferRejection::RequestsBlocked);
    }

    if (IsRequestOnCooldown())
    {
        return PlayerMotion->NotifyRejectedRequest(
            EMotionTransferVerb::Transfer,
            EMotionTransferRejection::CooldownActive);
    }

    RefreshTarget();
    if (!PlayerMotion->HasMotionState())
    {
        return PlayerMotion->NotifyRejectedRequest(
            EMotionTransferVerb::Transfer,
            EMotionTransferRejection::SourceEmpty);
    }

    if (!CurrentPreview.Target || !CurrentPreview.bEligible)
    {
        return PlayerMotion->NotifyRejectedRequest(
            EMotionTransferVerb::Transfer,
            CurrentPreview.Target
                ? CurrentPreview.Rejection
                : EMotionTransferRejection::InvalidTarget);
    }

    LastRequestTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    return PlayerMotion->TryTransferToActor(
        CurrentPreview.Target,
        CurrentTargetContext);
}

FMotionInteractionPreview UMotionInteractorComponent::GetCurrentPreview() const
{
    return CurrentPreview;
}

void UMotionInteractorComponent::SetRequestsBlocked(const bool bBlocked)
{
    bRequestsBlocked = bBlocked;
    if (bBlocked)
    {
        ClearTarget();
    }
}

bool UMotionInteractorComponent::AreRequestsBlocked() const
{
    return bRequestsBlocked;
}

float UMotionInteractorComponent::CalculateRawScore(
    const float DotProduct,
    const float NormalizedDistance,
    const float InAngleWeight,
    const float InDistanceWeight)
{
    return DotProduct * InAngleWeight
        - FMath::Clamp(NormalizedDistance, 0.0f, 1.0f) * InDistanceWeight;
}

bool UMotionInteractorComponent::ShouldSwitchTarget(
    const float CurrentRawScore,
    const float NewRawScore,
    const float InStickyBonus)
{
    return NewRawScore > CurrentRawScore + FMath::Max(0.0f, InStickyBonus);
}

UMotionTransferComponent* UMotionInteractorComponent::ResolvePlayerMotionComponent() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    if (Owner->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()))
    {
        return IMotionTransferable::Execute_GetMotionTransferComponent(Owner);
    }

    return nullptr;
}

bool UMotionInteractorComponent::GetViewPoint(
    FVector& OutOrigin,
    FRotator& OutRotation) const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    FRotator ViewRotation;
    Owner->GetActorEyesViewPoint(OutOrigin, ViewRotation);
    OutRotation = ViewRotation;
    return !ViewRotation.Vector().IsNearlyZero();
}

void UMotionInteractorComponent::GatherCandidates(
    TArray<FCandidateEvaluation>& OutCandidates) const
{
    const UWorld* World = GetWorld();
    UMotionTransferComponent* PlayerMotion = ResolvePlayerMotionComponent();
    FVector ViewOrigin;
    FRotator ViewRotation;
    if (!World || !PlayerMotion || !GetViewPoint(ViewOrigin, ViewRotation))
    {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjectParams(FCollisionObjectQueryParams::AllObjects);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MotionTargetCandidates), false, GetOwner());
    World->OverlapMultiByObjectType(
        Overlaps,
        ViewOrigin,
        FQuat::Identity,
        ObjectParams,
        FCollisionShape::MakeSphere(TargetingRange),
        QueryParams);

    TSet<TWeakObjectPtr<AActor>> SeenActors;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();
        if (!IsValid(Candidate)
            || Candidate == GetOwner()
            || SeenActors.Contains(Candidate)
            || !Candidate->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()))
        {
            continue;
        }

        SeenActors.Add(Candidate);
        FCandidateEvaluation Evaluation =
            EvaluateCandidate(Candidate, ViewOrigin, ViewRotation, PlayerMotion);
        if (Evaluation.RawScore > -BIG_NUMBER)
        {
            OutCandidates.Add(MoveTemp(Evaluation));
        }
    }
}

UMotionInteractorComponent::FCandidateEvaluation UMotionInteractorComponent::EvaluateCandidate(
    AActor* Candidate,
    const FVector& ViewOrigin,
    const FRotator& ViewRotation,
    const UMotionTransferComponent* PlayerMotion) const
{
    FCandidateEvaluation Evaluation;
    Evaluation.Actor = Candidate;
    Evaluation.Context.Requester = GetOwner();

    UMotionTransferComponent* TargetMotion =
        IMotionTransferable::Execute_GetMotionTransferComponent(Candidate);
    if (!TargetMotion)
    {
        Evaluation.Compatibility = FMotionCompatibilityResult::Reject(
            EMotionTransferRejection::MissingMotionComponent);
        return Evaluation;
    }

    Evaluation.ParticipantId = TargetMotion->GetParticipantId();

    const FVector ToTarget = Candidate->GetActorLocation() - ViewOrigin;
    const float Distance = ToTarget.Size();
    if (Distance <= KINDA_SMALL_NUMBER || Distance > TargetingRange)
    {
        Evaluation.Compatibility = FMotionCompatibilityResult::Reject(
            EMotionTransferRejection::OutOfRange);
        return Evaluation;
    }

    const float DotProduct = FVector::DotProduct(ViewRotation.Vector(), ToTarget / Distance);
    const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(AimConeHalfAngleDegrees));
    if (DotProduct < MinimumDot)
    {
        return Evaluation;
    }

    Evaluation.RawScore = CalculateRawScore(
        DotProduct,
        Distance / TargetingRange,
        AngleWeight,
        DistanceWeight);

    FCollisionQueryParams OcclusionParams(
        SCENE_QUERY_STAT(MotionTargetOcclusion),
        false,
        GetOwner());
    OcclusionParams.AddIgnoredActor(Candidate);
    FHitResult OcclusionHit;
    const bool bOccluded = GetWorld()->LineTraceSingleByChannel(
        OcclusionHit,
        ViewOrigin,
        Candidate->GetActorLocation(),
        ECC_Visibility,
        OcclusionParams);

    Evaluation.Context.bInRange = true;
    Evaluation.Context.bOccluded = bOccluded;
    Evaluation.Context.Distance = Distance;

    if (PlayerMotion->HasMotionState())
    {
        FMotionState CarriedState;
        PlayerMotion->TryGetMotionState(CarriedState);
        const FMotionDirectionResolution Resolution =
            DirectionResolver
                ? DirectionResolver->ResolveDirection(CarriedState.Direction, ViewRotation)
                : FMotionDirectionResolution();
        Evaluation.CanonicalDirection = Resolution.CanonicalDirection;
        Evaluation.ProjectedWorldDirection = Resolution.WorldDirection;
        Evaluation.bHasProjectedDirection = Resolution.bValid;
        Evaluation.Context.DirectionResolution = Resolution;

        FMotionState ResolvedState = CarriedState;
        if (Resolution.bValid)
        {
            ResolvedState.Direction = Resolution.WorldDirection.GetSafeNormal();
        }
        Evaluation.Compatibility = IMotionTransferable::Execute_CanReceiveMotion(
            Candidate,
            ResolvedState,
            Evaluation.Context);
        Evaluation.MagnitudeTier = PlayerMotion->GetMagnitudeTier();
    }
    else
    {
        Evaluation.Compatibility = IMotionTransferable::Execute_CanCaptureMotion(
            Candidate,
            Evaluation.Context);
        Evaluation.MagnitudeTier = TargetMotion->GetMagnitudeTier();
    }

    Evaluation.bEligible = Evaluation.Compatibility.bAllowed;
    return Evaluation;
}

bool UMotionInteractorComponent::IsBetterCandidate(
    const FCandidateEvaluation& Candidate,
    const FCandidateEvaluation& CurrentBest)
{
    if (!FMath::IsNearlyEqual(Candidate.RawScore, CurrentBest.RawScore))
    {
        return Candidate.RawScore > CurrentBest.RawScore;
    }

    return Candidate.ParticipantId.LexicalLess(CurrentBest.ParticipantId);
}

void UMotionInteractorComponent::ApplySelectedCandidate(
    const FCandidateEvaluation* Selection,
    const EMotionTransferVerb Verb)
{
    FMotionInteractionPreview NewPreview;
    NewPreview.Verb = Verb;

    if (Selection)
    {
        NewPreview.Target = Selection->Actor.Get();
        NewPreview.bEligible = Selection->bEligible;
        NewPreview.Rejection = Selection->Compatibility.Rejection;
        NewPreview.RawScore = Selection->RawScore;
        NewPreview.MagnitudeTier = Selection->MagnitudeTier;
        NewPreview.CanonicalDirection = Selection->CanonicalDirection;
        NewPreview.ProjectedWorldDirection = Selection->ProjectedWorldDirection;
        NewPreview.bHasProjectedDirection = Selection->bHasProjectedDirection;
        CurrentTargetContext = Selection->Context;
    }
    else
    {
        CurrentTargetContext = FMotionTransferContext();
    }

    const bool bMeaningfulChange = CurrentPreview.Target != NewPreview.Target
        || CurrentPreview.Verb != NewPreview.Verb
        || CurrentPreview.bEligible != NewPreview.bEligible
        || CurrentPreview.Rejection != NewPreview.Rejection
        || CurrentPreview.MagnitudeTier != NewPreview.MagnitudeTier
        || CurrentPreview.CanonicalDirection != NewPreview.CanonicalDirection
        || CurrentPreview.ProjectedWorldDirection != NewPreview.ProjectedWorldDirection
        || CurrentPreview.bHasProjectedDirection != NewPreview.bHasProjectedDirection;

    CurrentTarget = NewPreview.Target;
    CurrentPreview = NewPreview;
    if (bMeaningfulChange)
    {
        OnPreviewChanged.Broadcast(CurrentPreview);
    }
}

bool UMotionInteractorComponent::IsRequestOnCooldown() const
{
    const UWorld* World = GetWorld();
    return World
        && World->GetTimeSeconds() - LastRequestTimeSeconds < RequestCooldownSeconds;
}

FMotionTransferResult UMotionInteractorComponent::RejectRequest(
    const EMotionTransferVerb Verb,
    const EMotionTransferRejection Rejection) const
{
    FMotionTransferResult Result;
    Result.Verb = Verb;
    Result.Rejection = Rejection;
    return Result;
}
