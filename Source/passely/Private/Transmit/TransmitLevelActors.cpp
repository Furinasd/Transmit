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
#include "Motion/MotionInteractorComponent.h"
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
    OnArmed.Broadcast();

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
    OnImpact.Broadcast(Hits);
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

void ATransmitArenaCharger::RestartEncounter()
{
    bResetScheduled = false;
    StopChargerCycle();
    Motion->RestoreInitialState(true);
    ReturnToHome();
    SetEncounterActive(true);
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
    if (!bResetScheduled || !bEncounterActive)
    {
        return;
    }
    bResetScheduled = false;
    TActorIterator<ATransmitLevelDirector> It(GetWorld());
    if (It)
    {
        It->RequestLocalRetry();
        return;
    }

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
    RunStartSeconds = GetWorld()->GetTimeSeconds();

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
        RequestLocalRetry();
        return;
    }

    if (ArenaEntryMarker
        && !bEntryTriggered
        && Ram && Ram->bArmed
        && FVector::Dist2D(PlayerLocation, ArenaEntryMarker->GetActorLocation()) < 600.0f)
    {
        bEntryTriggered = true;
        EncounterStartSeconds = GetWorld()->GetTimeSeconds();

        if (Charger)
        {
            Charger->SetEncounterActive(true);
        }
    }

    UpdateFlow();

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
        CompletedRunSeconds = GetWorld()->GetTimeSeconds() - RunStartSeconds;
        SetFlowStep(ETransmitFlowStep::Complete);

        const float Elapsed = EncounterStartSeconds > 0.0f
            ? GetWorld()->GetTimeSeconds() - EncounterStartSeconds
            : 0.0f;

        UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] L_Transmit gate broken: elapsed=%.1fs"), Elapsed);
        UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Full run including local retries: %.1fs"), CompletedRunSeconds);

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
    Checkpoint = 0;
    LastRetrySeconds = -10.0f;
    bEntryTriggered = false;
    bGateBrokenHandled = false;
    bCompletionShown = false;
    SetFlowStep(ETransmitFlowStep::TakeMotion);
    EncounterStartSeconds = 0.0f;
    RunStartSeconds = GetWorld()->GetTimeSeconds();
    CompletedRunSeconds = 0.0f;

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
    // Cache the authored transition once, before the player can move its resources.
    // Full-room reset already restores the Motion snapshots for these participants.
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (ATransmitBridgeSlab* Slab = Cast<ATransmitBridgeSlab>(*It))
        {
            for (const FName Tag : Slab->Tags)
                if (Tag.ToString().StartsWith(TEXT("Transmit.Pacing."))) PacingBridges.Add(Tag, Slab);
        }
        if (!It->ActorHasTag(TEXT("Transmit.Pacing.Transition"))) continue;
        auto* Motion = It->FindComponentByClass<UMotionTransferComponent>();
        if (!Motion) continue;
        PacingRetryActors.Add({*It, It->GetActorTransform()});
        FMotionState State;
        if (Motion->TryGetMotionState(State) && !State.SourceId.IsNone()) PacingResourceIds.AddUnique(State.SourceId);
    }
    if (RouteSource)
    {
        RouteSourceStart = RouteSource->GetActorTransform();
        FMotionState SourceState;
        if (RouteSource->Motion->TryGetMotionState(SourceState)) RouteResourceId = SourceState.SourceId;
    }
    if (Ram && Ram->RouteCarrier) RouteCarrierStart = Ram->RouteCarrier->GetActorTransform();
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

void ATransmitLevelDirector::SetFlowStep(const ETransmitFlowStep NewStep)
{
    if (FlowStep == NewStep) return;
    FlowStep = NewStep;
    StepStartedSeconds = GetWorld()->GetTimeSeconds();
    OnFlowChanged.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_FLOW] Step=%d %s"), int32(FlowStep), *GetObjectiveText());
}

