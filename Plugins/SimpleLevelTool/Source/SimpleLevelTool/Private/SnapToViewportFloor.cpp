// El psy congroo.


#include "SnapToViewportFloor.h"

#include "ActorGroupingUtils.h"
#include "Selection.h"
#include "SLevelViewport.h"
#include "Editor/GroupActor.h"
#include "Elements/Framework/TypedElementSelectionSet.h"
#include "Elements/Interfaces/TypedElementObjectInterface.h"
#include "Elements/Framework/TypedElementRegistry.h"
#include "Elements/Interfaces/TypedElementWorldInterface.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/EditorElementSubsystem.h"
#include "Subsystems/UnrealEditorSubsystem.h"
#include "Widgets/Notifications/SNotificationList.h"

void USnapToViewportFloor::SnapToViewportFloor()
{
	if (!GetUnrealEditorSubsystem())
	{
		return;
	}

	const FVector TargetLocation = GetViewportTraceLocation();

	SnapSelectionsToFloor(TargetLocation);
}

FVector USnapToViewportFloor::GetViewportTraceLocation() const
{
	FVector TargetLocation;

	const UWorld* EditorWorld = UnrealEditorSubsystem->GetEditorWorld();

	FVector CameraLocation;
	FRotator CameraRotation;
	UnrealEditorSubsystem->GetLevelViewportCameraInfo(CameraLocation, CameraRotation);

	// Get the location in front of the camera
	const FVector TraceStart = CameraLocation;
	const FVector TraceEnd = CameraLocation + CameraRotation.Vector() * TraceDistance;

	DrawDebugLine(EditorWorld, TraceStart, TraceEnd, FColor::Red, false, 2.0f);

	FHitResult HitResult;
	EditorWorld->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility);

	if (HitResult.bBlockingHit)
	{
		TargetLocation = HitResult.ImpactPoint;
		DrawDebugSphere(EditorWorld, TargetLocation, 20.0f, 32, FColor::Green, false, 2.0f);
		TargetLocation.Z = CameraLocation.Z;
	}
	else
	{
		TargetLocation = TraceEnd;
		DrawDebugSphere(EditorWorld, TargetLocation, 20.0f, 32, FColor::Green, false, 2.0f);
	}

	return TargetLocation;
}

void USnapToViewportFloor::SnapSelectionsToFloor(const FVector& TargetCenterLocation) const
{
	if (const UTypedElementSelectionSet* SelectionSet = GEditor->GetSelectedActors()->GetElementSelectionSet())
	{
		const FScopedTransaction Transaction(NSLOCTEXT("SimpleLevelTool", "SnapSelectionsToFloor",
		                                               "Snap Elements To Floor"));

		FScopedLevelDirtied LevelDirtyCallback;

		FVector AverageSelectionLocation = UGameplayStatics::GetActorArrayAverageLocation(
			SelectionSet->GetSelectedObjects<AActor>());

		bool bSnappedElements = false;
		SelectionSet->ForEachSelectedElement<ITypedElementWorldInterface>(
			[this, &LevelDirtyCallback, &bSnappedElements, TargetCenterLocation, AverageSelectionLocation](
			const TTypedElement<ITypedElementWorldInterface>& InElement)
			{
				if (SnapElement(InElement, TargetCenterLocation, AverageSelectionLocation))
				{
					bSnappedElements = true;
					LevelDirtyCallback.Request();
				}
				return true;
			}
		);

		// Update the pivot location
		if (bSnappedElements)
		{
			if (TTypedElement<ITypedElementWorldInterface> LastElement =
				UEditorElementSubsystem::GetLastSelectedEditorManipulableElement(
					UEditorElementSubsystem::GetEditorNormalizedSelectionSet(*SelectionSet)))
			{
				FTransform LastElementTransform;
				if (LastElement.GetWorldTransform(LastElementTransform))
				{
					GEditor->SetPivot(LastElementTransform.GetLocation(), false, true);

					if (UActorGroupingUtils::IsGroupingActive())
					{
						if (TTypedElement<ITypedElementObjectInterface> LastObjectElement = SelectionSet->
							GetElementList()->GetElement<ITypedElementObjectInterface>(LastElement))
						{
							if (AActor* LastActor = Cast<AActor>(LastObjectElement.GetObject()))
							{
								// Set group pivot for the root-most group
								if (AGroupActor* ActorGroupRoot = AGroupActor::GetRootForActor(LastActor, true, true))
								{
									ActorGroupRoot->CenterGroupLocation();
								}
							}
						}
					}
				}
			}
		}
		else
		{
			// Notify the user that no elements were snapped
			FNotificationInfo Info(NSLOCTEXT("SimpleLevelTool", "SnapToFloorNoElementsSnapped",
			                                 "No elements were snapped to the floor."));
			Info.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}

		GEditor->RedrawLevelEditingViewports();
	}
}

