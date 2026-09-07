#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "Motion/MotionTransferTypes.h"
#include "MotionTransferComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FMotionStateChangedSignature,
    const FMotionTransferResult&,
    Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FMotionConsumedSignature,
    const FMotionTransferResult&,
    Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FMotionTransactionSignature,
    const FMotionTransferResult&,
    Result);

DECLARE_MULTICAST_DELEGATE_OneParam(
    FMotionTransactionNativeSignature,
    const FMotionTransferResult&);

#if WITH_DEV_AUTOMATION_TESTS
class FMotionGlobalNotificationFifoTest;
#endif

UCLASS(ClassGroup = (Motion), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class PASSELY_API UMotionTransferComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMotionTransferComponent();

    UPROPERTY(BlueprintAssignable, Category = "Motion|Events")
    FMotionStateChangedSignature OnMotionStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Motion|Events")
    FMotionConsumedSignature OnMotionConsumed;

    UPROPERTY(BlueprintAssignable, Category = "Motion|Events")
    FMotionTransactionSignature OnMotionTransaction;

    UFUNCTION(BlueprintPure, Category = "Motion")
    bool HasMotionState() const;

    UFUNCTION(BlueprintPure, Category = "Motion")
    bool TryGetMotionState(FMotionState& OutState) const;

    UFUNCTION(BlueprintPure, Category = "Motion")
    FGameplayTag GetMagnitudeTier() const;

    UFUNCTION(BlueprintPure, Category = "Motion")
    FName GetParticipantId() const;

    UFUNCTION(BlueprintPure, Category = "Motion")
    EMotionEndpointMode GetEndpointMode() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion|Identity")
    FName ParticipantId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion|Rules")
    bool bCanProvideMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion|Rules")
    bool bCanReceiveMotion = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion|Rules")
    EMotionEndpointMode EndpointMode = EMotionEndpointMode::Store;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion|Rules")
    EMotionCanonicalDirection RequiredCanonicalDirection = EMotionCanonicalDirection::None;

    FMotionCompatibilityResult CanProvideMotion() const;
    FMotionCompatibilityResult CanReceiveState(
        const FMotionState& State,
        const FMotionDirectionResolution* Resolution = nullptr) const;

    FMotionTransferResult TryCaptureFromActor(
        AActor* SourceActor,
        const FMotionTransferContext& Context);

    FMotionTransferResult TryTransferToActor(
        AActor* TargetActor,
        const FMotionTransferContext& Context);

    FMotionTransferResult TryCaptureFromComponent(UMotionTransferComponent* SourceComponent);
    FMotionTransferResult TryTransferToComponent(UMotionTransferComponent* TargetComponent);
    FMotionTransferResult TryTransferToComponent(
        UMotionTransferComponent* TargetComponent,
        const FMotionDirectionResolution& Resolution);
    UFUNCTION(BlueprintCallable, Category = "Motion")
    bool GrantMotionState(const FMotionState& State);
    FMotionTransferResult NotifyRejectedRequest(
        EMotionTransferVerb Verb,
        EMotionTransferRejection Rejection);

    bool RestoreInitialState(bool bBroadcastStateChange);
    bool IsTransactionInProgress() const;
    bool IsDispatchingNotifications() const;

    FMotionTransactionNativeSignature& OnMotionTransactionNative();

#if WITH_DEV_AUTOMATION_TESTS
    void ConfigureForTesting(
        FName InParticipantId,
        bool bInCanProvideMotion,
        bool bInCanReceiveMotion,
        EMotionEndpointMode InEndpointMode,
        const TOptional<FMotionState>& InInitialState);
#endif

protected:
    virtual void BeginPlay() override;

private:
    struct FMotionPendingNotification
    {
        TWeakObjectPtr<UMotionTransferComponent> Dispatcher;
        TWeakObjectPtr<UMotionTransferComponent> PreviousOwner;
        TWeakObjectPtr<UMotionTransferComponent> NewOwner;
        TWeakObjectPtr<UMotionTransferComponent> Consumer;
        TWeakObjectPtr<UMotionTransferComponent> ResetOwner;
        FMotionTransferResult Result;
    };

    UPROPERTY(EditAnywhere, Category = "Motion|Initial State")
    bool bStartsWithMotion = false;

    UPROPERTY(EditAnywhere, Category = "Motion|Initial State", meta = (EditCondition = "bStartsWithMotion"))
    FMotionState InitialMotion;

    UPROPERTY(EditAnywhere, Category = "Motion|Rules")
    FGameplayTagQuery AcceptedMagnitudeTiers;

    UPROPERTY(VisibleInstanceOnly, Category = "Motion|Runtime")
    bool bHasMotion = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Motion|Runtime")
    FMotionState CurrentMotion;

    bool bInitialHasMotion = false;
    FMotionState InitialMotionSnapshot;
    bool bTransactionInProgress = false;

    FMotionTransactionNativeSignature MotionTransactionNative;

    static TArray<FMotionPendingNotification> GlobalPendingNotifications;
    static bool bGlobalDispatchingNotifications;

#if WITH_DEV_AUTOMATION_TESTS
    friend class FMotionGlobalNotificationFifoTest;

    void HandleTestingStateChanged(const FMotionTransferResult& Result);

    TWeakObjectPtr<UMotionTransferComponent> TestingTransactionSource;
    TWeakObjectPtr<UMotionTransferComponent> TestingTransactionTarget;
#endif

    FMotionTransferResult TryMoveBetween(
        UMotionTransferComponent* SourceComponent,
        UMotionTransferComponent* TargetComponent,
        EMotionTransferVerb Verb,
        bool bRequireProviderFlag,
        const FMotionState* StateOverride = nullptr,
        const FMotionDirectionResolution* Resolution = nullptr);

    FMotionTransferResult MakeRejectedResult(
        EMotionTransferVerb Verb,
        EMotionTransferRejection Rejection) const;

    void EnqueueNotification(FMotionPendingNotification&& Notification);
    void FlushNotifications();
    void CaptureInitialSnapshot();
    void SetStateWithoutNotification(const FMotionState& State);
    void ClearStateWithoutNotification();
};
