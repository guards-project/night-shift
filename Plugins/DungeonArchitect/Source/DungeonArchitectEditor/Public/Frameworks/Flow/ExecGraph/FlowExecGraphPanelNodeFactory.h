//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class DUNGEONARCHITECTEDITOR_API FFlowExecGraphPanelNodeFactory : public FGraphPanelNodeFactory {
public:
    virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};

