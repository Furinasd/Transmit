#include "Presentation/TransmitPresentationRig.h"

#include "Components/AudioComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/PointLightComponent.h"
#include "Sound/SoundAttenuation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionDirectionIndicatorComponent.h"
#include "Motion/MotionRoomResetController.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/TransmitChargerActor.h"
#include "Motion/TransmitMotionEndpointActor.h"
#include "Transmit/TransmitLevelActors.h"
#include "UObject/ConstructorHelpers.h"

namespace { constexpr int32 PoolSize=256; }

ATransmitPresentationRig::ATransmitPresentationRig()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    MotionStrokes=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MotionStrokes"));
    ImpactStrokes=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ImpactStrokes"));
    StructureStrokes=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StructureStrokes"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (auto* Mesh : {MotionStrokes.Get(), ImpactStrokes.Get(), StructureStrokes.Get()})
    {
        Mesh->SetupAttachment(GetRootComponent());
        Mesh->SetStaticMesh(Cube.Object);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->SetCastShadow(false);
        Mesh->SetReceivesDecals(false);
        Mesh->NumCustomDataFloats=0;
    }
}

void ATransmitPresentationRig::BeginPlay()
{
    Super::BeginPlay();
    MotionStrokes->SetMaterial(0,MotionMaterial);
    ImpactStrokes->SetMaterial(0,ImpactMaterial);
    StructureStrokes->SetMaterial(0,StructureMaterial);
    for (int32 L=0;L<3;++L)
        for (int32 I=0;I<PoolSize;++I)
            LayerMesh(L)->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
    for (TActorIterator<AActor> It(GetWorld());It;++It)
        if (auto* Motion=It->FindComponentByClass<UMotionTransferComponent>()) { Participants.Add(Motion); ReplaceLegacyPresentation(*It); }
    if (TActorIterator<AMotionRoomResetController> It(GetWorld()); It)
    {
        ResetController=*It;
        It->OnPreRoomReset.AddDynamic(this,&ATransmitPresentationRig::OnPreReset);
        It->OnPostRoomReset.AddDynamic(this,&ATransmitPresentationRig::OnPostReset);
    }
    if (Ram) { LastHits=Ram->Hits; bLastArmed=Ram->bArmed; GateImpactAnchor=Ram->Gate ? OutputAnchor(Ram->Gate,-Ram->FixedAxis) : BodyAnchor(Ram);
        const FVector ImpactBody=BodyAnchor(Ram)+Ram->FixedAxis.GetSafeNormal()*Ram->ImpactDistance;
        const FVector Axis=Ram->FixedAxis.GetSafeNormal();
        GateImpactAnchor=ImpactBody+Axis*FVector::DotProduct(GateImpactAnchor-ImpactBody,Axis); }
    SpatialAttenuation=NewObject<USoundAttenuation>(this);
    SpatialAttenuation->Attenuation.bAttenuate=true;
    SpatialAttenuation->Attenuation.bSpatialize=true;
    SpatialAttenuation->Attenuation.AttenuationShapeExtents=FVector(300);
    SpatialAttenuation->Attenuation.FalloffDistance=2800;
    if (Ram)
    {
        Ram->OnArmed.AddDynamic(this,&ATransmitPresentationRig::OnArmed);
        Ram->OnImpact.AddDynamic(this,&ATransmitPresentationRig::OnImpact);
    }
    if (Director)
    {
        Director->OnLocalRetry.AddDynamic(this,&ATransmitPresentationRig::OnLocalRetry);
        Director->OnFlowChanged.AddDynamic(this,&ATransmitPresentationRig::OnFlowChanged);
    }
    DockAge=bLastArmed ? 2.0f : -1.0f;
    BindPlayer();
}

