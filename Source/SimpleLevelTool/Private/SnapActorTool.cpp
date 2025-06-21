// Copyright Epic Games, Inc. All Rights Reserved.

#include "SnapActorTool.h"

#include "SnapActor.h"
#include "LevelEditor.h"
#include "SnapActorToolCommands.h"

#define LOCTEXT_NAMESPACE "FSnapActorToolModule"

void FSnapActorToolModule::StartupModule()
{
	FSnapActorToolCommands::Register();

	const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	const TSharedRef<FUICommandList>& CommandList = LevelEditorModule.GetGlobalLevelEditorActions();

	CommandList->MapAction(
		FSnapActorToolCommands::Get().SnapActorToFloor,
		FExecuteAction::CreateRaw(this, &FSnapActorToolModule::OnAlignToFloorFrontViewport),
		FCanExecuteAction()
	);
}

void FSnapActorToolModule::ShutdownModule()
{
	FSnapActorToolCommands::Unregister();
}

void FSnapActorToolModule::OnAlignToFloorFrontViewport()
{
	USnapActor* AlignToFloorFrontViewport = NewObject<USnapActor>();
	AlignToFloorFrontViewport->SnapActor();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSnapActorToolModule, SnapActorTool)
