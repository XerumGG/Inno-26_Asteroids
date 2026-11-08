// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Core/InnoMatchContextSubsystem.h"

void UInnoMatchContextSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LocalParticipantName = TEXT("Pilot");
	SelectedCarId = FName(TEXT("AeroViper"));
	ActiveMatchMode = TEXT("B");
}

FString UInnoMatchContextSubsystem::GetTravelURLOptions() const
{
	return FString::Printf(TEXT("?Name=%s?Car=%s?Mode=%s?BuildId=1.0.0"),
		*LocalParticipantName,
		*SelectedCarId.ToString(),
		*ActiveMatchMode);
}
