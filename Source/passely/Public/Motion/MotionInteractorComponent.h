#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Motion/MotionCanonicalDirectionResolver.h"
#include "Motion/MotionTransferTypes.h"
#include "MotionInteractorComponent.generated.h"

class UMotionTransferComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FMotionPreviewChangedSignature,
    const FMotionInteractionPreview&,
    Preview);

UCLASS(ClassGroup = (Motion), BlueprintType, meta = (BlueprintSpawnableComponent))
class PASSELY_API UMotionInteractorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMotionInteractorComponent();

    UPROPERTY(BlueprintAssignable, Category = "Motion|Targeting")
    FMotionPreviewChangedSignature OnPreviewChanged;

    UFUNCTION(BlueprintCallable, Category = "Motion|Targeting")
    void RefreshTarget();

    UFUNCTION(BlueprintCallable, Category = "Motion|Targeting")
    void ClearTarget();

    UFUNCTION(BlueprintCallable, Category = "Motion|Interaction")
    FMotionTransferResult RequestCapture();

    UFUNCTION(BlueprintCallable, Category = "Motion|Interaction")
    FMotionTransferResult RequestTransfer();

    UPROPERTY(Instanced, EditAnywhere, Category = "Motion|Direction")
    TObjectPtr<UMotionCanonicalDirectionResolver> DirectionResolver;

    UFUNCTION(BlueprintPure, Category = "Motion|Targeting")
    FMotionInteractionPreview GetCurrentPreview() const;

    void SetRequestsBlocked(bool bBlocked);
    bool AreRequestsBlocked() const;

    static float CalculateRawScore(
        float DotProduct,
        float NormalizedDistance,
        float InAngleWeight,
        float InDistanceWeight);

    static bool ShouldSwitchTarget(
        float CurrentRawScore,
        float NewRawScore,
        float InStickyBonus);

    // Single authoritative outgoing-direction seam for both Preview and Commit.
    // CameraCanonical delegates to the existing resolver; PreserveSource keeps
    // the source-authored world direction carried by the Motion State.
    static FMotionDirectionResolution ResolveTransferDirection(
        const FMotionState& CarriedState,
        const FRotator& CameraRotation,
        UMotionCanonicalDirectionResolver* Resolver);

protected:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    struct FCandidateEvaluation
    {
        TWeakObjectPtr<AActor> Actor;
        FName ParticipantId = NAME_None;
        float RawScore = -BIG_NUMBER;
        bool bEligible = false;
        EMotionCanonicalDirection CanonicalDirection = EMotionCanonicalDirection::None;
        FVector ProjectedWorldDirection = FVector::ZeroVector;
        bool bHasProjectedDirection = false;
        FMotionCompatibilityResult Compatibility;
        FGameplayTag MagnitudeTier;
        FMotionTransferContext Context;
    };

    UPROPERTY(EditAnywhere, Category = "Motion|Targeting", meta = (ClampMin = "1.0"))
    float TargetingRange = 2000.0f;

    UPROPERTY(EditAnywhere, Category = "Motion|Targeting", meta = (ClampMin = "1.0", ClampMax = "89.0"))
    float AimConeHalfAngleDegrees = 18.0f;

    UPROPERTY(EditAnywhere, Category = "Motion|Targeting", meta = (ClampMin = "0.0"))
    float AngleWeight = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Motion|Targeting", meta = (ClampMin = "0.0"))
    float DistanceWeight = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Motion|Targeting", meta = (ClampMin = "0.0"))
    float StickyBonus = 0.15f;

    UPROPERTY(EditAnywhere, Category = "Motion|Interaction", meta = (ClampMin = "0.0"))
    float RequestCooldownSeconds = 0.1f;

    UPROPERTY(VisibleInstanceOnly, Category = "Motion|Targeting")
    FMotionInteractionPreview CurrentPreview;

    UPROPERTY(Transient)
    FMotionTransferContext CurrentTargetContext;

    TWeakObjectPtr<AActor> CurrentTarget;
    bool bRequestsBlocked = false;
    double LastRequestTimeSeconds = -BIG_NUMBER;

    UMotionTransferComponent* ResolvePlayerMotionComponent() const;
    bool GetViewPoint(FVector& OutOrigin, FRotator& OutRotation) const;
    void GatherCandidates(TArray<FCandidateEvaluation>& OutCandidates) const;
    FCandidateEvaluation EvaluateCandidate(
        AActor* Candidate,
        const FVector& ViewOrigin,
        const FRotator& ViewRotation,
        const UMotionTransferComponent* PlayerMotion) const;
    static bool IsBetterCandidate(
        const FCandidateEvaluation& Candidate,
        const FCandidateEvaluation& CurrentBest);
    void ApplySelectedCandidate(
        const FCandidateEvaluation* Selection,
        EMotionTransferVerb Verb);
    bool IsRequestOnCooldown() const;
    FMotionTransferResult RejectRequest(
        EMotionTransferVerb Verb,
        EMotionTransferRejection Rejection) const;
};
