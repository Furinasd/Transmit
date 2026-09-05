#include "Transmit/TransmitLevelActors.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Motion/MotionRoomResetController.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/TransmitCharacter.h"
#include "TimerManager.h"

ATransmitBridgeSlab::ATransmitBridgeSlab()
{
    SlabCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("SlabCollision"));
    SlabCollision->SetBoxExtent(FVector(500.0f, 220.0f, 30.0f));
    SlabCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    SlabCollision->SetCanEverAffectNavigation(false);

    SetRootComponent(SlabCollision);

    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Collision->SetupAttachment(SlabCollision);
    Collision->SetRelativeLocation(FVector::ZeroVector);

    Body->SetupAttachment(SlabCollision);
    Body->SetRelativeLocation(FVector::ZeroVector);
    Body->SetRelativeScale3D(FVector(10.0f, 4.4f, 0.6f));

    MovementSpeed = 180.0f;
}

ATransmitRam::ATransmitRam()
{
    PrimaryActorTick.bCanEverTick = true;

    bAnimateOwnedMotion = false;
    ConsumedBodyScaleMultiplier = 1.0f;

    Motion->ParticipantId = TEXT("TransmitRam");
    Motion->bCanProvideMotion = false;
    Motion->bCanReceiveMotion = true;
    Motion->EndpointMode = EMotionEndpointMode::ConsumeOnReceive;
}

void ATransmitRam::BeginPlay()
{
    Super::BeginPlay();

    CacheInitialTransforms();

    Motion->OnMotionConsumed.AddDynamic(this, &ATransmitRam::HandleRamMotionConsumed);

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &ATransmitRam::BindRamRoomResetController));
}

void ATransmitRam::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    LatchArmIfReady();
    // Arming is a spatial event; expose it at the device before any UI request.
    MotionIndicator->SetVisibility(bArmed);
    MotionIndicator->SetLightColor(FLinearColor(0.05f, 0.9f, 0.65f));
    DirectionIndicator->SetVisibility(bArmed && Hits < 2);
    DirectionIndicator->SetWorldRotation(FixedAxis.Rotation());

    if (!bInFlightImpact)
    {
        return;
    }

    ImpactElapsed += DeltaSeconds;

    if (!bReturningBody)
    {
        const float T = FMath::Clamp(
            ImpactElapsed / FMath::Max(0.001f, ImpactApproachSeconds),
            0.0f,
            1.0f);
        Body->SetRelativeLocation(FMath::Lerp(InitialBodyRelativeLocation, ExtendedBodyRelativeLocation, T));

        if (ImpactElapsed >= ImpactApproachSeconds)
        {
            Body->SetRelativeLocation(ExtendedBodyRelativeLocation);
            ApplyGateImpact();
            bReturningBody = true;
            ImpactElapsed = 0.0f;
        }
    }
    else
    {
        const float T = FMath::Clamp(
            ImpactElapsed / FMath::Max(0.001f, ImpactReturnSeconds),
            0.0f,
            1.0f);
        Body->SetRelativeLocation(FMath::Lerp(ExtendedBodyRelativeLocation, InitialBodyRelativeLocation, T));

        if (ImpactElapsed >= ImpactReturnSeconds)
        {
            Body->SetRelativeLocation(InitialBodyRelativeLocation);
            bInFlightImpact = false;
            bReturningBody = false;
            ImpactElapsed = 0.0f;
        }
    }
}

FMotionCompatibilityResult ATransmitRam::CanReceiveMotion_Implementation(
    const FMotionState& State,
    const FMotionTransferContext& Context) const
{
    const FMotionCompatibilityResult BaseResult =
        IMotionTransferable::CanReceiveMotion_Implementation(State, Context);
    if (!BaseResult.bAllowed)
    {
        return BaseResult;
    }

    if (!bArmed)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::TimingRejected);
    }

    if (Hits >= 2)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::RequestsBlocked);
    }

    if (bInFlightImpact)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::CooldownActive);
    }

    if (State.DirectionPolicy != EMotionDirectionPolicy::PreserveSource)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::IncompatibleType);
    }

    if (State.Magnitude < MinimumMagnitude)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::IncompatibleMagnitudeTier);
    }

    const FVector Axis = FixedAxis.GetSafeNormal();
    const FVector Direction = State.Direction.GetSafeNormal();
    if (Axis.IsNearlyZero()
        || Direction.IsNearlyZero()
        || FVector::DotProduct(Axis, Direction) < 0.98f)
    {
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::IncompatibleDirection);
    }

    return FMotionCompatibilityResult::Allow();
}

