// El psy congroo.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

/**
 * 
 */
class SNAPACTORTOOL_API FSnapActorToolCommands : public TCommands<FSnapActorToolCommands>
{
public:
	FSnapActorToolCommands()
		: TCommands(
			TEXT("SnapActorToolCommands"),
			NSLOCTEXT("Contexts", "SnapActorTool", "Snap Actor Tool"),
			NAME_None,
			FAppStyle::GetAppStyleSetName()// Can use your own style
		)
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> SnapActorToFloor;
};