void ATransmitPresentationRig::ReplaceLegacyPresentation(AActor* Actor)
{
    if(!bReplaceLegacyPresentation || !Actor) return;
    TInlineComponentArray<UPointLightComponent*> Lights(Actor);
    for(auto* Light:Lights)
    {
        if(LegacyLights.ContainsByPredicate([Light](const auto& L){return L.Light==Light;})) continue;
        LegacyLights.Add({Light,Light->Intensity});
        Light->SetIntensity(Light->Intensity*.06f);
    }
    TInlineComponentArray<UArrowComponent*> Arrows(Actor);
    for(auto* Arrow:Arrows)
    {
        if(LegacyArrows.ContainsByPredicate([Arrow](const auto& A){return A.Arrow==Arrow;})) continue;
        LegacyArrows.Add({Arrow,Arrow->bHiddenInGame});
        Arrow->SetHiddenInGame(true,true);
    }
}

void ATransmitPresentationRig::BindPlayer()
{
    auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0);
    auto* Motion=Pawn ? Pawn->FindComponentByClass<UMotionTransferComponent>() : nullptr;
    if (PlayerMotion.Get()==Motion) return;
    if (PlayerMotion.IsValid()) PlayerMotion->OnMotionTransactionNative().Remove(TransactionHandle);
    ClearCameraFeedback();
    PlayerMotion=Motion;
    if (Motion)
    {
        Participants.AddUnique(Motion);
        ReplaceLegacyPresentation(Pawn);
        TransactionHandle=Motion->OnMotionTransactionNative().AddUObject(this,&ATransmitPresentationRig::OnTransaction);
    }
}

FVector ATransmitPresentationRig::BodyAnchor(const AActor* Actor)
{
    if (!Actor) return FVector::ZeroVector;
    if (const auto* Endpoint=Cast<ATransmitMotionEndpointActor>(Actor)) return Endpoint->Body->Bounds.Origin;
    if (const auto* Boss=Cast<ATransmitChargerActor>(Actor)) return Boss->Body->Bounds.Origin;
    if (const auto* Carrier=Cast<ATransmitDirectionalCarrierActor>(Actor)) return Carrier->Body->Bounds.Origin;
    return Actor->GetActorLocation();
}

FVector ATransmitPresentationRig::OutputAnchor(const AActor* Actor,const FVector& Direction)
{
    if(!Actor) return FVector::ZeroVector;
    const UStaticMeshComponent* Mesh=Actor->FindComponentByClass<UStaticMeshComponent>();
    if(const auto* Endpoint=Cast<ATransmitMotionEndpointActor>(Actor)) Mesh=Endpoint->Body;
    if(const auto* Carrier=Cast<ATransmitDirectionalCarrierActor>(Actor)) Mesh=Carrier->Body;
    if(Mesh && Mesh->GetStaticMesh())
        return UMotionDirectionIndicatorComponent::CalculateFaceAnchor(Mesh->GetStaticMesh()->GetBoundingBox(),Direction,12,Mesh->GetComponentTransform());
    return BodyAnchor(Actor)+Direction*80;
}

FVector ATransmitPresentationRig::ParticipantAnchor(const FName Id) const
{
    for (const auto& P : Participants)
        if (P.IsValid() && P->GetParticipantId()==Id) return BodyAnchor(P->GetOwner());
    return FVector::ZeroVector;
}

void ATransmitPresentationRig::OnTransaction(const FMotionTransferResult& R)
{
    if (bResetting || !R.bSucceeded || (R.Verb!=EMotionTransferVerb::Capture && R.Verb!=EMotionTransferVerb::Transfer)) return;
    const FVector From=ParticipantAnchor(R.FromParticipantId), To=ParticipantAnchor(R.ToParticipantId);
    const bool bHigh=R.StateSnapshot.DirectionPolicy==EMotionDirectionPolicy::PreserveSource;
    const bool bCapture=R.Verb==EMotionTransferVerb::Capture;
    const int32 Layer=bHigh ? 1 : 0;
    AddPulse(0,From,To,R.StateSnapshot.Direction,0.42f,Layer,bHigh ? 1.4f : 1);
    AddPulse(1,bCapture ? From : To,To,R.StateSnapshot.Direction,0.5f,Layer,bHigh ? 1.5f : 0.7f);
    KickCamera(bHigh ? (bCapture ? -2.4f : 1.6f) : (bCapture ? -0.9f : 1.1f), bHigh ? .38f : .24f);
    if(bCapture && bHigh)
        AddPulse(3,From,From,R.StateSnapshot.Direction,.24f,1,1.5f);
    Cue(bCapture ? (bHigh ? 4 : 0) : 1,bCapture ? To : From);
    UE_LOG(LogTemp,Log,TEXT("[TRANSMIT_PRESENTATION] %s %s -> %s high=%d"),bCapture?TEXT("Capture"):TEXT("Transfer"),*R.FromParticipantId.ToString(),*R.ToParticipantId.ToString(),bHigh);
}

