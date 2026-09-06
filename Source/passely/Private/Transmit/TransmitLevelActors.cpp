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
    RailCenter = GetActorLocation();
    RailPhase = RailHalfSpan;
    Body->SetVisibility(false);
    SetActorEnableCollision(false);

    Motion->OnMotionConsumed.AddDynamic(this, &ATransmitRam::HandleRamMotionConsumed);

    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &ATransmitRam::BindRamRoomResetController));
}

void ATransmitRam::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    LatchArmIfReady();
    MotionIndicator->SetVisibility(false);
    DirectionIndicator->SetVisibility(false);
    if (!bArmed || !RouteCarrier) return;

    // The delivered actor remains the physical device and interaction target.
    if (bDockTransit)
    {
        DockTransitElapsed += DeltaSeconds;
        const float T = FMath::Clamp(DockTransitElapsed / 4.0f, 0.0f, 1.0f);
        const FVector P = FMath::Lerp(DockTransitStart, RailCenter, T)
            + FVector(0, 0, FMath::Sin(T * PI) * 1200.0f);
        RouteCarrier->SetActorLocation(P, false);
        if (T >= 1) bDockTransit = false;
        return;
    }
    if (!bInFlightImpact)
    {
        if (Hits >= 2) return;
        const float Span = FMath::Max(50.0f, RailHalfSpan);
        RailPhase = FMath::Fmod(RailPhase + DeltaSeconds * RailSpeed, 4 * Span);
        const float Offset = RailPhase <= 2 * Span ? RailPhase - Span : 3 * Span - RailPhase;
        const FVector Side = FVector::CrossProduct(FVector::UpVector, FixedAxis).GetSafeNormal();
        RouteCarrier->SetActorLocation(RailCenter + Side * Offset, false);
        return;
    }

    ImpactElapsed += DeltaSeconds;
    if (!bReturningBody)
    {
        const float T = FMath::Clamp(ImpactElapsed / ImpactApproachSeconds, 0.0f, 1.0f);
        const FVector Destination = StrokeStart + FixedAxis.GetSafeNormal() * ImpactDistance * T;
        FHitResult Hit;
        RouteCarrier->SetActorLocation(Destination, true, &Hit);
        if (Hit.bBlockingHit || T >= 1)
        {
            StrikePosition = RouteCarrier->GetActorLocation();
            ResolveStrike();
            bReturningBody = true;
            ImpactElapsed = 0;
        }
    }
    else
    {
        const float T = FMath::Clamp(ImpactElapsed / ImpactReturnSeconds, 0.0f, 1.0f);
        RouteCarrier->SetActorLocation(FMath::Lerp(StrikePosition, StrokeStart, T), false);
        if (T >= 1) { bInFlightImpact = false; bReturningBody = false; }
    }
}

void ATransmitRam::ResolveStrike()
{
    ++StrikeSerial;
    bStrikeHitBoss = false;
    for (TActorIterator<ATransmitArenaCharger> It(GetWorld()); It; ++It)
    {
        const FVector Delta = It->GetActorLocation() - StrikePosition;
        if (Delta.SizeSquared2D() <= FMath::Square(ImpactRadius)
            && FMath::Abs(Delta.Z) < 180.0f)
        {
            bStrikeHitBoss = true;
            It->ReceiveRailImpact();
        }
    }
    // Damage is a circular ground-plane impact. Animation time alone never scores.
    if (Gate && bStrikeHitBoss)
    {
        FVector Center, Extent;
        Gate->GetActorBounds(true, Center, Extent);
        const FVector Closest(FMath::Clamp(StrikePosition.X, Center.X-Extent.X, Center.X+Extent.X),
            FMath::Clamp(StrikePosition.Y, Center.Y-Extent.Y, Center.Y+Extent.Y), StrikePosition.Z);
        if (FVector::DistSquared2D(Closest, StrikePosition) <= FMath::Square(ImpactRadius)) ApplyGateImpact();
    }
    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_ARENA] Strike=%d boss=%d hits=%d center=%s"),
        StrikeSerial, bStrikeHitBoss, Hits, *StrikePosition.ToString());
}

