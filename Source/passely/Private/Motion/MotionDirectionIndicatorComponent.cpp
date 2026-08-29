#include "Motion/MotionDirectionIndicatorComponent.h"

#include "GameFramework/Actor.h"
#include "Motion/MotionTransferComponent.h"

UMotionDirectionIndicatorComponent::UMotionDirectionIndicatorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetArrowColor(FColor::Cyan);
    SetArrowSize(2.0f);
    SetHiddenInGame(false);
    SetVisibility(false);
}

void UMotionDirectionIndicatorComponent::BeginPlay()
{
    Super::BeginPlay();
    RefreshFromOwner();
}

void UMotionDirectionIndicatorComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    RefreshFromOwner();
}

void UMotionDirectionIndicatorComponent::RefreshFromOwner()
{
    const AActor* Owner = GetOwner();
    const UMotionTransferComponent* Motion = Owner
        ? Owner->FindComponentByClass<UMotionTransferComponent>()
        : nullptr;

    FMotionState State;
    const bool bHasMotion = Motion && Motion->TryGetMotionState(State);
    SetVisibility(bHasMotion);
    SetHiddenInGame(false);
    if (bHasMotion)
    {
        SetWorldRotation(State.Direction.Rotation());
        SetArrowSize(FMath::Clamp(State.Magnitude / 300.0f, 1.5f, 4.0f));
    }
}
