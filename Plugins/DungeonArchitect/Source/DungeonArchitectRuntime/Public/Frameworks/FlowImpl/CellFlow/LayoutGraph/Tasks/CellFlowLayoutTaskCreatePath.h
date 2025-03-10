//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskCreatePath.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowStructs.h"
#include "CellFlowLayoutTaskCreatePath.generated.h"

UCLASS(Meta = (AbstractTask, Title = "Create Path", Tooltip = "Create a path on an existing network", MenuPriority = 1200))
class UCellFlowLayoutTaskCreatePath  : public UBaseFlowLayoutTaskCreatePath {
	GENERATED_BODY()
};

