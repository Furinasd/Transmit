#include "Motion/TransmitHUD.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

#include "Components/ArrowComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "Transmit/TransmitLevelActors.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionTransferComponent.h"

namespace
{
    constexpr float ReticleGapPixels = 3.0f;
    constexpr float ReticleArmPixels = 3.0f;
    const FLinearColor NeutralColor(0.85f, 0.9f, 0.92f, 0.7f);
    const FLinearColor HudTransferReadyColor(0.25f, 0.9f, 0.55f, 0.95f);
    const FLinearColor CaptureReadyColor(0.25f, 0.8f, 0.95f, 0.95f);
    const FLinearColor HudDirectionMismatchColor(0.95f, 0.3f, 0.22f, 0.95f);
    const FLinearColor InvalidTargetColor(0.65f, 0.68f, 0.7f, 0.6f);
    const FLinearColor CueShadow(0.015f, 0.025f, 0.035f, 0.65f);

    FBox GetPhysicalMeshBounds(const AActor* Target)
    {
        FBox Bounds(ForceInit);
        TInlineComponentArray<UStaticMeshComponent*> Meshes;
        Target->GetComponents(Meshes);
        for (const UStaticMeshComponent* Mesh : Meshes)
        {
            if (!Mesh->IsVisible() || Mesh->bHiddenInGame || !Mesh->GetStaticMesh())
            {
                continue;
            }

            // Runtime direction meshes belong to an arrow component. They
            // communicate motion, but must never enlarge the target brackets.
            bool bIndicatorMesh = false;
            for (const USceneComponent* Parent = Mesh->GetAttachParent(); Parent;
                Parent = Parent->GetAttachParent())
            {
                if (Parent->IsA<UArrowComponent>())
                {
                    bIndicatorMesh = true;
                    break;
                }
            }
            if (!bIndicatorMesh)
            {
                Bounds += Mesh->Bounds.GetBox();
            }
        }
        if (!Bounds.IsValid)
        {
            FVector Origin;
            FVector Extent;
            Target->GetActorBounds(true, Origin, Extent);
            if (!Extent.IsNearlyZero())
            {
                Bounds = FBox(Origin - Extent, Origin + Extent);
            }
        }
        return Bounds;
    }
}

void ATransmitHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!PlayerOwner)
    {
        return;
    }

    if (!Canvas || !GEngine) return;
    const float Scale = FMath::Clamp(FMath::Min(Canvas->SizeX / 1440.0f, Canvas->SizeY / 900.0f), 0.65f, 1.5f);
    const float Margin = 32.0f * Scale;
    const bool bHelp = PlayerOwner->IsInputKeyDown(EKeys::Tab);
    if (!RuntimeFont)
    {
        RuntimeFont = NewObject<UFont>(this);
        RuntimeFont->FontCacheType = EFontCacheType::Runtime;
        RuntimeFont->GetMutableInternalCompositeFont() = *FCoreStyle::GetDefaultFontStyle("Regular", 12).GetCompositeFont();
    }
    const auto Font = [this, Scale](float Size)
    {
        FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle("Regular", FMath::RoundToInt(12 * Size * Scale));
        // Canvas requires a UFont even when Slate supplies the composite typeface.
        Info.FontObject = RuntimeFont;
        return Info;
    };
    const auto Measure = [&Font](const FString& Text, float Size) -> FVector2D
    {
        return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font(Size));
    };
    const auto Label = [this, &Font](const FString& Text, float X, float Y, float Size, FLinearColor Color)
    {
        FCanvasTextItem Item(FVector2D(X,Y), FText::FromString(Text), Font(Size), Color);
        Canvas->DrawItem(Item);
    };
    const auto Wrapped = [&Measure](const FString& Text, float Width, float Size)
    {
        TArray<FString> Lines;
        FString Line;
        for (const TCHAR Character : Text)
        {
            const FString Next = Line + FString::Chr(Character);
            if (!Line.IsEmpty() && (Character == '\n' || Measure(Next, Size).X > Width))
            { Lines.Add(Line); Line.Reset(); }
            if (Character != '\n') Line.AppendChar(Character);
        }
        if (!Line.IsEmpty()) Lines.Add(Line);
        return Lines;
    };

    // Presentation only: the director supplies facts, never a second progress model.
    TActorIterator<ATransmitLevelDirector> It(GetWorld());
    if (It)
    {
        const FString Objective = It->GetObjectiveText();
        const FString Hint = It->GetHintText();
        const float Now = GetWorld()->GetTimeSeconds();
        const float Width = 440 * Scale;
        const float HintWidth = FMath::Min(700 * Scale, Canvas->SizeX - 2*Margin);
        const bool bTextChanged = Objective != LastObjective || Hint != LastHint || It->GetRunStartSeconds() != LastRunStartSeconds;
        if (bTextChanged || LayoutScale != Scale || LayoutWidth != HintWidth)
        {
            LayoutScale = Scale; LayoutWidth = HintWidth;
            ObjectiveLines = Wrapped(Objective, Width - 40 * Scale, 1.9f);
            HintLines = Wrapped(Hint, HintWidth - 40 * Scale, 1.35f);
        }
        if (bTextChanged)
        {
            LastRunStartSeconds = It->GetRunStartSeconds();
            LastObjective = Objective;
            LastHint = Hint;
            GuidanceChangedSeconds = Now;
        }
        if (LastCheckpoint != It->GetCheckpointIndex() || Now - It->GetRunStartSeconds() < .1f)
        { LastCheckpoint = It->GetCheckpointIndex(); CheckpointNoticeUntil = Now + 4.0f; }
        if (bHelp || It->IsComplete())
        {
        const auto& Lines = ObjectiveLines;
        const float Height = (78 + 32 * Lines.Num()) * Scale;
        DrawRect(FLinearColor(.025f, .038f, .048f, .82f), Margin, Margin, Width, Height);
        DrawRect(FLinearColor(.72f, .81f, .82f, .85f), Margin, Margin, 2 * Scale, Height);
        Label(TEXT("TRANSMIT   /   临时检修工单"), Margin + 20*Scale, Margin + 14*Scale, .95f, NeutralColor);
        Label(It->GetChapterText(), Margin + 20*Scale, Margin + 38*Scale, 1.1f, FLinearColor(.7f,.8f,.82f));
        for (int32 Index = 0; Index < Lines.Num(); ++Index)
            Label(Lines[Index], Margin + 20*Scale, Margin + (65+32*Index)*Scale, 1.9f, FLinearColor::White);

        }
        else if (Now < CheckpointNoticeUntil || Now - It->GetLastRetrySeconds() < 2.0f)
        {
            DrawRect(FLinearColor(.025f,.038f,.048f,.72f),Margin,Margin,280*Scale,58*Scale);
            const FString Chapters[] = {TEXT("01 / 米工装配区"), TEXT("02 / 华序总线区"), TEXT("03 / 白果接口")};
            Label(Chapters[FMath::Clamp(LastCheckpoint,0,2)],Margin+16*Scale,Margin+10*Scale,1.15f,NeutralColor);
            Label(TEXT("检修记录已保存"),Margin+16*Scale,Margin+32*Scale,.9f,NeutralColor);
        }

        if (It->IsBossIntroduced() && It->Ram && It->Ram->bArmed && It->Charger
            && It->GetFlowStep() != ETransmitFlowStep::ReachArena && !It->IsComplete())
        {
            const float X = Canvas->SizeX*.5f-130*Scale;
            const auto State = It->Charger->StateMachine->GetState();
            const FString Phase = It->Ram->Hits>=2 ? TEXT("检修通行")
                : State==EMotionChargerState::Telegraph ? TEXT("锁定位置")
                : State==EMotionChargerState::Dash ? TEXT("冲刺")
                : State==EMotionChargerState::Recovery ? TEXT("回位") : TEXT("门前待机");
            DrawRect(FLinearColor(.025f,.038f,.048f,.86f),X-14*Scale,Margin-8*Scale,232*Scale,68*Scale);
            Label(TEXT("户晨风 / 首席分等官"),X,Margin,1.25f,FLinearColor(.96f,.89f,.74f));
            Label(Phase,X,Margin+24*Scale,1.0f,NeutralColor);
            for (int32 Hit=0;Hit<2;++Hit)
                DrawRect(Hit<It->Ram->Hits ? FLinearColor(.95f,.5f,.16f) : FLinearColor(.18f,.24f,.28f),
                    X+Hit*104*Scale,Margin+48*Scale,96*Scale,3*Scale);
        }

        // A short contextual lesson appears on a state change; hold Tab to recall it.
        // Moving/blocked/loaded distinctions are actual actor state, not tutorial timers.
        const float HintAlpha = bHelp ? 1.0f : 0.0f;
        if (HintAlpha > 0)
        {
            const auto& Hints = HintLines;
            const float HintHeight = (24 + 22*Hints.Num())*Scale;
            const float X = (Canvas->SizeX - HintWidth)*.5f;
            const float Y = Canvas->SizeY - 92*Scale - HintHeight;
            DrawRect(FLinearColor(.025f,.038f,.048f,.86f*HintAlpha), X, Y, HintWidth, HintHeight);
            for (int32 Index = 0; Index < Hints.Num(); ++Index)
                Label(Hints[Index], X+20*Scale, Y+(12+22*Index)*Scale, 1.35f, FLinearColor(.9f,.94f,.95f,HintAlpha));
        }
        const FString Narrative = It->GetNarrativeText();
        if (!Narrative.IsEmpty())
        {
            const float W = Measure(Narrative, 1.25f).X;
            DrawRect(FLinearColor(.015f,.022f,.03f,.8f), (Canvas->SizeX-W)*.5f-16*Scale, Canvas->SizeY*.73f-8*Scale, W+32*Scale, 38*Scale);
            Label(Narrative, (Canvas->SizeX-W)*.5f, Canvas->SizeY*.73f, 1.25f, FLinearColor(.96f,.89f,.74f));
        }
        Label(TEXT("TAB  检修器"), Margin, Canvas->SizeY-38*Scale, 1.0f, NeutralColor);
        const FString Controls = bHelp
            ? TEXT("WASD 移动   鼠标 瞄准   空格 跳跃   E 取出/截停   Q 传递   退格 本区重试   R 重新开始")
            : TEXT("退格  本区重试     R  重新开始");
        float W=0,H=0;
        W = Measure(Controls, .95f).X;
        Label(Controls, Canvas->SizeX-Margin-W, Canvas->SizeY-38*Scale, .95f, NeutralColor);
    }

    const APawn* Pawn = PlayerOwner->GetPawn();
    const UMotionInteractorComponent* Interactor = Pawn
        ? Pawn->FindComponentByClass<UMotionInteractorComponent>() : nullptr;
    const UMotionTransferComponent* Motion = Pawn
        ? Pawn->FindComponentByClass<UMotionTransferComponent>() : nullptr;
    if (!Interactor || !Motion)
    {
        DrawCrosshair(NeutralColor);
        return;
    }

    const bool bLoaded = Motion->HasMotionState();
    const FString CarryLabel = bLoaded ? TEXT("玄武能量 / 已携带") : TEXT("检修工具 / 空");
    const FLinearColor CarryColor = bLoaded ? HudTransferReadyColor : NeutralColor;
    const float StateX = Canvas->SizeX - Margin - 194*Scale;
    DrawRect(FLinearColor(.025f,.038f,.048f,.8f),StateX,Margin,194*Scale,36*Scale);
    DrawRect(CarryColor,StateX+12*Scale,Margin+15*Scale,5*Scale,5*Scale);
    Label(CarryLabel,StateX+28*Scale,Margin+10*Scale,1.0f,CarryColor);

    const FMotionInteractionPreview Preview = Interactor->GetCurrentPreview();
    if (!IsValid(Preview.Target)
        || Preview.Rejection == EMotionTransferRejection::Occluded
        || Preview.Rejection == EMotionTransferRejection::OutOfRange)
    {
        DrawCrosshair(NeutralColor);
        return;
    }

    FLinearColor Color = InvalidTargetColor;
    if (Preview.bEligible)
    {
        Color = Motion->HasMotionState() ? HudTransferReadyColor : CaptureReadyColor;
    }
    else if (Motion->HasMotionState()
        && Preview.Rejection == EMotionTransferRejection::IncompatibleDirection)
    {
        Color = HudDirectionMismatchColor;
    }
    DrawCrosshair(Color);
    DrawTargetBrackets(Preview.Target, Color);
    if (Preview.bEligible)
    {
        const FString Action = bLoaded ? TEXT("Q   传递能量") : TEXT("E   取出能量");
        float W=0,H=0;
        W = Measure(Action, 1.15f).X;
        const float X=(Canvas->SizeX-W)*.5f, Y=Canvas->SizeY*.5f+42*Scale;
        DrawRect(FLinearColor(.025f,.038f,.048f,.82f),X-12*Scale,Y-7*Scale,W+24*Scale,32*Scale);
        Label(Action,X,Y,1.15f,Color);
    }
    if (!Preview.bEligible)
    {
        FString Reason;
        switch (Preview.Rejection)
        {
        case EMotionTransferRejection::TimingRejected:
            Reason = Cast<ATransmitRam>(Preview.Target) ? TEXT("先将 C-01 送入接口") : TEXT("等待冲刺开始后取出"); break;
        case EMotionTransferRejection::SourceEmpty: Reason = TEXT("此处没有可取出的能量"); break;
        case EMotionTransferRejection::CarrierOccupied: Reason = TEXT("已携带能量，请先传递"); break;
        case EMotionTransferRejection::IncompatibleType: Reason = TEXT("需要截取冲刺能量"); break;
        case EMotionTransferRejection::IncompatibleMagnitudeTier: Reason = TEXT("需要更强的冲刺能量"); break;
        case EMotionTransferRejection::IncompatibleDirection: Reason = TEXT("运动方向与设备不兼容"); break;
        case EMotionTransferRejection::CooldownActive: Reason = TEXT("等待载体完成突进并返回"); break;
        default: break;
        }
        if (!Reason.IsEmpty())
        {
            float W = 0, H = 0;
            W = Measure(Reason, 1.0f).X;
            Label(Reason, (Canvas->SizeX-W)*0.5f, Canvas->SizeY*0.60f, 1.0f, Color);
        }
    }
}