void ATransmitRam::CancelStroke()
{
    if (bInFlightImpact && RouteCarrier) RouteCarrier->SetActorLocation(StrokeStart, false);
    bInFlightImpact = false;
    bReturningBody = false;
    ImpactElapsed = 0;
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

    if (bDockTransit || FixedAxis.IsNearlyZero())
        return FMotionCompatibilityResult::Reject(EMotionTransferRejection::TimingRejected);

    // This rail device converts captured energy into its authored forward stroke.
    // It does not alter PreserveSource for any other receiver.
    return FMotionCompatibilityResult::Allow();
}

void ATransmitRam::HandleRamMotionConsumed(const FMotionTransferResult& Result)
{
    if (!bArmed || !Result.bSucceeded || !Result.bConsumed)
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
    bDockTransit = false;
    RailPhase = RailHalfSpan;
    StrikeSerial = 0;
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
            InitialCarrierEndpoint = CarrierMotion->EndpointMode;
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

    // Atomically consume the ordinary routing state before changing the endpoint.
    const FMotionTransferResult DockResult = CarrierMotion->TryTransferToComponent(Motion);
    if (!DockResult.bSucceeded) return;
    bArmed = true;
    CarrierMotion->bCanProvideMotion = false;
    CarrierMotion->bCanReceiveMotion = true;
    CarrierMotion->EndpointMode = EMotionEndpointMode::ConsumeOnReceive;
    CarrierMotion->OnMotionConsumed.AddUniqueDynamic(this, &ATransmitRam::HandleRamMotionConsumed);
    RouteCarrier->SetRailController(this);
    DockTransitStart = RouteCarrier->GetActorLocation();
    DockTransitElapsed = 0;
    bDockTransit = true;
    OnArmed.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_PLAYABLE] Ram armed from carrier %s"), *GetNameSafe(RouteCarrier));
}

void ATransmitRam::BeginImpactAnimation()
{
    bInFlightImpact = true;
    bReturningBody = false;
    ImpactElapsed = 0.0f;

    StrokeStart = RouteCarrier ? RouteCarrier->GetActorLocation() : GetActorLocation();
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
        CarrierMotion->EndpointMode = InitialCarrierEndpoint;
        CarrierMotion->OnMotionConsumed.RemoveDynamic(this, &ATransmitRam::HandleRamMotionConsumed);
        RouteCarrier->SetRailController(nullptr);
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
    if (!StateMachine) return;
    const auto Before = StateMachine->GetState();
    // Track during idle only; telegraph and dash share one committed aim vector.
    if (bEncounterActive && Before == EMotionChargerState::Idle)
    {
        if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            const auto* Held = Player->FindComponentByClass<UMotionTransferComponent>();
            FMotionState HeldState;
            // One dash resource: do not regenerate it while the player still owns it.
            if (Held && Held->TryGetMotionState(HeldState) && HeldState.SourceId == DashSourceId) return;
            const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
            if (!ToPlayer.IsNearlyZero()) DashDirection = ToPlayer.GetSafeNormal2D();
        }
    }
    Super::Tick(DeltaSeconds);
    const auto Current = StateMachine->GetState();
    if (bEncounterActive && Current == EMotionChargerState::Recovery)
    {
        if (!bReturningHome)
        {
            bReturningHome = true;
            // A missed dash was never captured; retire it so the next commitment
            // grants the newly aimed vector instead of reusing stale ownership.
            if (Motion->HasMotionState()) Motion->RestoreInitialState(true);
            RecoveryStart = GetActorLocation();
            ReturnElapsed = 0;
        }
        ReturnElapsed += DeltaSeconds;
        const float T = FMath::Clamp((ReturnElapsed - 0.35f) / 0.8f, 0.0f, 1.0f);
        // Unobstructed recovery guarantees the gate-front anchor even after a miss,
        // capture, player collision, or scenery collision. The remaining recovery is a punish window.
        SetActorLocation(FMath::Lerp(RecoveryStart, HomeTransform.GetLocation(), FMath::SmoothStep(0.0f, 1.0f, T)), false);
    }
    if (bEncounterActive && Current == EMotionChargerState::Idle && LastFrameState == EMotionChargerState::Recovery)
    {
        ReturnToHome();
        ++CompletedReturns;
        UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_ARENA] Return=%d home=%s"), CompletedReturns, *GetActorLocation().ToString());
    }
    LastFrameState = Current;
}

