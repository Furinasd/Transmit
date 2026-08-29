#include "Motion/TransmitMotionEndpointActor.h"

#include "Components/ArrowComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Motion/MotionRoomResetController.h"
#include "Motion/MotionTransferComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATransmitMotionEndpointActor::ATransmitMotionEndpointActor()
{
    PrimaryActorTick.bCanEverTick = false;

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
    DirectionIndicator->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    DirectionIndicator->SetArrowColor(FColor::Cyan);
    DirectionIndicator->SetArrowSize(2.0f);
    DirectionIndicator->SetVisibility(false);

    Motion = CreateDefaultSubobject<UMotionTransferComponent>(TEXT("Motion"));
}

void ATransmitMotionEndpointActor::BeginPlay()
{
    Super::BeginPlay();

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

void ATransmitMotionEndpointActor::HandleMotionStateChanged(
    const FMotionTransferResult& Result)
{
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
    RefreshPresentation();
}

void ATransmitMotionEndpointActor::BindRoomResetController()
{
    for (TActorIterator<AMotionRoomResetController> ResetIt(GetWorld()); ResetIt; ++ResetIt)
    {
        ResetIt->OnPostRoomReset.AddDynamic(
            this,
            &ATransmitMotionEndpointActor::HandlePostRoomReset);
        break;
    }
}

void ATransmitMotionEndpointActor::RefreshPresentation()
{
    const bool bShowsMotion = Motion->HasMotionState() || bConsumedSinceReset;
    MotionIndicator->SetVisibility(bShowsMotion);
    DirectionIndicator->SetVisibility(Motion->HasMotionState());
}
