//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class FPatternEditorViewportClient;

class IMGPatternEditorInputInterface {
public:
	virtual ~IMGPatternEditorInputInterface() {}
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) { return false; }
	virtual void CapturedMouseMove(FPatternEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) {}
	virtual void ProcessClick(FPatternEditorViewportClient* InViewportClient, FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) {}
	virtual void CheckHoveredHitProxy(FPatternEditorViewportClient* InViewportClient, HHitProxy* HoveredHitProxy) {}
	virtual void TrackingStarted(FPatternEditorViewportClient* InViewportClient, const FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge) {}
	virtual void TrackingStopped(FPatternEditorViewportClient* InViewportClient) {}
};

