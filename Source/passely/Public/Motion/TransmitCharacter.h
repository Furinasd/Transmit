#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Motion/MotionTransferable.h"

#include "TransmitCharacter.generated.h"

UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitCharacter : public ACharacter, public IMotionTransferable
{
    GENERATED_BODY()
};