void ATransmitRam::HandleRamMotionConsumed(const FMotionTransferResult& Result)
{
    if (!Result.bSucceeded || !Result.bConsumed)
    {
        return;
    }

    if (bInFlightImpact || bReturningBody)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Ram received impact motion"));
    BeginImpactAnimation();
}

void ATransmitRam::HandleRamPostRoomReset()
{
    bArmed = false;
    Hits = 0;
    bInFlightImpact = false;
    bReturningBody = false;
    ImpactElapsed = 0.0f;

    if (Body)
    {
        Body->SetRelativeLocation(InitialBodyRelativeLocation);
    }

    RestoreCarrierPermissions();
    RestoreGate();

    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Ram reset"));
}

void ATransmitRam::BindRamRoomResetController()
{
    TActorIterator<AMotionRoomResetController> ResetIt(GetWorld());
    if (ResetIt)
    {
        ResetIt->OnPostRoomReset.AddDynamic(this, &ATransmitRam::HandleRamPostRoomReset);
    }
}

void ATransmitRam::CacheInitialTransforms()
{
    if (Body)
    {
        InitialBodyRelativeLocation = Body->GetRelativeLocation();
    }

    if (Gate)
    {
        InitialGateLocation = Gate->GetActorLocation();
        InitialGateRotation = Gate->GetActorRotation();
        bInitialGateCollisionEnabled = Gate->GetActorEnableCollision();
        bGateInitialCached = true;
    }

    if (RouteCarrier)
    {
        if (UMotionTransferComponent* CarrierMotion =
                RouteCarrier->GetMotionTransferComponent_Implementation())
        {
            bInitialCarrierCanProvide = CarrierMotion->bCanProvideMotion;
            bInitialCarrierCanReceive = CarrierMotion->bCanReceiveMotion;
            bCarrierInitialized = true;
        }
    }
}

void ATransmitRam::LatchArmIfReady()
{
    if (bArmed || !RouteCarrier)
    {
        return;
    }

    UMotionTransferComponent* CarrierMotion =
        RouteCarrier->GetMotionTransferComponent_Implementation();
    if (!CarrierMotion)
    {
        return;
    }

    FMotionState CarrierState;
    if (!CarrierMotion->TryGetMotionState(CarrierState))
    {
        return;
    }

    if (CarrierState.Type != EMotionType::Linear
        || CarrierState.DirectionPolicy != EMotionDirectionPolicy::CameraCanonical)
    {
        return;
    }

    const float DockRadiusSq = DockRadius * DockRadius;
    if (FVector::DistSquared(RouteCarrier->GetActorLocation(), GetDockCenter()) > DockRadiusSq)
    {
        return;
    }

    if (!bCarrierInitialized)
    {
        bInitialCarrierCanProvide = CarrierMotion->bCanProvideMotion;
        bInitialCarrierCanReceive = CarrierMotion->bCanReceiveMotion;
        bCarrierInitialized = true;
    }

    bArmed = true;
    CarrierMotion->bCanProvideMotion = false;
    CarrierMotion->bCanReceiveMotion = false;

    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Ram armed from carrier %s"), *GetNameSafe(RouteCarrier));
}

void ATransmitRam::BeginImpactAnimation()
{
    bInFlightImpact = true;
    bReturningBody = false;
    ImpactElapsed = 0.0f;

    if (Body)
    {
        Body->SetRelativeLocation(InitialBodyRelativeLocation);
    }

    ExtendedBodyRelativeLocation = InitialBodyRelativeLocation + GetLocalFixedAxis() * ImpactDistance;
}

