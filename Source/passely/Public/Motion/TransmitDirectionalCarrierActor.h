#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Motion/MotionTransferable.h"

#include "TransmitDirectionalCarrierActor.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UMotionTransferComponent;
class ATransmitRam;

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitDirectionalCarrierActor : public AActor, public IMotionTransferable
{
    GENERATED_BODY()

public:
    ATransmitDirectionalCarrierActor();

    virtual void Tick(float DeltaSeconds) override;
    void SetRailController(ATransmitRam* Controller);
    FVector GetReceiverOutputDirection(const FVector& Incoming) const;
    virtual FMotionCompatibilityResult CanReceiveMotion_Implementation(const FMotionState& State, const FMotionTransferContext& Context) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    TObjectPtr<UMotionTransferComponent> Motion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
    TObjectPtr<UCapsuleComponent> Collision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Motion|Carrier", meta = (ClampMin = "0.0"))
    float MovementSpeed = 400.0f;

    UFUNCTION(BlueprintPure, Category = "Motion|Carrier")
    bool IsMovementActive() const;

    UFUNCTION(BlueprintPure, Category = "Motion|Carrier")
    bool IsBlockedByCollision() const;

    virtual UMotionTransferComponent* GetMotionTransferComponent_Implementation() const override;

protected:
    virtual void BeginPlay() override;

private:
    TWeakObjectPtr<ATransmitRam> RailController;
    bool bMovementActive = false;
    bool bBlockedByCollision = false;
    bool bHadMotionLastFrame = false;

    UFUNCTION()
    void HandleMotionStateChanged(const FMotionTransferResult& Result);

    UFUNCTION()
    void HandlePostRoomReset();

    void BindRoomResetController();
    void RefreshMovementFromOwnership();
    void TickMovement(float DeltaSeconds);
};
