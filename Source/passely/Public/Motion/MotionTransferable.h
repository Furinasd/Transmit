#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "Motion/MotionTransferTypes.h"
#include "MotionTransferable.generated.h"

class UMotionTransferComponent;

UINTERFACE(BlueprintType)
class PASSELY_API UMotionTransferable : public UInterface
{
    GENERATED_BODY()
};

class PASSELY_API IMotionTransferable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Motion")
    UMotionTransferComponent* GetMotionTransferComponent() const;
    virtual UMotionTransferComponent* GetMotionTransferComponent_Implementation() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Motion")
    FMotionCompatibilityResult CanCaptureMotion(const FMotionTransferContext& Context) const;
    virtual FMotionCompatibilityResult CanCaptureMotion_Implementation(
        const FMotionTransferContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Motion")
    FMotionCompatibilityResult CanReceiveMotion(
        const FMotionState& State,
        const FMotionTransferContext& Context) const;
    virtual FMotionCompatibilityResult CanReceiveMotion_Implementation(
        const FMotionState& State,
        const FMotionTransferContext& Context) const;
};