void ATransmitLevelDirector::UpdateFlow()
{
    APawn* Player = GetPlayerPawn();
    if (!Player || bCompletionShown) return;
    const UMotionTransferComponent* Held = Player->FindComponentByClass<UMotionTransferComponent>();
    const bool bLoaded = Held && Held->HasMotionState();
    if (Ram && Ram->bArmed) Checkpoint = 2;
    else if (RouteEntryMarker && Player->GetActorLocation().X >= RouteEntryMarker->GetActorLocation().X)
        Checkpoint = FMath::Max(Checkpoint, 1);

    if (Ram && Ram->IsImpactInProgress()) SetFlowStep(ETransmitFlowStep::ObserveImpact);
    else if (Ram && Ram->Hits >= 2) SetFlowStep(ETransmitFlowStep::Exit);
    else if (Checkpoint == 2)
    {
        if (!bEntryTriggered) SetFlowStep(ETransmitFlowStep::ReachArena);
        else if (bLoaded) SetFlowStep(Ram->Hits == 0 ? ETransmitFlowStep::PowerRam : ETransmitFlowStep::BreakGate);
        else SetFlowStep(Ram->Hits == 0 ? ETransmitFlowStep::CaptureDash : ETransmitFlowStep::CaptureAgain);
    }
    else if (Checkpoint == 1 && Ram && Ram->RouteCarrier)
    {
        const auto* Carrier = Ram->RouteCarrier.Get();
        const bool bAtCatch = CatchMarker && FVector::Dist2D(Carrier->GetActorLocation(), CatchMarker->GetActorLocation()) < 230.0f;
        FMotionState State;
        const bool bCarrierLoaded = Carrier->Motion->TryGetMotionState(State);
        if (bCarrierLoaded && State.Direction.Y > 0.9f) SetFlowStep(ETransmitFlowStep::RerouteCarrier);
        else if (bAtCatch && bLoaded) SetFlowStep(ETransmitFlowStep::RerouteCarrier);
        else if (bAtCatch && bCarrierLoaded && FVector::Dist2D(Player->GetActorLocation(), CatchMarker->GetActorLocation()) < 850.0f) SetFlowStep(ETransmitFlowStep::RecaptureCarrier);
        else if (bCarrierLoaded) SetFlowStep(ETransmitFlowStep::ChaseCarrier);
        else SetFlowStep(ETransmitFlowStep::SendCarrier);
    }
    else if (Bridge && (Bridge->Motion->HasMotionState() || Bridge->GetActorLocation().X > 1500.0f))
        SetFlowStep(ETransmitFlowStep::CrossBridge);
    else SetFlowStep(bLoaded ? ETransmitFlowStep::GiveBridge : ETransmitFlowStep::TakeMotion);
}

bool ATransmitLevelDirector::RequestLocalRetry()
{
    APawn* Player = GetPlayerPawn();
    if (!Player || GetWorld()->GetTimeSeconds() - LastRetrySeconds < 0.5f) return false;
    auto* PlayerMotion = Player->FindComponentByClass<UMotionTransferComponent>();
    if (!PlayerMotion || PlayerMotion->IsTransactionInProgress() || PlayerMotion->IsDispatchingNotifications()) return false;
    if (Checkpoint == 0 || bCompletionShown) return TryRequestRoomReset();
    if (Checkpoint == 2 && !bEntryTriggered && !PacingRetryActors.IsEmpty())
        return RequestTransitionRetry(Player, PlayerMotion);

    // This is a level retry, not a new room snapshot: restore only this stage's
    // original resources. Completed bridge/dock and committed gate impacts remain.
    TArray<AActor*> RestoreActors;
    if (Checkpoint == 1)
    {
        if (RouteSource) RestoreActors.Add(RouteSource);
        if (Ram && Ram->RouteCarrier) RestoreActors.Add(Ram->RouteCarrier);
    }
    for (AActor* Actor : RestoreActors)
    {
        const auto* Motion = Actor->FindComponentByClass<UMotionTransferComponent>();
        if (Motion && (Motion->IsTransactionInProgress() || Motion->IsDispatchingNotifications())) return false;
    }
    // A player may backtrack and store the route resource in an earlier target.
    // Never recreate that source while another owner outside this retry still holds it.
    if (Checkpoint == 1 && !RouteResourceId.IsNone())
    {
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            if (*It == Player || RestoreActors.Contains(*It)) continue;
            const auto* Motion = It->FindComponentByClass<UMotionTransferComponent>();
            FMotionState State;
            if (Motion && Motion->TryGetMotionState(State) && State.SourceId == RouteResourceId)
                return TryRequestRoomReset();
        }
    }
    LastRetrySeconds = GetWorld()->GetTimeSeconds();
    if (auto* Interactor = Player->FindComponentByClass<UMotionInteractorComponent>()) Interactor->ClearTarget();
    PlayerMotion->RestoreInitialState(true);
    for (AActor* Actor : RestoreActors)
    {
        Actor->SetActorTransform(Actor == RouteSource ? RouteSourceStart : RouteCarrierStart, false, nullptr, ETeleportType::TeleportPhysics);
        if (auto* Motion = Actor->FindComponentByClass<UMotionTransferComponent>()) Motion->RestoreInitialState(true);
    }
    AActor* SafeMarker = Checkpoint == 1 ? RouteEntryMarker.Get() : ArenaEntryMarker.Get();
    if (SafeMarker)
    {
        Player->SetActorLocation(SafeMarker->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
        Player->SetActorRotation(SafeMarker->GetActorRotation());
        if (Player->GetController()) Player->GetController()->SetControlRotation(SafeMarker->GetActorRotation());
    }
    if (auto* Character = Cast<ACharacter>(Player)) Character->GetCharacterMovement()->StopMovementImmediately();
    if (Checkpoint == 2 && Charger)
    {
        if (Ram && Ram->Hits < 2) Charger->RestartEncounter();
        else Charger->SetEncounterActive(false);
    }
    UpdateFlow();
    OnLocalRetry.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_FLOW] LocalRetry checkpoint=%d hits=%d"), Checkpoint, Ram ? Ram->Hits : 0);
    return true;
}

