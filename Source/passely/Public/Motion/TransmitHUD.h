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
    void DrawCueLine(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color);
    void DrawTargetBrackets(const AActor* Target, const FLinearColor& Color);
};
