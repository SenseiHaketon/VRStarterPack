// Fill out your copyright notice in the Description page of Project Settings.

#include "VRPlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MotionControllerComponent.h"

#include "ConstructorHelpers.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

#include "VRMotionController.h"
#include "TimerManager.h"

// Sets default values
AVRPlayerPawn::AVRPlayerPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	MinNetUpdateFrequency = 60.0f;

	/*
	*-------------Component Creation-------------
	*/

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	RootComponent = VROrigin;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);

	// Create and Attach the Motion Controller Components
	LeftMotionControllerComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftMotionController"));
	LeftMotionControllerComponent->SetupAttachment(VROrigin);
	LeftMotionControllerComponent->PlayerIndex = -1;
	LeftMotionControllerComponent->MotionSource = TEXT("Left");
	LeftMotionControllerComponent->bDisableLowLatencyUpdate = false;

	RightMotionControllerComponent = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightMotionController"));
	RightMotionControllerComponent->SetupAttachment(VROrigin);
	RightMotionControllerComponent->PlayerIndex = -1;
	RightMotionControllerComponent->MotionSource = TEXT("Right");
	RightMotionControllerComponent->bDisableLowLatencyUpdate = false;

	/*
	*-------------Component Creation End---------
	*/

}

void AVRPlayerPawn::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AVRPlayerPawn, LeftController);
	DOREPLIFETIME(AVRPlayerPawn, RightController);
	DOREPLIFETIME_CONDITION(AVRPlayerPawn, HMDTransform, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AVRPlayerPawn, LeftControllerTransform, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AVRPlayerPawn, RightControllerTransform, COND_SkipOwner);
}

// Called when the game starts or when spawned
void AVRPlayerPawn::BeginPlay()
{
	Super::BeginPlay();
	
	HMDName = UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName();

	// Set the player index for the controllers to 0 if this is the local player, so they can get input
	if (IsLocallyControlled())
	{
		LeftMotionControllerComponent->PlayerIndex = 0;
		RightMotionControllerComponent->PlayerIndex = 0;
	}

	// Spawn and attach both VRMotionControllers
	if (HasAuthority())
	{
		if (DefaultMotionControllerClass == nullptr)
		{
			DefaultMotionControllerClass = AVRMotionController::StaticClass();
			// TODO: Change Log Category
			UE_LOG(LogTemp, Warning, TEXT("Default Motion Controller class not set. Using 'AVRMotionController' now."));
		}

		FActorSpawnParameters lSpawnParameters;
		lSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		LeftController = Cast<AVRMotionController>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, DefaultMotionControllerClass, FTransform(FRotator(0.0f), FVector(0.0f), FVector(1.0f)), ESpawnActorCollisionHandlingMethod::AlwaysSpawn, this));
		if (LeftController)
		{
			LeftController->Hand = TEXT("Left");
			UGameplayStatics::FinishSpawningActor(LeftController, FTransform(FRotator(0.0f), FVector(0.0f), FVector(1.0f)));
		}

		RightController = Cast<AVRMotionController>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, DefaultMotionControllerClass, FTransform(FRotator(0.0f), FVector(0.0f), FVector(1.0f)), ESpawnActorCollisionHandlingMethod::AlwaysSpawn, this));
		if (RightController)
		{
			RightController->Hand = TEXT("Right");
			UGameplayStatics::FinishSpawningActor(RightController, FTransform(FRotator(0.0f), FVector(0.0f), FVector(1.0f)));
		}
	}
	else if (!IsLocallyControlled())
		return;
	
	bShouldAttachMotionControllers = true;
}

void AVRPlayerPawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// Make sure to destroy the motion controllers when this actor gets destroyed.
	LeftController->Destroy();
	RightController->Destroy();
}

// Called every frame
void AVRPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShouldAttachMotionControllers && LeftController && RightController && !bIsDebug)
	{
		bShouldAttachMotionControllers = false;
		FAttachmentTransformRules lAttachmentTransformRules = FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
		LeftController->AttachToComponent(LeftMotionControllerComponent, lAttachmentTransformRules);
		RightController->AttachToComponent(RightMotionControllerComponent, lAttachmentTransformRules);
	}

	if (IsLocallyControlled())
	{
		if (LeftController && RightController)
			ReplicateVRComponents();
	}
}

// Called to bind functionality to input
void AVRPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AVRPlayerPawn::ReplicateVRComponents()
{
	UpdateVRComponentTransforms(Camera->GetComponentTransform(), LeftController->MotionControllerComponent->GetComponentTransform(), RightController->MotionControllerComponent->GetComponentTransform());
}

void AVRPlayerPawn::UpdateVRComponentTransforms_Implementation(FTransform NewHMDTransform, FTransform NewLeftControllerTransform, FTransform NewRightControllerTransform)
{
	HMDTransform = NewHMDTransform;
	LeftControllerTransform = NewLeftControllerTransform;
	RightControllerTransform = NewRightControllerTransform;

	OnRep_HMDTransform();
	OnRep_LeftControllerTransform();
	OnRep_RightControllerTransform();
}

bool AVRPlayerPawn::UpdateVRComponentTransforms_Validate(FTransform NewHMDTransform, FTransform NewLeftControllerTransform, FTransform NewRightControllerTransform)
{
	return true;
}

void AVRPlayerPawn::OnRep_HMDTransform()
{
	if (IsLocallyControlled() || !Camera)
		return;

	Camera->SetWorldLocationAndRotation(HMDTransform.GetLocation(), HMDTransform.GetRotation());
}

void AVRPlayerPawn::OnRep_LeftControllerTransform()
{
	if (IsLocallyControlled() || !LeftController)
		return;

	LeftController->MotionControllerComponent->SetWorldLocationAndRotation(LeftControllerTransform.GetLocation(), LeftControllerTransform.GetRotation());
}

void AVRPlayerPawn::OnRep_RightControllerTransform()
{
	if (IsLocallyControlled() || !RightController)
		return;

	RightController->MotionControllerComponent->SetWorldLocationAndRotation(RightControllerTransform.GetLocation(), RightControllerTransform.GetRotation());
}

void AVRPlayerPawn::ExecuteTeleportation(AVRMotionController* UsedController)
{
	if (!bIsTeleporting)
	{
		if (UsedController->bValidTeleportDestination)
		{
			bIsTeleporting = true;
			if (APlayerController* lPlayerController = Cast<APlayerController>(GetController()))
			{
				if (lPlayerController->IsLocalController())
				{
					lPlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, FadeOutDuration, TeleportFadeColor, false, true);
				}

				FTimerDelegate lTimerDel;
				lTimerDel.BindUFunction(this, FName("TeleportFadeOutTimerElapsed"), UsedController);

				FTimerHandle lFadeOutTimer;
				GetWorldTimerManager().SetTimer(lFadeOutTimer, lTimerDel, FadeOutDuration, false);
			}
		}
		else
		{
			UsedController->DisableTeleporter();
		}
	}
}

void AVRPlayerPawn::TeleportFadeOutTimerElapsed(AVRMotionController* UsedController)
{
	UsedController->DisableTeleporter();
	FVector lOutLocation;
	FRotator lOutRotation;

	UsedController->GetTeleportDestination(lOutLocation, lOutRotation);

	TeleportTo(lOutLocation, GetActorRotation());

	if (APlayerController* lPlayerController = Cast<APlayerController>(GetController()))
	{
		if (lPlayerController->IsLocalController())
		{
			lPlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeInDuration, TeleportFadeColor, false, true);
		}
		bIsTeleporting = false;
	}
}