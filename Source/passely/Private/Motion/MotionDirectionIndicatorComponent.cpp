#include "Motion/MotionDirectionIndicatorComponent.h"

#include "GameFramework/Actor.h"
#include "Motion/MotionInteractorComponent.h"
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
    const UMotionInteractorComponent* Interactor = Owner
        ? Owner->FindComponentByClass<UMotionInteractorComponent>()
        : nullptr;

    FMotionState State;
    const bool bHasMotion = Motion && Motion->TryGetMotionState(State);
    SetVisibility(bHasMotion);
    SetHiddenInGame(false);
    if (bHasMotion)
    {
        FVector DisplayDirection = State.Direction.GetSafeNormal();
        if (Interactor)
        {
            const FMotionInteractionPreview Preview = Interactor->GetCurrentPreview();
            if (Preview.bHasProjectedDirection)
            {
                DisplayDirection = Preview.ProjectedWorldDirection.GetSafeNormal();
            }
        }

        FVector GroundLocation = Owner->GetActorLocation();
        GroundLocation.Z = Owner->GetComponentsBoundingBox(true).Min.Z + 8.0f;
        SetWorldLocation(GroundLocation + DisplayDirection * 110.0f);
        SetWorldRotation(DisplayDirection.Rotation());
        SetArrowSize(FMath::Clamp(State.Magnitude / 600.0f, 1.0f, 2.0f));
    }
}
