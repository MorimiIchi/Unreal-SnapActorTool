// El psy congroo.


#include "SimpleLevelCommands.h"

#define LOCTEXT_NAMESPACE "FSimpleLevelToolModule"

void FSimpleLevelCommands::RegisterCommands()
{
	UI_COMMAND(
		SnapToViewportFloor,
		"Snap To Viewport Floor",
		"Snap the selected actors to the floor in front of viewport",
		EUserInterfaceActionType::Button,
		FInputChord(EKeys::T, EModifierKey::Control)
	);
}

#undef LOCTEXT_NAMESPACE
