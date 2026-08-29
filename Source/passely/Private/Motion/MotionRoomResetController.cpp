#include "Motion/MotionRoomResetController.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/MotionTransferable.h"
#include "TimerManager.h"
#include "passely.h"

AMotionRoomResetController::AMotionRoomResetController()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMotionRoomResetController::BeginPlay()
{
    Super::BeginPlay();
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &AMotionRoomResetController::CaptureSnapshots));
}

bool AMotionRoomResetController::RequestRoomReset()
{
    if (bResetInProgress)
    {
        return false;
    }

    if (!bSnapshotsCaptured)
    {
        bResetPending = true;
        SchedulePendingReset();
        return false;
    }

    if (HasBusyParticipant())
    {
        bResetPending = true;
        SchedulePendingReset();
        return false;
    }

    return ExecuteReset();
}

bool AMotionRoomResetController::RegisterTransientActor(AActor* TransientActor)
{
    if (!IsValid(TransientActor)
        || TransientActor == this
        || Participants.Contains(TransientActor))
    {
        return false;
    }

    RegisteredTransients.RemoveAll(
        [](const TWeakObjectPtr<AActor>& Entry)
        {
            return !Entry.IsValid();
        });

    if (RegisteredTransients.Contains(TransientActor))
    {
        return true;
    }

    RegisteredTransients.Add(TransientActor);
    return true;
}

bool AMotionRoomResetController::IsResetInProgress() const
{
    return bResetInProgress;
}

int32 AMotionRoomResetController::GetRegisteredTransientCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AActor>& Entry : RegisteredTransients)
    {
        Count += Entry.IsValid() ? 1 : 0;
    }
    return Count;
}

void AMotionRoomResetController::CaptureSnapshots()
{
    ParticipantSnapshots.Reset();
    TSet<FName> ParticipantIds;

    if (bAutoDiscoverTransferableParticipants)
    {
        for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
        {
            AActor* Candidate = *ActorIt;
            if (IsValid(Candidate)
                && Candidate->GetClass()->ImplementsInterface(UMotionTransferable::StaticClass()))
            {
                Participants.AddUnique(Candidate);
            }
        }
    }

    for (AActor* Participant : Participants)
    {
        if (!IsValid(Participant))
        {
            UE_LOG(LogMotionTransfer, Warning, TEXT("Room Reset has an invalid participant entry."));
            continue;
        }

        FParticipantSnapshot Snapshot;
        Snapshot.Actor = Participant;
        Snapshot.Transform = Participant->GetActorTransform();
        ParticipantSnapshots.Add(MoveTemp(Snapshot));

        if (const UMotionTransferComponent* Motion =
            Participant->FindComponentByClass<UMotionTransferComponent>())
        {
            const FName ParticipantId = Motion->GetParticipantId();
            if (ParticipantId.IsNone())
            {
                UE_LOG(
                    LogMotionTransfer,
                    Error,
                    TEXT("Room participant %s has no stable ParticipantId."),
                    *GetNameSafe(Participant));
            }
            else if (ParticipantIds.Contains(ParticipantId))
            {
                UE_LOG(
                    LogMotionTransfer,
                    Error,
                    TEXT("Room participant id %s is duplicated."),
                    *ParticipantId.ToString());
            }
            ParticipantIds.Add(ParticipantId);
        }
    }

    bSnapshotsCaptured = true;
}

bool AMotionRoomResetController::HasBusyParticipant() const
{
    for (const FParticipantSnapshot& Snapshot : ParticipantSnapshots)
    {
        const AActor* Participant = Snapshot.Actor.Get();
        const UMotionTransferComponent* Motion = Participant
            ? Participant->FindComponentByClass<UMotionTransferComponent>()
            : nullptr;
        if (Motion
            && (Motion->IsTransactionInProgress() || Motion->IsDispatchingNotifications()))
        {
            return true;
        }
    }

    return false;
}

bool AMotionRoomResetController::ExecuteReset()
{
    if (bResetInProgress || HasBusyParticipant())
    {
        return false;
    }

    bResetInProgress = true;
    bResetPending = false;
    SetInteractionBlocked(true);
    OnPreRoomReset.Broadcast();

    bool bSucceeded = true;
    for (const FParticipantSnapshot& Snapshot : ParticipantSnapshots)
    {
        AActor* Participant = Snapshot.Actor.Get();
        if (!IsValid(Participant))
        {
            bSucceeded = false;
            continue;
        }

        Participant->SetActorTransform(
            Snapshot.Transform,
            false,
            nullptr,
            ETeleportType::TeleportPhysics);

        if (UMotionTransferComponent* Motion =
            Participant->FindComponentByClass<UMotionTransferComponent>())
        {
            bSucceeded &= Motion->RestoreInitialState(false);
        }
    }

    ClearRegisteredTransients();
    OnPostRoomReset.Broadcast();
    SetInteractionBlocked(false);
    bResetInProgress = false;

    if (bSucceeded)
    {
        UE_LOG(
            LogMotionTransfer,
            Log,
            TEXT("Room Reset completed: participants=%d, success=true"),
            ParticipantSnapshots.Num());
    }
    else
    {
        UE_LOG(
            LogMotionTransfer,
            Error,
            TEXT("Room Reset completed: participants=%d, success=false"),
            ParticipantSnapshots.Num());
    }

    return bSucceeded;
}

void AMotionRoomResetController::SchedulePendingReset()
{
    if (bRetryScheduled || !GetWorld())
    {
        return;
    }

    bRetryScheduled = true;
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &AMotionRoomResetController::TryExecutePendingReset));
}

void AMotionRoomResetController::TryExecutePendingReset()
{
    bRetryScheduled = false;
    if (!bResetPending)
    {
        return;
    }


    if (!bSnapshotsCaptured)
    {
        CaptureSnapshots();
    }

    if (HasBusyParticipant())
    {
        SchedulePendingReset();
        return;
    }

    ExecuteReset();
}

void AMotionRoomResetController::SetInteractionBlocked(const bool bBlocked) const
{
    for (const FParticipantSnapshot& Snapshot : ParticipantSnapshots)
    {
        if (AActor* Participant = Snapshot.Actor.Get())
        {
            if (UMotionInteractorComponent* Interactor =
                Participant->FindComponentByClass<UMotionInteractorComponent>())
            {
                Interactor->SetRequestsBlocked(bBlocked);
            }
        }
    }
}

void AMotionRoomResetController::ClearRegisteredTransients()
{
    for (const TWeakObjectPtr<AActor>& Entry : RegisteredTransients)
    {
        if (AActor* TransientActor = Entry.Get())
        {
            TransientActor->Destroy();
        }
    }
    RegisteredTransients.Reset();
}
