// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "InnoPlayerState.generated.h"

/**
 * Replicated player state carrying identity, readiness, checkpoint progress,
 * and accumulated series statistics across seamless travel.
 */
UCLASS()
class INNO_RACING_API AInnoPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AInnoPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* NewPlayerState) override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Player")
	FString ParticipantName;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Player")
	FName SelectedCarId;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Lobby")
	bool bIsReady = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Lobby")
	bool bHasLoadedMap = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Race")
	int32 LastPassedCheckpoint = -1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Race")
	EParticipantStatus ParticipantStatus = EParticipantStatus::Racing;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Race")
	int64 FinishTimeMs = 0;

	// Accumulated series statistics for Type A
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Series")
	int32 SeriesRaceWins = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Series")
	int64 SeriesTotalTimeMs = 0;
};
