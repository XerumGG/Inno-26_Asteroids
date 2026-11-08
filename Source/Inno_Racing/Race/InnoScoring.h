// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inno_Racing/Race/InnoRaceTypes.h"

/**
 * Pure C++ Scoring Library.
 * Independent of UObjects for direct automation testing.
 * Rules:
 * - Single Race: Lowest finish time wins.
 * - DNF Penalty: Slowest finisher time + 10s.
 * - Series Winner (Type A): Most race wins (Best-of-3 format).
 *   Tie-break: Lowest cumulative total time over all races, then faster final race.
 */
class INNO_RACING_API FInnoScoring
{
public:
	// Ranks a single race and assigns finishing places
	static void RankRace(TArray<FInnoRaceResultEntry>& Entries, int64 TrackTimeLimitMs, int64 DnfPenaltyMs = 10000);

	// Computes cumulative match standings across all completed playlist races
	static void ComputeStandings(const TArray<FInnoRaceResult>& Races, TArray<FInnoStandingEntry>& OutStandings);
};
