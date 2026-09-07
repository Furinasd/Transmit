#include "Motion/MotionCanonicalDirectionResolver.h"

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

    // Retain the legacy parameter for API/asset compatibility. Six-direction
    // CameraCanonical always returns world Z for Up/Down.
    (void)bInUseWorldUpForUpDown;
    const FVector WorldUp = FVector::UpVector;

    // Source direction remains valid carry-state data, but never selects output.
    // Both horizontal sectors and vertical thresholds are camera-authored.
    const float CameraPitchDegrees = FRotator::NormalizeAxis(CameraRotation.Pitch);
    const float UpPitchScore = CameraPitchDegrees;
    const float DownPitchScore = -CameraPitchDegrees;

    const float UpEnter = FMath::Clamp(InUpEnterPitchDegrees, 1.0f, 89.0f);
    const float UpExit = FMath::Clamp(InUpExitPitchDegrees, 0.0f, UpEnter);

    if (PreviousDirection == EMotionCanonicalDirection::Up)
    {
        if (UpPitchScore >= UpExit)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Up,
                WorldUp);
        }
        if (DownPitchScore >= UpEnter)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Down,
                -WorldUp);
        }
    }
    else if (PreviousDirection == EMotionCanonicalDirection::Down)
    {
        if (DownPitchScore >= UpExit)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Down,
                -WorldUp);
        }
        if (UpPitchScore >= UpEnter)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Up,
                WorldUp);
        }
    }
    else
    {
        if (UpPitchScore >= UpEnter)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Up,
                WorldUp);
        }
        if (DownPitchScore >= UpEnter)
        {
            return FMotionDirectionResolution::Make(
                EMotionCanonicalDirection::Down,
                -WorldUp);
        }
    }

    // Quantize gameplay camera yaw against fixed world axes, not the carried
    // vector against a rotating camera basis. Canonical outputs are world-fixed.
    const FVector Forward = FVector::ForwardVector;
    const FVector Right = FVector::RightVector;
    const FVector CameraForward = FRotator(0.0f, CameraRotation.Yaw, 0.0f).Vector();
    const float ForwardScore = FVector::DotProduct(CameraForward, Forward);
    const float RightScore = FVector::DotProduct(CameraForward, Right);

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
