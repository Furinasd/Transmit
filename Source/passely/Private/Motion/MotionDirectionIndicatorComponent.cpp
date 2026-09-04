#include "Motion/MotionDirectionIndicatorComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionTransferComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr float IndicatorLengthScaleDivisor = 400.0f;
    constexpr float IndicatorMinLengthScale = 1.0f;
    constexpr float IndicatorMaxLengthScale = 3.0f;
    constexpr float IndicatorRadiusScale = 0.5f;
    constexpr float IndicatorGroundOffsetZ = 8.0f;
    constexpr float IndicatorDistanceFromOwner = 110.0f;
}

UMotionDirectionIndicatorComponent::UMotionDirectionIndicatorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

    // The inherited UArrowComponent stays as the native base class so existing
    // Blueprint SCS templates keep their serialized class layout. Its arrow
    // primitive is never the runtime visual; RuntimeIndicatorMesh is.
    SetHiddenInGame(true);
    SetVisibility(false);

    RuntimeIndicatorMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RuntimeIndicatorMesh"));
    InitializeRuntimeIndicatorMesh(RuntimeIndicatorMesh);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(
        TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (ConeMesh.Succeeded())
    {
        RuntimeIndicatorMesh->SetStaticMesh(ConeMesh.Object);
    }
}

void UMotionDirectionIndicatorComponent::OnRegister()
{
    Super::OnRegister();

    // Blueprint templates saved against the arrow-based class may carry
    // bHiddenInGame=false from the old constructor. Re-assert it at runtime so
    // the inherited arrow can never render in game, while RuntimeIndicatorMesh
    // (which is attached below this component) keeps its own visibility.
    SetHiddenInGame(true);
    EnsureRuntimeIndicatorMesh();
}

void UMotionDirectionIndicatorComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyDirectionColor();
    RefreshFromOwner();
}

void UMotionDirectionIndicatorComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bAutoRefreshFromOwner)
    {
        RefreshFromOwner();
    }
}

bool UMotionDirectionIndicatorComponent::IsAutoRefreshEnabled() const
{
    return bAutoRefreshFromOwner;
}

void UMotionDirectionIndicatorComponent::SetAutoRefreshEnabled(const bool bEnabled)
{
    bAutoRefreshFromOwner = bEnabled;
}

void UMotionDirectionIndicatorComponent::ShowDirection(
    const FVector& Direction,
    const float Magnitude)
{
    EnsureRuntimeIndicatorMesh();

    const FVector SafeDirection = Direction.GetSafeNormal();
    if (SafeDirection.IsNearlyZero())
    {
        HideDirection();
        return;
    }
    ApplyDirectionVisual(Direction, Magnitude);
    ApplyDirectionColor();
    RuntimeIndicatorMesh->SetVisibility(true);
    SetVisibility(true);
}

void UMotionDirectionIndicatorComponent::HideDirection()
{
    EnsureRuntimeIndicatorMesh();
    RuntimeIndicatorMesh->SetVisibility(false);
    SetVisibility(false);
}

void UMotionDirectionIndicatorComponent::SetDirectionColor(
    const FLinearColor& NewColor)
{
    DirectionColor = NewColor;
    ApplyDirectionColor();
}

void UMotionDirectionIndicatorComponent::RefreshFromOwner()
{
    if (!bAutoRefreshFromOwner)
    {
        return;
    }

    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        HideDirection();
        return;
    }

    const UMotionTransferComponent* Motion =
        Owner->FindComponentByClass<UMotionTransferComponent>();
    const UMotionInteractorComponent* Interactor =
        Owner->FindComponentByClass<UMotionInteractorComponent>();

    FMotionState State;
    const bool bHasMotion = Motion && Motion->TryGetMotionState(State);
    if (!bHasMotion)
    {
        HideDirection();
        return;
    }

    FVector DisplayDirection = State.Direction.GetSafeNormal();
    if (Interactor)
    {
        const FMotionInteractionPreview Preview = Interactor->GetCurrentPreview();
        if (Preview.bHasProjectedDirection)
        {
            DisplayDirection = Preview.ProjectedWorldDirection.GetSafeNormal();
        }
    }

    if (DisplayDirection.IsNearlyZero())
    {
        HideDirection();
        return;
    }

    FVector IndicatorLocation = Owner->GetActorLocation();
    const FBox OwnerBounds = Owner->GetComponentsBoundingBox(true);
    if (OwnerBounds.IsValid)
    {
        IndicatorLocation.Z = OwnerBounds.Min.Z + IndicatorGroundOffsetZ;
    }
    IndicatorLocation += DisplayDirection * IndicatorDistanceFromOwner;
    SetWorldLocation(IndicatorLocation);

    ShowDirection(DisplayDirection, State.Magnitude);
}

void UMotionDirectionIndicatorComponent::ApplyDirectionVisual(
    const FVector& Direction,
    const float Magnitude)
{
    // The Engine BasicShapes cone is authored with its apex along local +Z, so
    // align local Z with the world-space direction that the indicator shows.
    SetWorldRotation(FRotationMatrix::MakeFromZ(Direction.GetSafeNormal()).Rotator());

    const float LengthScale = FMath::Clamp(
        Magnitude / IndicatorLengthScaleDivisor,
        IndicatorMinLengthScale,
        IndicatorMaxLengthScale);
    RuntimeIndicatorMesh->SetRelativeScale3D(
        FVector(IndicatorRadiusScale, IndicatorRadiusScale, LengthScale));
}

void UMotionDirectionIndicatorComponent::ApplyDirectionColor()
{
    EnsureRuntimeIndicatorMesh();

    if (!DirectionMaterial)
    {
        if (!IsRegistered() || !RuntimeIndicatorMesh->IsRegistered())
        {
            // Store the color now; the dynamic instance is created once the
            // component is registered (BeginPlay), never during CDO creation.
            return;
        }
        UMaterialInterface* BaseMaterial = RuntimeIndicatorMesh->GetMaterial(0);
        if (!BaseMaterial)
        {
            return;
        }
        DirectionMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        RuntimeIndicatorMesh->SetMaterial(0, DirectionMaterial);
    }

    DirectionMaterial->SetVectorParameterValue(TEXT("Color"), DirectionColor);
}

void UMotionDirectionIndicatorComponent::InitializeRuntimeIndicatorMesh(
    UStaticMeshComponent* Mesh)
{
    check(Mesh);

    Mesh->SetupAttachment(this);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->SetCastShadow(false);
    Mesh->SetHiddenInGame(false);
    Mesh->SetVisibility(false);
}

void UMotionDirectionIndicatorComponent::EnsureRuntimeIndicatorMesh()
{
    if (!RuntimeIndicatorMesh)
    {
        RuntimeIndicatorMesh = NewObject<UStaticMeshComponent>(
            this,
            TEXT("RuntimeIndicatorMesh"),
            RF_Transient);
        InitializeRuntimeIndicatorMesh(RuntimeIndicatorMesh);

        UStaticMesh* ConeMesh = LoadObject<UStaticMesh>(
            nullptr,
            TEXT("/Engine/BasicShapes/Cone.Cone"));
        if (ConeMesh)
        {
            RuntimeIndicatorMesh->SetStaticMesh(ConeMesh);
        }
    }

    if (!RuntimeIndicatorMesh->IsRegistered() && GetWorld())
    {
        RuntimeIndicatorMesh->RegisterComponentWithWorld(GetWorld());
    }
}
