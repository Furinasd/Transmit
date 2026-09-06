#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Motion/MotionCanonicalDirectionResolver.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/TransmitMotionEndpointActor.h"

namespace CameraCanonicalTests
{
    struct FCase
    {
        FRotator Camera;
        FVector World;
        EMotionCanonicalDirection Canonical;
    };
    static const FCase Cases[] = {
        {FRotator(0, 0, 0), FVector::ForwardVector, EMotionCanonicalDirection::Forward},
        {FRotator(0, 90, 0), FVector::RightVector, EMotionCanonicalDirection::Right},
        {FRotator(0, 180, 0), FVector::BackwardVector, EMotionCanonicalDirection::Back},
        {FRotator(0, -90, 0), FVector::LeftVector, EMotionCanonicalDirection::Left},
        {FRotator(50, 23, 0), FVector::UpVector, EMotionCanonicalDirection::Up},
        {FRotator(-50, -23, 0), FVector::DownVector, EMotionCanonicalDirection::Down}
    };
    static const FVector Inputs[] = {FVector::ForwardVector, FVector::BackwardVector,
        FVector::RightVector, FVector::LeftVector, FVector::UpVector, FVector::DownVector};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionCameraWorldMatrixTest,
    "Transmit.MotionTransfer.CameraAuthored.WorldMatrixAndInputIndependence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMotionCameraWorldMatrixTest::RunTest(const FString& Parameters)
{
    using namespace CameraCanonicalTests;
    for (const FCase& Case : Cases)
    {
        for (const FVector& Input : Inputs)
        {
            const auto Resolution = UMotionCanonicalDirectionResolver::ResolveDirectionDeterministic(
                Input, Case.Camera, EMotionCanonicalDirection::None, 45, 35, 8, true);
            const FString Label = FString::Printf(TEXT("Camera %s input %s"),
                *Case.Camera.ToString(), *Input.ToString());
            TestTrue(Label + TEXT(" valid"), Resolution.bValid);
            TestEqual(Label + TEXT(" enum"), Resolution.CanonicalDirection, Case.Canonical);
            TestTrue(Label + TEXT(" world"), Resolution.WorldDirection.Equals(Case.World, 1.e-4f));
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionCameraHysteresisTest,
    "Transmit.MotionTransfer.CameraAuthored.HorizontalAndVerticalHysteresis",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMotionCameraHysteresisTest::RunTest(const FString& Parameters)
{
    UMotionCanonicalDirectionResolver* Resolver = NewObject<UMotionCanonicalDirectionResolver>();
    // Preserve the existing score-margin hysteresis (8 degrees -> sin(8)).
    // Exercise every horizontal boundary in both directions, including yaw wrap.
    const FVector Worlds[] = {FVector::ForwardVector, FVector::RightVector,
        FVector::BackwardVector, FVector::LeftVector, FVector::ForwardVector};
    for (int32 Sector = 0; Sector < 4; ++Sector)
    {
        const float Base = Sector * 90.f;
        Resolver->ResetHysteresis();
        Resolver->ResolveDirection(FVector::UpVector, FRotator(0, Base, 0));
        for (float Offset : {44.f, 46.f, 44.5f, 45.5f, 50.f})
        {
            const auto R = Resolver->ResolveDirection(FVector::DownVector, FRotator(0, Base + Offset, 0));
            TestTrue(TEXT("Forward sweep holds inside boundary band"), R.WorldDirection.Equals(Worlds[Sector]));
        }
        auto R = Resolver->ResolveDirection(FVector::UpVector, FRotator(0, Base + 52, 0));
        TestTrue(TEXT("Forward sweep crosses outside band"), R.WorldDirection.Equals(Worlds[Sector + 1]));
        for (float Offset : {46.f, 44.f, 45.5f, 44.5f, 40.f})
        {
            R = Resolver->ResolveDirection(FVector::UpVector, FRotator(0, Base + Offset, 0));
            TestTrue(TEXT("Reverse sweep holds inside boundary band"), R.WorldDirection.Equals(Worlds[Sector + 1]));
        }
        R = Resolver->ResolveDirection(FVector::DownVector, FRotator(0, Base + 38, 0));
        TestTrue(TEXT("Reverse sweep crosses outside band"), R.WorldDirection.Equals(Worlds[Sector]));
    }
    for (float Sign : {1.f, -1.f})
    {
        Resolver->ResetHysteresis();
        const FVector Incoming = -Sign * FVector::UpVector;
        const float Pitches[] = {44, 45, 44, 36, 35, 34, 44, 46};
        const bool Vertical[] = {false, true, true, true, true, false, false, true};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Pitches); ++Index)
        {
            const auto R = Resolver->ResolveDirection(Incoming, FRotator(Sign * Pitches[Index], 90, 0));
            const FVector Expected = Vertical[Index] ? Sign * FVector::UpVector : FVector::RightVector;
            TestTrue(TEXT("Camera pitch enter/exit ignores opposite incoming pitch"), R.WorldDirection.Equals(Expected));
        }
        const auto Opposite = Resolver->ResolveDirection(Sign * FVector::UpVector, FRotator(-Sign * 50, 90, 0));
        TestTrue(TEXT("Camera can switch directly to opposite vertical sector"),
            Opposite.WorldDirection.Equals(-Sign * FVector::UpVector));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionCameraPreviewCommitTest,
    "Transmit.MotionTransfer.CameraAuthored.InteractorPreviewCommit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMotionCameraPreviewCommitTest::RunTest(const FString& Parameters)
{
    using namespace CameraCanonicalTests;
    for (const FCase& Case : Cases)
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!TestNotNull(TEXT("Test world"), World)) return false;
        auto* Player = World->SpawnActor<ATransmitMotionEndpointActor>();
        auto* Source = World->SpawnActor<ATransmitMotionEndpointActor>();
        auto* Target = World->SpawnActor<ATransmitMotionEndpointActor>();
        auto* Interactor = NewObject<UMotionInteractorComponent>(Player);
        Interactor->RegisterComponent();
        Player->SetActorRotation(Case.Camera);
        Source->SetActorLocation(FVector(0, 0, -1500));
        Target->SetActorLocation(Case.Camera.Vector() * 500);
        auto* QueryBox = NewObject<UBoxComponent>(Target);
        QueryBox->SetBoxExtent(FVector(30));
        QueryBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        QueryBox->RegisterComponent();
        QueryBox->SetWorldLocation(Target->GetActorLocation());
        FMotionState Initial;
        Initial.Type = EMotionType::Linear;
        Initial.Direction = FVector::UpVector;
        Initial.Magnitude = 600;
        Initial.SourceId = TEXT("CameraRegression.Source");
        Source->Motion->ConfigureForTesting(TEXT("Source"), true, false, EMotionEndpointMode::Store, Initial);
        Player->Motion->ConfigureForTesting(TEXT("Player"), true, true, EMotionEndpointMode::Store, {});
        Target->Motion->ConfigureForTesting(TEXT("Target"), true, true, EMotionEndpointMode::Store, {});
        TestTrue(TEXT("Capture through Actor interface"), Player->Motion->TryCaptureFromActor(Source, {}).bSucceeded);
        FMotionState Carried;
        TestTrue(TEXT("Carry exists"), Player->Motion->TryGetMotionState(Carried));
        TestTrue(TEXT("Capture preserves incoming direction"), Carried.Direction.Equals(Initial.Direction));
        Interactor->RefreshTarget();
        const FMotionInteractionPreview Preview = Interactor->GetCurrentPreview();
        TestTrue(TEXT("Interactor selected intended target"), Preview.Target == Target);
        TestTrue(TEXT("Preview eligible"), Preview.bEligible);
        TestTrue(TEXT("Preview world matches camera"), Preview.ProjectedWorldDirection.Equals(Case.World));
        const auto Commit = Interactor->RequestTransfer();
        TestTrue(TEXT("Interactor commit succeeds"), Commit.bSucceeded);
        FMotionState Received;
        TestTrue(TEXT("Target stores committed state"), Target->Motion->TryGetMotionState(Received));
        TestTrue(TEXT("Preview world equals committed direction"), Preview.ProjectedWorldDirection.Equals(Received.Direction));
        TestTrue(TEXT("Commit world matches camera"), Received.Direction.Equals(Case.World));
        TestEqual(TEXT("Source identity survives reroute"), Received.SourceId, Initial.SourceId);
        World->DestroyWorld(false);
    }
    return true;
}
#endif