void ATransmitArenaCharger::ReceiveRailImpact()
{
    if (StateMachine) StateMachine->ForceRecovery();
    Body->SetRelativeScale3D(FVector(1.5f, 1.5f, 0.8f));
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
        ReturnToHome();
    }

    LastFrameState = StateMachine ? StateMachine->GetState() : EMotionChargerState::Idle;
}

void ATransmitArenaCharger::RestartEncounter()
{
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

    if (auto* Player = Cast<ACharacter>(OtherActor))
    {
        Player->LaunchCharacter(DashDirection * 420.0f + FVector(0,0,180), true, true);
    }
    StateMachine->ForceRecovery();
    UE_LOG(LogTemp, Log, TEXT("[TRANSMIT_ARENA] Player contact -> guaranteed return"));
}

void ATransmitArenaCharger::HandleArenaPostRoomReset()
{
    bEncounterActive = false;
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

void ATransmitArenaCharger::ReturnToHome()
{
    bReturningHome = false;
    ReturnElapsed = 0;
    Body->SetRelativeScale3D(FVector(1.25f));
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
    const float Now = GetWorld()->GetTimeSeconds();
    const auto Say = [this, Now](const FString& Line) { Narrative = Line; NarrativeUntil = Now + 5.0f; };
    if (!(NarrativeFlags & 1) && Now - RunStartSeconds > 1.5f)
    { NarrativeFlags |= 1; Say(TEXT("板上的元件都有型号，修板的人却只叫临时工。")); }
    if (Ram && Ram->bArmed && !(NarrativeFlags & 2))
    { NarrativeFlags |= 2; Say(TEXT("他们都说自己领先。他只好先把路接上。")); }
    if (bEntryTriggered && !(NarrativeFlags & 4))
    { NarrativeFlags |= 4; Say(TEXT("户晨风：板修好了，人还没验。")); }
    if (Ram && Ram->Hits == 1 && Charger && Charger->StateMachine->GetState() == EMotionChargerState::Recovery && !(NarrativeFlags & 8))
    { NarrativeFlags |= 8; Say(TEXT("户晨风：用上苹果级动力，也不等于你就是苹果人。")); }
    if (Ram && Ram->Hits >= 2 && !(NarrativeFlags & 16))
    { NarrativeFlags |= 16; Say(TEXT("门上只写了检修通行。他替门加了出身。")); }

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


    }
}

