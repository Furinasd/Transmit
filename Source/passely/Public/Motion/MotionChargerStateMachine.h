#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "MotionChargerStateMachine.generated.h"

UENUM(BlueprintType)
enum class EMotionChargerState : uint8
{
    Idle,
    Telegraph,
    Dash,
    Recovery
};

UCLASS(BlueprintType, Blueprintable, DefaultToInstanced, EditInlineNew)
class PASSELY_API UMotionChargerStateMachine : public UObject
{
    GENERATED_BODY()

public:
    UMotionChargerStateMachine();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger", meta = (ClampMin = "0.0"))
    float IdleDurationSeconds = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger", meta = (ClampMin = "0.0"))
    float TelegraphDurationSeconds = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger", meta = (ClampMin = "0.0"))
    float DashDurationSeconds = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger", meta = (ClampMin = "0.0"))
    float RecoveryDurationSeconds = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger", meta = (ClampMin = "0.0"))
    float DashCommitWindowDelaySeconds = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger")
    bool bLoopAfterRecovery = true;

    UFUNCTION(BlueprintCallable, Category = "Motion|Charger")
    void Start();

    UFUNCTION(BlueprintCallable, Category = "Motion|Charger")
    void Reset();

    UFUNCTION(BlueprintCallable, Category = "Motion|Charger")
    void Tick(float DeltaTime);

    UFUNCTION(BlueprintCallable, Category = "Motion|Charger")
    void ForceRecovery();

    UFUNCTION(BlueprintPure, Category = "Motion|Charger")
    EMotionChargerState GetState() const;

    UFUNCTION(BlueprintPure, Category = "Motion|Charger")
    float GetElapsedInState() const;

    UFUNCTION(BlueprintPure, Category = "Motion|Charger")
    bool IsCaptureWindowOpen() const;

    UFUNCTION(BlueprintPure, Category = "Motion|Charger")
    bool IsDashCommitted() const;

private:
    EMotionChargerState State = EMotionChargerState::Idle;
    float ElapsedInState = 0.0f;
    bool bRunning = false;

    void EnterState(EMotionChargerState NextState);
    float GetStateDuration(EMotionChargerState InState) const;
};
