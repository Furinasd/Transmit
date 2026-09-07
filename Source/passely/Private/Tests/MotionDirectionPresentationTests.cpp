#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Motion/MotionDirectionIndicatorComponent.h"
#include "Motion/TransmitMotionEndpointActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionFaceIndicatorTest,
    "Transmit.Presentation.OutputFaceIndicator",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMotionFaceIndicatorTest::RunTest(const FString& Parameters)
{
    const FVector Center(100, -200, 50);
    const FVector Extent(80, 35, 15);
    const FBox Box(Center - Extent, Center + Extent);
    const FVector Directions[] = {FVector::ForwardVector, FVector::BackwardVector,
        FVector::RightVector, FVector::LeftVector, FVector::UpVector, FVector::DownVector};
    for (const FVector& Direction : Directions)
    {
        const FVector Anchor = UMotionDirectionIndicatorComponent::CalculateFaceAnchor(Box, Direction, 8);
        const FVector Expected = Center + Direction * (FVector::DotProduct(Direction.GetAbs(), Extent) + 8);
        TestTrue(TEXT("Each output starts just outside its corresponding physical face"), Anchor.Equals(Expected));
        TestFalse(TEXT("Arrow tail cannot be embedded inside target"), Box.IsInside(Anchor));
    }
    const FVector Diagonal = FVector(1, 1, 0).GetSafeNormal();
    const FVector DiagonalAnchor = UMotionDirectionIndicatorComponent::CalculateFaceAnchor(Box, Diagonal, 0);
    TestTrue(TEXT("Preserved arbitrary direction intersects a face without being quantized"),
        DiagonalAnchor.Equals(Center + FVector(35, 35, 0)));

    const FBox LongLocalBox(FVector(-100, -10, -10), FVector(100, 10, 10));
    const FTransform RotatedBody(FRotator(0, 45, 0), Center, FVector(2, 1, 1));
    const FVector RotatedAnchor = UMotionDirectionIndicatorComponent::CalculateFaceAnchor(
        LongLocalBox, FVector::ForwardVector, 8, RotatedBody);
    TestTrue(TEXT("Rotated scaled body uses the physical face rather than empty world-AABB space"),
        RotatedAnchor.Equals(Center + FVector(10 * FMath::Sqrt(2.0) + 8, 0, 0), 1.e-3));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Indicator world"), World)) return false;
    auto* Owner = World->SpawnActor<ATransmitMotionEndpointActor>();
    auto* Indicator = NewObject<UMotionDirectionIndicatorComponent>(Owner);
    Indicator->SetupAttachment(Owner->GetRootComponent());
    Indicator->RegisterComponent();
    for (const FVector& Direction : Directions)
    {
        Indicator->ShowDirection(Direction, 600);
        const auto* Mesh = Indicator->RuntimeIndicatorMesh.Get();
        TestTrue(TEXT("Runtime arrow aligns with the unchanged world output"),
            Mesh->GetForwardVector().Equals(Direction, 1.e-4f));
        TestTrue(TEXT("Small arrow is visible"), Mesh->IsVisible());
        TestFalse(TEXT("Cue cannot block targeting or movement"), Mesh->IsCollisionEnabled());
        TestTrue(TEXT("Arrow length remains compact"),
            FMath::IsNearlyEqual(Mesh->GetStaticMesh()->GetBoundingBox().GetSize().X
                * Mesh->GetComponentScale().X, 48.0, 1.e-3));
    }
    Indicator->HideDirection();
    TestFalse(TEXT("Clearing preview hides runtime arrow"), Indicator->RuntimeIndicatorMesh->IsVisible());
    World->DestroyWorld(false);
    return true;
}
#endif