void ATransmitPresentationRig::KickCamera(const float Strength,const float Duration)
{
    // A stronger impact wins over a pending ordinary transfer; never stack FOV kicks.
    if(CameraAge<CameraDuration && FMath::Abs(Strength)<FMath::Abs(CameraStrength)) return;
    CameraStrength=Strength; CameraDuration=Duration; CameraAge=0;
}

void ATransmitPresentationRig::ClearCameraFeedback()
{
    if(FeedbackCamera.IsValid() && !FMath::IsNearlyZero(AppliedFOV))
        FeedbackCamera->AddAdditiveOffset(FTransform::Identity,-AppliedFOV);
    FeedbackCamera.Reset(); AppliedFOV=0; CameraStrength=0; CameraAge=0; CameraDuration=0;
}

void ATransmitPresentationRig::UpdateCameraFeedback(const float Dt)
{
    auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0);
    UCameraComponent* Camera=nullptr;
    if(Pawn)
    {
        TInlineComponentArray<UCameraComponent*> Cameras(Pawn);
        for(auto* Candidate:Cameras) if(Candidate->IsActive()) { Camera=Candidate; break; }
    }
    if(FeedbackCamera.Get()!=Camera)
    {
        // Unwind only this rig's contribution; preserve authored FOV and other offsets.
        if(FeedbackCamera.IsValid()) FeedbackCamera->AddAdditiveOffset(FTransform::Identity,-AppliedFOV);
        FeedbackCamera=Camera; AppliedFOV=0;
    }
    CameraAge+=Dt;
    const float T=CameraDuration>0 ? FMath::Clamp(CameraAge/CameraDuration,0.0f,1.0f) : 1;
    const float Attack=FMath::Clamp(T/.12f,0.0f,1.0f);
    const float Offset=CameraStrength*Attack*FMath::Square(1-T)*FMath::Clamp(EffectScale,0.0f,1.0f);
    if(Camera) { Camera->AddAdditiveOffset(FTransform::Identity,Offset-AppliedFOV); AppliedFOV=Offset; }
}

void ATransmitPresentationRig::AddPulse(int32 Kind,const FVector& Start,const FVector& End,const FVector& Axis,float Duration,int32 Layer,float Strength)
{
    if (Pulses.Num()>=16) Pulses.RemoveAt(0);
    Pulses.Add({Start,End,Axis.GetSafeNormal(),0,Duration,Kind,Layer,Strength});
}

void ATransmitPresentationRig::Cue(const int32 Index,const FVector& Location)
{
    if (!Cues.IsValidIndex(Index) || !Cues[Index] || SoundVolume<=0) return;
    // All cues belong to this rig so reset and exit can stop exactly these sounds.
    static const float Gains[]={0.65f,0.65f,0.85f,0.42f,0.85f,0.90f,1.0f,0.35f};
    auto* Audio=Index==7
        ? UGameplayStatics::SpawnSound2D(this,Cues[Index],SoundVolume*Gains[Index])
        : UGameplayStatics::SpawnSoundAtLocation(this,Cues[Index],Location,FRotator::ZeroRotator,SoundVolume*Gains[Index],1,0,SpatialAttenuation);
    if(Audio) { PlayingAudio.Add(Audio); if(Index==3) TelegraphAudio=Audio; }
    PlayingAudio.RemoveAll([](const auto& A){return !IsValid(A) || !A->IsPlaying();});
}

