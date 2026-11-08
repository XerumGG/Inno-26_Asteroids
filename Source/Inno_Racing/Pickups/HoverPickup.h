// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HoverPickup.generated.h"

class UStaticMeshComponent;
class URotatingMovementComponent;

UENUM(BlueprintType)
enum class EPickupType : uint8
{
	FuelCell UMETA(DisplayName = "Fuel Cell (+20)"),
	BoostOrb UMETA(DisplayName = "Boost Orb (+1 Charge)")
};

/**
 * Local track pickup actor (Fuel Cell or Boost Orb).
 * Non-replicated, collectable once per race per player.
 */
UCLASS()
class INNO_RACING_API AHoverPickup : public AActor
{
	GENERATED_BODY()

public:
	AHoverPickup();

	// Unique ID per track for validation and one-time collection
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	int32 PickupId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	EPickupType PickupType = EPickupType::FuelCell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float ValueAmount = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

	UFUNCTION(BlueprintPure, Category = "Pickup")
	bool HasBeenCollected() const { return bCollected; }

	// Resets collection status for a new race
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void ResetPickup();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	bool bCollected = false;
};
