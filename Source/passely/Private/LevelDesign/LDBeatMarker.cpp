#include "LevelDesign/LDBeatMarker.h"

#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
FColor GetBeatTypeColor(const ELDBeatType BeatType)
{
    switch (BeatType)
    {
    case ELDBeatType::Teach:
        return FColor::Cyan;
    case ELDBeatType::Test:
        return FColor::Yellow;
    case ELDBeatType::Twist:
        return FColor::Red;
    case ELDBeatType::Payoff:
        return FColor::Green;
    case ELDBeatType::Debug:
        return FColor(128, 128, 128);
    default:
        return FColor::White;
    }
}
}

ALDBeatMarker::ALDBeatMarker()
{
    PrimaryActorTick.bCanEverTick = false;
    bIsEditorOnlyActor = true;
    SetCanBeDamaged(false);

    DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
    SetRootComponent(DefaultSceneRoot);

    Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    Billboard->SetupAttachment(DefaultSceneRoot);
    Billboard->SetHiddenInGame(true);
    Billboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Billboard->bIsEditorOnly = true;

    TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
    TextRender->SetupAttachment(DefaultSceneRoot);
    TextRender->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
    TextRender->SetHorizontalAlignment(EHTA_Center);
    TextRender->SetVerticalAlignment(EVRTA_TextBottom);
    TextRender->SetWorldSize(50.0f);
    TextRender->SetHiddenInGame(true);
    TextRender->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TextRender->bIsEditorOnly = true;

#if WITH_EDITORONLY_DATA
    static ConstructorHelpers::FObjectFinder<UTexture2D> NoteSprite(
        TEXT("/Engine/EditorResources/S_Note.S_Note"));
    if (NoteSprite.Succeeded())
    {
        Billboard->SetSprite(NoteSprite.Object);
    }
#endif
}

void ALDBeatMarker::RefreshMarker()
{
    const FString BeatTypeName =
        UEnum::GetDisplayValueAsText(BeatType).ToString().ToUpper();
    const FString MarkerText = FString::Printf(TEXT("[%s %s]"), *BeatID.ToString(), *BeatTypeName);

    TextRender->SetText(FText::FromString(MarkerText));
    TextRender->SetTextRenderColor(GetBeatTypeColor(BeatType));

    Billboard->SetVisibility(bShowInEditor);
    TextRender->SetVisibility(bShowInEditor);
}
