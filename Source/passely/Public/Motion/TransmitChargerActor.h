#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Motion/MotionChargerStateMachine.h"
#include "Motion/MotionTransferable.h"

#include "TransmitChargerActor.generated.h"

class UCapsuleComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UMotionTransferComponent;
class UMotionDirectionIndicatorComponent;

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitChargerActor : public AActor, public IMotionTransferable
{
    GENERATED_BODY()

public:
    ATransmitChargerActor();

    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    TObjectPtr<UMotionTransferComponent> Motion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
    TObjectPtr<UCapsuleComponent> Collision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UPointLightComponent> ThreatIndicator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UMotionDirectionIndicatorComponent> DirectionIndicator;

    UPROPERTY(Instanced, EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger")
    TObjectPtr<UMotionChargerStateMachine> StateMachine;

    virtual UMotionTransferComponent* GetMotionTransferComponent_Implementation() const override;
    virtual FMotionCompatibilityResult CanCaptureMotion_Implementation(
        const FMotionTransferContext& Context) const override;

    UFUNCTION(BlueprintCallable, Category = "Motion|Charger")
    void StartChargerCycle();

    UFUNCTION(BlueprintCallable, Category = "Motion|Charger")
    void StopChargerCycle();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger", meta = (ClampMin = "1.0"))
    float DashSpeed = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger")
    FVector DashDirection = FVector::ForwardVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger", meta = (ClampMin = "0.0"))
    float DashMotionMagnitude = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger")
    FName DashSourceId = TEXT("Charger.Dash.001");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Charger")
    bool bAutoStartCycle = true;

private:
    bool bCycleRunning = false;
    bool bWasDashing = false;

    UFUNCTION()
    void HandleMotionStateChanged(const FMotionTransferResult& Result);

    UFUNCTION()
    void HandlePostRoomReset();

    void BindRoomResetController();
    void GrantDashMotionState();
    void RefreshPresentation();
};
