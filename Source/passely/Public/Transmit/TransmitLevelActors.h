#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Motion/TransmitChargerActor.h"
#include "Motion/TransmitDirectionalCarrierActor.h"
#include "Motion/TransmitMotionEndpointActor.h"

#include "TransmitLevelActors.generated.h"

class UBoxComponent;
class USceneComponent;
class AMotionRoomResetController;
class APawn;

UENUM(BlueprintType)
enum class ETransmitFlowStep : uint8
{
    TakeMotion, GiveBridge, CrossBridge, SendCarrier, ChaseCarrier,
    RecaptureCarrier, RerouteCarrier, ReachArena, CaptureDash,
    PowerRam, CaptureAgain, BreakGate, Exit, Complete, ObserveImpact
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTransmitLevelEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTransmitImpactEvent, int32, ImpactNumber);


UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitBridgeSlab : public ATransmitDirectionalCarrierActor
{
    GENERATED_BODY()

public:
    ATransmitBridgeSlab();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transmit|Slab")
    TObjectPtr<UBoxComponent> SlabCollision;
};

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitRam : public ATransmitMotionEndpointActor
{
    GENERATED_BODY()

public:
    ATransmitRam();

    bool IsImpactInProgress() const { return bInFlightImpact; }

    UPROPERTY(BlueprintAssignable, Category = "Transmit|Events")
    FTransmitLevelEvent OnArmed;

    UPROPERTY(BlueprintAssignable, Category = "Transmit|Events")
    FTransmitImpactEvent OnImpact;


    virtual void Tick(float DeltaSeconds) override;
    virtual FMotionCompatibilityResult CanReceiveMotion_Implementation(
        const FMotionState& State,
        const FMotionTransferContext& Context) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Ram")
    TObjectPtr<AActor> Gate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Ram")
    TObjectPtr<AActor> DockMarker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Ram")
    TObjectPtr<ATransmitDirectionalCarrierActor> RouteCarrier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Ram", meta = (ClampMin = "1.0"))
    float DockRadius = 160.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Ram")
    FVector FixedAxis = FVector::ForwardVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Ram", meta = (ClampMin = "1.0"))
    float MinimumMagnitude = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Ram", meta = (ClampMin = "1.0"))
    float ImpactDistance = 500.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transmit|Ram")
    bool bArmed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transmit|Ram")
    int32 Hits = 0;

protected:
    virtual void BeginPlay() override;

private:
    float ImpactApproachSeconds = 0.45f;
    float ImpactReturnSeconds = 0.6f;

    bool bInFlightImpact = false;
    bool bReturningBody = false;
    float ImpactElapsed = 0.0f;

    bool bCarrierInitialized = false;
    bool bInitialCarrierCanProvide = false;
    bool bInitialCarrierCanReceive = false;

    FVector InitialBodyRelativeLocation = FVector::ZeroVector;
    FVector ExtendedBodyRelativeLocation = FVector::ZeroVector;
    FVector InitialGateLocation = FVector::ZeroVector;
    FRotator InitialGateRotation = FRotator::ZeroRotator;
    bool bInitialGateCollisionEnabled = true;
    bool bGateInitialCached = false;

    UFUNCTION()
    void HandleRamMotionConsumed(const FMotionTransferResult& Result);

    UFUNCTION()
    void HandleRamPostRoomReset();

    void BindRamRoomResetController();
    void CacheInitialTransforms();
    void LatchArmIfReady();
    void BeginImpactAnimation();
    void ApplyGateImpact();
    void RestoreCarrierPermissions();
    void RestoreGate();
    FVector GetLocalFixedAxis() const;
    FVector GetDockCenter() const;
};

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitArenaCharger : public ATransmitChargerActor
{
    GENERATED_BODY()

public:
    ATransmitArenaCharger();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "Transmit|Arena")
    void SetEncounterActive(bool bActive);

    void RestartEncounter();

protected:
    virtual void BeginPlay() override;

