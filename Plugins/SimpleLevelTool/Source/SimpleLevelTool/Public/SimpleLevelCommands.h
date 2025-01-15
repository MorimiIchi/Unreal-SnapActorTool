// El psy congroo.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

/**
 * 
 */
class SIMPLELEVELTOOL_API FSimpleLevelCommands : public TCommands<FSimpleLevelCommands>
{
public:
	FSimpleLevelCommands()
		: TCommands(
			TEXT("SimpleLevelCommands"),
			NSLOCTEXT("Contexts", "SimpleLevelTool", "Simple Level Tool"),
			NAME_None,
			FAppStyle::GetAppStyleSetName()// Can use your own style
		)
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> SnapToViewportFloor;
};
