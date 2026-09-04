#include "Motion/TransmitDirectionalCarrierActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Motion/MotionRoomResetController.h"
#include "Motion/MotionTransferComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATransmitDirectionalCarrierActor::ATransmitDirectionalCarrierActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
    Collision->InitCapsuleSize(50.0f, 50.0f);
    Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Collision->SetCanEverAffectNavigation(false);
    SetRootComponent(Collision);

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Collision);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetRelativeScale3D(FVector(1.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Body->SetStaticMesh(CubeMesh.Object);
    }

    Motion = CreateDefaultSubobject<UMotionTransferComponent>(TEXT("Motion"));
    Motion->ParticipantId = TEXT("DirectionalCarrier");
    Motion->bCanProvideMotion = true;
    Motion->bCanReceiveMotion = true;
    Motion->EndpointMode = EMotionEndpointMode::Store;

    Motion->OnMotionStateChanged.AddDynamic(
        this,
        &ATransmitDirectionalCarrierActor::HandleMotionStateChanged);
}

void ATransmitDirectionalCarrierActor::BeginPlay()
{
    Super::BeginPlay();

    RefreshMovementFromOwnership();
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(
            this,
            &ATransmitDirectionalCarrierActor::BindRoomResetController));
}

void ATransmitDirectionalCarrierActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TickMovement(DeltaSeconds);
}

UMotionTransferComponent* ATransmitDirectionalCarrierActor::GetMotionTransferComponent_Implementation() const
{
    return Motion;
}

bool ATransmitDirectionalCarrierActor::IsMovementActive() const
{
    return bMovementActive;
}

bool ATransmitDirectionalCarrierActor::IsBlockedByCollision() const
{
    return bBlockedByCollision;
}

void ATransmitDirectionalCarrierActor::HandleMotionStateChanged(
    const FMotionTransferResult& Result)
{
    RefreshMovementFromOwnership();
}

void ATransmitDirectionalCarrierActor::HandlePostRoomReset()
{
    bBlockedByCollision = false;
    RefreshMovementFromOwnership();
}

void ATransmitDirectionalCarrierActor::BindRoomResetController()
{
    TActorIterator<AMotionRoomResetController> ResetIt(GetWorld());
    if (ResetIt)
    {
        ResetIt->OnPostRoomReset.AddDynamic(
            this,
            &ATransmitDirectionalCarrierActor::HandlePostRoomReset);
    }
}

void ATransmitDirectionalCarrierActor::RefreshMovementFromOwnership()
{
    const bool bHasMotion = Motion && Motion->HasMotionState();
    bMovementActive = bHasMotion;
    bBlockedByCollision = false;
    bHadMotionLastFrame = bHasMotion;
}

void ATransmitDirectionalCarrierActor::TickMovement(const float DeltaSeconds)
{
    if (!Motion)
    {
        return;
    }

    FMotionState State;
    const bool bHasMotion = Motion->TryGetMotionState(State);
    if (!bHasMotion)
    {
        bMovementActive = false;
        bBlockedByCollision = false;
        bHadMotionLastFrame = false;
        return;
    }

    // A newly-entered ownership cycle clears any previous collision stop so a
    // re-transferred state can move again. After this point the flag is only
    // cleared by Capture, Reset, or a state-changed event.
    if (!bHadMotionLastFrame)
    {
        bBlockedByCollision = false;
        bMovementActive = true;
    }
    bHadMotionLastFrame = true;

    if (bBlockedByCollision)
    {
        bMovementActive = false;
        return;
    }

    bMovementActive = true;
    if (!State.IsValid() || State.Direction.IsNearlyZero())
    {
        return;
    }

    const float Distance = FMath::Max(0.0f, MovementSpeed) * DeltaSeconds;
    if (Distance <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    FHitResult SweepHit;
    AddActorWorldOffset(
        State.Direction.GetSafeNormal() * Distance,
        true,
        &SweepHit,
        ETeleportType::None);

    if (SweepHit.bBlockingHit)
    {
        bMovementActive = false;
        bBlockedByCollision = true;
    }
}
