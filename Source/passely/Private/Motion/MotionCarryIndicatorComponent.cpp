#include "Motion/MotionCarryIndicatorComponent.h"

#include "GameFramework/Actor.h"
#include "Motion/MotionTransferComponent.h"

UMotionCarryIndicatorComponent::UMotionCarryIndicatorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetIntensity(3000.0f);
    SetAttenuationRadius(250.0f);
    SetLightColor(FLinearColor(0.05f, 0.8f, 1.0f));
    SetVisibility(false);
}

void UMotionCarryIndicatorComponent::BeginPlay()
{
    Super::BeginPlay();
    RefreshFromOwner();
}

void UMotionCarryIndicatorComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RefreshFromOwner();
}

void UMotionCarryIndicatorComponent::RefreshFromOwner()
{
    const AActor* Owner = GetOwner();
    const UMotionTransferComponent* Motion = Owner
        ? Owner->FindComponentByClass<UMotionTransferComponent>()
        : nullptr;
    const bool bHasMotion = Motion && Motion->HasMotionState();
    if (bHasMotion != bLastHadMotion || IsVisible() != bHasMotion)
    {
        bLastHadMotion = bHasMotion;
        SetVisibility(bHasMotion);
    }
}
