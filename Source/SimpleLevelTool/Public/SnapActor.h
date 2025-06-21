// El psy congroo.

#pragma once

#include "CoreMinimal.h"
#include "Editor/Blutility/Classes/ActorActionUtility.h"
#include "SnapActor.generated.h"

/**
 * 
 */
UCLASS()
class SNAPACTORTOOL_API USnapActor : public UActorActionUtility
{
	GENERATED_BODY()

public:
	void SnapActor();

	UPROPERTY()
	float TraceDistance = 15000.0f;

	UPROPERTY()
	float TraceDownDistance = 500.0f;

private:
	void GetViewportTraceLocation(FVector& TargetLocation) const;
	void GetCursorLocation(FVector& TargetLocation) const;
	void SnapSelectionsToFloor(const FVector& TargetCenterLocation) const;
	static bool SnapElement(const FTypedElementHandle& InElementHandle, const FVector& TargetCenterLocation, const FVector& AverageSelectionLocation);

	UPROPERTY()
	class UUnrealEditorSubsystem* UnrealEditorSubsystem;

	bool GetUnrealEditorSubsystem();
};
