#include "Motion/MotionInteractorComponent.h"
#include "Motion/TransmitDirectionalCarrierActor.h"

#include "CollisionQueryParams.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Motion/MotionTransferable.h"
#include "Motion/MotionTransferComponent.h"

namespace
{
    constexpr float TargetReleaseMarginDegrees = 12.0f;
    constexpr float MaxTargetSizeAssistDegrees = 10.0f;

    FBox GetTargetMeshBounds(const AActor* Target)
    {
        FBox Bounds(ForceInit);
        TInlineComponentArray<UStaticMeshComponent*> Meshes;
        Target->GetComponents(Meshes);
        for (const UStaticMeshComponent* Mesh : Meshes)
        {
            if (!Mesh->GetStaticMesh() || !Mesh->IsVisible() || Mesh->bHiddenInGame)
            {
                continue;
            }
            bool bIndicatorMesh = false;
            for (const USceneComponent* Parent = Mesh->GetAttachParent(); Parent;
                Parent = Parent->GetAttachParent())
            {
                if (Parent->IsA<UArrowComponent>())
                {
                    bIndicatorMesh = true;
                    break;
                }
            }
            if (!bIndicatorMesh)
            {
                Bounds += Mesh->Bounds.GetBox();
            }
        }
        return Bounds;
    }
}

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

FMotionDirectionResolution UMotionInteractorComponent::ResolveTransferDirection(
    const FMotionState& CarriedState,
    const FRotator& CameraRotation,
    UMotionCanonicalDirectionResolver* Resolver)
{
    if (CarriedState.DirectionPolicy == EMotionDirectionPolicy::PreserveSource)
    {
        return FMotionDirectionResolution::PreserveSource(CarriedState.Direction);
    }

    return Resolver
        ? Resolver->ResolveDirection(CarriedState.Direction, CameraRotation)
        : FMotionDirectionResolution::Invalid();
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
        return IMotionTransferable::CallGetMotionTransferComponent(Owner);
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

    // The reticle is drawn at the gameplay camera's center. A third-person
    // pawn's eye position is offset from that camera and disagrees at close range.
    const APawn* Pawn = Cast<APawn>(Owner);
    const APlayerController* Controller = Pawn
        ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    if (Controller)
    {
        Controller->GetPlayerViewPoint(OutOrigin, OutRotation);
    }
    else
    {
        Owner->GetActorEyesViewPoint(OutOrigin, OutRotation);
    }
    return !OutRotation.Vector().IsNearlyZero();
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

    FVector InteractionOrigin;
    FRotator InteractionRotation;
    GetOwner()->GetActorEyesViewPoint(InteractionOrigin, InteractionRotation);

    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams ObjectParams(FCollisionObjectQueryParams::AllObjects);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MotionTargetCandidates), false, GetOwner());
    World->OverlapMultiByObjectType(
        Overlaps,
        InteractionOrigin,
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
            EvaluateCandidate(Candidate, InteractionOrigin, ViewOrigin, ViewRotation, PlayerMotion);
        if (Evaluation.RawScore > -BIG_NUMBER)
        {
            OutCandidates.Add(MoveTemp(Evaluation));
        }
    }
}