bool ATransmitLevelDirector::RequestTransitionRetry(APawn* Player, UMotionTransferComponent* PlayerMotion)
{
    TArray<AActor*> RestoreActors;
    for (const FPacingRetryActor& Saved : PacingRetryActors)
    {
        AActor* Actor = Saved.Actor.Get();
        if (!Actor) return TryRequestRoomReset();
        auto* Motion = Actor->FindComponentByClass<UMotionTransferComponent>();
        if (!Motion || Motion->IsTransactionInProgress() || Motion->IsDispatchingNotifications()) return false;
        RestoreActors.Add(Actor);
    }
    // An earlier actor can legally hold a practice resource. Never duplicate it.
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (*It == Player || RestoreActors.Contains(*It)) continue;
        const auto* Motion = It->FindComponentByClass<UMotionTransferComponent>();
        FMotionState State;
        if (Motion && Motion->TryGetMotionState(State) && PacingResourceIds.Contains(State.SourceId))
            return TryRequestRoomReset();
    }
    LastRetrySeconds = GetWorld()->GetTimeSeconds();
    if (auto* Interactor = Player->FindComponentByClass<UMotionInteractorComponent>()) Interactor->ClearTarget();
    PlayerMotion->RestoreInitialState(true);
    for (const FPacingRetryActor& Saved : PacingRetryActors)
    {
        AActor* Actor = Saved.Actor.Get();
        Actor->SetActorTransform(Saved.InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
        Actor->FindComponentByClass<UMotionTransferComponent>()->RestoreInitialState(true);
    }
    Player->SetActorLocation(FVector(5350.0f, -650.0f, 100.0f), false, nullptr, ETeleportType::TeleportPhysics);
    const FRotator Facing(0.0f, -90.0f, 0.0f);
    Player->SetActorRotation(Facing);
    if (Player->GetController()) Player->GetController()->SetControlRotation(Facing);
    if (auto* Character = Cast<ACharacter>(Player)) Character->GetCharacterMovement()->StopMovementImmediately();
    UpdateFlow();
    OnLocalRetry.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_FLOW] TransitionRetry preserved dock and ram; restored=%d"), RestoreActors.Num());
    return true;
}

