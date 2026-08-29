#include "Motion/MotionTransferable.h"

#include "GameFramework/Actor.h"
#include "Motion/MotionTransferComponent.h"

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
        ? Component->CanReceiveState(State)
        : FMotionCompatibilityResult::Reject(EMotionTransferRejection::MissingMotionComponent);
}
