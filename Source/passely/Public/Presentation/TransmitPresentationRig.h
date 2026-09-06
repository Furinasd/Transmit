#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Motion/MotionTransferTypes.h"
#include "TransmitPresentationRig.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class USoundBase;
class UAudioComponent;
class UMotionTransferComponent;
class AMotionRoomResetController;
class ATransmitRam;
class ATransmitChargerActor;
class ATransmitLevelDirector;
class USoundAttenuation;
class UPointLightComponent;
class UArrowComponent;

/** Level-local, read-only presentation. All spatial references are authored actors, never map coordinates. */
UCLASS(BlueprintType, Blueprintable)
class PASSELY_API ATransmitPresentationRig : public AActor
{
    GENERATED_BODY()
public:
    ATransmitPresentationRig();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<ATransmitRam> Ram;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<ATransmitChargerActor> Charger;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<AActor> ExitAnchor;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<ATransmitLevelDirector> Director;
    /** Ordered authored cable corners between Dock and Ram; endpoints follow live actors. */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Presentation")
    TArray<TObjectPtr<AActor>> DockCableAnchors;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<UMaterialInterface> MotionMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<UMaterialInterface> ImpactMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TObjectPtr<UMaterialInterface> StructureMaterial;
    /** Capture, Transfer, Dock, Telegraph, Intercept, Impact1, Impact2, Complete. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    TArray<TObjectPtr<USoundBase>> Cues;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ClampMin="0", ClampMax="1"))
    float SoundVolume = 0.65f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation", meta=(ClampMin="0.1", ClampMax="2"))
    float EffectScale = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Presentation")
    bool bReplaceLegacyPresentation=true;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Presentation")
    int32 ActiveSoundCount=0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Presentation")
    int32 ActivePulseCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Presentation")
    int32 VisibleStrokeCount = 0;

    UFUNCTION(BlueprintCallable, Category="Presentation")
    void ClearPresentation();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    struct FPulse
    {
        FVector Start, End, Axis;
        float Age=0, Duration=0.5f;
        int32 Kind=0; // 0 causal packet, 1 radial pressure, 2 shrapnel
        int32 Layer=0;
        float Strength=1;
    };
    UPROPERTY(VisibleAnywhere, Category="Presentation")
    TObjectPtr<UInstancedStaticMeshComponent> MotionStrokes;
    UPROPERTY(VisibleAnywhere, Category="Presentation")
    TObjectPtr<UInstancedStaticMeshComponent> ImpactStrokes;
    UPROPERTY(VisibleAnywhere, Category="Presentation")
    TObjectPtr<UInstancedStaticMeshComponent> StructureStrokes;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UAudioComponent>> PlayingAudio;
    UPROPERTY(Transient)
    TObjectPtr<USoundAttenuation> SpatialAttenuation;
    TWeakObjectPtr<UAudioComponent> TelegraphAudio;
    TWeakObjectPtr<UMotionTransferComponent> PlayerMotion;
    TWeakObjectPtr<AMotionRoomResetController> ResetController;
    TArray<TWeakObjectPtr<UMotionTransferComponent>> Participants;
    TArray<FPulse> Pulses;
    struct FLegacyLight { TWeakObjectPtr<UPointLightComponent> Light; float Intensity; };
    struct FLegacyArrow { TWeakObjectPtr<UArrowComponent> Arrow; bool bHidden; };
    TArray<FLegacyLight> LegacyLights;
    TArray<FLegacyArrow> LegacyArrows;
    void ReplaceLegacyPresentation(AActor* Actor);
    FDelegateHandle TransactionHandle;
    int32 Used[3] = {0,0,0};
    int32 PreviouslyUsed[3] = {0,0,0};
    int32 LastHits=0;
    int32 LastChargerState=0;
    bool bLastArmed=false;
    bool bCompleted=false;
    bool bResetting=false;
    float Phase=0;
    float DockAge=-1;
    FVector GateImpactAnchor=FVector::ZeroVector;

    UFUNCTION()
    void OnPreReset();
    UFUNCTION()
    void OnPostReset();
    UFUNCTION()
    void OnArmed();
    UFUNCTION()
    void OnImpact(int32 ImpactNumber);
    UFUNCTION()
    void OnLocalRetry();
    UFUNCTION()
    void OnFlowChanged();
    void BindPlayer();
    void OnTransaction(const FMotionTransferResult& Result);
    void Cue(int32 Index, const FVector& Location);
    void AddPulse(int32 Kind, const FVector& Start, const FVector& End, const FVector& Axis, float Duration, int32 Layer, float Strength=1);
    void DrawLine(const FVector& A, const FVector& B, float Width, int32 Layer);
    void DrawRing(const FVector& Center, const FVector& Normal, float Radius, float Width, int32 Layer, float Fraction=1);
    void DrawDevice(const FVector& Center, const FVector& Axis, float Radius, int32 Layer);
    void DrawPulses(float DeltaSeconds);
    void DrawOwnership();
    void DrawHousing(const AActor* Actor);
    void DrawEncounter(float DeltaSeconds);
    void FinishStrokes();
    UInstancedStaticMeshComponent* LayerMesh(int32 Layer) const;
    FVector ParticipantAnchor(FName Id) const;
    static FVector BodyAnchor(const AActor* Actor);
    static FVector OutputAnchor(const AActor* Actor,const FVector& Direction);
};
