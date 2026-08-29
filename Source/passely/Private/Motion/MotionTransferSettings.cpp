#include "Motion/MotionTransferSettings.h"

#include "Motion/MotionGameplayTags.h"

#define LOCTEXT_NAMESPACE "MotionTransferSettings"

UMotionTransferSettings::UMotionTransferSettings()
{
    MagnitudeBands = {
        {MotionGameplayTags::Motion_Tier_Weak, 0.0f, 400.0f},
        {MotionGameplayTags::Motion_Tier_Medium, 400.0f, 800.0f},
        {MotionGameplayTags::Motion_Tier_Strong, 800.0f, MAX_flt}
    };
}

FGameplayTag UMotionTransferSettings::ResolveMagnitudeTier(const float Magnitude) const
{
    return ResolveMagnitudeTierFromBands(Magnitude, MagnitudeBands);
}

FGameplayTag UMotionTransferSettings::ResolveMagnitudeTierFromBands(
    const float Magnitude,
    const TArray<FMotionMagnitudeBand>& Bands)
{
    if (!FMath::IsFinite(Magnitude) || Magnitude < 0.0f)
    {
        return FGameplayTag();
    }

    for (const FMotionMagnitudeBand& Band : Bands)
    {
        if (Magnitude >= Band.MinInclusive && Magnitude < Band.MaxExclusive)
        {
            return Band.TierTag;
        }
    }

    return FGameplayTag();
}

bool UMotionTransferSettings::ValidateMagnitudeBands(
    const TArray<FMotionMagnitudeBand>& Bands,
    FText& OutError)
{
    if (Bands.IsEmpty())
    {
        OutError = LOCTEXT("NoBands", "At least one magnitude band is required.");
        return false;
    }

    TSet<FGameplayTag> SeenTags;
    float ExpectedMin = 0.0f;

    for (int32 Index = 0; Index < Bands.Num(); ++Index)
    {
        const FMotionMagnitudeBand& Band = Bands[Index];
        if (!Band.TierTag.IsValid())
        {
            OutError = FText::Format(
                LOCTEXT("InvalidTag", "Magnitude band {0} has no valid Gameplay Tag."),
                FText::AsNumber(Index));
            return false;
        }

        if (SeenTags.Contains(Band.TierTag))
        {
            OutError = FText::Format(
                LOCTEXT("DuplicateTag", "Magnitude tag {0} is used more than once."),
                FText::FromName(Band.TierTag.GetTagName()));
            return false;
        }

        if (!FMath::IsNearlyEqual(Band.MinInclusive, ExpectedMin))
        {
            OutError = FText::Format(
                LOCTEXT("GapOrOverlap", "Magnitude band {0} must begin at {1}."),
                FText::AsNumber(Index),
                FText::AsNumber(ExpectedMin));
            return false;
        }

        if (!FMath::IsFinite(Band.MinInclusive)
            || !FMath::IsFinite(Band.MaxExclusive)
            || Band.MaxExclusive <= Band.MinInclusive)
        {
            OutError = FText::Format(
                LOCTEXT("InvalidRange", "Magnitude band {0} has an invalid range."),
                FText::AsNumber(Index));
            return false;
        }

        SeenTags.Add(Band.TierTag);
        ExpectedMin = Band.MaxExclusive;
    }

    OutError = FText::GetEmpty();
    return true;
}

FName UMotionTransferSettings::GetCategoryName() const
{
    return TEXT("Game");
}

#undef LOCTEXT_NAMESPACE
