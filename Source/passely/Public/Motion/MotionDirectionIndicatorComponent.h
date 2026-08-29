#pragma once

#include "CoreMinimal.h"
#include "Components/ArrowComponent.h"

#include "MotionDirectionIndicatorComponent.generated.h"

UCLASS(ClassGroup = (Motion), meta = (BlueprintSpawnableComponent))
class PASSELY_API UMotionDirectionIndicatorComponent : public UArrowComponent
{
    GENERATED_BODY()

public:
    UMotionDirectionIndicatorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    void RefreshFromOwner();
};