void ATransmitRam::ApplyGateImpact()
{
    Hits++;

    if (Hits == 1)
    {
        UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Ram impact 1: gate cracked"));

        if (Gate)
        {
            if (!bGateInitialCached)
            {
                InitialGateLocation = Gate->GetActorLocation();
                InitialGateRotation = Gate->GetActorRotation();
                bInitialGateCollisionEnabled = Gate->GetActorEnableCollision();
                bGateInitialCached = true;
            }

            Gate->AddActorWorldOffset(FVector(0.0f, 0.0f, 20.0f));
            Gate->AddActorLocalRotation(FRotator(4.0f, 0.0f, 3.0f));
        }
    }
    else if (Hits == 2)
    {
        UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Ram impact 2: gate broken"));

        if (Gate)
        {
            if (!bGateInitialCached)
            {
                InitialGateLocation = Gate->GetActorLocation();
                InitialGateRotation = Gate->GetActorRotation();
                bInitialGateCollisionEnabled = Gate->GetActorEnableCollision();
                bGateInitialCached = true;
            }

            Gate->SetActorLocation(
                InitialGateLocation + FVector(0.0f, 0.0f, 650.0f),
                false,
                nullptr,
                ETeleportType::TeleportPhysics);
            Gate->SetActorEnableCollision(false);
        }
    }
}

void ATransmitRam::RestoreCarrierPermissions()
{
    if (!bCarrierInitialized || !RouteCarrier)
    {
        return;
    }

    if (UMotionTransferComponent* CarrierMotion =
            RouteCarrier->GetMotionTransferComponent_Implementation())
    {
        CarrierMotion->bCanProvideMotion = bInitialCarrierCanProvide;
        CarrierMotion->bCanReceiveMotion = bInitialCarrierCanReceive;
    }
}

void ATransmitRam::RestoreGate()
{
    if (!bGateInitialCached || !Gate)
    {
        return;
    }

    Gate->SetActorLocation(InitialGateLocation, false, nullptr, ETeleportType::TeleportPhysics);
    Gate->SetActorRotation(InitialGateRotation);
    Gate->SetActorEnableCollision(bInitialGateCollisionEnabled);
}

FVector ATransmitRam::GetLocalFixedAxis() const
{
    const FVector WorldAxis = FixedAxis.GetSafeNormal();
    if (WorldAxis.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }

    return GetActorTransform().InverseTransformVectorNoScale(WorldAxis).GetSafeNormal();
}

FVector ATransmitRam::GetDockCenter() const
{
    return DockMarker ? DockMarker->GetActorLocation() : GetActorLocation();
}

ATransmitArenaCharger::ATransmitArenaCharger()
{
    bAutoStartCycle = false;
    DashDirection = FVector::ForwardVector;
    LastFrameState = EMotionChargerState::Idle;

    Collision->OnComponentHit.AddDynamic(this, &ATransmitArenaCharger::HandleArenaComponentHit);
}

void ATransmitArenaCharger::BeginPlay()
{
    Super::BeginPlay();

    HomeTransform = GetActorTransform();
    LastFrameState = StateMachine ? StateMachine->GetState() : EMotionChargerState::Idle;

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &ATransmitArenaCharger::BindArenaRoomResetController));
}

void ATransmitArenaCharger::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!StateMachine)
    {
        LastFrameState = EMotionChargerState::Idle;
        return;
    }

    const EMotionChargerState CurrentState = StateMachine->GetState();
    if (LastFrameState == EMotionChargerState::Recovery
        && CurrentState == EMotionChargerState::Idle)
    {
        ReturnToHome();
    }

    LastFrameState = CurrentState;
}

void ATransmitArenaCharger::SetEncounterActive(const bool bActive)
{
    bEncounterActive = bActive;

    if (bActive)
    {
        StartChargerCycle();
    }
    else
    {
        StopChargerCycle();
    }

    LastFrameState = StateMachine ? StateMachine->GetState() : EMotionChargerState::Idle;
}

void ATransmitArenaCharger::HandleArenaComponentHit(
    UPrimitiveComponent*,
    AActor* OtherActor,
    UPrimitiveComponent*,
    FVector,
    const FHitResult&)
{
    if (!bEncounterActive)
    {
        return;
    }

    if (!StateMachine || StateMachine->GetState() != EMotionChargerState::Dash)
    {
        return;
    }

    if (!Cast<ATransmitCharacter>(OtherActor))
    {
        return;
    }

    if (bResetScheduled)
    {
        return;
    }

    if (!FindArenaResetController())
    {
        return;
    }

    bResetScheduled = true;
    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Arena charger hit player; scheduling clean reset"));
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &ATransmitArenaCharger::TryArenaResetFromHit));
}

void ATransmitArenaCharger::HandleArenaPostRoomReset()
{
    bEncounterActive = false;
    bResetScheduled = false;
    LastFrameState = StateMachine ? StateMachine->GetState() : EMotionChargerState::Idle;
    ReturnToHome();

    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Arena charger reset/inactive"));
}

