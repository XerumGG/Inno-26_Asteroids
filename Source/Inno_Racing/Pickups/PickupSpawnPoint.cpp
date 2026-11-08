// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Pickups/PickupSpawnPoint.h"
#include "Engine/World.h"

APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	PickupClass = AHoverPickup::StaticClass();
	PickupType = EPickupType::FuelCell;
	ValueAmount = 20.0f;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (!PickupClass || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedPickup = GetWorld()->SpawnActor<AHoverPickup>(PickupClass, GetActorTransform(), SpawnParams);
	if (SpawnedPickup)
	{
		SpawnedPickup->PickupId = PickupId;
		SpawnedPickup->PickupType = PickupType;
		SpawnedPickup->ValueAmount = ValueAmount;
	}
}
