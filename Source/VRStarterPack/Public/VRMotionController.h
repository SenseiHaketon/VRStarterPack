// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRMotionController.generated.h"

class UMotionControllerComponent;
class USphereComponent;
class USplineComponent;
class UArrowComponent;
class USplineMeshComponent;

UENUM(BlueprintType)
enum class EGripState : uint8
{
	GS_Open		UMETA(DisplayName = "Open"),
	GS_CanGrab	UMETA(DisplayName = "CanGrab"),
	GS_Grab		UMETA(DisplayName = "Grab")
};

UENUM(BlueprintType)
enum class EHandPose : uint8
{
	HP_Default			UMETA(DisplayName = "Open")
};

UCLASS()
class VRSTARTERPACK_API AVRMotionController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVRMotionController();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UMotionControllerComponent* MotionControllerComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USkeletalMeshComponent* HandMesh;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UArrowComponent* TeleportArcDirection;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USphereComponent* GrabSphere;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USplineComponent* TeleportArcSpline;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* TeleportArcEndPoint;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* TeleportMarker;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* TeleportDestinationMarker;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Replicated, Category = "Settings")
	FName Hand;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Animation")
	EGripState GripState = EGripState::GS_Open;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Animation")
	EHandPose HandPose;

	UPROPERTY(BlueprintReadWrite, Category = "Motion Controller")
	AActor* AttachedActor;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Motion Controller")
	bool bWantsToGrip;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "Motion Controller")
	bool bTeleporterActive;

	UPROPERTY(BlueprintReadWrite, Category = "Teleportation")
	bool bValidTeleportDestination;

	UPROPERTY(BlueprintReadWrite, Category = "Teleportation")
	TArray<USplineMeshComponent*> SplineMeshes;

	UStaticMesh* TeleportSplineMesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	UMaterialInterface* SplineArcMat;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	AActor* GetActorNearHand();

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void SetGripState(EGripState NewGripState);			

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void SetHandPose(EHandPose NewHandPose);

	// Input
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Grabbing")
	void GrabActor();
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Grabbing")
	void ReleaseActor();
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Teleportation")
	void ActivateTeleporter();
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Teleportation")
	void DisableTeleporter();
	
	void ClearArc();
	bool TraceTeleportDestination(TArray<FVector>& TracePoints, FVector & NavMeshLocation, FVector & TraceLocation);
	
	void UpdateArcSpline(bool FoundValidLocation, TArray<FVector> SplinePoints);
	void UpdateArcEndPoint(bool FoundValidLocation, FVector NewLocation);

	void CreateSpline(TArray<FVector> SplinePoints);
	void GetTeleportDestination(FVector& OutLocation, FRotator& OutRotation);
};
