// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "InnoMatchContextSubsystem.generated.h"

/**
 * GameInstance Subsystem surviving all map travel.
 * Retains local player identity, selected vehicle, active playlist config,
 * and the last authoritative match summary for offline Results view.
 */
UCLASS()
class INNO_RACING_API UInnoMatchContextSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintReadWrite, Category = "Inno|MatchContext")
	FString LocalParticipantName = TEXT("Pilot");

	UPROPERTY(BlueprintReadWrite, Category = "Inno|MatchContext")
	FName SelectedCarId = FName(TEXT("AeroViper"));

	UPROPERTY(BlueprintReadWrite, Category = "Inno|MatchContext")
	FString ActiveMatchMode = TEXT("B");

	UPROPERTY(BlueprintReadWrite, Category = "Inno|MatchContext")
	FInnoMatchSummary LastMatchSummary;

	// Generates connection URL options string for seamless / direct travel
	UFUNCTION(BlueprintPure, Category = "Inno|MatchContext")
	FString GetTravelURLOptions() const;
};
