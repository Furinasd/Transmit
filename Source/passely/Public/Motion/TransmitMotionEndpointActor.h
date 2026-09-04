#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Motion/MotionTransferable.h"

#include "TransmitMotionEndpointActor.generated.h"

class UMotionTransferComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UMotionDirectionIndicatorComponent;

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitMotionEndpointActor : public AActor, public IMotionTransferable
{
    GENERATED_BODY()

public:
    ATransmitMotionEndpointActor();

    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion")
    TObjectPtr<UMotionTransferComponent> Motion;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UPointLightComponent> MotionIndicator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<UMotionDirectionIndicatorComponent> DirectionIndicator;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion Preview")
    bool bAnimateOwnedMotion = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion Preview", meta = (ClampMin = "1.0"))
    float MotionPreviewDistance = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion Preview", meta = (ClampMin = "0.0"))
    float MotionPreviewSpeedScale = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Receiver", meta = (ClampMin = "1.0"))
    float ConsumedBodyScaleMultiplier = 1.2f;

private:
    UPROPERTY(VisibleAnywhere, Category = "Presentation")
    TObjectPtr<USceneComponent> SceneRoot;

    bool bConsumedSinceReset = false;
    bool bHadMotionLastFrame = false;
    float MotionPreviewDistanceTravelled = 0.0f;
    FVector InitialBodyRelativeLocation = FVector::ZeroVector;
    FVector InitialBodyRelativeScale = FVector::OneVector;

    UFUNCTION()
    void HandleMotionStateChanged(const FMotionTransferResult& Result);

    UFUNCTION()
    void HandleMotionConsumed(const FMotionTransferResult& Result);

    UFUNCTION()
    void HandlePostRoomReset();

    void BindRoomResetController();
    void RefreshPresentation();
};
