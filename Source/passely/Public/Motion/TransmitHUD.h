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
    UPROPERTY(Transient)
    TObjectPtr<class UFont> RuntimeFont;
    TArray<FString> ObjectiveLines;
    TArray<FString> HintLines;
    float LayoutScale = -1;
    float LayoutWidth = -1;
    FString LastObjective;
    FString LastHint;
    float GuidanceChangedSeconds = 0.0f;
    float LastRunStartSeconds = -1.0f;

    void DrawCrosshair(const FLinearColor& Color);
    void DrawCueLine(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color);
    void DrawTargetBrackets(const AActor* Target, const FLinearColor& Color);
};
