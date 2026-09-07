#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Motion/MotionTransferTypes.h"
#include "MotionCanonicalDirectionResolver.generated.h"

UCLASS(BlueprintType, Blueprintable, DefaultToInstanced, EditInlineNew)
class PASSELY_API UMotionCanonicalDirectionResolver : public UObject
{
    GENERATED_BODY()

public:
    UMotionCanonicalDirectionResolver();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Direction", meta = (ClampMin = "1.0", ClampMax = "89.0"))
    float UpEnterPitchDegrees = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Direction", meta = (ClampMin = "0.0", ClampMax = "89.0"))
    float UpExitPitchDegrees = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Direction", meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float HorizontalBoundaryHysteresisDegrees = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Direction")
    bool bUseWorldUpForUpDown = true;

    UFUNCTION(BlueprintCallable, Category = "Motion|Direction")
    FMotionDirectionResolution ResolveDirection(
        const FVector& InputDirection,
        const FRotator& CameraRotation);

    UFUNCTION(BlueprintCallable, Category = "Motion|Direction")
    void ResetHysteresis();

    UFUNCTION(BlueprintPure, Category = "Motion|Direction")
    EMotionCanonicalDirection GetLastResolvedDirection() const;

    static FMotionDirectionResolution ResolveDirectionDeterministic(
        const FVector& InputDirection,
        const FRotator& CameraRotation,
        EMotionCanonicalDirection PreviousDirection,
        float InUpEnterPitchDegrees,
        float InUpExitPitchDegrees,
        float InHorizontalBoundaryHysteresisDegrees,
        bool bInUseWorldUpForUpDown);

private:
    static bool IsHorizontalDirection(EMotionCanonicalDirection Direction);
    static float GetHorizontalScore(
        EMotionCanonicalDirection Direction,
        float ForwardScore,
        float RightScore);

    EMotionCanonicalDirection LastResolvedDirection = EMotionCanonicalDirection::None;
};
