// Fill out your copyright notice in the Description page of Project Settings.

#include "VRMotionController.h"
#include "UnrealNetwork.h"
#include "VRPlayerPawn.h"
#include "MotionControllerComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SplineMeshComponent.h"
#include "ConstructorHelpers.h"
#include "PickupActorInterface.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"

// Sets default values
AVRMotionController::AVRMotionController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	MotionControllerComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftMotionController"));
	MotionControllerComponent->SetupAttachment(Root);
	MotionControllerComponent->PlayerIndex = -1;
	MotionControllerComponent->bDisableLowLatencyUpdate = false;

	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	HandMesh->SetupAttachment(MotionControllerComponent);

	// Get referneces to the Static Meshes
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshObj(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshObj(TEXT("StaticMesh'/Game/VRTemplate/Meshes/SM_FatCylinder.SM_FatCylinder'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> TeleportSplineMeshObj(TEXT("StaticMesh'/Game/VRTemplate/Meshes/BeamMesh.BeamMesh'"));

	if(TeleportSplineMeshObj.Succeeded())
		TeleportSplineMesh = TeleportSplineMeshObj.Object;

	// Get referneces to the Materials
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArcEndPointMat(TEXT("Material'/Game/VRTemplate/Materials/M_ArcEndpoint.M_ArcEndpoint'"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SplineArcMatObj(TEXT("Material'/Game/VRTemplate/Materials/M_SplineArcMat.M_SplineArcMat'"));

	if (SplineArcMatObj.Succeeded())
		SplineArcMat = SplineArcMatObj.Object;

	// Create and Attach the Grab Sphere
	GrabSphere = CreateDefaultSubobject<USphereComponent>(TEXT("GrabSphere"));
	GrabSphere->SetupAttachment(HandMesh);
	GrabSphere->SetRelativeLocation(FVector(14.3f, 0.2f, 1.48f));
	GrabSphere->SetSphereRadius(10.0f);

	// Create and Attach the ArcDirection Arrow Component
	TeleportArcDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ArcDirection"));
	TeleportArcDirection->SetupAttachment(HandMesh);

	// Create and Attach the Spline for Teleportation
	TeleportArcSpline = CreateDefaultSubobject<USplineComponent>(TEXT("ArcSpline"));
	TeleportArcSpline->SetupAttachment(HandMesh);
	//TeleportArcSpline->SetRelativeLocation(FVector(12.5f, -1.76f, 2.6f));

	// Create and Attach the ArcEndPoint Mesh, used for Teleportation
	TeleportArcEndPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArcEndPoint"));
	TeleportArcEndPoint->SetupAttachment(RootComponent);
	TeleportArcEndPoint->SetRelativeScale3D(FVector(0.15f));
	TeleportArcEndPoint->SetVisibility(false);
	TeleportArcEndPoint->SetIsReplicated(true);
	if (SphereMeshObj.Succeeded())
	{
		TeleportArcEndPoint->SetStaticMesh(SphereMeshObj.Object);
	}
	if (ArcEndPointMat.Succeeded())
	{
		TeleportArcEndPoint->SetMaterial(0, ArcEndPointMat.Object);
	}

	// Create and Attach the TeleportMarker Mesh, used for Teleportation
	TeleportDestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportDestinationMarker"));
	//TeleportDestinationMarker->SetupAttachment(RootComponent);

	TeleportDestinationMarker->SetRelativeScale3D(FVector(0.25f));
	TeleportDestinationMarker->SetAbsolute(true, true, false);
	TeleportDestinationMarker->SetVisibility(false);
	TeleportDestinationMarker->SetIsReplicated(true);
	if (CylinderMeshObj.Succeeded())
	{
		TeleportDestinationMarker->SetStaticMesh(CylinderMeshObj.Object);
	}
}

void AVRMotionController::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVRMotionController, Hand);
	DOREPLIFETIME(AVRMotionController, GripState);
	DOREPLIFETIME(AVRMotionController, HandPose);
	DOREPLIFETIME(AVRMotionController, bWantsToGrip);
	DOREPLIFETIME(AVRMotionController, bTeleporterActive);
}

// Called when the game starts or when spawned
void AVRMotionController::BeginPlay()
{
	Super::BeginPlay();

	if (Hand == TEXT("Left"))
	{
		HandMesh->SetWorldScale3D(FVector(1.0f, 1.0f, -1.0f));
	}
}

void AVRMotionController::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (AttachedActor)
		ReleaseActor();
}

// Called every frame
void AVRMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* lActorNearHand = GetActorNearHand();

	if (AttachedActor != nullptr || bWantsToGrip)
	{
		GripState = EGripState::GS_Grab;
	}
	else if (lActorNearHand != nullptr)
	{
		GripState = EGripState::GS_CanGrab;
	}
	else
	{
		GripState = EGripState::GS_Open;
	}


	// Only let hand collide with environment while gripping
	switch (GripState)
	{
	case EGripState::GS_Open:
		HandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EGripState::GS_CanGrab:
		HandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EGripState::GS_Grab:
		HandMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}


	// Handle Teleportation Arc
	ClearArc();

	if (bTeleporterActive)
	{
		// Check if the teleport destination is valid
		TArray<FVector> lTracePoints;
		FVector lNavMeshLocation;
		FVector lTraceLocation;
		bValidTeleportDestination = TraceTeleportDestination(lTracePoints, lNavMeshLocation, lTraceLocation);

		FCollisionQueryParams lTraceParams = FCollisionQueryParams(FName(TEXT("Teleport_Trace")), false, this);
		lTraceParams.bTraceComplex = false;
		lTraceParams.bReturnPhysicalMaterial = false;

		FHitResult lOutHit(ForceInit);

		FCollisionObjectQueryParams lObjectParams = FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_WorldStatic));

		GetWorld()->LineTraceSingleByObjectType(
			lOutHit,
			lNavMeshLocation,
			lNavMeshLocation + FVector(0.0f, 0.0f, -200.0f),
			lObjectParams,
			lTraceParams
		);

		TeleportDestinationMarker->SetVisibility(true);
		TeleportDestinationMarker->SetWorldLocation(lOutHit.bBlockingHit ? lOutHit.ImpactPoint : lNavMeshLocation, false, nullptr, ETeleportType::TeleportPhysics);
		TeleportDestinationMarker->SetWorldRotation(FRotator(0.0f, 0.0f, 0.0f));

		UpdateArcSpline(bValidTeleportDestination, lTracePoints);
		UpdateArcEndPoint(bValidTeleportDestination, lTraceLocation);
	}
}

