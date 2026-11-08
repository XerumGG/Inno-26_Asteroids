// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "InnoFrontendGameMode.generated.h"

/**
 * GameMode for L_Frontend (Main Menu, Name Entry, Car Selection).
 * Manages the camera rig between the cosmic space vista and the garage showcase.
 */
UCLASS()
class INNO_RACING_API AInnoFrontendGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AInnoFrontendGameMode();
};
