#include "Motion/TransmitChargerActor.h"

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

ATransmitChargerActor::ATransmitChargerActor()
{
    PrimaryActorTick.bCanEverTick = true;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
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

    ThreatIndicator = CreateDefaultSubobject<UPointLightComponent>(TEXT("ThreatIndicator"));
    ThreatIndicator->SetupAttachment(SceneRoot);
    ThreatIndicator->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
    ThreatIndicator->SetIntensity(6000.0f);
    ThreatIndicator->SetAttenuationRadius(400.0f);
    ThreatIndicator->SetLightColor(FLinearColor(1.0f, 0.12f, 0.05f));
    ThreatIndicator->SetVisibility(false);

    DirectionIndicator = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionIndicator"));
    DirectionIndicator->SetupAttachment(SceneRoot);
    DirectionIndicator->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
    DirectionIndicator->SetArrowColor(FColor::Red);
    DirectionIndicator->SetArrowSize(2.5f);
    DirectionIndicator->SetHiddenInGame(false);
    DirectionIndicator->SetVisibility(false);

    Motion = CreateDefaultSubobject<UMotionTransferComponent>(TEXT("Motion"));
    Motion->ParticipantId = TEXT("Charger");
    Motion->bCanProvideMotion = true;
    Motion->bCanReceiveMotion = false;
    Motion->EndpointMode = EMotionEndpointMode::Store;

    StateMachine = CreateDefaultSubobject<UMotionChargerStateMachine>(TEXT("StateMachine"));
}

void ATransmitChargerActor::BeginPlay()
{
    Super::BeginPlay();

    Motion->OnMotionStateChanged.AddDynamic(
        this,
        &ATransmitChargerActor::HandleMotionStateChanged);
    RefreshPresentation();

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(
            this,
            &ATransmitChargerActor::BindRoomResetController));

    if (bAutoStartCycle)
    {
        StartChargerCycle();
    }
}

void ATransmitChargerActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bCycleRunning || !StateMachine)
    {
        bWasDashing = false;
        return;
    }

    StateMachine->Tick(DeltaSeconds);

    if (StateMachine->IsDashCommitted())
    {
        if (!bWasDashing)
        {
            bWasDashing = true;
            GrantDashMotionState();
        }

        FMotionState OwnedState;
        if (Motion->TryGetMotionState(OwnedState))
        {
            AddActorWorldOffset(
                OwnedState.Direction.GetSafeNormal() * DashSpeed * DeltaSeconds,
                false,
                nullptr,
                ETeleportType::None);
        }
    }
    else
    {
        bWasDashing = false;
    }

    RefreshPresentation();
}

UMotionTransferComponent* ATransmitChargerActor::GetMotionTransferComponent_Implementation() const
{
    return Motion;
}

FMotionCompatibilityResult ATransmitChargerActor::CanCaptureMotion_Implementation(
    const FMotionTransferContext& Context) const
{
    const FMotionCompatibilityResult BaseResult =
        IMotionTransferable::CanCaptureMotion_Implementation(Context);
    if (!BaseResult.bAllowed)
    {
        return BaseResult;
    }

    if (!StateMachine || !StateMachine->IsCaptureWindowOpen())
    {
        return FMotionCompatibilityResult::Reject(
            EMotionTransferRejection::TimingRejected);
    }

    return Motion ? Motion->CanProvideMotion()
        : FMotionCompatibilityResult::Reject(
            EMotionTransferRejection::MissingMotionComponent);
}

void ATransmitChargerActor::StartChargerCycle()
{
    bCycleRunning = true;
    if (StateMachine)
    {
        StateMachine->Reset();
        StateMachine->Start();
    }
    RefreshPresentation();
}

void ATransmitChargerActor::StopChargerCycle()
{
    bCycleRunning = false;
    bWasDashing = false;
    if (StateMachine)
    {
        StateMachine->Reset();
    }
    RefreshPresentation();
}

void ATransmitChargerActor::HandleMotionStateChanged(const FMotionTransferResult& Result)
{
    if (!Motion->HasMotionState()
        && StateMachine
        && StateMachine->GetState() == EMotionChargerState::Dash)
    {
        // The Player captured the dash: the Charger stops/staggers immediately.
        StateMachine->ForceRecovery();
        bWasDashing = false;
    }
    RefreshPresentation();
}

void ATransmitChargerActor::HandlePostRoomReset()
{
    bCycleRunning = false;
    bWasDashing = false;
    if (StateMachine)
    {
        StateMachine->Reset();
    }
    RefreshPresentation();
    if (bAutoStartCycle)
    {
        StartChargerCycle();
    }
}

void ATransmitChargerActor::BindRoomResetController()
{
    for (TActorIterator<AMotionRoomResetController> ResetIt(GetWorld()); ResetIt; ++ResetIt)
    {
        ResetIt->OnPostRoomReset.AddDynamic(
            this,
            &ATransmitChargerActor::HandlePostRoomReset);
        break;
    }
}

void ATransmitChargerActor::GrantDashMotionState()
{
    FMotionState DashState;
    DashState.Type = EMotionType::Linear;
    DashState.Direction = DashDirection.GetSafeNormal();
    DashState.Magnitude = DashMotionMagnitude;
    DashState.SourceId = DashSourceId;
    if (DashState.IsValid())
    {
        Motion->GrantMotionState(DashState);
    }
}

void ATransmitChargerActor::RefreshPresentation()
{
    const bool bActive = StateMachine
        && StateMachine->GetState() != EMotionChargerState::Idle;
    ThreatIndicator->SetVisibility(bActive);

    const bool bShowsDirection = StateMachine
        && (StateMachine->GetState() == EMotionChargerState::Telegraph
            || StateMachine->GetState() == EMotionChargerState::Dash);
    DirectionIndicator->SetVisibility(bShowsDirection);
    DirectionIndicator->SetHiddenInGame(false);
    if (bShowsDirection)
    {
        DirectionIndicator->SetWorldRotation(DashDirection.GetSafeNormal().Rotation());
    }
}
