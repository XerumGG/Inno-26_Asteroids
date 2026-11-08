// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Race/InnoScoring.h"

void FInnoScoring::RankRace(TArray<FInnoRaceResultEntry>& Entries, int64 TrackTimeLimitMs, int64 DnfPenaltyMs)
{
	int64 SlowestFinishedMs = 0;

	for (const FInnoRaceResultEntry& Entry : Entries)
	{
		if (Entry.Status == EParticipantStatus::Finished && Entry.TimeMs > SlowestFinishedMs)
		{
			SlowestFinishedMs = Entry.TimeMs;
		}
	}

	const int64 PenaltyBaseline = (SlowestFinishedMs > 0) ? SlowestFinishedMs : TrackTimeLimitMs;

	// Apply DNF penalties
	for (FInnoRaceResultEntry& Entry : Entries)
	{
		if (Entry.Status != EParticipantStatus::Finished)
		{
			Entry.TimeMs = PenaltyBaseline + DnfPenaltyMs;
		}
	}

	// Sort by finish time (ascending)
	Entries.Sort([](const FInnoRaceResultEntry& A, const FInnoRaceResultEntry& B)
	{
		if (A.Status == EParticipantStatus::Finished && B.Status != EParticipantStatus::Finished)
		{
			return true;
		}
		if (A.Status != EParticipantStatus::Finished && B.Status == EParticipantStatus::Finished)
		{
			return false;
		}
		return A.TimeMs < B.TimeMs;
	});

	// Assign Ranks (shared rank on millisecond tie)
	int32 CurrentRank = 1;
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (i > 0 && Entries[i].TimeMs == Entries[i - 1].TimeMs)
		{
			Entries[i].Rank = Entries[i - 1].Rank;
		}
		else
		{
			Entries[i].Rank = CurrentRank;
		}
		CurrentRank++;
	}
}

void FInnoScoring::ComputeStandings(const TArray<FInnoRaceResult>& Races, TArray<FInnoStandingEntry>& OutStandings)
{
	OutStandings.Empty();
	if (Races.Num() == 0)
	{
		return;
	}

	TMap<FString, FInnoStandingEntry> PlayerMap;
	TMap<FString, int64> LastRaceTimeMap;

	for (const FInnoRaceResult& Race : Races)
	{
		for (const FInnoRaceResultEntry& Entry : Race.Entries)
		{
			FInnoStandingEntry& Standing = PlayerMap.FindOrAdd(Entry.PlayerName);
			Standing.PlayerName = Entry.PlayerName;
			Standing.TotalMs += Entry.TimeMs;

			if (Entry.Rank == 1 && Entry.Status == EParticipantStatus::Finished)
			{
				Standing.RaceWins++;
			}

			LastRaceTimeMap.Add(Entry.PlayerName, Entry.TimeMs);
		}
	}

	PlayerMap.GenerateValueArray(OutStandings);

	// Primary sort: Most race wins
	// Tie-break 1: Lowest total time
	// Tie-break 2: Faster last race time
	OutStandings.Sort([&LastRaceTimeMap](const FInnoStandingEntry& A, const FInnoStandingEntry& B)
	{
		if (A.RaceWins != B.RaceWins)
		{
			return A.RaceWins > B.RaceWins;
		}

		if (A.TotalMs != B.TotalMs)
		{
			return A.TotalMs < B.TotalMs;
		}

		const int64 LastA = LastRaceTimeMap.FindRef(A.PlayerName);
		const int64 LastB = LastRaceTimeMap.FindRef(B.PlayerName);
		return LastA < LastB;
	});

	// Assign final ranks
	for (int32 i = 0; i < OutStandings.Num(); ++i)
	{
		OutStandings[i].FinalRank = i + 1;
	}
}