UInstancedStaticMeshComponent* ATransmitPresentationRig::LayerMesh(const int32 Layer) const
{
    return Layer==0 ? MotionStrokes.Get() : Layer==1 ? ImpactStrokes.Get() : StructureStrokes.Get();
}

void ATransmitPresentationRig::DrawLine(const FVector& A,const FVector& B,const float Width,const int32 Layer)
{
    if (Used[Layer]>=PoolSize || A.Equals(B,0.01f)) return;
    const FVector D=B-A;
    const FTransform T(D.Rotation(),(A+B)*0.5f,FVector(D.Size()/100.0f,Width/100.0f,Width/100.0f));
    LayerMesh(Layer)->UpdateInstanceTransform(Used[Layer]++,T,true,false,true);
}

void ATransmitPresentationRig::DrawRing(const FVector& C,const FVector& N,const float Radius,const float Width,const int32 Layer,const float Fraction)
{
    FVector U,V; N.GetSafeNormal(SMALL_NUMBER,FVector::UpVector).FindBestAxisVectors(U,V);
    const int32 Segments=24;
    for (int32 I=0;I<Segments*FMath::Clamp(Fraction,0.0f,1.0f);++I)
    {
        const float A=2*PI*I/Segments, B=2*PI*(I+0.8f)/Segments;
        DrawLine(C+Radius*(U*FMath::Cos(A)+V*FMath::Sin(A)),C+Radius*(U*FMath::Cos(B)+V*FMath::Sin(B)),Width,Layer);
    }
}

void ATransmitPresentationRig::DrawDevice(const FVector& C,const FVector& Axis,const float Radius,const int32 Layer)
{
    FVector U,V; Axis.GetSafeNormal(SMALL_NUMBER,FVector::ForwardVector).FindBestAxisVectors(U,V);
    // Cropped square corners, without crossing strokes or a debug-gizmo silhouette.
    const float R=Radius*.7071f, Arm=Radius*.3f;
    for(float X : {-1.0f,1.0f}) for(float Y : {-1.0f,1.0f})
    {
        const FVector Corner=C+U*X*R+V*Y*R;
        DrawLine(Corner-U*X*Arm,Corner,3*EffectScale,Layer);
        DrawLine(Corner,Corner-V*Y*Arm,3*EffectScale,Layer);
    }
}

void ATransmitPresentationRig::DrawPulses(const float Dt)
{
    TArray<FPulse> Arrivals;
    for (auto& P : Pulses)
    {
        P.Age+=Dt;
        const float T=FMath::Clamp(P.Age/P.Duration,0.0f,1.0f), Fade=1-T;
        if (P.Kind==0)
        {
            if(T>=2.0f/3.0f && !P.bArrivalShown)
            {
                P.bArrivalShown=true;
                Arrivals.Add({P.End,P.End,P.Axis,0,.2f,1,P.Layer,P.Strength*.32f});
            }
            // A-to-B packet is ownership travel, deliberately has no directional arrowhead.
            for (int32 I=0;I<9;++I)
            {
                const float A=FMath::Clamp(T*1.5f-I*.035f,0.0f,1.0f), B=FMath::Clamp(A-.028f,0.0f,1.0f);
                auto Point=[&](float S){return FMath::Lerp(P.Start,P.End,S)+FVector(0,0,45*FMath::Sin(PI*S));};
                DrawLine(Point(A),Point(B),(7-I*.55f)*Fade*EffectScale,P.Layer);
            }
        }
        else if (P.Kind==1)
        {
            DrawRing(P.Start,P.Axis,20+150*FMath::Sqrt(T)*P.Strength,5*Fade*EffectScale,P.Layer);
        }
        else if(P.Kind==3)
        {
            // Converging braces punctuate the actual captured Dash, never a predicted hit.
            FVector U,V; P.Axis.FindBestAxisVectors(U,V);
            const float Radius=FMath::Lerp(150.0f,38.0f,1-FMath::Square(1-T));
            for(int32 I=0;I<8;++I)
            {
                const float Angle=I*PI/4;
                const FVector Radial=U*FMath::Cos(Angle)+V*FMath::Sin(Angle);
                DrawLine(P.Start+Radial*Radius,P.Start+Radial*(Radius+38*Fade),7*Fade*EffectScale,1);
            }
        }
        else
        {
            FRandomStream Random(371);
            for(int32 I=0;I<22;++I)
            {
                const FVector Velocity=(Random.VRand()+P.Axis*.8f)*P.Strength*250;
                const FVector Pos=P.Start+Velocity*P.Age-FVector(0,0,140*P.Age*P.Age);
                DrawLine(Pos,Pos+Velocity*.04f,6*Fade*EffectScale,P.Layer);
            }
        }
    }
    Pulses.RemoveAll([](const FPulse& P){return P.Age>=P.Duration;});
    for(const auto& Arrival:Arrivals) if(Pulses.Num()<16) Pulses.Add(Arrival);
    ActivePulseCount=Pulses.Num();
}