UMotionInteractorComponent::FCandidateEvaluation UMotionInteractorComponent::EvaluateCandidate(
    AActor* Candidate,
    const FVector& InteractionOrigin,
    const FVector& ViewOrigin,
    const FRotator& ViewRotation,
    const UMotionTransferComponent* PlayerMotion) const
{
    FCandidateEvaluation Evaluation;
    Evaluation.Actor = Candidate;
    Evaluation.Context.Requester = GetOwner();

    UMotionTransferComponent* TargetMotion =
        IMotionTransferable::CallGetMotionTransferComponent(Candidate);
    if (!TargetMotion)
    {
        Evaluation.Compatibility = FMotionCompatibilityResult::Reject(
            EMotionTransferRejection::MissingMotionComponent);
        return Evaluation;
    }

    Evaluation.ParticipantId = TargetMotion->GetParticipantId();

    // Aim at what is visible, including a Source whose presentation mesh moves
    // relative to its pivot. Lights and direction arrows cannot enlarge selection.
    const FBox TargetBounds = GetTargetMeshBounds(Candidate);
    const FVector TargetCenter = TargetBounds.IsValid
        ? TargetBounds.GetCenter() : Candidate->GetActorLocation();
    const FVector ToTarget = TargetCenter - ViewOrigin;
    const float ViewDistance = ToTarget.Size();
    const float Distance = FVector::Distance(TargetCenter, InteractionOrigin);
    if (ViewDistance <= KINDA_SMALL_NUMBER || Distance <= KINDA_SMALL_NUMBER || Distance > TargetingRange)
    {
        Evaluation.Compatibility = FMotionCompatibilityResult::Reject(
            EMotionTransferRejection::OutOfRange);
        return Evaluation;
    }

    const float DotProduct = FVector::DotProduct(ViewRotation.Vector(), ToTarget / ViewDistance);
    const float SizeAssistDegrees = TargetBounds.IsValid
        ? FMath::Min(MaxTargetSizeAssistDegrees, FMath::RadiansToDegrees(FMath::Asin(
            FMath::Clamp(static_cast<float>(TargetBounds.GetExtent().Size() / ViewDistance), 0.0f, 1.0f))))
        : 0.0f;
    // Acquisition and release use different cones, so small mouse movements
    // do not drop a target that was just selected. The existing ranking still
    // lets a clearly better candidate replace it; this is not a target lock.
    const float ReleaseMargin = Candidate == CurrentTarget.Get()
        ? TargetReleaseMarginDegrees : 0.0f;
    const float ConeDegrees = FMath::Clamp(
        AimConeHalfAngleDegrees + SizeAssistDegrees + ReleaseMargin, 1.0f, 89.0f);
    const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(ConeDegrees));
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
    // Camera alignment must not extend reach or let the player interact around
    // a wall. Preserve the player's range/LOS and require camera visibility too.
    bool bOccluded = GetWorld()->LineTraceSingleByChannel(
        OcclusionHit,
        InteractionOrigin,
        TargetCenter,
        ECC_Visibility,
        OcclusionParams);
    if (!bOccluded && !ViewOrigin.Equals(InteractionOrigin, 1.0f))
    {
        bOccluded = GetWorld()->LineTraceSingleByChannel(
            OcclusionHit, ViewOrigin, TargetCenter, ECC_Visibility, OcclusionParams);
    }

    Evaluation.Context.bInRange = true;
    Evaluation.Context.bOccluded = bOccluded;
    Evaluation.Context.Distance = Distance;

    if (PlayerMotion->HasMotionState())
    {
        FMotionState CarriedState;
        PlayerMotion->TryGetMotionState(CarriedState);
        const FMotionDirectionResolution Resolution =
            UMotionInteractorComponent::ResolveTransferDirection(
                CarriedState,
                ViewRotation,
                DirectionResolver);
        Evaluation.CanonicalDirection = Resolution.CanonicalDirection;
        Evaluation.ProjectedWorldDirection = Resolution.WorldDirection;
        Evaluation.bHasProjectedDirection = Resolution.bValid;
        Evaluation.Context.DirectionResolution = Resolution;

        FMotionState ResolvedState = CarriedState;
        if (Resolution.bValid)
        {
            ResolvedState.Direction = Resolution.WorldDirection.GetSafeNormal();
        }
        Evaluation.Compatibility = IMotionTransferable::CallCanReceiveMotion(
            Candidate,
            ResolvedState,
            Evaluation.Context);
        if (const auto* Carrier = Cast<ATransmitDirectionalCarrierActor>(Candidate))
            Evaluation.ProjectedWorldDirection = Carrier->GetReceiverOutputDirection(Evaluation.ProjectedWorldDirection);
        Evaluation.MagnitudeTier = PlayerMotion->GetMagnitudeTier();
    }
    else
    {
        Evaluation.Compatibility = IMotionTransferable::CallCanCaptureMotion(
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
