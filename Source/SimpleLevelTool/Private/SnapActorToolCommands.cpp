// El psy congroo.


#include "SnapActorToolCommands.h"

#define LOCTEXT_NAMESPACE "FSnapActorToolModule"

void FSnapActorToolCommands::RegisterCommands()
{
	UI_COMMAND(
		SnapActorToFloor,
		"Snap Actor To Floor",
		"Snap the selected actors to the floor under cusor.",
		EUserInterfaceActionType::Button,
		FInputChord(EKeys::T, EModifierKey::Control)
	);
}

#undef LOCTEXT_NAMESPACE