AActor* AVRMotionController::GetActorNearHand()
{
	float lNearestOverlap = 9999999.0f;
	AActor* lNearestOverlappingActor = nullptr;

	TArray<AActor*> lOverlappingActors;
	GrabSphere->GetOverlappingActors(lOverlappingActors, AActor::StaticClass());

	for (auto lActor : lOverlappingActors)
	{
		if (lActor->GetClass()->ImplementsInterface(UPickupActorInterface::StaticClass()))
		{
			if (IPickupActorInterface::Execute_CanBeGrabbed(lActor))
			{
				float CurrentDistance = (lActor->GetActorLocation() - GrabSphere->GetComponentLocation()).Size();
				if (CurrentDistance < lNearestOverlap)
				{
					lNearestOverlappingActor = lActor;
					lNearestOverlap = CurrentDistance;
				}
			}
		}
	}

	return lNearestOverlappingActor;
}

void AVRMotionController::GetTeleportDestination(FVector& OutLocation, FRotator& OutRotation)
{
	FRotator lDeviceRotation;
	FVector lDeviceLocation;

	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(lDeviceRotation, lDeviceLocation);

	OutRotation = FRotator(0.0f, 0.0f, 0.0f);
	OutLocation = TeleportDestinationMarker->GetComponentLocation() - OutRotation.RotateVector(FVector(lDeviceLocation.X, lDeviceLocation.Y, 0.0f));
}

void AVRMotionController::GrabActor_Implementation()
{
}

bool AVRMotionController::GrabActor_Validate()
{
	return true;
}

void AVRMotionController::ReleaseActor_Implementation()
{
}

bool AVRMotionController::ReleaseActor_Validate()
{
	return true;
}

void AVRMotionController::ActivateTeleporter_Implementation()
{
	bTeleporterActive = true;
}

bool AVRMotionController::ActivateTeleporter_Validate()
{
	return true;
}

void AVRMotionController::DisableTeleporter_Implementation()
{
	if (!bTeleporterActive)
		return;

	bTeleporterActive = false;
	TeleportArcEndPoint->SetVisibility(false);
	TeleportDestinationMarker->SetVisibility(false);
}

bool AVRMotionController::DisableTeleporter_Validate()
{
	return true;
}

void AVRMotionController::ClearArc()
{
	for (auto lSplineMesh : SplineMeshes)
	{
		lSplineMesh->DestroyComponent();
	}
	SplineMeshes.Empty();
	TeleportArcSpline->ClearSplinePoints();
}