void ATransmitPresentationRig::DrawHousing(const AActor* Actor)
{
    if(!Actor) return;
    const auto* Mesh=Actor->FindComponentByClass<UStaticMeshComponent>();
    if(const auto* Endpoint=Cast<ATransmitMotionEndpointActor>(Actor)) Mesh=Endpoint->Body.Get();
    if(const auto* Carrier=Cast<ATransmitDirectionalCarrierActor>(Actor)) Mesh=Carrier->Body.Get();
    if(!Mesh || !Mesh->GetStaticMesh()) return;
    const FBox Bounds=Mesh->GetStaticMesh()->GetBoundingBox();
    const FTransform Transform=Mesh->GetComponentTransform();
    const FVector Extent=Bounds.GetExtent(), Center=Bounds.GetCenter();
    int32 EnergyLayer=0;
    const auto* Motion=Actor->FindComponentByClass<UMotionTransferComponent>();
    bool Active=Motion && Motion->HasMotionState();
    if(const auto* Boss=Cast<ATransmitChargerActor>(Actor))
    {
        EnergyLayer=1;
        Active=Boss->StateMachine && (Boss->StateMachine->GetState()==EMotionChargerState::Telegraph || Boss->StateMachine->GetState()==EMotionChargerState::Dash);
    }
    for(int32 Corner=0;Corner<8;++Corner)
    {
        FVector Local;
        for(int32 Axis=0;Axis<3;++Axis) Local[Axis]=Center[Axis]+((Corner&(1<<Axis))?1:-1)*(Extent[Axis]+1.5f);
        for(int32 Axis=0;Axis<3;++Axis)
        {
            FVector End=Local;
            const float WorldScale=FMath::Max(.01f,Transform.GetScale3D().GetAbs()[Axis]);
            End[Axis]-=((Corner&(1<<Axis))?1:-1)*FMath::Min(Extent[Axis]*.4f,45/WorldScale);
            const int32 Layer=Active && Axis==0 ? EnergyLayer : 2;
            DrawLine(Transform.TransformPosition(Local),Transform.TransformPosition(End),3.5f,Layer);
        }
    }
}

