#include "Motion/TransmitPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionRoomResetController.h"
#include "Transmit/TransmitLevelActors.h"
#include "InputCoreTypes.h"

ATransmitPlayerController::ATransmitPlayerController()
{
    bShowMouseCursor = false;
}

void ATransmitPlayerController::BeginPlay()
{
    Super::BeginPlay();

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetLocalPlayer()
        ? GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
        : nullptr;
    if (!InputSubsystem)
    {
        return;
    }

    for (UInputMappingContext* MappingContext : MappingContexts)
    {
        if (MappingContext)
        {
            InputSubsystem->AddMappingContext(MappingContext, 0);
        }
    }
}

void ATransmitPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInput)
    {
        return;
    }

    InputComponent->BindKey(EKeys::BackSpace, IE_Pressed, this, &ATransmitPlayerController::HandleRetry);

    if (CaptureAction)
    {
        EnhancedInput->BindAction(
            CaptureAction,
            ETriggerEvent::Started,
            this,
            &ATransmitPlayerController::HandleCapture);
    }
    if (TransferAction)
    {
        EnhancedInput->BindAction(
            TransferAction,
            ETriggerEvent::Started,
            this,
            &ATransmitPlayerController::HandleTransfer);
    }
    if (ResetAction)
    {
        EnhancedInput->BindAction(
            ResetAction,
            ETriggerEvent::Started,
            this,
            &ATransmitPlayerController::HandleReset);
    }
}

void ATransmitPlayerController::HandleCapture()
{
    APawn* ControlledPawn = GetPawn();
    if (UMotionInteractorComponent* Interactor = ControlledPawn
        ? ControlledPawn->FindComponentByClass<UMotionInteractorComponent>()
        : nullptr)
    {
        Interactor->RequestCapture();
    }
}

void ATransmitPlayerController::HandleTransfer()
{
    APawn* ControlledPawn = GetPawn();
    if (UMotionInteractorComponent* Interactor = ControlledPawn
        ? ControlledPawn->FindComponentByClass<UMotionInteractorComponent>()
        : nullptr)
    {
        Interactor->RequestTransfer();
    }
}

void ATransmitPlayerController::HandleReset()
{
    TActorIterator<AMotionRoomResetController> ResetIt(GetWorld());
    if (ResetIt)
    {
        ResetIt->RequestRoomReset();
    }
}

void ATransmitPlayerController::HandleRetry()
{
    TActorIterator<ATransmitLevelDirector> Director(GetWorld());
    if (Director) Director->RequestLocalRetry();
}
