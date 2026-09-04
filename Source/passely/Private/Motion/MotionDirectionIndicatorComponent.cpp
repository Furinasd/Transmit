#include "Motion/MotionDirectionIndicatorComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
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

    const FLinearColor DefaultDirectionColor(0.05f, 0.8f, 1.0f);
    const FLinearColor IndicatorTransferReadyColor(0.0f, 1.0f, 0.35f);
    const FLinearColor IndicatorDirectionMismatchColor(1.0f, 0.15f, 0.05f);
}

UMotionDirectionIndicatorComponent::UMotionDirectionIndicatorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

    // Keep UArrowComponent as the native base so existing Blueprint SCS
    // templates keep their serialized class layout. Its arrow primitive is
    // never the runtime visual; RuntimeIndicatorMesh is.
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

    // The Engine BasicShapes Cone mesh itself references DefaultMaterial,
    // which has no Color parameter. Use BasicShapeMaterial so the runtime
    // dynamic instance can actually tint the cone (green/red/debug cyan).
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (BasicShapeMaterial.Succeeded())
    {
        RuntimeIndicatorMesh->SetMaterial(0, BasicShapeMaterial.Object);
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

    AActor* Owner = GetOwner();
    if (Owner)
    {
        UMotionInteractorComponent* Interactor =
            Owner->FindComponentByClass<UMotionInteractorComponent>();
        if (Interactor)
        {
            Interactor->OnPreviewChanged.AddDynamic(
                this,
                &UMotionDirectionIndicatorComponent::HandlePreviewChanged);
        }

        UMotionTransferComponent* Motion =
            Owner->FindComponentByClass<UMotionTransferComponent>();
        if (Motion)
        {
            Motion->OnMotionStateChanged.AddDynamic(
                this,
                &UMotionDirectionIndicatorComponent::HandleMotionStateChanged);
        }
    }

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

void UMotionDirectionIndicatorComponent::HandlePreviewChanged(
    const FMotionInteractionPreview&)
{
    RefreshFromOwner();
}

void UMotionDirectionIndicatorComponent::HandleMotionStateChanged(
    const FMotionTransferResult&)
{
    RefreshFromOwner();
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
        CurrentMode = EMotionDirectionIndicatorMode::Hidden;
        HideDirection();
        return;
    }

    const UMotionTransferComponent* Motion = Owner
        ? Owner->FindComponentByClass<UMotionTransferComponent>()
        : nullptr;
    const UMotionInteractorComponent* Interactor = Owner
        ? Owner->FindComponentByClass<UMotionInteractorComponent>()
        : nullptr;

    FMotionState State;
    const bool bHasMotion = Motion && Motion->TryGetMotionState(State);
    if (!bHasMotion)
    {
        CurrentMode = EMotionDirectionIndicatorMode::NoMotion;
        HideDirection();
        return;
    }

    const FMotionInteractionPreview Preview =
        Interactor ? Interactor->GetCurrentPreview() : FMotionInteractionPreview();
    FVector DisplayDirection = State.Direction.GetSafeNormal();

    if (bShowOwnerDebugArrow)
    {
        // Debug-only owner-side visualization: the gameplay receiver preview
        // must never depend on this path.
        if (Preview.bHasProjectedDirection)
        {
            DisplayDirection = Preview.ProjectedWorldDirection.GetSafeNormal();
        }

        FVector GroundLocation = Owner->GetActorLocation();
        const FBox OwnerBounds = Owner->GetComponentsBoundingBox(true);
        if (OwnerBounds.IsValid)
        {
            GroundLocation.Z = OwnerBounds.Min.Z + IndicatorGroundOffsetZ;
        }
        SetWorldLocation(GroundLocation + DisplayDirection * IndicatorDistanceFromOwner);
        SetDirectionColor(DefaultDirectionColor);
        ShowDirection(DisplayDirection, State.Magnitude);
        return;
    }

    const AActor* Target = Preview.Target;
    if (!Target)
    {
        CurrentMode = EMotionDirectionIndicatorMode::Hidden;
        HideDirection();
        return;
    }

    if (Preview.Verb != EMotionTransferVerb::Transfer
        || !Preview.bHasProjectedDirection)
    {
        CurrentMode = EMotionDirectionIndicatorMode::InvalidTarget;
        HideDirection();
        return;
    }

    DisplayDirection = Preview.ProjectedWorldDirection.GetSafeNormal();
    if (DisplayDirection.IsNearlyZero())
    {
        CurrentMode = EMotionDirectionIndicatorMode::InvalidTarget;
        HideDirection();
        return;
    }

    if (Preview.bEligible)
    {
        CurrentMode = EMotionDirectionIndicatorMode::TransferReady;
        SetDirectionColor(IndicatorTransferReadyColor);
    }
    else if (Preview.Rejection == EMotionTransferRejection::IncompatibleDirection)
    {
        CurrentMode = EMotionDirectionIndicatorMode::DirectionMismatch;
        SetDirectionColor(IndicatorDirectionMismatchColor);
    }
    else
    {
        CurrentMode = EMotionDirectionIndicatorMode::InvalidTarget;
        HideDirection();
        return;
    }

    FVector IndicatorLocation = Target->GetActorLocation();
    const FBox TargetBounds = Target->GetComponentsBoundingBox(true);
    if (TargetBounds.IsValid)
    {
        IndicatorLocation = TargetBounds.GetCenter();
        IndicatorLocation.Z = TargetBounds.Max.Z + ReceiverPreviewHoverHeight;
    }
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

        UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (BaseMaterial)
        {
            RuntimeIndicatorMesh->SetMaterial(0, BaseMaterial);
        }
    }

    if (!RuntimeIndicatorMesh->IsRegistered() && GetWorld())
    {
        RuntimeIndicatorMesh->RegisterComponentWithWorld(GetWorld());
    }
}
