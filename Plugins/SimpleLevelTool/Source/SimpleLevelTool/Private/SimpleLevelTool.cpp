// Copyright Epic Games, Inc. All Rights Reserved.

#include "SimpleLevelTool.h"

#include "SnapToViewportFloor.h"
#include "LevelEditor.h"
#include "SimpleLevelCommands.h"

#define LOCTEXT_NAMESPACE "FSimpleLevelToolModule"

void FSimpleLevelToolModule::StartupModule()
{
	FSimpleLevelCommands::Register();

	const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	const TSharedRef<FUICommandList>& CommandList = LevelEditorModule.GetGlobalLevelEditorActions();

	CommandList->MapAction(
		FSimpleLevelCommands::Get().SnapToViewportFloor,
		FExecuteAction::CreateRaw(this, &FSimpleLevelToolModule::OnAlignToFloorFrontViewport),
		FCanExecuteAction()
	);
}

void FSimpleLevelToolModule::ShutdownModule()
{
	FSimpleLevelCommands::Unregister();
}

void FSimpleLevelToolModule::OnAlignToFloorFrontViewport()
{
	USnapToViewportFloor* AlignToFloorFrontViewport = NewObject<USnapToViewportFloor>();
	AlignToFloorFrontViewport->SnapToViewportFloor();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSimpleLevelToolModule, SimpleLevelTool)
