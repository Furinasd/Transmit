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

    // Call interface events from native code.
    //
    // The generated Execute_* helpers dispatch through UFunction even when the
    // only implementation is a native C++ _Implementation override with no
    // class-level UFunction. That path leaves the return value untouched for
    // C++-only interface implementers, so these helpers prefer the native
    // interface vtable unless a class (usually a Blueprint) actually owns the
    // event function.
    static UMotionTransferComponent* CallGetMotionTransferComponent(
        const UObject* Object);

    static FMotionCompatibilityResult CallCanCaptureMotion(
        const UObject* Object,
        const FMotionTransferContext& Context);

    static FMotionCompatibilityResult CallCanReceiveMotion(
        const UObject* Object,
        const FMotionState& State,
        const FMotionTransferContext& Context);
};
