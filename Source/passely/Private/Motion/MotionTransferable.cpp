#include "Motion/MotionTransferable.h"

#include "GameFramework/Actor.h"
#include "Motion/MotionTransferComponent.h"
#include "UObject/Class.h"

namespace
{
    UFunction* FindClassLevelFunction(const UObject* Object, const FName FunctionName)
    {
        if (!Object)
        {
            return nullptr;
        }

        for (UClass* Class = Object->GetClass(); Class; Class = Class->GetSuperClass())
        {
            if (UFunction* Function = Class->FindFunctionByName(
                    FunctionName,
                    EIncludeSuperFlag::ExcludeSuper))
            {
                return Function;
            }
        }
        return nullptr;
    }

    const IMotionTransferable* ResolveNativeInterface(const UObject* Object)
    {
        return Object
            ? static_cast<const IMotionTransferable*>(
                Object->GetNativeInterfaceAddress(
                    UMotionTransferable::StaticClass()))
            : nullptr;
    }
}

UMotionTransferComponent* IMotionTransferable::GetMotionTransferComponent_Implementation() const
{
    const AActor* Actor = Cast<AActor>(this);
    return Actor ? Actor->FindComponentByClass<UMotionTransferComponent>() : nullptr;
}

FMotionCompatibilityResult IMotionTransferable::CanCaptureMotion_Implementation(
    const FMotionTransferContext& Context) const
{
    if (!Context.bInRange)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::OutOfRange);
    }

    if (Context.bOccluded)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::Occluded);
    }

    const UMotionTransferComponent* Component = GetMotionTransferComponent_Implementation();
    return Component
        ? Component->CanProvideMotion()
        : FMotionCompatibilityResult::Reject(EMotionTransferRejection::MissingMotionComponent);
}

FMotionCompatibilityResult IMotionTransferable::CanReceiveMotion_Implementation(
    const FMotionState& State,
    const FMotionTransferContext& Context) const
{
    if (!Context.bInRange)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::OutOfRange);
    }

    if (Context.bOccluded)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::Occluded);
    }

    const UMotionTransferComponent* Component = GetMotionTransferComponent_Implementation();
    return Component
        ? Component->CanReceiveState(State, &Context.DirectionResolution)
        : FMotionCompatibilityResult::Reject(EMotionTransferRejection::MissingMotionComponent);
}

UMotionTransferComponent* IMotionTransferable::CallGetMotionTransferComponent(
    const UObject* Object)
{
    if (!Object
        || !Object->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()))
    {
        return nullptr;
    }

    if (FindClassLevelFunction(Object, TEXT("GetMotionTransferComponent")))
    {
        return Execute_GetMotionTransferComponent(Object);
    }

    const IMotionTransferable* Native = ResolveNativeInterface(Object);
    return Native ? Native->GetMotionTransferComponent_Implementation() : nullptr;
}

FMotionCompatibilityResult IMotionTransferable::CallCanCaptureMotion(
    const UObject* Object,
    const FMotionTransferContext& Context)
{
    if (!Object
        || !Object->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()))
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::InvalidSource);
    }

    if (FindClassLevelFunction(Object, TEXT("CanCaptureMotion")))
    {
        return Execute_CanCaptureMotion(Object, Context);
    }

    const IMotionTransferable* Native = ResolveNativeInterface(Object);
    return Native
        ? Native->CanCaptureMotion_Implementation(Context)
        : FMotionCompatibilityResult::Reject(EMotionTransferRejection::InvalidSource);
}

FMotionCompatibilityResult IMotionTransferable::CallCanReceiveMotion(
    const UObject* Object,
    const FMotionState& State,
    const FMotionTransferContext& Context)
{
    if (!Object
        || !Object->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()))
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::InvalidTarget);
    }

    if (FindClassLevelFunction(Object, TEXT("CanReceiveMotion")))
    {
        return Execute_CanReceiveMotion(Object, State, Context);
    }

    const IMotionTransferable* Native = ResolveNativeInterface(Object);
    return Native
        ? Native->CanReceiveMotion_Implementation(State, Context)
        : FMotionCompatibilityResult::Reject(EMotionTransferRejection::InvalidTarget);
}
