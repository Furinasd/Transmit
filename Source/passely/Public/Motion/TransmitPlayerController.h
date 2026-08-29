#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "TransmitPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATransmitPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion|Input")
    TArray<TObjectPtr<UInputMappingContext>> MappingContexts;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion|Input")
    TObjectPtr<UInputAction> CaptureAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion|Input")
    TObjectPtr<UInputAction> TransferAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion|Input")
    TObjectPtr<UInputAction> ResetAction;

private:
    void HandleCapture();
    void HandleTransfer();
    void HandleReset();
};
