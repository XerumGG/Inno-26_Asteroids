// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "InnoPrimaryLayout.generated.h"

/**
 * Root UI Layout containing the 4 primary CommonUI activatable widget stacks:
 * 1. Menu Layer (Menus, car selection, lobbies)
 * 2. Game / HUD Layer (Race HUD, speed, clutch, fuel)
 * 3. Modal Layer (Confirm leave, dialogs, PIN input)
 * 4. Toast Layer (Notifications, disconnects)
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class INNO_RACING_API UInnoPrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> GameLayerStack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ModalStack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCommonActivatableWidgetStack> ToastStack;
};
