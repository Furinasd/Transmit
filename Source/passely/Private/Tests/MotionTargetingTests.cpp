#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "CollisionQueryParams.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Motion/TransmitCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Motion/MotionInteractorComponent.h"
#include "Motion/MotionTransferComponent.h"
#include "Motion/TransmitMotionEndpointActor.h"

namespace MotionTargetingTests
{
    struct FFixture
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        ATransmitMotionEndpointActor* Player = nullptr;
        UMotionInteractorComponent* Interactor = nullptr;

        explicit FFixture(bool bCreateEndpointPlayer = true)
        {
            if (World && bCreateEndpointPlayer)
            {
                Player = World->SpawnActor<ATransmitMotionEndpointActor>();
                Interactor = NewObject<UMotionInteractorComponent>(Player);
                Interactor->RegisterComponent();
                Player->Motion->ConfigureForTesting(TEXT("Player"), true, true,
                    EMotionEndpointMode::Store, {});
            }
        }
        ~FFixture()
        {
            if (World) World->DestroyWorld(false);
        }
        ATransmitMotionEndpointActor* AddSource(const FVector& Location, FName Id = TEXT("Source"))
        {
            auto* Source = World->SpawnActor<ATransmitMotionEndpointActor>();
            Source->SetActorLocation(Location);
            auto* QueryBox = NewObject<UBoxComponent>(Source);
            QueryBox->SetupAttachment(Source->GetRootComponent());
            QueryBox->SetBoxExtent(FVector(30));
            QueryBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
            QueryBox->RegisterComponent();
            FMotionState State;
            State.Type = EMotionType::Linear;
            State.Direction = FVector::ForwardVector;
            State.Magnitude = 600;
            State.SourceId = Id;
            Source->Motion->ConfigureForTesting(Id, true, false, EMotionEndpointMode::Store, State);
            return Source;
        }
        FMotionInteractionPreview Look(float Yaw)
        {
            Player->SetActorRotation(FRotator(0, Yaw, 0));
            Interactor->RefreshTarget();
            return Interactor->GetCurrentPreview();
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionTargetAcquireRetainTest,
    "Transmit.MotionTransfer.Targeting.AcquisitionAndReleaseBand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMotionTargetAcquireRetainTest::RunTest(const FString& Parameters)
{
    using namespace MotionTargetingTests;
    FFixture Fixture;
    if (!TestNotNull(TEXT("World"), Fixture.World)) return false;
    auto* Source = Fixture.AddSource(FVector(500, 0, 0));
    auto Preview = Fixture.Look(30);
    TestTrue(TEXT("Slight reticle offset acquires visible source"), Preview.Target == Source && Preview.bEligible);
    Fixture.Interactor->ClearTarget();
    Preview = Fixture.Look(41);
    TestNull(TEXT("Fresh acquisition does not extend into release band"), Preview.Target.Get());
    Preview = Fixture.Look(0);
    TestTrue(TEXT("Centered source acquired"), Preview.Target == Source);
    for (const float Yaw : {41.f, 44.f, 42.f, 45.f, 43.f, 41.f})
    {
        Preview = Fixture.Look(Yaw);
        TestTrue(FString::Printf(TEXT("Retained without flicker at %.0f degrees"), Yaw),
            Preview.Target == Source && Preview.bEligible);
    }
    Fixture.Interactor->ClearTarget();
    TestNull(TEXT("ClearTarget removes preview immediately"), Fixture.Interactor->GetCurrentPreview().Target.Get());
    TestNull(TEXT("ClearTarget removes retention history"), Fixture.Look(45).Target.Get());
    Fixture.Look(0);
    TestNull(TEXT("Turning clearly away releases target"), Fixture.Look(56).Target.Get());
    TestFalse(TEXT("No capture after target release"), Fixture.Interactor->RequestCapture().bSucceeded);
    TestTrue(TEXT("Rejected request preserves source Motion"), Source->Motion->HasMotionState());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionTargetSafetyTest,
    "Transmit.MotionTransfer.Targeting.OcclusionRangeAndBehindCamera",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMotionTargetSafetyTest::RunTest(const FString& Parameters)
{
    using namespace MotionTargetingTests;
    {
        FFixture Fixture;
        if (!TestNotNull(TEXT("Occlusion world"), Fixture.World)) return false;
        auto* Source = Fixture.AddSource(FVector(500, 0, 0));
        TestTrue(TEXT("Acquire before blocker"), Fixture.Look(0).bEligible);
        auto* Blocker = Fixture.World->SpawnActor<AActor>();
        auto* Box = NewObject<UBoxComponent>(Blocker);
        Blocker->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(40, 100, 100));
        Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        Box->RegisterComponent();
        Box->SetWorldLocation(FVector(250, 0, 0));
        const auto Preview = Fixture.Look(0);
        TestFalse(TEXT("Retained target never stays eligible through blocker"), Preview.bEligible);
        TestFalse(TEXT("Occluded capture rejected"), Fixture.Interactor->RequestCapture().bSucceeded);
        TestTrue(TEXT("Occluded source retains state"), Source->Motion->HasMotionState());
        TestFalse(TEXT("Player remains empty"), Fixture.Player->Motion->HasMotionState());
    }
    for (const FVector Location : {FVector(2500, 0, 0), FVector(-500, 0, 0)})
    {
        FFixture Fixture;
        if (!TestNotNull(TEXT("Range/behind world"), Fixture.World)) return false;
        auto* Source = Fixture.AddSource(Location);
        TestFalse(TEXT("Distant or behind source not eligible"), Fixture.Look(0).bEligible);
        TestFalse(TEXT("Distant or behind capture rejected"), Fixture.Interactor->RequestCapture().bSucceeded);
        TestTrue(TEXT("Rejected capture preserves source"), Source->Motion->HasMotionState());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionTargetSwitchAndBoundsTest,
    "Transmit.MotionTransfer.Targeting.SwitchPriorityAndPhysicalCenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMotionTargetSwitchAndBoundsTest::RunTest(const FString& Parameters)
{
    using namespace MotionTargetingTests;
    {
        FFixture Fixture;
        if (!TestNotNull(TEXT("Switch world"), Fixture.World)) return false;
        auto* Original = Fixture.AddSource(FVector(500, 0, 0), TEXT("Original"));
        TestTrue(TEXT("Initial source selected"), Fixture.Look(0).Target == Original);
        auto* Better = Fixture.AddSource(FRotator(0, 35, 0).Vector() * 500, TEXT("Better"));
        const auto Preview = Fixture.Look(35);
        TestTrue(TEXT("Centered candidate beats retained candidate by existing score margin"),
            Preview.Target == Better && Preview.bEligible);
        TestTrue(TEXT("Capture commits to replacement target"), Fixture.Interactor->RequestCapture().bSucceeded);
        TestFalse(TEXT("Replacement source emptied"), Better->Motion->HasMotionState());
        TestTrue(TEXT("Original source untouched"), Original->Motion->HasMotionState());
    }
    {
        FFixture Fixture;
        if (!TestNotNull(TEXT("Offset mesh world"), Fixture.World)) return false;
        auto* Source = Fixture.AddSource(FVector(0, 500, 0));
        auto* Body = Source->FindComponentByClass<UStaticMeshComponent>();
        if (!TestNotNull(TEXT("Visible body"), Body)) return false;
        Body->SetWorldLocation(FVector(500, 0, 0));
        const auto Preview = Fixture.Look(0);
        TestTrue(TEXT("Visible body selected although actor pivot is ninety degrees away"),
            Preview.Target == Source && Preview.bEligible);
        TestTrue(TEXT("Offset body capture succeeds"), Fixture.Interactor->RequestCapture().bSucceeded);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionCameraOffsetSafetyTest,
    "Transmit.MotionTransfer.Targeting.PossessedCameraPreservesRangeAndOcclusion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMotionCameraOffsetSafetyTest::RunTest(const FString& Parameters)
{
    using namespace MotionTargetingTests;
    for (const bool bRangeCase : {true, false})
    {
        FFixture Fixture(false);
        if (!TestNotNull(TEXT("Camera safety world"), Fixture.World)) return false;
        auto* Character = Fixture.World->SpawnActor<ATransmitCharacter>();
        auto* Controller = Fixture.World->SpawnActor<APlayerController>();
        auto* Camera = Fixture.World->SpawnActor<ACameraActor>();
        if (!TestNotNull(TEXT("Character"), Character)
            || !TestNotNull(TEXT("Controller"), Controller)
            || !TestNotNull(TEXT("Camera"), Camera)) return false;
        auto* Motion = NewObject<UMotionTransferComponent>(Character);
        Motion->RegisterComponent();
        Motion->ConfigureForTesting(TEXT("CameraPlayer"), true, true, EMotionEndpointMode::Store, {});
        auto* Interactor = NewObject<UMotionInteractorComponent>(Character);
        Interactor->RegisterComponent();
        Controller->Possess(Character);
        if (!TestTrue(TEXT("Controller possesses gameplay character"), Character->GetController() == Controller))
            return false;
        FVector Eyes;
        FRotator EyeRotation;
        Character->GetActorEyesViewPoint(Eyes, EyeRotation);
        const FVector SourceLocation = Eyes + FVector(bRangeCase ? 2100 : 500, 0, 0);
        auto* Source = Fixture.AddSource(SourceLocation);
        const FVector CameraLocation = Eyes + (bRangeCase ? FVector(400, 0, 0) : FVector(0, 300, 0));
        Camera->SetActorLocation(CameraLocation);
        Camera->SetActorRotation((SourceLocation - CameraLocation).Rotation());
        // Transient worlds have not initialized actors: PostInitializeComponents
        // may not have spawned the controller's camera manager. SetViewTarget is
        // a no-op without it. Install the same native manager explicitly.
        if (!Controller->PlayerCameraManager)
        {
            FActorSpawnParameters CameraSpawn;
            CameraSpawn.Owner = Controller;
            Controller->PlayerCameraManager = Fixture.World->SpawnActor<APlayerCameraManager>(CameraSpawn);
            if (!TestNotNull(TEXT("Native camera manager"), Controller->PlayerCameraManager.Get())) return false;
            Controller->PlayerCameraManager->InitializeFor(Controller);
        }
        // No local network Player exists in this fixture. Calculate POV locally
        // rather than waiting for client camera updates that cannot arrive.
        Controller->PlayerCameraManager->bUseClientSideCameraUpdates = false;
        Controller->SetViewTarget(Camera);
        Controller->PlayerCameraManager->UpdateCamera(0.0f);
        if (!TestTrue(TEXT("Controller view target is the camera"), Controller->GetViewTarget() == Camera)) return false;
        FVector ActualCamera;
        FRotator ActualRotation;
        Controller->GetPlayerViewPoint(ActualCamera, ActualRotation);
        if (!TestTrue(FString::Printf(TEXT("Real controller reports offset view target: actual %s expected %s cache time %.3f"),
                *ActualCamera.ToString(), *CameraLocation.ToString(), Controller->PlayerCameraManager->GetCameraCacheTime()),
                ActualCamera.Equals(CameraLocation, 0.1))
            || !TestTrue(TEXT("Camera aims directly at source"),
                ActualRotation.Vector().Equals((SourceLocation - CameraLocation).GetSafeNormal(), 0.001)))
            return false;
        TestTrue(TEXT("Source within camera-based range"), FVector::Dist(ActualCamera, SourceLocation) < 2000);
        if (!bRangeCase)
        {
            auto* Blocker = Fixture.World->SpawnActor<AActor>();
            auto* Box = NewObject<UBoxComponent>(Blocker);
            Blocker->SetRootComponent(Box);
            Box->SetBoxExtent(FVector(25, 50, 80));
            Box->SetCollisionProfileName(TEXT("BlockAllDynamic"));
            Box->RegisterComponent();
            Box->SetWorldLocation(Eyes + FVector(250, 0, 0));
            FCollisionQueryParams Query;
            Query.AddIgnoredActor(Character);
            Query.AddIgnoredActor(Source);
            FHitResult Hit;
            TestFalse(TEXT("Camera ray clears player-eye blocker"), Fixture.World->LineTraceSingleByChannel(
                Hit, ActualCamera, SourceLocation, ECC_Visibility, Query));
            TestTrue(TEXT("Player eye ray is blocked"), Fixture.World->LineTraceSingleByChannel(
                Hit, Eyes, SourceLocation, ECC_Visibility, Query));
        }
        else
        {
            TestTrue(TEXT("Source beyond player-eye range"), FVector::Dist(Eyes, SourceLocation) > 2000);
        }
        Interactor->RefreshTarget();
        TestFalse(bRangeCase ? TEXT("Camera offset cannot extend range") : TEXT("Camera cannot permit capture around player blocker"),
            Interactor->GetCurrentPreview().bEligible);
        TestFalse(TEXT("Capture rejected at commit"), Interactor->RequestCapture().bSucceeded);
        TestTrue(TEXT("Rejected capture preserves source"), Source->Motion->HasMotionState());
        TestFalse(TEXT("Player remains empty"), Motion->HasMotionState());
    }
    return true;
}
#endif
