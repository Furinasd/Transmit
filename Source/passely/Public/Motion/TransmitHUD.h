#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "TransmitHUD.generated.h"

UCLASS(ClassGroup = (Motion))
class PASSELY_API ATransmitHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    void DrawCrosshair(const FLinearColor& Color);
    void DrawInvalidTargetMarker(const FLinearColor& Color);
};
