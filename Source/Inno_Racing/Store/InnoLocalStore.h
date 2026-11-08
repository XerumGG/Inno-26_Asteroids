// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "InnoLocalStore.generated.h"

/**
 * Local Data Storage subsystem.
 * Saves results to Saved/InnoRace/results.jsonl, manages participants.json,
 * and exports combined race spreadsheets to Saved/InnoRace/Exports/*.csv.
 */
UCLASS()
class INNO_RACING_API UInnoLocalStore : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Appends a completed match summary to local JSON-lines store and updates rolling backup
	UFUNCTION(BlueprintCallable, Category = "Inno|Store")
	bool RecordMatchSummary(const FInnoMatchSummary& Summary);

	// Exports all recorded match results to a CSV file for organizers
	UFUNCTION(BlueprintCallable, Category = "Inno|Store")
	FString ExportResultsToCSV();

protected:
	FString StorageDir;
	FString JsonlPath;
	FString BackupPath;
	FString ExportsDir;
};
