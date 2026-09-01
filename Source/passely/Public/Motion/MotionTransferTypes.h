#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "MotionTransferTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EMotionType : uint8
{
    Linear
};

UENUM(BlueprintType)
enum class EMotionEndpointMode : uint8
{
    Store,
    ConsumeOnReceive
};

UENUM(BlueprintType)
enum class EMotionTransferVerb : uint8
{
    None,
    Capture,
    Transfer,
    Reset,
    Grant
};

UENUM(BlueprintType)
enum class EMotionTransferRejection : uint8
{
    None,
    InvalidRequester,
    InvalidSource,
    InvalidTarget,
    MissingMotionComponent,
    InvalidMotionState,
    SourceEmpty,
    CarrierOccupied,
    OutOfRange,
    Occluded,
    IncompatibleType,
    IncompatibleDirection,
    IncompatibleMagnitudeTier,
    TargetInvalidated,
    TransactionBusy,
    CooldownActive,
    RequestsBlocked,
    TimingRejected
};

UENUM(BlueprintType)
enum class EMotionCanonicalDirection : uint8
{
    None,
    Forward,
    Back,
    Left,
    Right,
    Up,
    Down
};

USTRUCT(BlueprintType)
struct PASSELY_API FMotionState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
    EMotionType Type = EMotionType::Linear;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
    FVector Direction = FVector::ForwardVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion", meta = (ClampMin = "0.0"))
    float Magnitude = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion")
    FName SourceId = NAME_None;

    bool IsValid() const;
};

USTRUCT(BlueprintType)
struct PASSELY_API FMotionDirectionResolution
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    bool bValid = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    EMotionCanonicalDirection CanonicalDirection = EMotionCanonicalDirection::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    FVector WorldDirection = FVector::ZeroVector;

    static FMotionDirectionResolution Invalid();
    static FMotionDirectionResolution Make(
        EMotionCanonicalDirection InCanonicalDirection,
        const FVector& InWorldDirection);
};

USTRUCT(BlueprintType)
struct PASSELY_API FMotionCompatibilityResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    bool bAllowed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    EMotionTransferRejection Rejection = EMotionTransferRejection::InvalidTarget;

    static FMotionCompatibilityResult Allow();
    static FMotionCompatibilityResult Reject(EMotionTransferRejection Reason);
};

USTRUCT(BlueprintType)
struct PASSELY_API FMotionTransferContext
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    TObjectPtr<AActor> Requester = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    bool bInRange = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    bool bOccluded = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    float Distance = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    FMotionDirectionResolution DirectionResolution;
};

USTRUCT(BlueprintType)
struct PASSELY_API FMotionTransferResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    bool bSucceeded = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    EMotionTransferVerb Verb = EMotionTransferVerb::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    EMotionTransferRejection Rejection = EMotionTransferRejection::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    FMotionState StateSnapshot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    FName FromParticipantId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    FName ToParticipantId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    bool bConsumed = false;
};

USTRUCT(BlueprintType)
struct PASSELY_API FMotionInteractionPreview
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    TObjectPtr<AActor> Target = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    EMotionTransferVerb Verb = EMotionTransferVerb::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    bool bEligible = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    EMotionTransferRejection Rejection = EMotionTransferRejection::InvalidTarget;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    float RawScore = -BIG_NUMBER;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    FGameplayTag MagnitudeTier;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    EMotionCanonicalDirection CanonicalDirection = EMotionCanonicalDirection::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    FVector ProjectedWorldDirection = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    bool bHasProjectedDirection = false;
};
