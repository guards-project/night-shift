//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"
#include "SelectFlowExecTask.generated.h"

UCLASS(Meta = (Title="Select Branch", Tooltip="Select one of the incoming branches", MenuPriority = 0))
class DUNGEONARCHITECTRUNTIME_API USelectFlowExecTask : public UFlowExecTask {
	GENERATED_BODY()
public:
	USelectFlowExecTask();
	virtual void Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) override;
	virtual TArray<int32> SelectIncomingNodeBranches(const FRandomStream& InRandom, const TArray<UFlowExecScriptGraphNode*> IncomingNodes) override;
};

