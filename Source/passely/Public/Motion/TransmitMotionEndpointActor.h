#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Motion/MotionTransferable.h"

#include "TransmitMotionEndpointActor.generated.h"

class UArrowComponent;
class UMotionTransferComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitMotionEndpointActor : public AActor, public IMotionTransferable
{
    GENERATED_BODY()

public:
    ATransmitMotionEndpointActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    TObjectPtr<UMotionTransferComponent> Motion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UPointLightComponent> MotionIndicator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UArrowComponent> DirectionIndicator;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Presentation")
    TObjectPtr<USceneComponent> SceneRoot;

    bool bConsumedSinceReset = false;

    UFUNCTION()
    void HandleMotionStateChanged(const FMotionTransferResult& Result);

    UFUNCTION()
    void HandleMotionConsumed(const FMotionTransferResult& Result);

    UFUNCTION()
    void HandlePostRoomReset();

    void BindRoomResetController();
    void RefreshPresentation();
};