private:
    FTransform HomeTransform = FTransform::Identity;
    EMotionChargerState LastFrameState = EMotionChargerState::Idle;
    bool bEncounterActive = false;
    bool bResetScheduled = false;

    UFUNCTION()
    void HandleArenaComponentHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        FVector NormalImpulse,
        const FHitResult& Hit);

    UFUNCTION()
    void HandleArenaPostRoomReset();

    void BindArenaRoomResetController();
    void TryArenaResetFromHit();
    void ReturnToHome();
    AMotionRoomResetController* FindArenaResetController() const;
};

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitLevelDirector : public AActor
{
    GENERATED_BODY()

public:
    ATransmitLevelDirector();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Flow")
    TObjectPtr<ATransmitBridgeSlab> Bridge;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Flow")
    TObjectPtr<ATransmitMotionEndpointActor> RouteSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Flow")
    TObjectPtr<AActor> RouteEntryMarker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Flow")
    TObjectPtr<AActor> CatchMarker;

    UPROPERTY(BlueprintAssignable, Category = "Transmit|Events")
    FTransmitLevelEvent OnFlowChanged;

    UPROPERTY(BlueprintAssignable, Category = "Transmit|Events")
    FTransmitLevelEvent OnLocalRetry;

    UFUNCTION(BlueprintPure, Category = "Transmit|Flow")
    ETransmitFlowStep GetFlowStep() const { return FlowStep; }

    UFUNCTION(BlueprintPure, Category = "Transmit|Flow")
    bool IsComplete() const { return bCompletionShown; }

    UFUNCTION(BlueprintCallable, Category = "Transmit|Flow")
    bool RequestLocalRetry();

    FString GetObjectiveText() const;
    FString GetHintText() const;
    FString GetChapterText() const;
    float GetRunStartSeconds() const { return RunStartSeconds; }


    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Director")
    TObjectPtr<ATransmitRam> Ram;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Director")
    TObjectPtr<ATransmitArenaCharger> Charger;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Director")
    TObjectPtr<AActor> ArenaEntryMarker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Director")
    TObjectPtr<AActor> ExitMarker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Director")
    float FallZ = -600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmit|Director", meta = (ClampMin = "1.0"))
    float ExitDistance = 250.0f;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Transmit|Director")
    TObjectPtr<USceneComponent> SceneRoot;

    ETransmitFlowStep FlowStep = ETransmitFlowStep::TakeMotion;
    int32 Checkpoint = 0;
    float StepStartedSeconds = 0.0f;
    float LastRetrySeconds = -10.0f;
    FName RouteResourceId = NAME_None;
    FTransform RouteSourceStart = FTransform::Identity;
    FTransform RouteCarrierStart = FTransform::Identity;
    bool bEntryTriggered = false;
    bool bGateBrokenHandled = false;
    bool bCompletionShown = false;
    float EncounterStartSeconds = 0.0f;
    float RunStartSeconds = 0.0f;
    float CompletedRunSeconds = 0.0f;

    UFUNCTION()
    void HandleDirectorPostRoomReset();

    // Authored pacing content uses tags; it does not add a gameplay contract.
    struct FPacingRetryActor
    {
        TWeakObjectPtr<AActor> Actor;
        FTransform InitialTransform;
    };
    TArray<FPacingRetryActor> PacingRetryActors;
    TArray<FName> PacingResourceIds;
    TMap<FName, TWeakObjectPtr<ATransmitBridgeSlab>> PacingBridges;

    bool GetPacingTutorial(FString& Chapter, FString& Objective, FString& Hint) const;
    bool RequestTransitionRetry(APawn* Player, UMotionTransferComponent* PlayerMotion);
    void UpdateFlow();
    void SetFlowStep(ETransmitFlowStep NewStep);
    void BindDirectorRoomResetController();
    bool TryRequestRoomReset();
    AMotionRoomResetController* FindRoomResetController() const;
    APawn* GetPlayerPawn() const;
};