void ATransmitArenaCharger::BindArenaRoomResetController()
{
    AMotionRoomResetController* Reset = FindArenaResetController();
    if (Reset)
    {
        Reset->OnPostRoomReset.AddDynamic(this, &ATransmitArenaCharger::HandleArenaPostRoomReset);
    }
}

void ATransmitArenaCharger::TryArenaResetFromHit()
{
    bResetScheduled = false;

    AMotionRoomResetController* Reset = FindArenaResetController();
    if (!Reset)
    {
        return;
    }

    if (Reset->IsResetInProgress())
    {
        bResetScheduled = true;
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &ATransmitArenaCharger::TryArenaResetFromHit));
        return;
    }

    Reset->RequestRoomReset();
}

void ATransmitArenaCharger::ReturnToHome()
{
    SetActorTransform(HomeTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

AMotionRoomResetController* ATransmitArenaCharger::FindArenaResetController() const
{
    TActorIterator<AMotionRoomResetController> ResetIt(GetWorld());
    return ResetIt ? *ResetIt : nullptr;
}

ATransmitLevelDirector::ATransmitLevelDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

void ATransmitLevelDirector::BeginPlay()
{
    Super::BeginPlay();

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &ATransmitLevelDirector::BindDirectorRoomResetController));
}

void ATransmitLevelDirector::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    APawn* Player = GetPlayerPawn();
    if (!Player)
    {
        return;
    }

    const FVector PlayerLocation = Player->GetActorLocation();

    if (PlayerLocation.Z < FallZ)
    {
        TryRequestRoomReset();
        return;
    }

    if (ArenaEntryMarker
        && !bEntryTriggered
        && PlayerLocation.X >= ArenaEntryMarker->GetActorLocation().X)
    {
        bEntryTriggered = true;
        EncounterStartSeconds = GetWorld()->GetTimeSeconds();

        if (Charger)
        {
            Charger->SetEncounterActive(true);
        }
    }

    const bool bGateBroken = Ram && Ram->Hits >= 2;
    if (bGateBroken && Charger && !bGateBrokenHandled)
    {
        bGateBrokenHandled = true;
        Charger->SetEncounterActive(false);
    }

    if (ExitMarker
        && bGateBroken
        && !bCompletionShown
        && FVector::DistSquared(PlayerLocation, ExitMarker->GetActorLocation()) < ExitDistance * ExitDistance)
    {
        bCompletionShown = true;

        const float Elapsed = EncounterStartSeconds > 0.0f
            ? GetWorld()->GetTimeSeconds() - EncounterStartSeconds
            : 0.0f;

        UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] L_Transmit gate broken: elapsed=%.1fs"), Elapsed);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                6.0f,
                FColor::Green,
                TEXT("TRANSMITTED"));
        }
    }
}

void ATransmitLevelDirector::HandleDirectorPostRoomReset()
{
    bEntryTriggered = false;
    bGateBrokenHandled = false;
    bCompletionShown = false;
    EncounterStartSeconds = 0.0f;

    APawn* Player = GetPlayerPawn();
    if (!Player)
    {
        return;
    }

    if (ACharacter* Character = Cast<ACharacter>(Player))
    {
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }

    if (AController* Controller = Player->GetController())
    {
        Controller->SetControlRotation(Player->GetActorRotation());
    }
}

void ATransmitLevelDirector::BindDirectorRoomResetController()
{
    AMotionRoomResetController* Reset = FindRoomResetController();
    if (Reset)
    {
        Reset->OnPostRoomReset.AddDynamic(this, &ATransmitLevelDirector::HandleDirectorPostRoomReset);
    }
}

bool ATransmitLevelDirector::TryRequestRoomReset()
{
    AMotionRoomResetController* Reset = FindRoomResetController();
    return Reset && Reset->RequestRoomReset();
}

AMotionRoomResetController* ATransmitLevelDirector::FindRoomResetController() const
{
    TActorIterator<AMotionRoomResetController> ResetIt(GetWorld());
    return ResetIt ? *ResetIt : nullptr;
}

APawn* ATransmitLevelDirector::GetPlayerPawn() const
{
    return UGameplayStatics::GetPlayerPawn(this, 0);
}
