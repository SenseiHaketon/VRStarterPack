// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PickupActorInterface.generated.h"

/**
 * 
 */
UINTERFACE(BlueprintType)
class VRSTARTERPACK_API UPickupActorInterface : public UInterface
{
	GENERATED_BODY()
	
};

class VRSTARTERPACK_API IPickupActorInterface
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Pickup")
	void Pickup(USceneComponent* AttachTo);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Pickup")
	void Drop(USceneComponent* DetachFrom);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pickup")
	bool CanBeGrabbed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pickup")
	bool CanBeDropped();
};