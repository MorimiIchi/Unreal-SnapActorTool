// El psy congroo.

#include "SnapActor.h"

#include "ActorGroupingUtils.h"
#include "LevelEditor.h"
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

void USnapActor::SnapActor()
{
	if (!GetUnrealEditorSubsystem())
	{
		return;
	}

	FVector TargetLocation;
	GetCursorLocation(TargetLocation);

	SnapSelectionsToFloor(TargetLocation);
}

void USnapActor::GetCursorLocation(FVector& TargetLocation) const
{
	FLevelEditorModule& LevelEditorModule =
		FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	// 拿到第一个打开的 LevelEditor 窗口
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
	if (!LevelEditor.IsValid())
	{
		return;
	}

	TSharedPtr<SLevelViewport> LevelViewportWidget = LevelEditor->GetActiveViewportInterface();
	if (!LevelViewportWidget.IsValid())
	{
		return;
	}

	// 拿到底层的 FEditorViewportClient
	FEditorViewportClient& ViewportClient = LevelViewportWidget->GetLevelViewportClient();

	const UWorld* EditorWorld = ViewportClient.GetWorld();

	// 先从 ViewportClient 拿 FViewport*
	FViewport* Viewport = ViewportClient.Viewport;

	// 拿到鼠标当前位置（像素坐标）
	FIntPoint MousePos;
	Viewport->GetMousePos(MousePos);

	// 构造一个 SceneView 来做 deproject
	FSceneViewFamilyContext ViewFamily(
		FSceneViewFamily::ConstructionValues(
			Viewport,
			EditorWorld->Scene,
			ViewportClient.EngineShowFlags
		).SetRealtimeUpdate(true)
	);

	FSceneView* SceneView = ViewportClient.CalcSceneView(&ViewFamily);

	// 把屏幕坐标反投影到一条世界射线
	FVector TraceStart, TraceDirection;
	SceneView->DeprojectFVector2D(
		FVector2D(MousePos), // 屏幕坐标
		/*out*/ TraceStart, // 射线起点（通常是摄像机位置）
		/*out*/ TraceDirection // 射线方向（单位向量）
	);

	const FVector TraceEnd = TraceStart + TraceDirection * TraceDistance;

	FHitResult HitResult;
	EditorWorld->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility);

	if (HitResult.bBlockingHit)
	{
		TargetLocation = HitResult.ImpactPoint;
		TargetLocation.Z = TraceStart.Z;
	}
	else
	{
		GetViewportCenterLocation(TargetLocation);
	}
}

void USnapActor::GetViewportCenterLocation(FVector& TargetLocation) const
{
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
		TargetLocation.Z = CameraLocation.Z;
	}
	else
	{
		TargetLocation = TraceEnd;
	}
}

void USnapActor::SnapSelectionsToFloor(const FVector& TargetCenterLocation) const
{
	if (const UTypedElementSelectionSet* SelectionSet = GEditor->GetSelectedActors()->GetElementSelectionSet())
	{
		// 加入 Undo
		const FScopedTransaction Transaction(NSLOCTEXT("SnapActorTool", "SnapSelectionsToFloor",
		                                               "Snap Elements To Floor"));

		FScopedLevelDirtied LevelDirtyCallback;

		// 获取选中元素的平均位置
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
			});

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
			FNotificationInfo Info(NSLOCTEXT("SnapActorTool", "SnapToFloorNoElementsSnapped",
			                                 "No elements were snapped to the floor."));
			Info.ExpireDuration = 5.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}

		GEditor->RedrawLevelEditingViewports();
	}
}

bool USnapActor::SnapElement(const FTypedElementHandle& InElementHandle,
                             const FVector& TargetCenterLocation,
                             const FVector& AverageSelectionLocation)
{
	// 元素有效性检查
	if (!InElementHandle)
	{
		return false;
	}

	// 获取当前元素对应的世界接口
	const UTypedElementRegistry* Registry = UTypedElementRegistry::GetInstance();
	const TTypedElement<ITypedElementWorldInterface> ElementWorldHandle = Registry->GetElement<
		ITypedElementWorldInterface>(
		InElementHandle);
	if (!ElementWorldHandle)
	{
		return false;
	}

	// 获取元素世界变换和包围盒
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

	// 直接用包围盒中心和自身大小进行一个向下的 Sweep
	const FVector Extent = ElementBounds.BoxExtent;
	FVector LocationOffset = AverageSelectionLocation - ElementTransform.GetLocation();
	LocationOffset.Z = 0.f;

	// 计算半高
	const float HalfHeight = ElementBounds.BoxExtent.Z;

	// 改成从“包围盒底”往上抬半高，也就是把包围盒中心放到地面上
	const FVector StartLocation = LocationOffset
		+ TargetCenterLocation
		+ FVector(0.f, 0.f, HalfHeight);


	// 朝向：垂直向下
	const FVector Direction(0.f, 0.f, -1.f);

	// 向下对齐
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

		// 更新变换并通知编辑器
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

bool USnapActor::GetUnrealEditorSubsystem()
{
	if (!UnrealEditorSubsystem)
	{
		UnrealEditorSubsystem = GEditor->GetEditorSubsystem<UUnrealEditorSubsystem>();
	}

	return UnrealEditorSubsystem != nullptr;
}
