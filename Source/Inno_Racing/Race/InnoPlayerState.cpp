// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Race/InnoPlayerState.h"
#include "Net/UnrealNetwork.h"

AInnoPlayerState::AInnoPlayerState()
{
	bReplicates = true;
	ParticipantName = TEXT("Pilot");
	SelectedCarId = FName(TEXT("AeroViper"));
	bIsReady = false;
	bHasLoadedMap = false;
	LastPassedCheckpoint = -1;
	ParticipantStatus = EParticipantStatus::Racing;
	FinishTimeMs = 0;
	SeriesRaceWins = 0;
	SeriesTotalTimeMs = 0;
}

void AInnoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInnoPlayerState, ParticipantName);
	DOREPLIFETIME(AInnoPlayerState, SelectedCarId);
	DOREPLIFETIME(AInnoPlayerState, bIsReady);
	DOREPLIFETIME(AInnoPlayerState, bHasLoadedMap);
	DOREPLIFETIME(AInnoPlayerState, LastPassedCheckpoint);
	DOREPLIFETIME(AInnoPlayerState, ParticipantStatus);
	DOREPLIFETIME(AInnoPlayerState, FinishTimeMs);
	DOREPLIFETIME(AInnoPlayerState, SeriesRaceWins);
	DOREPLIFETIME(AInnoPlayerState, SeriesTotalTimeMs);
}

void AInnoPlayerState::CopyProperties(APlayerState* NewPlayerState)
{
	Super::CopyProperties(NewPlayerState);

	if (AInnoPlayerState* InnoPS = Cast<AInnoPlayerState>(NewPlayerState))
	{
		InnoPS->ParticipantName = ParticipantName;
		InnoPS->SelectedCarId = SelectedCarId;
		InnoPS->SeriesRaceWins = SeriesRaceWins;
		InnoPS->SeriesTotalTimeMs = SeriesTotalTimeMs;
		// Reset per-race transient properties
		InnoPS->bHasLoadedMap = false;
		InnoPS->LastPassedCheckpoint = -1;
		InnoPS->ParticipantStatus = EParticipantStatus::Racing;
		InnoPS->FinishTimeMs = 0;
	}
}