bool USnapToViewportFloor::SnapElement(const FTypedElementHandle& InElementHandle,
                                       const FVector& TargetCenterLocation,
                                       const FVector& AverageSelectionLocation)
{
	// 1. 元素有效性检查
	if (!InElementHandle)
	{
		return false;
	}

	// 2. 获取当前元素对应的世界接口
	const UTypedElementRegistry* Registry = UTypedElementRegistry::GetInstance();
	const TTypedElement<ITypedElementWorldInterface> ElementWorldHandle = Registry->GetElement<
		ITypedElementWorldInterface>(
		InElementHandle);
	if (!ElementWorldHandle)
	{
		return false;
	}

	// 3. 获取元素世界变换和包围盒
	FTransform ElementTransform;
	if (!ElementWorldHandle.GetWorldTransform(ElementTransform))
	{
		return false;
	}

	FBoxSphereBounds ElementBounds;
	if (!ElementWorldHandle.GetBounds(ElementBounds))
	{
		return false;
	}

	// 4. 直接用包围盒中心和自身大小进行一个向下的 Sweep
	const FVector Extent = ElementBounds.BoxExtent;
	FVector LocationOffset = AverageSelectionLocation - ElementTransform.GetLocation();
	LocationOffset.Z = 0.f;
	const FVector StartLocation = LocationOffset + TargetCenterLocation;

	// 朝向：垂直向下
	const FVector Direction(0.f, 0.f, -1.f);

	// 5. 向下对齐
	FTransform NewTransform;
	if (ElementWorldHandle.FindSuitableTransformAlongPath(
		StartLocation,
		StartLocation + Direction * WORLD_MAX,
		FCollisionShape::MakeBox(Extent),
		TArray<FTypedElementHandle>(),
		NewTransform))
	{
		// 保留原本的旋转和缩放
		NewTransform.SetRotation(ElementTransform.GetRotation());
		NewTransform.SetScale3D(ElementTransform.GetScale3D());

		// 6. 更新变换并通知编辑器
		FScopedConditionalWorldSwitcher WorldSwitcher(GCurrentLevelEditingViewportClient);
		ElementWorldHandle.NotifyMovementStarted();
		ElementWorldHandle.SetWorldTransform(NewTransform);
		ElementWorldHandle.NotifyMovementEnded();

		// 通知 Editor 刷新状态
		GCurrentLevelEditingViewportClient->GetModeTools()->ActorMoveNotify();

		return true;
	}

	// 直接移动到目标位置
	NewTransform.SetLocation(StartLocation);
	NewTransform.SetRotation(ElementTransform.GetRotation());
	NewTransform.SetScale3D(ElementTransform.GetScale3D());
	ElementWorldHandle.NotifyMovementStarted();
	ElementWorldHandle.SetWorldTransform(NewTransform);
	ElementWorldHandle.NotifyMovementEnded();
	GCurrentLevelEditingViewportClient->GetModeTools()->ActorMoveNotify();
	return true;
}

bool USnapToViewportFloor::GetUnrealEditorSubsystem()
{
	if (!UnrealEditorSubsystem)
	{
		UnrealEditorSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
	}

	return UnrealEditorSubsystem != nullptr;
}
