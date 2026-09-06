#include "Motion/TransmitHUD.h"

#include "Components/ArrowComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
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

    // Level-authored guidance follows committed gameplay state even without a target.
    TActorIterator<ATransmitLevelDirector> It(GetWorld());
    if (It)
    {
        const float Scale = Canvas ? FMath::Clamp(Canvas->SizeX / 1600.0f, 0.75f, 1.3f) : 1.0f;
        const float X = 38.0f * Scale;
        const float TextWidth = 660.0f * Scale;
        const auto Wrap = [this, TextWidth](const FString& Text, UFont* Font, float TextScale)
        {
            TArray<FString> Words;
            Text.ParseIntoArrayWS(Words);
            TArray<FString> Lines;
            FString Line;
            for (const FString& Word : Words)
            {
                const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
                float W = 0, H = 0;
                GetTextSize(Candidate, W, H, Font, TextScale);
                if (!Line.IsEmpty() && W > TextWidth)
                {
                    Lines.Add(Line);
                    Line = Word;
                }
                else
                {
                    Line = Candidate;
                }
            }
            if (!Line.IsEmpty()) { Lines.Add(Line); }
            return Lines;
        };
        const auto Objectives = Wrap(It->GetObjectiveText(), GEngine->GetMediumFont(), 1.8f * Scale);
        const auto Hints = Wrap(It->GetHintText(), GEngine->GetSmallFont(), 1.35f * Scale);
        const float HintY = (77.0f + 29.0f * Objectives.Num()) * Scale;
        const float PanelBottom = HintY + (23.0f * Hints.Num() + 17.0f) * Scale;
        DrawRect(FLinearColor(0.015f, 0.025f, 0.035f, 0.78f), X - 14 * Scale, 30 * Scale,
            TextWidth + 28 * Scale, PanelBottom - 30 * Scale);
        DrawText(It->GetChapterText(), FLinearColor(0.2f, 0.85f, 0.9f), X, 42 * Scale,
            GEngine->GetSmallFont(), 1.25f * Scale);
        for (int32 Index = 0; Index < Objectives.Num(); ++Index)
        {
            DrawText(Objectives[Index], FLinearColor::White, X, (73 + 29 * Index) * Scale,
                GEngine->GetMediumFont(), 1.8f * Scale);
        }
        for (int32 Index = 0; Index < Hints.Num(); ++Index)
        {
            DrawText(Hints[Index], FLinearColor(0.78f, 0.85f, 0.87f), X, HintY + 23 * Index * Scale,
                GEngine->GetSmallFont(), 1.35f * Scale);
        }
        DrawText(TEXT("E  CAPTURE    Q  TRANSFER    BACKSPACE  RETRY AREA    R  RESTART"), FLinearColor(0.75f, 0.8f, 0.83f), X,
            Canvas->SizeY - 40 * Scale, GEngine->GetSmallFont(), 1.15f * Scale);
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
    if (!Preview.bEligible)
    {
        FString Reason;
        switch (Preview.Rejection)
        {
        case EMotionTransferRejection::TimingRejected:
            Reason = Cast<ATransmitRam>(Preview.Target) ? TEXT("Deliver the relay to arm this Ram") : TEXT("Wait for the committed dash"); break;
        case EMotionTransferRejection::SourceEmpty: Reason = TEXT("No motion here to capture"); break;
        case EMotionTransferRejection::CarrierOccupied: Reason = TEXT("Already carrying motion — transfer it first"); break;
        case EMotionTransferRejection::IncompatibleType: Reason = TEXT("This Ram needs a captured charge"); break;
        case EMotionTransferRejection::IncompatibleMagnitudeTier: Reason = TEXT("More force is required"); break;
        case EMotionTransferRejection::IncompatibleDirection: Reason = TEXT("The motion points away from this device's axis"); break;
        case EMotionTransferRejection::CooldownActive: Reason = TEXT("Let the Ram finish its stroke"); break;
        default: break;
        }
        if (!Reason.IsEmpty())
        {
            float W = 0, H = 0;
            GetTextSize(Reason, W, H, GEngine->GetSmallFont());
            DrawText(Reason, Color, (Canvas->SizeX-W)*0.5f, Canvas->SizeY*0.60f, GEngine->GetSmallFont());
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
