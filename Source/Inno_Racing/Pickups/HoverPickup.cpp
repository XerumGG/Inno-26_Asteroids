// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Pickups/HoverPickup.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Inno_Racing/Hover/HoverFuelComponent.h"
#include "Inno_Racing/Hover/HoverBoostComponent.h"

AHoverPickup::AHoverPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false; // Local actors only!

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	RootComponent = PickupMesh;
	PickupMesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PickupMesh->SetGenerateOverlapEvents(true);

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);

	PickupType = EPickupType::FuelCell;
	ValueAmount = 20.0f;
	bCollected = false;
}

void AHoverPickup::BeginPlay()
{
	Super::BeginPlay();
	PickupMesh->OnComponentBeginOverlap.AddDynamic(this, &AHoverPickup::OnOverlapBegin);
}

void AHoverPickup::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bCollected || !OtherActor)
	{
		return;
	}

	// Verify it is the local player pawn
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	bool bApplied = false;

	if (PickupType == EPickupType::FuelCell)
	{
		if (UHoverFuelComponent* FuelComp = Pawn->FindComponentByClass<UHoverFuelComponent>())
		{
			FuelComp->AddFuel(ValueAmount);
			bApplied = true;
		}
	}
	else if (PickupType == EPickupType::BoostOrb)
	{
		if (UHoverBoostComponent* BoostComp = Pawn->FindComponentByClass<UHoverBoostComponent>())
		{
			BoostComp->TriggerInstantBoost();
			bApplied = true;
		}
	}

	if (bApplied)
	{
		bCollected = true;
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
}

void AHoverPickup::ResetPickup()
{
	bCollected = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
}
