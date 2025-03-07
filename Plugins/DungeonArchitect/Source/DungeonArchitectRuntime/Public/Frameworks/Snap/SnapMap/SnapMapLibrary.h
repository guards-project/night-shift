//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Snap/Lib/SnapLibrary.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleDatabase.h"

class UGrammarScriptGraphNode;

//////////////////////////////////////// Graph Node Impl //////////////////////////////////////////////
class DUNGEONARCHITECTRUNTIME_API FSnapGraphGrammarNode final : public SnapLib::ISnapGraphNode {
public:
    explicit FSnapGraphGrammarNode(UGrammarScriptGraphNode* InGraphNode);
    
    virtual FGuid GetNodeID() const override;;
    virtual FName GetCategory() const override;
    virtual TArray<SnapLib::ISnapGraphNodePtr> GetOutgoingNodes(const FGuid& IncomingNodeId) const override;
    UGrammarScriptGraphNode* GetGraphNode() const;

private:
    TWeakObjectPtr<UGrammarScriptGraphNode> GraphNode;
};


//////////////////////////////////////// Module Database Adapter //////////////////////////////////////////////
class DUNGEONARCHITECTRUNTIME_API FSnapMapGraphModDBItemImpl
    : public SnapLib::IModuleDatabaseItem
{
public:
    FSnapMapGraphModDBItemImpl(const FSnapMapModuleDatabaseItem& InItem) : Item(InItem) {}
    FSnapMapModuleDatabaseItem GetItem() const { return Item; }
    virtual SnapLib::FModuleNodePtr CreateModuleNode(const FGuid& InNodeId) override;

    virtual FBox GetBounds() const override { return Item.ModuleBounds; }
    virtual FDABoundsShapeList GetBoundShapes() const override;
    virtual TSoftObjectPtr<UWorld> GetLevel() const override { return Item.Level; }
    virtual TArray<FName> GetTags() const override { return {}; }
    virtual bool ShouldAllowRotation() const override { return Item.bAllowRotation; }
    virtual const TMap<FString, TSoftObjectPtr<UWorld>> GetThemedLevels() const override { return Item.ThemedLevels; }
    virtual FName GetCategory() const override { return Item.Category; }
    
private:
    FSnapMapModuleDatabaseItem Item;
};

class DUNGEONARCHITECTRUNTIME_API FSnapMapModuleDatabaseImpl : public SnapLib::IModuleDatabase {
public:
    FSnapMapModuleDatabaseImpl(USnapMapModuleDatabase* ModuleDB);
};

class DUNGEONARCHITECTRUNTIME_API FSnapMapModuleDatabaseUtils {
public:
    static void CreateHullPointsFromBoxExtents(const FVector2D& InExtents, TArray<FVector2D>& OutConvexHull);
    static FDABoundsShapeConvexPoly CreateFallbackModuleConvexHull(const FBox& ModuleBounds);
};

