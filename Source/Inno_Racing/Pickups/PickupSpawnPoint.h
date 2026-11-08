// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inno_Racing/Pickups/HoverPickup.h"
#include "PickupSpawnPoint.generated.h"

class UBillboardComponent;

/**
 * Placed in race levels by designers to define pickup positions, types, and amounts.
 * Spawns the local AHoverPickup at BeginPlay.
 */
UCLASS()
class INNO_RACING_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	int32 PickupId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	EPickupType PickupType = EPickupType::FuelCell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float ValueAmount = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TSubclassOf<AHoverPickup> PickupClass;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<AHoverPickup> SpawnedPickup;
};
