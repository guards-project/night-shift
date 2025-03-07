//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/Tasks/SnapGridFlowLayoutTaskCreatePath.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/SnapGridFlowAbstractGraphDomain.h"
#include "Frameworks/FlowImpl/SnapGridFlow/LayoutGraph/Utils/Grid3DFlowAbstractGraphPathUtils.h"
#include "Frameworks/Snap/SnapGridFlow/SnapGridFlowModuleDatabase.h"

DEFINE_LOG_CATEGORY_STATIC(LogSGFCreatePath, Log, All);

IFlowAGNodeGroupGeneratorPtr USnapGridFlowLayoutTaskCreatePath::CreateNodeGroupGenerator(TWeakPtr<const IFlowDomain> InDomainPtr) const {
	const TSharedPtr<const IFlowDomain> Domain = InDomainPtr.Pin();
	if (!Domain.IsValid() || Domain->GetDomainID() != FSnapGridFlowAbstractGraphDomain::DomainID) {
		return MakeShareable(new FNullFlowAGNodeGroupGenerator);
	}
    
	const TSharedPtr<const FSnapGridFlowAbstractGraphDomain> SGFLayoutGraphDomain = StaticCastSharedPtr<const FSnapGridFlowAbstractGraphDomain>(Domain);
	USnapGridFlowModuleDatabase* ModuleDatabase = SGFLayoutGraphDomain->GetModuleDatabase();
	if (!ModuleDatabase) {
		return MakeShareable(new FNullFlowAGNodeGroupGenerator); 
	}
	
	const bool bSupportsDoorCategory = SGFLayoutGraphDomain->SupportsDoorCategory();
	const TSharedPtr<FSGFNodeCategorySelector> NodeCategorySelector = MakeShareable(new FSGFNodeCategorySelector(
				ModuleCategories, ModuleCategoryOverrideMethod, StartNodeCategoryOverride, EndNodeCategoryOverride, CategoryOverrideLogic));
	
	if (bSupportsDoorCategory) {
		return MakeShareable(new FSnapFlowAGNodeGroupGenerator(ModuleDatabase, NodeCategorySelector));
	}
	else {
		return MakeShareable(new FSnapFlowAGIgnoreDoorCategoryNodeGroupGenerator(ModuleDatabase, NodeCategorySelector));
	}
}

FFlowAbstractGraphConstraintsPtr USnapGridFlowLayoutTaskCreatePath::CreateGraphConstraints(TWeakPtr<const IFlowDomain> InDomainPtr) const {
	const TSharedPtr<const IFlowDomain> Domain = InDomainPtr.Pin();
	if (!Domain.IsValid() || Domain->GetDomainID() != FSnapGridFlowAbstractGraphDomain::DomainID) {
		return MakeShareable(new FNullFlowAbstractGraphConstraints);
	}
    
	const TSharedPtr<const FSnapGridFlowAbstractGraphDomain> SGFLayoutGraphDomain = StaticCastSharedPtr<const FSnapGridFlowAbstractGraphDomain>(Domain);
	USnapGridFlowModuleDatabase* ModuleDatabase = SGFLayoutGraphDomain->GetModuleDatabase();
	if (!ModuleDatabase) {
	    return MakeShareable(new FNullFlowAbstractGraphConstraints);
    }

	const bool bSupportsDoorCategory = SGFLayoutGraphDomain->SupportsDoorCategory();
	const TSharedPtr<FSGFNodeCategorySelector> NodeCategorySelector = MakeShareable(new FSGFNodeCategorySelector(
					ModuleCategories, ModuleCategoryOverrideMethod, StartNodeCategoryOverride, EndNodeCategoryOverride, CategoryOverrideLogic));
	return MakeShareable(new FSnapGridFlowAbstractGraphConstraints(ModuleDatabase, NodeCategorySelector, bSupportsDoorCategory));
}

UFlowLayoutNodeCreationConstraint* USnapGridFlowLayoutTaskCreatePath::GetNodeCreationConstraintLogic() const {
	return bUseNodeCreationConstraint ? NodeCreationConstraint : nullptr;
}

void USnapGridFlowLayoutTaskCreatePath::FinalizePathNode(UFlowAbstractNode* Node, const FFlowAGGrowthState_PathItem& PathItem) const {
	const FSGFNodeCategorySelector NodeCategorySelector(ModuleCategories, ModuleCategoryOverrideMethod, StartNodeCategoryOverride, EndNodeCategoryOverride, CategoryOverrideLogic);
	UFANodeSnapDomainData* NodeDomainData = Node->FindOrAddDomainData<UFANodeSnapDomainData>();
	TSet<FVector> NodeCoords;
	FFlowAbstractGraphPathUtils::GetNodeCoords(Node, NodeCoords);
	NodeDomainData->ModuleCategories = NodeCategorySelector.GetCategoriesAtNode(Node->PathIndex, Node->PathLength, NodeCoords.Array());
		
	if (PathItem.UserData.IsValid()) {
		const TSharedPtr<FSGFNodeGroupUserData> GroupUserData = StaticCastSharedPtr<FSGFNodeGroupUserData>(PathItem.UserData);
		USGFNodeGroupUserData* GroupDomainData = Node->FindOrAddDomainData<USGFNodeGroupUserData>();
		GroupDomainData->CopyFrom(*GroupUserData);
	}
}

