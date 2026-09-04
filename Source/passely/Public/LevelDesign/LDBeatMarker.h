#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelDesign/LDBeatTypes.h"

#include "LDBeatMarker.generated.h"

class UBillboardComponent;
class USceneComponent;
class UTextRenderComponent;

/** Editor-only marker that documents the design intent of a level-design beat. */
UCLASS(Blueprintable, ClassGroup = (LevelDesign))
class PASSELY_API ALDBeatMarker : public AActor
{
    GENERATED_BODY()

public:
    ALDBeatMarker();

    /** Reformats the marker text and visibility from the current beat metadata. */
    UFUNCTION(BlueprintCallable, Category = "LevelDesign|Beat Marker")
    void RefreshMarker();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|Beat Marker")
    FName BeatID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|Beat Marker")
    ELDBeatType BeatType = ELDBeatType::Teach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|Beat Marker",
        meta = (MultiLine = "true"))
    FText ExpectedKnowledge;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|Beat Marker",
        meta = (MultiLine = "true"))
    FText ExpectedAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|Beat Marker",
        meta = (MultiLine = "true"))
    FText Notes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|Beat Marker")
    TObjectPtr<AActor> TargetActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelDesign|Beat Marker")
    bool bShowInEditor = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelDesign|Beat Marker")
    TObjectPtr<USceneComponent> DefaultSceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelDesign|Beat Marker")
    TObjectPtr<UBillboardComponent> Billboard;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelDesign|Beat Marker")
    TObjectPtr<UTextRenderComponent> TextRender;
};
