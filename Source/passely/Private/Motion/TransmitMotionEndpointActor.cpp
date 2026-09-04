#include "Motion/TransmitMotionEndpointActor.h"

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

    DirectionIndicator = CreateDefaultSubobject<UMotionDirectionIndicatorComponent>(
        TEXT("DirectionIndicator"));
    DirectionIndicator->SetupAttachment(SceneRoot);
    DirectionIndicator->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    DirectionIndicator->SetDirectionColor(FLinearColor(0.05f, 0.8f, 1.0f));
    DirectionIndicator->SetAutoRefreshEnabled(false);

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
    if (!bHasMotion || !bAnimateOwnedMotion)
    {
        bHadMotionLastFrame = bHasMotion;
        return;
    }

    if (!bHadMotionLastFrame)
    {
        MotionPreviewDistanceTravelled = 0.0f;
    }
    bHadMotionLastFrame = true;

    const float PreviewSpeed = State.Magnitude * MotionPreviewSpeedScale;
    MotionPreviewDistanceTravelled = FMath::Fmod(
        MotionPreviewDistanceTravelled + PreviewSpeed * DeltaSeconds,
        FMath::Max(1.0f, MotionPreviewDistance));

    const FVector LocalDirection = GetActorTransform()
        .InverseTransformVectorNoScale(State.Direction)
        .GetSafeNormal();
    Body->SetRelativeLocation(
        InitialBodyRelativeLocation + LocalDirection * MotionPreviewDistanceTravelled);
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
    RefreshPresentation();
}

void ATransmitMotionEndpointActor::HandlePostRoomReset()
{
    bConsumedSinceReset = false;
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
    if (bHasMotion)
    {
        DirectionIndicator->ShowDirection(State.Direction, State.Magnitude);
    }
    else
    {
        DirectionIndicator->HideDirection();
    }

    Body->SetRelativeScale3D(
        bConsumedSinceReset
            ? InitialBodyRelativeScale * ConsumedBodyScaleMultiplier
            : InitialBodyRelativeScale);
}