void ATransmitPresentationRig::DrawOwnership()
{
    for (const auto& P : Participants)
    {
        if (!P.IsValid() || P==PlayerMotion || !P->GetOwner()) continue;
        DrawHousing(P->GetOwner());
        FMotionState State;
        if (!P->TryGetMotionState(State)) continue;
        if (Cast<ATransmitChargerActor>(P->GetOwner())) continue;
        const FVector Center=BodyAnchor(P->GetOwner());
        const int32 Layer=State.DirectionPolicy==EMotionDirectionPolicy::PreserveSource ? 1 : 0;

        // Actual stored vector tail follows the moving body, never the transaction line.
        const auto* Carrier=Cast<ATransmitDirectionalCarrierActor>(P->GetOwner());
        if (Carrier && !Carrier->IsMovementActive()) continue;
        for (int32 I=0;I<3;++I)
        {
            const float Offset=90+I*35+FMath::Fmod(Phase*120,35.0f);
            DrawLine(Center-State.Direction*Offset,Center-State.Direction*(Offset+22),3,Layer);
        }
    }
    if (!PlayerMotion.IsValid()) return;
    auto* Pawn=PlayerMotion->GetOwner();
    FMotionState State;
    if (!PlayerMotion->TryGetMotionState(State)) return;
    const bool bHigh=State.DirectionPolicy==EMotionDirectionPolicy::PreserveSource;
    const int32 Layer=bHigh ? 1 : 0;
    const FVector C=Pawn->GetActorLocation()+Pawn->GetActorRightVector()*36+FVector(0,0,8);
    DrawRing(C,Pawn->GetActorForwardVector(),bHigh?28:21,3.2f,Layer);
    DrawRing(C,Pawn->GetActorForwardVector(),bHigh?35:27,1.5f,Layer,.66f);
    if(auto* Interactor=Pawn->FindComponentByClass<UMotionInteractorComponent>())
    {
        const auto Preview=Interactor->GetCurrentPreview();
        if (Preview.Target && Preview.bEligible && Preview.bHasProjectedDirection)
        {
            const FVector D=Preview.ProjectedWorldDirection.GetSafeNormal();
            const FVector Anchor=OutputAnchor(Preview.Target,D);
            DrawDevice(Anchor,D,105,Layer);
            // One short output-axis stroke at the recipient shows Preview == Commit.
            DrawLine(Anchor,Anchor+D*85,4,Layer);
            FVector U,V; D.FindBestAxisVectors(U,V);
            DrawLine(Anchor+D*85,Anchor+D*60+U*14,4,Layer);
            DrawLine(Anchor+D*85,Anchor+D*60-U*14,4,Layer);
        }
    }
}

