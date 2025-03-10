//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskCreatePath.h"
#include "Frameworks/Flow/Domains/Snap/SnapFlowAbstractGraphSupport.h"
#include "SnapGridFlowLayoutTaskCreatePath.generated.h"

class UGrid3DLayoutNodeCreationConstraint;

UCLASS(Meta = (AbstractTask, Title = "Create Path", Tooltip = "Create a path on an existing network", MenuPriority = 1200))
class DUNGEONARCHITECTRUNTIME_API USnapGridFlowLayoutTaskCreatePath : public UBaseFlowLayoutTaskCreatePath {
    GENERATED_BODY()
public:
    /**
        Enable this to control where the nodes are created

        Variable Name: bUseNodeCreationConstraint
    */
    UPROPERTY(EditAnywhere, Category = "Node Constraints")
    bool bUseNodeCreationConstraint = false;

    /**
        Use a blueprint to control where the layout nodes are allowed to be created   
        This is useful if you want static entrances to your dungeon
        This parameter requires bUseNodeCreationConstraint flag to be set

        Variable Name: N/A
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, SimpleDisplay, Category = "Node Constraints", Meta=(EditCondition = "bUseNodeCreationConstraint"))
    UGrid3DLayoutNodeCreationConstraint* NodeCreationConstraint;

    UPROPERTY(EditAnywhere, Category="Snap")
    TArray<FName> ModuleCategories = { "Room" };

    UPROPERTY(EditAnywhere, Category="Snap")
    ESnapFlowAGTaskModuleCategoryOverrideMethod ModuleCategoryOverrideMethod = ESnapFlowAGTaskModuleCategoryOverrideMethod::None;
    
    UPROPERTY(EditAnywhere, Category="Snap", Meta=(EditCondition="ModuleCategoryOverrideMethod == ESnapFlowAGTaskModuleCategoryOverrideMethod::StartEnd"))
    TArray<FName> StartNodeCategoryOverride;
    
    UPROPERTY(EditAnywhere, Category="Snap", Meta=(EditCondition="ModuleCategoryOverrideMethod == ESnapFlowAGTaskModuleCategoryOverrideMethod::StartEnd"))
    TArray<FName> EndNodeCategoryOverride;
    
    
    // Use a blueprint logic to change the category list on a per-node basis, if needed.
    // Create a blueprint inherited from "Snap Flow Node Category Selection Override". Then override the "TryOverrideCategories" function
    UPROPERTY(EditAnywhere, Instanced, SimpleDisplay, Category="Snap", Meta=(EditCondition="ModuleCategoryOverrideMethod == ESnapFlowAGTaskModuleCategoryOverrideMethod::Blueprint"))
    TArray<USnapFlowNodeCategorySelectionOverride*> CategoryOverrideLogic;

protected:
    virtual IFlowAGNodeGroupGeneratorPtr CreateNodeGroupGenerator(TWeakPtr<const IFlowDomain> InDomain) const override;
    virtual FFlowAbstractGraphConstraintsPtr CreateGraphConstraints(TWeakPtr<const IFlowDomain> InDomain) const override;
    virtual UFlowLayoutNodeCreationConstraint* GetNodeCreationConstraintLogic() const override;
    virtual void FinalizePathNode(UFlowAbstractNode* InPathNode, const FFlowAGGrowthState_PathItem& PathItem) const override;
    
};