void ATransmitHUD::DrawCueLine(
    const FVector2D& Start, const FVector2D& End, const FLinearColor& Color)
{
    DrawLine(Start.X, Start.Y, End.X, End.Y, CueShadow, 3.0f);
    DrawLine(Start.X, Start.Y, End.X, End.Y, Color, 1.0f);
}

void ATransmitHUD::DrawCrosshair(const FLinearColor& Color)
{
    int32 Width = 0;
    int32 Height = 0;
    PlayerOwner->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0)
    {
        return;
    }
    const FVector2D Center(Width * 0.5f, Height * 0.5f);
    for (const FVector2D Axis : {FVector2D(1, 0), FVector2D(0, 1)})
    {
        for (const float Sign : {-1.0f, 1.0f})
        {
            DrawCueLine(Center + Axis * (Sign * ReticleGapPixels),
                Center + Axis * (Sign * (ReticleGapPixels + ReticleArmPixels)), Color);
        }
    }
}

void ATransmitHUD::DrawTargetBrackets(const AActor* Target, const FLinearColor& Color)
{
    if (Target->IsHidden())
    {
        return;
    }
    const FBox Bounds = GetPhysicalMeshBounds(Target);
    if (!Bounds.IsValid)
    {
        return;
    }

    FVector CameraLocation;
    FRotator CameraRotation;
    PlayerOwner->GetPlayerViewPoint(CameraLocation, CameraRotation);
    const FVector CameraForward = CameraRotation.Vector();
    FBox2D ScreenBounds(ForceInit);
    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        const FVector WorldPoint(
            (Corner & 1) ? Bounds.Max.X : Bounds.Min.X,
            (Corner & 2) ? Bounds.Max.Y : Bounds.Min.Y,
            (Corner & 4) ? Bounds.Max.Z : Bounds.Min.Z);
        // Suppress boxes crossing the camera plane: projecting those corners
        // creates enormous or inverted screen rectangles.
        if (FVector::DotProduct(WorldPoint - CameraLocation, CameraForward) <= 1.0f)
        {
            return;
        }
        FVector2D ScreenPoint;
        if (!PlayerOwner->ProjectWorldLocationToScreen(WorldPoint, ScreenPoint)
            || !FMath::IsFinite(ScreenPoint.X) || !FMath::IsFinite(ScreenPoint.Y))
        {
            return;
        }
        ScreenBounds += ScreenPoint;
    }

    int32 Width = 0;
    int32 Height = 0;
    PlayerOwner->GetViewportSize(Width, Height);
    constexpr float Margin = 8.0f;
    constexpr float Padding = 6.0f;
    if (Width <= Margin * 2 || Height <= Margin * 2
        || ScreenBounds.Max.X < 0 || ScreenBounds.Min.X > Width
        || ScreenBounds.Max.Y < 0 || ScreenBounds.Min.Y > Height)
    {
        return;
    }
    const FVector2D Min(
        FMath::Clamp(ScreenBounds.Min.X - Padding, double(Margin), double(Width - Margin)),
        FMath::Clamp(ScreenBounds.Min.Y - Padding, double(Margin), double(Height - Margin)));
    const FVector2D Max(
        FMath::Clamp(ScreenBounds.Max.X + Padding, double(Margin), double(Width - Margin)),
        FMath::Clamp(ScreenBounds.Max.Y + Padding, double(Margin), double(Height - Margin)));
    const float Arm = FMath::Min(12.0, FMath::Min(Max.X - Min.X, Max.Y - Min.Y) * 0.2);
    if (Arm < 2.0f)
    {
        return;
    }
    for (int32 Corner = 0; Corner < 4; ++Corner)
    {
        const FVector2D Point((Corner & 1) ? Max.X : Min.X, (Corner & 2) ? Max.Y : Min.Y);
        DrawCueLine(Point, Point + FVector2D((Corner & 1) ? -Arm : Arm, 0), Color);
        DrawCueLine(Point, Point + FVector2D(0, (Corner & 2) ? -Arm : Arm), Color);
    }
}
