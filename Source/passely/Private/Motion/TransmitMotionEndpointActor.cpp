#include "Motion/TransmitMotionEndpointActor.h"

#include "Components/ArrowComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Motion/MotionDirectionIndicatorComponent.h"
#include "Motion/MotionRoomResetController.h"
#include "Motion/MotionTransferComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATransmitMotionEndpointActor::ATransmitMotionEndpointActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(SceneRoot);
    Body->SetRelativeScale3D(FVector(1.25f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Body->SetStaticMesh(CubeMesh.Object);
    }

    MotionIndicator = CreateDefaultSubobject<UPointLightComponent>(TEXT("MotionIndicator"));
    MotionIndicator->SetupAttachment(SceneRoot);
    MotionIndicator->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    MotionIndicator->SetIntensity(5000.0f);
    MotionIndicator->SetAttenuationRadius(350.0f);
    MotionIndicator->SetLightColor(FLinearColor(0.05f, 0.8f, 1.0f));
    MotionIndicator->SetVisibility(false);

    DirectionIndicator = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionIndicator"));
    DirectionIndicator->SetupAttachment(SceneRoot);
    DirectionIndicator->SetArrowColor(FColor::Cyan);
    DirectionIndicator->SetArrowSize(0.6f);
    DirectionIndicator->SetHiddenInGame(false);
    DirectionIndicator->SetVisibility(false);

    Motion = CreateDefaultSubobject<UMotionTransferComponent>(TEXT("Motion"));
}

void ATransmitMotionEndpointActor::BeginPlay()
{
    Super::BeginPlay();

    InitialBodyRelativeLocation = Body->GetRelativeLocation();
    InitialBodyRelativeScale = Body->GetRelativeScale3D();

    Motion->OnMotionStateChanged.AddDynamic(
        this,
        &ATransmitMotionEndpointActor::HandleMotionStateChanged);
    Motion->OnMotionConsumed.AddDynamic(
        this,
        &ATransmitMotionEndpointActor::HandleMotionConsumed);
    RefreshPresentation();

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(
            this,
            &ATransmitMotionEndpointActor::BindRoomResetController));
}

void ATransmitMotionEndpointActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    FMotionState State;
    const bool bHasMotion = Motion->TryGetMotionState(State);
    const bool bHasDirectionToLoop = bHasMotion || bConsumedSinceReset;
    if (!bHasDirectionToLoop || !bAnimateOwnedMotion)
    {
        bHadMotionLastFrame = false;
        MotionPreviewDistanceTravelled = 0.0f;
        return;
    }

    if (!bHadMotionLastFrame)
    {
        MotionPreviewDistanceTravelled = 0.0f;
    }
    bHadMotionLastFrame = true;

    const float LoopMagnitude = bHasMotion ? State.Magnitude : ConsumedLoopMagnitude;
    const float PreviewSpeed = FMath::Max(0.0f, LoopMagnitude) * MotionPreviewSpeedScale;
    MotionPreviewDistanceTravelled = FMath::Fmod(
        MotionPreviewDistanceTravelled + PreviewSpeed * DeltaSeconds,
        FMath::Max(1.0f, MotionPreviewDistance));

    const FVector LoopWorldDirection = bHasMotion
        ? State.Direction.GetSafeNormal()
        : ConsumedLoopDirection.GetSafeNormal();
    if (LoopWorldDirection.IsNearlyZero())
    {
        return;
    }

    const FVector LocalDirection = GetActorTransform()
        .InverseTransformVectorNoScale(LoopWorldDirection)
        .GetSafeNormal();
    Body->SetRelativeLocation(
        InitialBodyRelativeLocation + LocalDirection * MotionPreviewDistanceTravelled);
    if (bHasMotion)
    {
        UpdateDirectionIndicator(State);
    }
}

void ATransmitMotionEndpointActor::HandleMotionStateChanged(
    const FMotionTransferResult& Result)
{
    if (!Motion->HasMotionState())
    {
        bHadMotionLastFrame = false;
    }
    RefreshPresentation();
}

void ATransmitMotionEndpointActor::HandleMotionConsumed(
    const FMotionTransferResult& Result)
{
    bConsumedSinceReset = Result.bSucceeded && Result.bConsumed;
    ConsumedLoopDirection = bConsumedSinceReset
        ? Result.StateSnapshot.Direction.GetSafeNormal()
        : FVector::ZeroVector;
    ConsumedLoopMagnitude = bConsumedSinceReset
        ? Result.StateSnapshot.Magnitude
        : 0.0f;
    RefreshPresentation();
}

void ATransmitMotionEndpointActor::HandlePostRoomReset()
{
    bConsumedSinceReset = false;
    ConsumedLoopDirection = FVector::ZeroVector;
    ConsumedLoopMagnitude = 0.0f;
    bHadMotionLastFrame = false;
    MotionPreviewDistanceTravelled = 0.0f;
    Body->SetRelativeLocation(InitialBodyRelativeLocation);
    RefreshPresentation();
}

void ATransmitMotionEndpointActor::BindRoomResetController()
{
    TActorIterator<AMotionRoomResetController> ResetIt(GetWorld());
    if (ResetIt)
    {
        ResetIt->OnPostRoomReset.AddDynamic(
            this,
            &ATransmitMotionEndpointActor::HandlePostRoomReset);
    }
}

void ATransmitMotionEndpointActor::RefreshPresentation()
{
    FMotionState State;
    const bool bHasMotion = Motion->TryGetMotionState(State);
    const bool bShowsMotion = bHasMotion || bConsumedSinceReset;
    MotionIndicator->SetVisibility(bShowsMotion);
    DirectionIndicator->SetVisibility(bHasMotion);
    DirectionIndicator->SetHiddenInGame(false);

    Body->SetRelativeScale3D(
        bConsumedSinceReset
            ? InitialBodyRelativeScale * ConsumedBodyScaleMultiplier
            : InitialBodyRelativeScale);
    if (bHasMotion)
    {
        UpdateDirectionIndicator(State);
    }
}

void ATransmitMotionEndpointActor::UpdateDirectionIndicator(const FMotionState& State)
{
    if (const UStaticMesh* BodyMesh = Body->GetStaticMesh())
    {
        DirectionIndicator->SetWorldLocation(
            UMotionDirectionIndicatorComponent::CalculateFaceAnchor(
                BodyMesh->GetBoundingBox(), State.Direction, 8.0f, Body->GetComponentTransform()));
    }
    DirectionIndicator->SetWorldRotation(State.Direction.Rotation());
    DirectionIndicator->SetArrowSize(0.6f * FMath::Clamp(State.Magnitude / 600.0f, 0.8f, 1.25f));
}