void ATransmitPresentationRig::DrawEncounter(const float Dt)
{
    if (Ram)
    {
        if (DockAge>=0) DockAge+=Dt;
        if(Ram->bArmed)
        {
            TArray<FVector> Points;
            Points.Add(Ram->DockMarker ? Ram->DockMarker->GetActorLocation() : BodyAnchor(Ram));
            for (const auto& Anchor: DockCableAnchors) if(Anchor) Points.Add(Anchor->GetActorLocation());
            Points.Add(BodyAnchor(Ram));
            float Total=0; for(int32 I=1;I<Points.Num();++I) Total+=FVector::Distance(Points[I-1],Points[I]);
            float Distance=0;
            for(int32 I=1;I<Points.Num();++I)
            {
                const float Length=FVector::Distance(Points[I-1],Points[I]);
                const float Fill=FMath::Clamp((DockAge/1.1f*Total-Distance)/FMath::Max(Length,1.0f),0.0f,1.0f);
                DrawLine(Points[I-1],FMath::Lerp(Points[I-1],Points[I],Fill),3,0); Distance+=Length;
            }
            DrawDevice(BodyAnchor(Ram),Ram->FixedAxis,160,0);
        }
        if (Ram->Hits==1)
        {
            const FVector C=GateImpactAnchor, U=FVector::UpVector, V=FVector::CrossProduct(U,Ram->FixedAxis).GetSafeNormal();
            for(int32 I=-2;I<3;++I) DrawLine(C+U*(I*47)+V*((I%2)*24),C+U*((I+1)*47)+V*(((I+1)%2)*24),5,1);
        }
        if(Ram->Hits>=2 && ExitAnchor)
        {
            const FVector C=ExitAnchor->GetActorLocation();
            DrawRing(C-FVector(0,0,70),FVector::UpVector,140,3,0);
            DrawDevice(C+FVector(0,0,80),Ram->FixedAxis,140,0);
        }
    }
    if (Charger && Charger->StateMachine)
    {
        const auto State=Charger->StateMachine->GetState();
        const bool Warning=State==EMotionChargerState::Telegraph;
        const bool Dash=State==EMotionChargerState::Dash;
        if(Warning && LastChargerState!=static_cast<int32>(State)) Cue(3,BodyAnchor(Charger));
        if(!Warning && TelegraphAudio.IsValid()) { TelegraphAudio->Stop(); TelegraphAudio.Reset(); }
        LastChargerState=static_cast<int32>(State);

        if (Warning || Dash)
        {
            const FVector Axis=Charger->GetDashDirection();
            const FVector Side=FVector::CrossProduct(FVector::UpVector,Axis).GetSafeNormal();
            const FVector Center=BodyAnchor(Charger);
            const float Remaining=FMath::Max(0.0f,Charger->StateMachine->DashDurationSeconds-(Dash?Charger->StateMachine->GetElapsedInState():0));
            float Length=Charger->GetDashSpeed()*Remaining;
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(TransmitPresentation),false,Charger);
            if (const auto* Player=UGameplayStatics::GetPlayerPawn(this,0)) Query.AddIgnoredActor(Player);
            if(GetWorld()->LineTraceSingleByChannel(Hit,Center,Center+Axis*Length,ECC_Visibility,Query)) Length=FMath::Min(Length,Hit.Distance);
            FHitResult Floor;
            FVector Base=Center-FVector(0,0,80);
            if(GetWorld()->LineTraceSingleByChannel(Floor,Center,Center-FVector(0,0,500),ECC_Visibility,Query)) Base=Floor.ImpactPoint+FVector(0,0,3);
            const float T=Warning ? FMath::Clamp(Charger->StateMachine->GetElapsedInState()/FMath::Max(Charger->StateMachine->TelegraphDurationSeconds,.01f),0.0f,1.0f) : 1;
            for (float Sign : {-1.0f,1.0f})
            {
                DrawLine(Base+Side*100*Sign,Base+Axis*Length+Side*100*Sign,Warning?2.5f:5,1);
                DrawLine(Base+Side*100*Sign,Base+Axis*(Length*T)+Side*100*Sign,Warning?5:7,1);
            }
            const int32 Count=FMath::Min(12,FMath::FloorToInt(Length/150));
            for(int32 I=0;I<Count;++I)
            {
                const FVector P=Base+Axis*(I*150+60);
                DrawLine(P+Side*30,P+Axis*25,3,1); DrawLine(P-Side*30,P+Axis*25,3,1);
            }
            DrawDevice(Center,Axis,Warning?100-18*T:86,1);
        }
    }
}


void ATransmitPresentationRig::OnArmed()
{
    if(bResetting) return;
    DockAge=0; bLastArmed=true;
    const FVector Dock=Ram && Ram->DockMarker ? Ram->DockMarker->GetActorLocation() : BodyAnchor(Ram);
    AddPulse(1,Dock,Dock,FVector::UpVector,.8f,0,1.5f); Cue(2,Dock);
    UE_LOG(LogTemp,Log,TEXT("[TRANSMIT_PRESENTATION] Dock -> Ram armed"));
}

void ATransmitPresentationRig::OnImpact(const int32 Number)
{
    if(bResetting || !Ram || Number<=LastHits) return;
    LastHits=Number;
    const bool Final=Number>=2;
    AddPulse(1,GateImpactAnchor,GateImpactAnchor,Ram->FixedAxis,Final?1.1f:.65f,1,Final?3:1.7f);
    AddPulse(2,GateImpactAnchor,GateImpactAnchor,-Ram->FixedAxis,Final?1.4f:.75f,1,Final?2:1);
    Cue(Final?6:5,GateImpactAnchor);
    KickCamera(Final ? 3.0f : 1.8f,Final ? .55f : .3f);
    UE_LOG(LogTemp,Log,TEXT("[TRANSMIT_PRESENTATION] Actual impact %d"),Number);
}

void ATransmitPresentationRig::OnLocalRetry()
{
    ClearPresentation();
    LastHits=Ram ? Ram->Hits : 0; bLastArmed=Ram && Ram->bArmed;
    DockAge=bLastArmed ? 2 : -1; LastChargerState=0;
    bCompleted=Director && Director->IsComplete();
}