void ATransmitLevelDirector::HandleDirectorPostRoomReset()
{
    NarrativeFlags = 0; Narrative.Reset(); NarrativeUntil = 0;
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
    if (Checkpoint == 2 && Ram) Ram->CancelStroke();
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
        Chapter = TEXT("01 / 主板装配");
        if (Position.Y > 2600.0f)
        {
            Objective = TEXT("登上检修回廊");
            Hint = TEXT("沿白色标记上坡。回望接通的桥，再下到另一块主板。");
        }
        else if (Position.X < -6100.0f)
        {
            const auto* Slab = SlabFor(TEXT("Transmit.Pacing.LearnA"));
            Objective = Slab->Motion->HasMotionState() ? (Slab->IsMovementActive() ? TEXT("桥正在移动，留在岸上") : TEXT("沿桥通过"))
                : bLoaded ? TEXT("把能量交给第一座桥") : TEXT("从运动的物体取出能量");
            Hint = bLoaded ? TEXT("留在岸上，面向缺口，按 Q 传递。箭头显示运动方向。")
                : Slab->Motion->HasMotionState() ? TEXT("能量源停了，桥接过了它的运动。桥停稳后再通过。")
                : TEXT("玄武能量：E 取出并停止运动，Q 交出并驱动物体。一次只能携带一份。");
        }
        else
        {
            const auto* Slab = SlabFor(TEXT("Transmit.Pacing.LearnB"));
            Objective = Slab && Slab->Motion->HasMotionState() ? (Slab->IsMovementActive() ? TEXT("桥正在移动，留在岸上") : TEXT("前往检修回廊"))
                : TEXT("接通向北的桥");
            Hint = Slab && Slab->Motion->HasMotionState()
                ? TEXT("桥停稳后通过，在平台右转登上回廊。")
                : TEXT("E 取出附近能量。站在桥南侧，面向北方按 Q；先确认箭头。");
        }
        return true;
    }
    if (Ram && Ram->bArmed && !bEntryTriggered && SlabFor(TEXT("Transmit.Pacing.RouteA")))
    {
        Chapter = TEXT("02 / 上层检修台");
        const auto* First = SlabFor(TEXT("Transmit.Pacing.RouteA"));
        const auto* Second = SlabFor(TEXT("Transmit.Pacing.RouteB"));
        if (Second && Second->Motion->HasMotionState())
        {
            Objective = Second->IsMovementActive() ? TEXT("桥正在移动，留在岸上") : TEXT("下行至上层接口");
            Hint = TEXT("同一份能量接通了两座桥。沿坡道下行，前往接口检查站。");
        }
        else if (Position.X < 6900.0f)
        {
            Objective = First->Motion->HasMotionState() ? (First->IsMovementActive() ? TEXT("桥正在移动，留在岸上") : TEXT("通过检修桥"))
                : bLoaded ? TEXT("从西岸把能量交给桥") : TEXT("登上检修台");
            Hint = First->Motion->HasMotionState()
                ? TEXT("桥停后向东通过，再回头按 E 取回同一份能量。踩上移动的桥也会令它停止。")
                : bLoaded ? TEXT("留在西岸，面向东方按 Q。需要提前停止时，瞄准移动的桥再按 E。")
                : TEXT("沿白色标记登上南侧坡道。上层的两座桥共用一份能量。");
        }
        else
        {
            Objective = bLoaded ? TEXT("接通第二处检修缺口") : TEXT("回头，取回你的能量");
            Hint = bLoaded ? TEXT("带着能量向北走。在岸上面向第二座桥按 Q。")
                : TEXT("回头瞄准刚通过的桥，按 E。下一座桥没有新的能量源。");
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
        return FString::Printf(TEXT("TRANSMIT / 检修完成 / %02d:%02d"), Seconds / 60, Seconds % 60);
    }
    return Checkpoint == 0 ? TEXT("01 / 主板装配") : Checkpoint == 1 ? TEXT("02 / 总线转接") : TEXT("03 / 户晨风的检查站");
}

FString ATransmitLevelDirector::GetObjectiveText() const
{
    FString Chapter, Objective, Hint;
    if (GetPacingTutorial(Chapter, Objective, Hint)) return Objective;
    switch (FlowStep)
    {
    case ETransmitFlowStep::TakeMotion: return TEXT("恢复通路");
    case ETransmitFlowStep::GiveBridge: return TEXT("把能量交给桥");
    case ETransmitFlowStep::CrossBridge:
        return Bridge && Bridge->GetActorLocation().X > 1500.0f
            ? TEXT("通过连接桥") : TEXT("将桥移入缺口");
    case ETransmitFlowStep::SendCarrier: return TEXT("把载体送入低矮通道");
    case ETransmitFlowStep::ChaseCarrier: return TEXT("沿检修廊追上 C-01");
    case ETransmitFlowStep::RecaptureCarrier: return TEXT("再次取回能量");
    case ETransmitFlowStep::RerouteCarrier:
        return Ram && Ram->RouteCarrier && Ram->RouteCarrier->Motion->HasMotionState()
            ? TEXT("把载体送入接口") : TEXT("让载体转向接口");
    case ETransmitFlowStep::ReachArena: return TEXT("前往上层接口检查站");
    case ETransmitFlowStep::CaptureDash: return TEXT("截取户晨风的冲刺");
    case ETransmitFlowStep::PowerRam: return TEXT("给往返载体注入冲刺能量");
    case ETransmitFlowStep::CaptureAgain: return TEXT("门已开裂，再截取一次冲刺");
    case ETransmitFlowStep::BreakGate: return TEXT("在门前完成最后一次对撞");
    case ETransmitFlowStep::ObserveImpact:
        return Ram && Ram->Hits >= 2 ? TEXT("通道已释放")
            : Ram && Ram->Hits == 1 ? TEXT("门已开裂") : TEXT("载体突进中");
    case ETransmitFlowStep::Exit: return TEXT("连接恢复，继续前进");
    case ETransmitFlowStep::Complete: return TEXT("工单完成");
    }
    return FString();
}

