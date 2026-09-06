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
    constexpr float IndicatorLength = 48.0f;
    constexpr float IndicatorFaceClearance = 8.0f;
    constexpr float IndicatorGroundOffsetZ = 8.0f;
    constexpr float IndicatorDistanceFromOwner = 110.0f;

    const FLinearColor DefaultDirectionColor(0.05f, 0.8f, 1.0f);
    const FLinearColor IndicatorTransferReadyColor(0.25f, 0.9f, 0.55f);
    const FLinearColor IndicatorDirectionMismatchColor(0.95f, 0.3f, 0.22f);
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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ArrowMesh(
        TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle"));
    if (ArrowMesh.Succeeded())
    {
        RuntimeIndicatorMesh->SetStaticMesh(ArrowMesh.Object);
    }

    // Runtime ToolsFramework material keeps a self-occluded arrow legible,
    // dimmed on the far face. Occluded targets still hide the preview entirely.
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> IndicatorMaterial(
        TEXT("/Engine/InteractiveToolsFramework/Materials/GizmoComponentMaterial.GizmoComponentMaterial"));
    if (IndicatorMaterial.Succeeded())
    {
        RuntimeIndicatorMesh->SetMaterial(0, IndicatorMaterial.Object);
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

    // Existing Blueprint SCS templates may still serialize the former cone and
    // material. Replace only this runtime presentation, without resaving assets.
    RuntimeIndicatorMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,
        TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle")));
    RuntimeIndicatorMesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/InteractiveToolsFramework/Materials/GizmoComponentMaterial.GizmoComponentMaterial")));
    DirectionMaterial = nullptr;

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

    // Physical meshes only: a target's arrows/lights must not push the cue away
    // from its surface. This also follows animated Source bodies, not the pivot.
    FBox TargetBounds(ForceInit);
    const UStaticMeshComponent* BodyMesh = nullptr;
    double LargestMeshVolume = -1.0;
    TInlineComponentArray<UStaticMeshComponent*> Meshes;
    Target->GetComponents(Meshes);
    for (const UStaticMeshComponent* Mesh : Meshes)
    {
        if (!Mesh->GetStaticMesh() || !Mesh->IsVisible() || Mesh->bHiddenInGame)
        {
            continue;
        }
        bool bIndicatorMesh = false;
        for (const USceneComponent* Parent = Mesh->GetAttachParent(); Parent;
            Parent = Parent->GetAttachParent())
        {
            if (Parent->IsA<UArrowComponent>())
            {
                bIndicatorMesh = true;
                break;
            }
        }
        if (!bIndicatorMesh)
        {
            const double Volume = Mesh->Bounds.GetBox().GetVolume();
            if (Volume > LargestMeshVolume)
            {
                LargestMeshVolume = Volume;
                BodyMesh = Mesh;
            }
        }
    }
    const FTransform BoundsToWorld = BodyMesh
        ? BodyMesh->GetComponentTransform() : FTransform::Identity;
    TargetBounds = BodyMesh
        ? BodyMesh->GetStaticMesh()->GetBoundingBox() : Target->GetComponentsBoundingBox(false);
    const FVector IndicatorLocation = TargetBounds.IsValid
        ? CalculateFaceAnchor(TargetBounds, DisplayDirection, IndicatorFaceClearance, BoundsToWorld)
        : Target->GetActorLocation() + DisplayDirection * IndicatorFaceClearance;
    SetWorldLocation(IndicatorLocation);

    ShowDirection(DisplayDirection, State.Magnitude);
}

FVector UMotionDirectionIndicatorComponent::CalculateFaceAnchor(
    const FBox& TargetBounds, const FVector& Direction, const float Clearance,
    const FTransform& BoundsToWorld)
{
    const FVector WorldDirection = Direction.GetSafeNormal();
    const FVector SafeDirection = BoundsToWorld.InverseTransformVector(WorldDirection).GetSafeNormal();
    if (!TargetBounds.IsValid)
    {
        return FVector::ZeroVector;
    }
    const FVector Extent = TargetBounds.GetExtent();
    double DistanceToFace = TNumericLimits<double>::Max();
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        if (FMath::Abs(SafeDirection[Axis]) > KINDA_SMALL_NUMBER)
        {
            DistanceToFace = FMath::Min(DistanceToFace,
                Extent[Axis] / FMath::Abs(SafeDirection[Axis]));
        }
    }
    if (SafeDirection.IsNearlyZero())
    {
        return BoundsToWorld.TransformPosition(TargetBounds.GetCenter());
    }
    // Intersect in mesh space so rotated/non-uniformly scaled bodies do not
    // anchor the arrow on empty space at the edge of their world AABB.
    return BoundsToWorld.TransformPosition(TargetBounds.GetCenter() + SafeDirection * DistanceToFace)
        + WorldDirection * FMath::Max(0.0f, Clearance);
}

void UMotionDirectionIndicatorComponent::ApplyDirectionVisual(
    const FVector& Direction,
    const float Magnitude)
{
    // GizmoArrowHandle is a runtime Engine mesh authored from its tail along +X.
    SetWorldRotation(FRotationMatrix::MakeFromX(Direction.GetSafeNormal()).Rotator());
    SetWorldScale3D(FVector::OneVector);
    const UStaticMesh* Mesh = RuntimeIndicatorMesh->GetStaticMesh();
    if (!Mesh)
    {
        return;
    }
    const FBox LocalBounds = Mesh->GetBoundingBox();
    const float Length = IndicatorLength * FMath::Clamp(Magnitude / 600.0f, 0.8f, 1.25f);
    const float Scale = Length / FMath::Max(1.0, LocalBounds.GetSize().X);
    RuntimeIndicatorMesh->SetRelativeScale3D(FVector(Scale));
    RuntimeIndicatorMesh->SetRelativeLocation(FVector(-LocalBounds.Min.X * Scale, 0, 0));
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

    DirectionMaterial->SetVectorParameterValue(TEXT("GizmoColor"), DirectionColor);
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

        UStaticMesh* ArrowMesh = LoadObject<UStaticMesh>(
            nullptr,
            TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle"));
        if (ArrowMesh)
        {
            RuntimeIndicatorMesh->SetStaticMesh(ArrowMesh);
        }

        UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Engine/InteractiveToolsFramework/Materials/GizmoComponentMaterial.GizmoComponentMaterial"));
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
