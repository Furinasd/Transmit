#include "Motion/TransmitHUD.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionTransferComponent.h"

namespace
{
    constexpr float ReticleGapPixels = 8.0f;
    constexpr float ReticleArmPixels = 16.0f;
    constexpr float ReticleThickness = 2.0f;

    const FLinearColor NeutralColor(1.0f, 1.0f, 1.0f, 0.9f);
    const FLinearColor TransferReadyColor(0.0f, 1.0f, 0.25f, 1.0f);
    const FLinearColor CaptureReadyColor(0.0f, 0.85f, 1.0f, 1.0f);
    const FLinearColor DirectionMismatchColor(1.0f, 0.12f, 0.05f, 1.0f);
    const FLinearColor InvalidTargetColor(0.75f, 0.75f, 0.75f, 0.9f);
}

void ATransmitHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!PlayerOwner)
    {
        return;
    }

    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    PlayerOwner->GetViewportSize(ViewportSizeX, ViewportSizeY);
    if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
    {
        return;
    }

    const APawn* Pawn = PlayerOwner->GetPawn();
    const UMotionInteractorComponent* Interactor = Pawn
        ? Pawn->FindComponentByClass<UMotionInteractorComponent>()
        : nullptr;
    const UMotionTransferComponent* Motion = Pawn
        ? Pawn->FindComponentByClass<UMotionTransferComponent>()
        : nullptr;
    if (!Interactor || !Motion)
    {
        DrawCrosshair(NeutralColor);
        return;
    }

    const FMotionInteractionPreview Preview = Interactor->GetCurrentPreview();
    if (!Preview.Target)
    {
        DrawCrosshair(NeutralColor);
        return;
    }

    if (Preview.bEligible)
    {
        DrawCrosshair(
            Motion->HasMotionState() ? TransferReadyColor : CaptureReadyColor);
        return;
    }

    if (Motion->HasMotionState()
        && Preview.Rejection == EMotionTransferRejection::IncompatibleDirection)
    {
        DrawCrosshair(DirectionMismatchColor);
        return;
    }

    DrawInvalidTargetMarker(InvalidTargetColor);
}

void ATransmitHUD::DrawCrosshair(const FLinearColor& Color)
{
    if (!PlayerOwner)
    {
        return;
    }

    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    PlayerOwner->GetViewportSize(ViewportSizeX, ViewportSizeY);
    const float CenterX = ViewportSizeX * 0.5f;
    const float CenterY = ViewportSizeY * 0.5f;

    DrawLine(
        CenterX - ReticleGapPixels - ReticleArmPixels,
        CenterY,
        CenterX - ReticleGapPixels,
        CenterY,
        Color,
        ReticleThickness);
    DrawLine(
        CenterX + ReticleGapPixels,
        CenterY,
        CenterX + ReticleGapPixels + ReticleArmPixels,
        CenterY,
        Color,
        ReticleThickness);
    DrawLine(
        CenterX,
        CenterY - ReticleGapPixels - ReticleArmPixels,
        CenterX,
        CenterY - ReticleGapPixels,
        Color,
        ReticleThickness);
    DrawLine(
        CenterX,
        CenterY + ReticleGapPixels,
        CenterX,
        CenterY + ReticleGapPixels + ReticleArmPixels,
        Color,
        ReticleThickness);
}

void ATransmitHUD::DrawInvalidTargetMarker(const FLinearColor& Color)
{
    if (!PlayerOwner)
    {
        return;
    }

    int32 ViewportSizeX = 0;
    int32 ViewportSizeY = 0;
    PlayerOwner->GetViewportSize(ViewportSizeX, ViewportSizeY);
    const float CenterX = ViewportSizeX * 0.5f;
    const float CenterY = ViewportSizeY * 0.5f;
    const float Radius = ReticleGapPixels + ReticleArmPixels * 0.5f;

    DrawLine(
        CenterX - Radius,
        CenterY - Radius,
        CenterX + Radius,
        CenterY + Radius,
        Color,
        ReticleThickness);
    DrawLine(
        CenterX + Radius,
        CenterY - Radius,
        CenterX - Radius,
        CenterY + Radius,
        Color,
        ReticleThickness);
}
