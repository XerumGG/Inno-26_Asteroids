// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "InnoRaceGameMode.generated.h"

class AHoverCarPawn;
class AInnoPlayerController;
class AInnoRaceGameState;

/**
 * Host-only authoritative GameMode for race tracks (L_Track1 to L_Track4).
 * Orchestrates: LoadGate -> Countdown -> Racing -> Finishing -> Results -> Playlist Intermission -> Detach.
 */
UCLASS()
class INNO_RACING_API AInnoRaceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AInnoRaceGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Tick(float DeltaSeconds) override;

	// Called when a player car crosses the finish line
	UFUNCTION(BlueprintCallable, Category = "Inno|Race")
	void OnPlayerCrossedFinishLine(AInnoPlayerController* PC, int64 CrossingTimeMs);

	// Client reports that map loading is finished
	void OnPlayerReportedLoaded(AInnoPlayerController* PC);

protected:
	void AdvanceToPhase(EMatchPhase NewPhase);
	void CheckLoadGate();
	void StartCountdown();
	void StartRacing();
	void CheckGracePeriod(float DeltaSeconds);
	void FinalizeRace();
	void CompleteMatch();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Timing")
	float CountdownSec = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Timing")
	float GracePeriodSec = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Timing")
	float IntermissionSec = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Timing")
	float LoadGateTimeoutSec = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inno|Timing")
	int64 TrackTimeLimitMs = 180000; // 3 minutes

	float PhaseTimer = 0.0f;
	bool bFirstFinisherArrived = false;
	FGuid CurrentMatchId;

	TArray<FInnoRaceResult> CompletedRaceResults;
	TArray<FInnoRaceResultEntry> CurrentRaceEntries;
};
