#pragma once

#include "CoreMinimal.h"
#include "Components/PointLightComponent.h"

#include "MotionCarryIndicatorComponent.generated.h"

UCLASS(ClassGroup = (Motion), meta = (BlueprintSpawnableComponent))
class PASSELY_API UMotionCarryIndicatorComponent : public UPointLightComponent
{
    GENERATED_BODY()

public:
    UMotionCarryIndicatorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bLastHadMotion = false;

    void RefreshFromOwner();
};
