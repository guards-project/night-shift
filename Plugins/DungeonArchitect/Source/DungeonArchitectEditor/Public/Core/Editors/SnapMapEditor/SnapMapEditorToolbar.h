//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class FSnapMapEditor;
class FExtender;
class FToolBarBuilder;

class FSnapMapEditorToolbar : public TSharedFromThis<FSnapMapEditorToolbar> {
public:
    FSnapMapEditorToolbar(TSharedPtr<FSnapMapEditor> InFlowEditor)
        : FlowEditor(InFlowEditor) {
    }

    void AddModesToolbar(TSharedPtr<FExtender> Extender);
    void AddFlowDesignerToolbar(TSharedPtr<FExtender> Extender);
    void AddFlowVisualizerToolbar(TSharedPtr<FExtender> Extender);
    void AddFlowDebugToolbar(TSharedPtr<FExtender> Extender);

private:
    void FillModesToolbar(FToolBarBuilder& ToolbarBuilder);
    void FillFlowDesignerToolbar(FToolBarBuilder& ToolbarBuilder);
    void FillFlowVisualizerToolbar(FToolBarBuilder& ToolbarBuilder);
    void FillFlowDebugToolbar(FToolBarBuilder& ToolbarBuilder);

    void FillSupportToolItems(FToolBarBuilder& ToolbarBuilder);

protected:
    /** Pointer back to the blueprint editor tool that owns us */
    TWeakPtr<FSnapMapEditor> FlowEditor;
};

