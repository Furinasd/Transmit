#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MotionRoomResetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMotionRoomResetSignature);

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API AMotionRoomResetController : public AActor
{
    GENERATED_BODY()

public:
    AMotionRoomResetController();

    UPROPERTY(BlueprintAssignable, Category = "Motion|Reset")
    FMotionRoomResetSignature OnPreRoomReset;

    UPROPERTY(BlueprintAssignable, Category = "Motion|Reset")
    FMotionRoomResetSignature OnPostRoomReset;

    UFUNCTION(BlueprintCallable, Category = "Motion|Reset")
    bool RequestRoomReset();

    UFUNCTION(BlueprintCallable, Category = "Motion|Reset")
    bool RegisterTransientActor(AActor* TransientActor);

    UFUNCTION(BlueprintPure, Category = "Motion|Reset")
    bool IsResetInProgress() const;

    UFUNCTION(BlueprintPure, Category = "Motion|Reset")
    int32 GetRegisteredTransientCount() const;

protected:
    virtual void BeginPlay() override;

private:
    struct FParticipantSnapshot
    {
        TWeakObjectPtr<AActor> Actor;
        FTransform Transform = FTransform::Identity;
    };

    UPROPERTY(EditInstanceOnly, Category = "Motion|Reset")
    TArray<TObjectPtr<AActor>> Participants;

    UPROPERTY(EditAnywhere, Category = "Motion|Reset")
    bool bAutoDiscoverTransferableParticipants = false;

    TArray<FParticipantSnapshot> ParticipantSnapshots;
    TArray<TWeakObjectPtr<AActor>> RegisteredTransients;
    bool bResetInProgress = false;
    bool bResetPending = false;
    bool bRetryScheduled = false;
    bool bSnapshotsCaptured = false;

    void CaptureSnapshots();
    bool HasBusyParticipant() const;
    bool ExecuteReset();
    void SchedulePendingReset();
    void TryExecutePendingReset();
    void SetInteractionBlocked(bool bBlocked) const;
    void ClearRegisteredTransients();
};
