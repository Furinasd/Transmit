#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"

#include "MotionDirectionIndicatorComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

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

    bool IsAutoRefreshEnabled() const;
    void SetAutoRefreshEnabled(bool bEnabled);

protected:
    virtual void BeginPlay() override;
    virtual void OnRegister() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion|Presentation", meta = (AllowPrivateAccess = "true"))
    FLinearColor DirectionColor = FLinearColor(0.05f, 0.8f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Motion|Presentation")
    bool bAutoRefreshFromOwner = true;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DirectionMaterial;

    void RefreshFromOwner();
    void ApplyDirectionVisual(const FVector& Direction, float Magnitude);
    void ApplyDirectionColor();
    void EnsureRuntimeIndicatorMesh();
    void InitializeRuntimeIndicatorMesh(UStaticMeshComponent* Mesh);
};