bool ATransmitLevelDirector::GetPacingTutorial(FString& Chapter, FString& Objective, FString& Hint) const
{
    const APawn* Player = GetPlayerPawn();
    if (!Player || bCompletionShown) return false;
    const FVector Position = Player->GetActorLocation();
    const auto SlabFor = [this](const FName Tag) -> ATransmitBridgeSlab*
    {
        const auto* Found = PacingBridges.Find(Tag);
        return Found ? Found->Get() : nullptr;
    };
    const auto* Held = Player->FindComponentByClass<UMotionTransferComponent>();
    const bool bLoaded = Held && Held->HasMotionState();
    if (Position.X < -2500.0f && SlabFor(TEXT("Transmit.Pacing.LearnA")))
    {
        Chapter = TEXT("01 / LEARN - PRACTICE");
        if (Position.X < -5500.0f)
        {
            const auto* Slab = SlabFor(TEXT("Transmit.Pacing.LearnA"));
            Objective = Slab->Motion->HasMotionState() ? TEXT("Follow the bridge across")
                : bLoaded ? TEXT("Give the motion to the first bridge") : TEXT("Take motion from the moving source");
            Hint = TEXT("E takes motion; the source stops. Face across the gap and Q gives it to the bridge. BACKSPACE retries; R restarts everything.");
        }
        else
        {
            const auto* Slab = SlabFor(TEXT("Transmit.Pacing.LearnB"));
            Objective = Slab && Slab->Motion->HasMotionState() ? TEXT("Cross the northbound bridge, then follow the passage")
                : TEXT("Turn the next crossing north");
            Hint = TEXT("Take the nearby source. Stand south of the bridge, face north and check the direction preview before Q. Motion follows the shown direction.");
        }
        return true;
    }
    if (Ram && Ram->bArmed && !bEntryTriggered && SlabFor(TEXT("Transmit.Pacing.RouteA")))
    {
        Chapter = TEXT("02 / ROUTE - REUSE");
        const auto* First = SlabFor(TEXT("Transmit.Pacing.RouteA"));
        const auto* Second = SlabFor(TEXT("Transmit.Pacing.RouteB"));
        if (Second && Second->Motion->HasMotionState())
        {
            Objective = TEXT("Cross north and approach the impact chamber");
            Hint = TEXT("You reused one motion for two crossings. Follow the north passage. The Boss telegraphs first; capture only its committed dash.");
        }
        else if (Position.X < 7100.0f)
        {
            Objective = First->Motion->HasMotionState() ? TEXT("Cross the service bridge") : TEXT("Follow the south gallery and restore its crossing");
            Hint = TEXT("The Ram is armed. Follow the south passage to the next source; send its motion east into the service bridge.");
        }
        else
        {
            Objective = bLoaded ? TEXT("Carry the recovered motion to the northbound bridge") : TEXT("Take the motion back from the bridge you crossed");
            Hint = bLoaded ? TEXT("Walk north to the next bridge. Stand south of it, face north and Q. The preview shows the new direction.")
                : TEXT("From the far bank, aim back at the service bridge and E. Its motion can open another route; you do not need another source.");
        }
        return true;
    }
    return false;
}

FString ATransmitLevelDirector::GetChapterText() const
{
    FString Chapter, Objective, Hint;
    if (GetPacingTutorial(Chapter, Objective, Hint)) return Chapter;
    if (bCompletionShown)
    {
        const int32 Seconds = FMath::FloorToInt(CompletedRunSeconds);
        return FString::Printf(TEXT("TRANSMIT / CONNECTION RESTORED / %02d:%02d"), Seconds / 60, Seconds % 60);
    }
    return Checkpoint == 0 ? TEXT("01 / LEARN") : Checkpoint == 1 ? TEXT("02 / ROUTE") : TEXT("03 / WEAPONIZE");
}

