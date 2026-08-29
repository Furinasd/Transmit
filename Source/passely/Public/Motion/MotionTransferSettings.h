#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "MotionTransferSettings.generated.h"

USTRUCT(BlueprintType)
struct PASSELY_API FMotionMagnitudeBand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Magnitude")
    FGameplayTag TierTag;

    UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Magnitude", meta = (ClampMin = "0.0"))
    float MinInclusive = 0.0f;

    UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Magnitude", meta = (ClampMin = "0.0"))
    float MaxExclusive = 1.0f;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Motion Transfer"))
class PASSELY_API UMotionTransferSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMotionTransferSettings();

    UPROPERTY(EditAnywhere, Config, BlueprintReadOnly, Category = "Magnitude")
    TArray<FMotionMagnitudeBand> MagnitudeBands;

    UFUNCTION(BlueprintPure, Category = "Motion|Magnitude")
    FGameplayTag ResolveMagnitudeTier(float Magnitude) const;

    static FGameplayTag ResolveMagnitudeTierFromBands(
        float Magnitude,
        const TArray<FMotionMagnitudeBand>& Bands);

    static bool ValidateMagnitudeBands(
        const TArray<FMotionMagnitudeBand>& Bands,
        FText& OutError);

    virtual FName GetCategoryName() const override;
};
