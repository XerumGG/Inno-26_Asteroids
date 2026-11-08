// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InnoRaceTypes.generated.h"

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	Lobby,
	LoadGate,
	Countdown,
	Racing,
	Finishing,
	RaceResults,
	Intermission,
	MatchComplete,
	Detach
};

UENUM(BlueprintType)
enum class EParticipantStatus : uint8
{
	Racing,
	Finished,
	DNF,
	Disconnected
};

UENUM(BlueprintType)
enum class EInnoScreen : uint8
{
	None,
	MainMenu,
	NameEntry,
	CarSelection,
	Searching,
	MatchLobby,
	Loading,
	Countdown,
	RaceHUD,
	PlacementBanner,
	Intermission,
	Results,
	Leaderboard,
	Settings,
	Credits,
	ConnectionLost
};

USTRUCT(BlueprintType)
struct FInnoRaceResultEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	FName CarId;

	UPROPERTY(BlueprintReadOnly)
	EParticipantStatus Status = EParticipantStatus::Racing;

	UPROPERTY(BlueprintReadOnly)
	int64 TimeMs = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Rank = 1;
};

USTRUCT(BlueprintType)
struct FInnoRaceResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 RaceIndex = 0;

	UPROPERTY(BlueprintReadOnly)
	FName TrackId;

	UPROPERTY(BlueprintReadOnly)
	TArray<FInnoRaceResultEntry> Entries;
};

USTRUCT(BlueprintType)
struct FInnoStandingEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly)
	int32 RaceWins = 0;

	UPROPERTY(BlueprintReadOnly)
	int64 TotalMs = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 FinalRank = 1;
};

USTRUCT(BlueprintType)
struct FInnoMatchSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid MatchId;

	UPROPERTY(BlueprintReadOnly)
	FString Mode = TEXT("B");

	UPROPERTY(BlueprintReadOnly)
	TArray<FInnoRaceResult> Races;

	UPROPERTY(BlueprintReadOnly)
	TArray<FInnoStandingEntry> Standings;
};