FString ATransmitLevelDirector::GetObjectiveText() const
{
    FString Chapter, Objective, Hint;
    if (GetPacingTutorial(Chapter, Objective, Hint)) return Objective;
    switch (FlowStep)
    {
    case ETransmitFlowStep::TakeMotion: return TEXT("Restore the crossing");
    case ETransmitFlowStep::GiveBridge: return TEXT("Give the motion to the bridge");
    case ETransmitFlowStep::CrossBridge:
        return Bridge && Bridge->GetActorLocation().X > 1500.0f
            ? TEXT("Cross the bridge") : TEXT("Move the bridge into the gap");
    case ETransmitFlowStep::SendCarrier: return TEXT("Send motion through the low passage");
    case ETransmitFlowStep::ChaseCarrier: return TEXT("Follow your motion to the relay");
    case ETransmitFlowStep::RecaptureCarrier: return TEXT("Take the motion back");
    case ETransmitFlowStep::RerouteCarrier:
        return Ram && Ram->RouteCarrier && Ram->RouteCarrier->Motion->HasMotionState()
            ? TEXT("Deliver the relay to the dock") : TEXT("Turn the relay toward the dock");
    case ETransmitFlowStep::ReachArena: return TEXT("Ram online. Reach the impact chamber");
    case ETransmitFlowStep::CaptureDash: return TEXT("Intercept a committed charge");
    case ETransmitFlowStep::PowerRam: return TEXT("Deliver the captured charge to the Ram");
    case ETransmitFlowStep::CaptureAgain: return TEXT("Gate fractured. Capture one more charge");
    case ETransmitFlowStep::BreakGate: return TEXT("Break through with the final impact");
    case ETransmitFlowStep::ObserveImpact:
        return Ram && Ram->Hits >= 2 ? TEXT("Gate released")
            : Ram && Ram->Hits == 1 ? TEXT("Gate fractured") : TEXT("Ram charged. Watch the gate");
    case ETransmitFlowStep::Exit: return TEXT("Transmission restored. Walk through");
    case ETransmitFlowStep::Complete: return TEXT("You moved motion. The way is open.");
    }
    return FString();
}

FString ATransmitLevelDirector::GetHintText() const
{
    FString Chapter, Objective, Hint;
    if (GetPacingTutorial(Chapter, Objective, Hint)) return Hint;
    if (GetWorld()->GetTimeSeconds() - LastRetrySeconds < 3.0f) return TEXT("Recovered here. Your completed work is safe.");
    switch (FlowStep)
    {
    case ETransmitFlowStep::TakeMotion: return TEXT("Aim at the moving source. E to capture.");
    case ETransmitFlowStep::GiveBridge: return TEXT("You are carrying it. Face across the gap; aim at the bridge and press Q.");
    case ETransmitFlowStep::CrossBridge:
        if (Bridge && (FMath::Abs(Bridge->GetActorLocation().Y) > 300.0f || Bridge->GetActorLocation().Z > 150.0f))
            return TEXT("Off course. BACKSPACE restores the crossing; face across the gap before sending.");
        return TEXT("The source stopped. The bridge now carries its motion.");
    case ETransmitFlowStep::SendCarrier: return TEXT("Take the nearby source with E. Face down the passage; Q to send the carrier.");
    case ETransmitFlowStep::ChaseCarrier: return TEXT("Motion takes the low route. You take the outer gallery.");
    case ETransmitFlowStep::RecaptureCarrier: return TEXT("Stand at the relay's south side. Aim at the carrier; E to capture again.");
    case ETransmitFlowStep::RerouteCarrier:
        if (Ram && Ram->RouteCarrier && Ram->RouteCarrier->Motion->HasMotionState())
            return Ram->RouteCarrier->IsMovementActive()
                ? TEXT("The relay carries the motion. Watch it connect to the Ram.")
                : TEXT("Stopped short? Take it back with E, or BACKSPACE to retry this area.");
        return TEXT("Face the dock across the relay. The preview shows the new direction. Q to send.");
    case ETransmitFlowStep::ReachArena: return TEXT("The delivered carrier armed the Ram. Follow the connected line.");
    case ETransmitFlowStep::CaptureDash: return TEXT("Wait for the dash, then E. Its direction stays locked.");
    case ETransmitFlowStep::PowerRam: case ETransmitFlowStep::BreakGate:
        return TEXT("Circle to the marked rear of the Ram. Aim at it and press Q; watch the gate ahead.");
    case ETransmitFlowStep::ObserveImpact:
        return TEXT("The captured charge is driving the Ram. Follow the impact along its rail.");
    case ETransmitFlowStep::CaptureAgain: return TEXT("The first hit held. Take another dash to finish the gate.");
    case ETransmitFlowStep::Exit: return TEXT("The threat is over. Follow the open passage.");
    case ETransmitFlowStep::Complete: return TEXT("E / Capture    Q / Transfer    R / Play again");
    }
    return FString();
}
