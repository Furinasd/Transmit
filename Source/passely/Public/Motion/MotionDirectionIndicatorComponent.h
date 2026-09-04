#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"
#include "Motion/MotionTransferTypes.h"

#include "MotionDirectionIndicatorComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EMotionDirectionIndicatorMode : uint8
{
    Hidden,
    NoMotion,
    InvalidTarget,
    DirectionMismatch,
    TransferReady
};

UCLASS(ClassGroup = (Motion), meta = (BlueprintSpawnableComponent))
class PASSELY_API UMotionDirectionIndicatorComponent : public UArrowComponent
{
    GENERATED_BODY()

public:
    UMotionDirectionIndicatorComponent();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Motion|Presentation")
    TObjectPtr<UStaticMeshComponent> RuntimeIndicatorMesh;

    UFUNCTION(BlueprintCallable, Category = "Motion|Presentation")
    void ShowDirection(const FVector& Direction, float Magnitude);

    UFUNCTION(BlueprintCallable, Category = "Motion|Presentation")
    void HideDirection();

    UFUNCTION(BlueprintCallable, Category = "Motion|Presentation")
    void SetDirectionColor(const FLinearColor& NewColor);

protected:
    virtual void BeginPlay() override;
    virtual void OnRegister() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Motion|Presentation", meta = (AllowPrivateAccess = "true"))
    EMotionDirectionIndicatorMode CurrentMode = EMotionDirectionIndicatorMode::Hidden;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion|Presentation", meta = (AllowPrivateAccess = "true"))
    FLinearColor DirectionColor = FLinearColor(0.05f, 0.8f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion|Debug", meta = (AllowPrivateAccess = "true"))
    bool bShowOwnerDebugArrow = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion|Presentation", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
    float ReceiverPreviewHoverHeight = 45.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Motion|Presentation")
    bool bAutoRefreshFromOwner = true;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DirectionMaterial;

    UFUNCTION()
    void HandlePreviewChanged(const FMotionInteractionPreview& Preview);

    UFUNCTION()
    void HandleMotionStateChanged(const FMotionTransferResult& Result);

    void RefreshFromOwner();
    void ApplyDirectionVisual(const FVector& Direction, float Magnitude);
    void ApplyDirectionColor();
    void EnsureRuntimeIndicatorMesh();
    void InitializeRuntimeIndicatorMesh(UStaticMeshComponent* Mesh);
};
