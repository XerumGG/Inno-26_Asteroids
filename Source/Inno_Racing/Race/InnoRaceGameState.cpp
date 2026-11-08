// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Race/InnoRaceGameState.h"
#include "Net/UnrealNetwork.h"

AInnoRaceGameState::AInnoRaceGameState()
{
	bReplicates = true;
	MatchPhase = EMatchPhase::Lobby;
	StartServerTime = 0.0;
	CurrentRaceIndex = 0;
	TotalRacesInPlaylist = 1;
	GraceDeadlineServerTime = 0.0;
}

void AInnoRaceGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInnoRaceGameState, MatchPhase);
	DOREPLIFETIME(AInnoRaceGameState, StartServerTime);
	DOREPLIFETIME(AInnoRaceGameState, CurrentRaceIndex);
	DOREPLIFETIME(AInnoRaceGameState, TotalRacesInPlaylist);
	DOREPLIFETIME(AInnoRaceGameState, GraceDeadlineServerTime);
}

void AInnoRaceGameState::OnRep_MatchPhase()
{
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}
