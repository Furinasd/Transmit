#include "Motion/MotionCanonicalDirectionResolver.h"

#include "Math/RotationMatrix.h"

UMotionCanonicalDirectionResolver::UMotionCanonicalDirectionResolver()
{
}

FMotionDirectionResolution UMotionCanonicalDirectionResolver::ResolveDirection(
    const FVector& InputDirection,
    const FRotator& CameraRotation)
{
    const FMotionDirectionResolution Resolution = ResolveDirectionDeterministic(
        InputDirection,
        CameraRotation,
        LastResolvedDirection,
        UpEnterPitchDegrees,
        UpExitPitchDegrees,
        HorizontalBoundaryHysteresisDegrees,
        bUseWorldUpForUpDown);
    LastResolvedDirection = Resolution.CanonicalDirection;
    return Resolution;
}

void UMotionCanonicalDirectionResolver::ResetHysteresis()
{
    LastResolvedDirection = EMotionCanonicalDirection::None;
}

EMotionCanonicalDirection UMotionCanonicalDirectionResolver::GetLastResolvedDirection() const
{
    return LastResolvedDirection;
}

FMotionDirectionResolution UMotionCanonicalDirectionResolver::ResolveDirectionDeterministic(
    const FVector& InputDirection,
    const FRotator& CameraRotation,
    const EMotionCanonicalDirection PreviousDirection,
    const float InUpEnterPitchDegrees,
    const float InUpExitPitchDegrees,
    const float InHorizontalBoundaryHysteresisDegrees,
    const bool bInUseWorldUpForUpDown)
{
    const FVector Input = InputDirection.GetSafeNormal();
    if (Input.IsNearlyZero() || !Input.IsNormalized())
    {
        return FMotionDirectionResolution::Invalid();
    }

    const FVector WorldUp = FVector::UpVector;
    const float VerticalDot = FVector::DotProduct(Input, WorldUp);
    const float VerticalPitchDegrees = FMath::RadiansToDegrees(
        FMath::Asin(FMath::Clamp(VerticalDot, -1.0f, 1.0f)));

    const float UpEnter = FMath::Clamp(InUpEnterPitchDegrees, 1.0f, 89.0f);
    const float UpExit = FMath::Clamp(InUpExitPitchDegrees, 0.0f, UpEnter);

    if (PreviousDirection == EMotionCanonicalDirection::Up)
    {
        if (VerticalPitchDegrees >= UpExit)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Up,
                bInUseWorldUpForUpDown ? WorldUp : CameraRotation.RotateVector(FVector::UpVector));
        }
        if (VerticalPitchDegrees <= -UpEnter)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Down,
                bInUseWorldUpForUpDown ? -WorldUp : -CameraRotation.RotateVector(FVector::UpVector));
        }
    }
    else if (PreviousDirection == EMotionCanonicalDirection::Down)
    {
        if (VerticalPitchDegrees <= -UpExit)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Down,
                bInUseWorldUpForUpDown ? -WorldUp : -CameraRotation.RotateVector(FVector::UpVector));
        }
        if (VerticalPitchDegrees >= UpEnter)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Up,
                bInUseWorldUpForUpDown ? WorldUp : CameraRotation.RotateVector(FVector::UpVector));
        }
    }
    else
    {
        if (VerticalPitchDegrees >= UpEnter)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Up,
                bInUseWorldUpForUpDown ? WorldUp : CameraRotation.RotateVector(FVector::UpVector));
        }
        if (VerticalPitchDegrees <= -UpEnter)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Down,
                bInUseWorldUpForUpDown ? -WorldUp : -CameraRotation.RotateVector(FVector::UpVector));
        }
    }

    // Horizontal basis comes from camera yaw only; this keeps the four horizontal
    // directions stable when the camera pitches, and Up/Down stay world-aligned.
    const FRotator YawOnly(0.0f, CameraRotation.Yaw, 0.0f);
    const FVector Forward = YawOnly.Vector().GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        return FMotionDirectionResolution::Invalid();
    }
    const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y).GetSafeNormal();

    const float ForwardScore = FVector::DotProduct(Input, Forward);
    const float RightScore = FVector::DotProduct(Input, Right);

    EMotionCanonicalDirection Best = EMotionCanonicalDirection::Forward;
    float BestScore = ForwardScore;
    {
        const float BackScore = -ForwardScore;
        if (BackScore > BestScore)
        {
            Best = EMotionCanonicalDirection::Back;
            BestScore = BackScore;
        }
        if (RightScore > BestScore)
        {
            Best = EMotionCanonicalDirection::Right;
            BestScore = RightScore;
        }
        if (-RightScore > BestScore)
        {
            Best = EMotionCanonicalDirection::Left;
            BestScore = -RightScore;
        }
    }

    if (IsHorizontalDirection(PreviousDirection))
    {
        const float PreviousScore = GetHorizontalScore(
            PreviousDirection,
            ForwardScore,
            RightScore);
        const float HysteresisThreshold = FMath::Sin(
            FMath::DegreesToRadians(
                FMath::Clamp(InHorizontalBoundaryHysteresisDegrees, 0.0f, 45.0f)));
        if (BestScore - PreviousScore <= HysteresisThreshold)
        {
            Best = PreviousDirection;
        }
    }

    FVector WorldDirection = FVector::ZeroVector;
    switch (Best)
    {
    case EMotionCanonicalDirection::Forward:
        WorldDirection = Forward;
        break;
    case EMotionCanonicalDirection::Back:
        WorldDirection = -Forward;
        break;
    case EMotionCanonicalDirection::Right:
        WorldDirection = Right;
        break;
    case EMotionCanonicalDirection::Left:
        WorldDirection = -Right;
        break;
    default:
        return FMotionDirectionResolution::Invalid();
    }

    return FMotionDirectionResolution::Make(Best, WorldDirection);
}

bool UMotionCanonicalDirectionResolver::IsHorizontalDirection(
    const EMotionCanonicalDirection Direction)
{
    return Direction == EMotionCanonicalDirection::Forward
        || Direction == EMotionCanonicalDirection::Back
        || Direction == EMotionCanonicalDirection::Left
        || Direction == EMotionCanonicalDirection::Right;
}

float UMotionCanonicalDirectionResolver::GetHorizontalScore(
    const EMotionCanonicalDirection Direction,
    const float ForwardScore,
    const float RightScore)
{
    switch (Direction)
    {
    case EMotionCanonicalDirection::Forward:
        return ForwardScore;
    case EMotionCanonicalDirection::Back:
        return -ForwardScore;
    case EMotionCanonicalDirection::Right:
        return RightScore;
    case EMotionCanonicalDirection::Left:
        return -RightScore;
    default:
        return -BIG_NUMBER;
    }
}