void ATransmitPresentationRig::OnFlowChanged()
{
    if(bResetting || (ResetController.IsValid() && ResetController->IsResetInProgress())) return;
    if(Director && Director->IsComplete() && !bCompleted)
    {
        bCompleted=true; Cue(7,ExitAnchor ? ExitAnchor->GetActorLocation() : GetActorLocation());
        UE_LOG(LogTemp,Log,TEXT("[TRANSMIT_PRESENTATION] Completion cadence"));
    }
}

void ATransmitPresentationRig::Tick(float Dt)
{
    Super::Tick(Dt);
    BindPlayer();
    for(const auto& A:LegacyArrows) if(A.Arrow.IsValid()) A.Arrow->SetHiddenInGame(true,true);
    PlayingAudio.RemoveAll([](const auto& A){return !IsValid(A) || !A->IsPlaying();});
    ActiveSoundCount=PlayingAudio.Num();
    for(int32 L=0;L<3;++L) Used[L]=0;
    if(!bResetting)
    {
        Phase+=Dt;
        UpdateCameraFeedback(Dt);
        // Event feedback takes priority over persistent housings in the bounded pools.
        DrawPulses(Dt); DrawOwnership(); DrawEncounter(Dt);
    }
    FinishStrokes();
}

void ATransmitPresentationRig::FinishStrokes()
{
    VisibleStrokeCount=0;
    for(int32 L=0;L<3;++L)
    {
        auto* Mesh=LayerMesh(L);
        for(int32 I=Used[L];I<PreviouslyUsed[L];++I) Mesh->UpdateInstanceTransform(I,FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector),true,false,true);
        Mesh->MarkRenderStateDirty(); VisibleStrokeCount+=Used[L]; PreviouslyUsed[L]=Used[L];
    }
}

void ATransmitPresentationRig::ClearPresentation()
{
    ClearCameraFeedback();
    Pulses.Reset(); ActivePulseCount=0; Phase=0; DockAge=-1;
    for(const auto& Audio:PlayingAudio) if(IsValid(Audio)) Audio->Stop();
    PlayingAudio.Reset(); ActiveSoundCount=0; TelegraphAudio.Reset();
    for(int32 L=0;L<3;++L) Used[L]=0;
    FinishStrokes();
}
void ATransmitPresentationRig::OnPreReset() { bResetting=true; ClearPresentation(); }
void ATransmitPresentationRig::OnPostReset() { bResetting=false; LastHits=0; bLastArmed=false; LastChargerState=0; bCompleted=false; }
void ATransmitPresentationRig::EndPlay(const EEndPlayReason::Type Reason)
{
    if(PlayerMotion.IsValid()) PlayerMotion->OnMotionTransactionNative().Remove(TransactionHandle);
    if(ResetController.IsValid())
    {
        ResetController->OnPreRoomReset.RemoveDynamic(this,&ATransmitPresentationRig::OnPreReset);
        ResetController->OnPostRoomReset.RemoveDynamic(this,&ATransmitPresentationRig::OnPostReset);
    }
    if(Ram) { Ram->OnArmed.RemoveDynamic(this,&ATransmitPresentationRig::OnArmed); Ram->OnImpact.RemoveDynamic(this,&ATransmitPresentationRig::OnImpact); }
    if(Director) { Director->OnLocalRetry.RemoveDynamic(this,&ATransmitPresentationRig::OnLocalRetry); Director->OnFlowChanged.RemoveDynamic(this,&ATransmitPresentationRig::OnFlowChanged); }
    ClearPresentation();
    for(const auto& L:LegacyLights) if(L.Light.IsValid()) L.Light->SetIntensity(L.Intensity);
    for(const auto& A:LegacyArrows) if(A.Arrow.IsValid()) A.Arrow->SetHiddenInGame(A.bHidden,true);
    Super::EndPlay(Reason);
}
