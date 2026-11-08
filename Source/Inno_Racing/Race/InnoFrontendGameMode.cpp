// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inno_Racing/Race/InnoFrontendGameMode.h"
#include "Inno_Racing/Race/InnoPlayerController.h"
#include "Inno_Racing/Race/InnoPlayerState.h"

AInnoFrontendGameMode::AInnoFrontendGameMode()
{
	PlayerControllerClass = AInnoPlayerController::StaticClass();
	PlayerStateClass = AInnoPlayerState::StaticClass();
	DefaultPawnClass = nullptr;
}
