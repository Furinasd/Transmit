#include "Motion/MotionTransferTypes.h"

bool FMotionState::IsValid() const
{
    return Type == EMotionType::Linear
        && !Direction.IsNearlyZero()
        && Direction.IsNormalized()
        && FMath::IsFinite(Magnitude)
        && Magnitude > 0.0f
        && !SourceId.IsNone();
}

FMotionCompatibilityResult FMotionCompatibilityResult::Allow()
{
    FMotionCompatibilityResult Result;
    Result.bAllowed = true;
    Result.Rejection = EMotionTransferRejection::None;
    return Result;
}

FMotionCompatibilityResult FMotionCompatibilityResult::Reject(const EMotionTransferRejection Reason)
{
    FMotionCompatibilityResult Result;
    Result.bAllowed = false;
    Result.Rejection = Reason;
    return Result;
}
