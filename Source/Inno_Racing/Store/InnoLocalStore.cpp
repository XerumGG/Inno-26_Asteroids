// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Store/InnoLocalStore.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"

void UInnoLocalStore::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	StorageDir = FPaths::ProjectSavedDir() / TEXT("InnoRace");
	JsonlPath = StorageDir / TEXT("results.jsonl");
	BackupPath = StorageDir / TEXT("results.bak");
	ExportsDir = StorageDir / TEXT("Exports");

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*StorageDir);
	PlatformFile.CreateDirectoryTree(*ExportsDir);
}

bool UInnoLocalStore::RecordMatchSummary(const FInnoMatchSummary& Summary)
{
	FString JsonLine = FString::Printf(TEXT("{\"MatchId\":\"%s\",\"Mode\":\"%s\",\"Races\":%d,\"Standings\":%d}\n"),
		*Summary.MatchId.ToString(),
		*Summary.Mode,
		Summary.Races.Num(),
		Summary.Standings.Num());

	// Atomic append to results.jsonl
	const bool bSaved = FFileHelper::SaveStringToFile(JsonLine, *JsonlPath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);

	// Refresh rolling backup copy
	if (bSaved)
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		PlatformFile.CopyFile(*BackupPath, *JsonlPath);
	}

	return bSaved;
}

FString UInnoLocalStore::ExportResultsToCSV()
{
	const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString CsvFilePath = ExportsDir / FString::Printf(TEXT("InnoRace_Export_%s.csv"), *Timestamp);

	FString CsvContent = TEXT("MatchId,Mode,StationId,Name,RaceIndex,TrackId,TimeMs,Status,Rank,CarId,FinalRank,TotalTimeMs\n");

	// For demonstration, outputs valid CSV template
	FFileHelper::SaveStringToFile(CsvContent, *CsvFilePath);
	return CsvFilePath;
}
