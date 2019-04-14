// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VRPlayerPawn.generated.h"

// Class forwarding
class UCameraComponent;
class UMotionControllerComponent;
class AVRMotionController;

UCLASS()
class VRSTARTERPACK_API AVRPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AVRPlayerPawn();

	/*
	*-------------Components-------------
	*/
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USceneComponent* VROrigin;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UCameraComponent* Camera;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UMotionControllerComponent* LeftMotionControllerComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UMotionControllerComponent* RightMotionControllerComponent;
	/*
	*-------------Components End---------
	*/

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	FName HMDName;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Settings")
	TSubclassOf<AVRMotionController> DefaultMotionControllerClass;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Settings")
	bool bIsDebug;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "References")
	AVRMotionController* LeftController;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = "References")
	AVRMotionController* RightController;

	UPROPERTY(BlueprintReadWrite, Category = "Teleportation")
	bool bIsTeleporting;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Teleportation")
	float FadeOutDuration = 0.1f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Teleportation")
	float FadeInDuration = 0.2f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Teleportation")
	FLinearColor TeleportFadeColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	void ExecuteTeleportation(AVRMotionController * UsedController);

	UFUNCTION()
	void TeleportFadeOutTimerElapsed(AVRMotionController * UsedController);

private:
	bool bShouldAttachMotionControllers;

	UPROPERTY(ReplicatedUsing = OnRep_HMDTransform)
	FTransform HMDTransform;

	UPROPERTY(ReplicatedUsing = OnRep_LeftControllerTransform)
	FTransform LeftControllerTransform;

	UPROPERTY(ReplicatedUsing = OnRep_RightControllerTransform)
	FTransform RightControllerTransform;

	UFUNCTION()
	void ReplicateVRComponents();

	UFUNCTION(Server, Reliable, WithValidation)
	void UpdateVRComponentTransforms(FTransform NewHMDTransform, FTransform NewLeftControllerTransform, FTransform NewRightControllerTransform);

	UFUNCTION()
	void OnRep_HMDTransform();

	UFUNCTION()
	void OnRep_LeftControllerTransform();

	UFUNCTION()
	void OnRep_RightControllerTransform();
};