FString ATransmitLevelDirector::GetHintText() const
{
    FString Chapter, Objective, Hint;
    if (GetPacingTutorial(Chapter, Objective, Hint)) return Hint;
    if (GetWorld()->GetTimeSeconds() - LastRetrySeconds < 3.0f) return TEXT("已在本区恢复，完成的进度已保留。");
    switch (FlowStep)
    {
    case ETransmitFlowStep::TakeMotion: return TEXT("瞄准运动的能量源，按 E 取出。");
    case ETransmitFlowStep::GiveBridge: return TEXT("你已携带能量。面向缺口，瞄准桥按 Q。");
    case ETransmitFlowStep::CrossBridge:
        if (Bridge && (FMath::Abs(Bridge->GetActorLocation().Y) > 300.0f || Bridge->GetActorLocation().Z > 150.0f))
            return TEXT("方向偏了。退格键恢复本区；传递前先面向缺口。");
        return TEXT("能量源已停止，桥正在延续它的运动。");
    case ETransmitFlowStep::SendCarrier: return TEXT("E 取出附近能量。面向通道，按 Q 驱动载体。");
    case ETransmitFlowStep::ChaseCarrier: return TEXT("让运动走低处，你走外侧检修廊。");
    case ETransmitFlowStep::RecaptureCarrier: return TEXT("站到载体南侧，瞄准它按 E，随时截停运动。");
    case ETransmitFlowStep::RerouteCarrier:
        if (Ram && Ram->RouteCarrier && Ram->RouteCarrier->Motion->HasMotionState())
            return Ram->RouteCarrier->IsMovementActive()
                ? TEXT("载体正在驶入接口，随后会送上竞技场轨道。")
                : TEXT("中途停止了？按 E 取回再传递，或用退格键重试本区。");
        return TEXT("隔着载体面向接口，确认方向预览后按 Q。");
    case ETransmitFlowStep::ReachArena: return TEXT("你送来的 C-01 已接入往返轨道。沿连线前往检查站。");
    case ETransmitFlowStep::CaptureDash: return TEXT("他会向你冲刺。红色预告锁定后侧移，在冲刺中瞄准他按 E。");
    case ETransmitFlowStep::PowerRam: case ETransmitFlowStep::BreakGate:
        return TEXT("等他回到门前；载体与他对齐时，瞄准往返的 C-01 按 Q。它只向门前突进。");
    case ETransmitFlowStep::ObserveImpact:
        return TEXT("圆形冲击范围必须覆盖户晨风和门。落空不会造成门的损伤。");
    case ETransmitFlowStep::CaptureAgain: return TEXT("第一次对撞已生效。再取出一次冲刺，在他回位后完成对撞。");
    case ETransmitFlowStep::Exit: return TEXT("检修通道已开放。前往上层接口。");
    case ETransmitFlowStep::Complete: return TEXT("连接已恢复。操作员：临时工。按 R 开始下一张工单。");
    }
    return FString();
}

FString ATransmitLevelDirector::GetNarrativeText() const
{
    return GetWorld()->GetTimeSeconds() < NarrativeUntil ? Narrative : FString();
}
