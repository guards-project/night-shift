//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/AppModes/ThemeGraph/Graph/EdGraphNode_DungeonBase.h"

#include "Core/Editors/ThemeEditor/AppModes/ThemeGraph/Graph/EdGraphNode_DungeonMesh.h"

#define LOCTEXT_NAMESPACE "EdGraphNode_DungeonBase"

UEdGraphNode_DungeonBase::UEdGraphNode_DungeonBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), bHighlightChildNodeIndices(false) {
}

void UEdGraphNode_DungeonBase::PostEditChangeProperty(struct FPropertyChangedEvent& e) {
    const TSharedPtr<FNodePropertyObserver> PropertyObserverPtr = PropertyObserver.Pin();
    if (PropertyObserverPtr.IsValid()) {
        const FName PropertyName = (e.Property != nullptr) ? e.Property->GetFName() : NAME_None;
        PropertyObserverPtr->OnPropertyChanged(this, PropertyName);
    }

    Super::PostEditChangeProperty(e);
}

struct Compare_UEdGraphNode_DungeonBase {
    bool operator()(const UEdGraphNode_DungeonBase& A, const UEdGraphNode_DungeonBase& B) const {
        return A.NodePosX < B.NodePosX;
    }
};

void UEdGraphNode_DungeonBase::NodeConnectionListChanged() {
    UEdGraphNode::NodeConnectionListChanged();
    UpdateChildExecutionOrder();
    GetGraph()->NotifyGraphChanged();
}

void UEdGraphNode_DungeonBase::UpdateChildExecutionOrder() {
    UEdGraphPin* OutputPin = GetOutputPin();
    if (!OutputPin) return;

    TArray<UEdGraphNode_DungeonActorBase*> Children;

    for (UEdGraphPin* ChildPin : OutputPin->LinkedTo) {
        UEdGraphNode_DungeonActorBase* ChildNode = Cast<UEdGraphNode_DungeonActorBase>(ChildPin->GetOwningNode());
        if (ChildNode) {
            Children.Add(ChildNode);
        }
    }

    // Sort the sibling nodes based on the X axis
    Children.Sort(Compare_UEdGraphNode_DungeonBase());

    int32 ExecutionOrder = 1;
    for (UEdGraphNode_DungeonActorBase* Child : Children) {
        Child->ExecutionOrder = ExecutionOrder;
        ExecutionOrder++;
    }
}

#undef LOCTEXT_NAMESPACE

