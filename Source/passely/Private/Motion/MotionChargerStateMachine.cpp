#include "Motion/MotionChargerStateMachine.h"

UMotionChargerStateMachine::UMotionChargerStateMachine()
{
}

void UMotionChargerStateMachine::Start()
{
    bRunning = true;
    EnterState(EMotionChargerState::Idle);
}

void UMotionChargerStateMachine::Reset()
{
    bRunning = false;
    EnterState(EMotionChargerState::Idle);
}

void UMotionChargerStateMachine::Tick(const float DeltaTime)
{
    if (!bRunning)
    {
        return;
    }

    ElapsedInState += FMath::Max(0.0f, DeltaTime);
    const float Duration = GetStateDuration(State);
    if (Duration > 0.0f && ElapsedInState < Duration)
    {
        return;
    }

    switch (State)
    {
    case EMotionChargerState::Idle:
        EnterState(EMotionChargerState::Telegraph);
        break;
    case EMotionChargerState::Telegraph:
        EnterState(EMotionChargerState::Dash);
        break;
    case EMotionChargerState::Dash:
        EnterState(EMotionChargerState::Recovery);
        break;
    case EMotionChargerState::Recovery:
        if (bLoopAfterRecovery)
        {
            EnterState(EMotionChargerState::Idle);
        }
        else
        {
            bRunning = false;
        }
        break;
    }
}

void UMotionChargerStateMachine::ForceRecovery()
{
    if (State != EMotionChargerState::Idle && State != EMotionChargerState::Recovery)
    {
        EnterState(EMotionChargerState::Recovery);
    }
}

EMotionChargerState UMotionChargerStateMachine::GetState() const
{
    return State;
}

float UMotionChargerStateMachine::GetElapsedInState() const
{
    return ElapsedInState;
}

bool UMotionChargerStateMachine::IsCaptureWindowOpen() const
{
    return bRunning
        && State == EMotionChargerState::Dash
        && ElapsedInState >= FMath::Max(0.0f, DashCommitWindowDelaySeconds);
}

bool UMotionChargerStateMachine::IsDashCommitted() const
{
    return bRunning && State == EMotionChargerState::Dash;
}

void UMotionChargerStateMachine::EnterState(const EMotionChargerState NextState)
{
    State = NextState;
    ElapsedInState = 0.0f;
}

float UMotionChargerStateMachine::GetStateDuration(const EMotionChargerState InState) const
{
    switch (InState)
    {
    case EMotionChargerState::Idle:
        return IdleDurationSeconds;
    case EMotionChargerState::Telegraph:
        return TelegraphDurationSeconds;
    case EMotionChargerState::Dash:
        return DashDurationSeconds;
    case EMotionChargerState::Recovery:
        return RecoveryDurationSeconds;
    default:
        return 0.0f;
    }
}
