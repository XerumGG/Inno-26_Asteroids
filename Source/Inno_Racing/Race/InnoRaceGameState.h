// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"
#include "InnoRaceGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChangedSignature, EMatchPhase, NewPhase);

/**
 * Replicated GameState. The authoritative single source that UI watches.
 */
UCLASS()
class INNO_RACING_API AInnoRaceGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AInnoRaceGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Inno|Match")
	EMatchPhase MatchPhase = EMatchPhase::Lobby;

	// Synced host server time at which countdown reaches GO
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Timing")
	double StartServerTime = 0.0;

	// Current race index in the playlist (0 to N-1)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Match")
	int32 CurrentRaceIndex = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Match")
	int32 TotalRacesInPlaylist = 1;

	// Host server time deadline for finishing grace period
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inno|Timing")
	double GraceDeadlineServerTime = 0.0;

	UFUNCTION()
	void OnRep_MatchPhase();

	UPROPERTY(BlueprintAssignable, Category = "Inno|Events")
	FOnMatchPhaseChangedSignature OnMatchPhaseChanged;
};