bool AVRMotionController::TraceTeleportDestination(TArray<FVector>& TracePoints, FVector& NavMeshLocation, FVector& TraceLocation)
{
	FHitResult lOutHit(ForceInit);
	TArray<FVector> lOutPathPositions;
	FVector lOutLastTraceDestination;

	FVector lLaunchVelocity = TeleportArcDirection->GetForwardVector() * 900.0f;

	TArray<TEnumAsByte<EObjectTypeQuery>> lObjectTypes;
	lObjectTypes.Add(EObjectTypeQuery::ObjectTypeQuery1);

	TArray<AActor*> lActorsToIgnore;

	if (!UGameplayStatics::Blueprint_PredictProjectilePath_ByObjectType(this, lOutHit, lOutPathPositions, lOutLastTraceDestination, TeleportArcDirection->GetComponentLocation(), lLaunchVelocity, true, 0.0f, lObjectTypes, false, lActorsToIgnore, EDrawDebugTrace::None, 0.0f, 30.0f))
		return false;

	FNavLocation lNavLoc;

	UWorld* lWorld = GetWorld();
	if (!lWorld)
		return false;

	UNavigationSystemBase* lNavBase = lWorld->GetNavigationSystem();
	if (!lNavBase)
		return false;

	if (!Cast<UNavigationSystemV1>(lNavBase)->ProjectPointToNavigation(lOutHit.Location, lNavLoc, FVector(500.0f)))
		return false;


	TracePoints = lOutPathPositions;
	NavMeshLocation = lNavLoc.Location;
	TraceLocation = lOutHit.Location;

	return true;
}

void AVRMotionController::UpdateArcSpline(bool FoundValidLocation, TArray<FVector> SplinePoints)
{
	if (FoundValidLocation)
	{
		CreateSpline(SplinePoints);
	}
	else
	{
		// Create Small Stub line when we failed to find a teleport location
		TArray<FVector> lNewSplinePoints;
		lNewSplinePoints.Add(TeleportArcDirection->GetComponentLocation());
		lNewSplinePoints.Add(TeleportArcDirection->GetComponentLocation() + (TeleportArcDirection->GetForwardVector() * 20.0f));

		CreateSpline(lNewSplinePoints);
	}
}

void AVRMotionController::UpdateArcEndPoint(bool FoundValidLocation, FVector NewLocation)
{
	TeleportArcEndPoint->SetVisibility(FoundValidLocation && bTeleporterActive);
	TeleportArcEndPoint->SetWorldLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

	FRotator lHmdOrientation;
	FVector lHmdLocation;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(lHmdOrientation, lHmdLocation);
}

void AVRMotionController::CreateSpline(TArray<FVector> SplinePoints)
{
	for (auto lPoint : SplinePoints)
	{
		// Build a spline from all trace points. This generates tangents we can use to build a smoothly curved spline mesh.
		TeleportArcSpline->AddSplinePoint(lPoint, ESplineCoordinateSpace::Local);
	}

	// Update the point type to create the curve
	TeleportArcSpline->SetSplinePointType(SplinePoints.Num() - 1, ESplinePointType::CurveClamped);

	for (int x = 0; x <= TeleportArcSpline->GetNumberOfSplinePoints() - 2; x++)
	{
		// Add new Cylinder Mesh

		USplineMeshComponent* lSplineMesh = NewObject<USplineMeshComponent>(this);
		if (!lSplineMesh)
			return;

		lSplineMesh->RegisterComponent();
		lSplineMesh->SetMobility(EComponentMobility::Movable);


		if (TeleportSplineMesh)
			lSplineMesh->SetStaticMesh(TeleportSplineMesh);
		else
			UE_LOG(LogTemp, Error, TEXT("BeamMesh not found!"));

		if (SplineArcMat)
			lSplineMesh->SetMaterial(0, SplineArcMat);
		else
			UE_LOG(LogTemp, Error, TEXT("SplineArcMat not found!"));

		SplineMeshes.Add(lSplineMesh);

		lSplineMesh->SetStartAndEnd(SplinePoints[x], TeleportArcSpline->GetTangentAtSplinePoint(x, ESplineCoordinateSpace::Local), SplinePoints[x + 1], TeleportArcSpline->GetTangentAtSplinePoint(x + 1, ESplineCoordinateSpace::Local));
	}
}

void AVRMotionController::SetGripState_Implementation(EGripState NewGripState)
{
	GripState = NewGripState;
}

bool AVRMotionController::SetGripState_Validate(EGripState NewGripState)
{
	return true;
}

void AVRMotionController::SetHandPose_Implementation(EHandPose NewHandPose)
{
	HandPose = NewHandPose;
}

bool AVRMotionController::SetHandPose_Validate(EHandPose NewHandPose)
{
	return true;
}