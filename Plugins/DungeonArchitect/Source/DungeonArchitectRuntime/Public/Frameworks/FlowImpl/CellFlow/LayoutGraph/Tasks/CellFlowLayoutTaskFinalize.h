//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskFinalize.h"
#include "CellFlowLayoutTaskFinalize.generated.h"

struct FCellFlowGridEdgeInfo;
class UCellFlowVoronoiGraph;
class UCellFlowLayoutGraph;
class UDAFlowCellGraph;

UCLASS(Meta = (AbstractTask, Title = "Finalize Graph", Tooltip = "Call this to finalize the layout graph", MenuPriority = 1500))
class UCellFlowLayoutTaskFinalize : public UBaseFlowLayoutTaskFinalize {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	int MinHeight{-5};
	
	UPROPERTY(EditAnywhere, Category = "CellFlow")
	int MaxHeight{5};

	UPROPERTY(EditAnywhere, Category = "CellFlow")
	int MaxClimbHeight = 1;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow")
	int HeightResolveIterations = 100;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow")
	int StairResolveIterations = 10;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow")
	int SpawnSeparationIterations = 100;
	
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow", meta=(UIMin="0.0", UIMax="5.0"))
	float SpawnSeparationDistance = 0.25f;
	
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow", meta=(UIMin="0.0", UIMax="1.0"))
	float SpawnEdgeSeparationSensitivity = 0.25f;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow", meta=(UIMin="0.0", UIMax="1.0"))
	float SpawnItemSeparationSensitivity = 0.25f;
	
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow", meta=(UIMin="0.0", UIMax="1.0"))
	bool bDebugFlagCrossFlip{};
	
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "CellFlow")
	bool bDebugStairPlacement{};
	
public:
	virtual void Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) override;

	/**
	 * Records adjacent chunk configurations that cannot have a lower/upper height config due to the topology.  This might be due to the
	 * fact that we cannot create a stair, or the created stair might block an existing path
	 */
	struct FIncompatibleChunkHeights {
		FGuid LowerLayoutNodeId{};
		FGuid HigherLayoutNodeId{};
	};
	class FHeightIncompatibilityState {
	public:
		void RegisterIncompatibleHeights(const FGuid& GroupLow, const FGuid& GroupHi);
		bool IsIncompatible(const FGuid& GroupLow, const FGuid& GroupHi) const;
		
	private:
		static uint32 GetKeyHash(const FGuid& GroupLow, const FGuid& GroupHi);

	private:
		TMap<uint32, FIncompatibleChunkHeights> IncompatibleChunkHeights;
	};
	
private:
	void InitializeCellGraph(UDAFlowCellGraph* CellGraph);
	bool AssignGridConnections(UDAFlowCellGraph* CellGraph, UCellFlowLayoutGraph* LayoutGraph, const FFlowAbstractGraphQuery& LayoutGraphQuery,
			const FRandomStream& Random, FHeightIncompatibilityState& IncompatibilityState) const;
	bool AssignDCELConnections(UDAFlowCellGraph* CellGraph, UCellFlowVoronoiGraph* VoronoiData, UCellFlowLayoutGraph* LayoutGraph,
			const FFlowAbstractGraphQuery& LayoutGraphQuery, const FRandomStream& Random, FHeightIncompatibilityState& IncompatibilityState);

	bool SpawnItemsGrid(UDAFlowCellGraph* CellGraph, const FFlowAbstractGraphQuery& LayoutGraphQuery, const FRandomStream& Random) const;
	bool SpawnItemsVoronoi(UDAFlowCellGraph* InCellGraph, UCellFlowVoronoiGraph* InVoronoiData, const FFlowAbstractGraphQuery& InGraphQuery, const FRandomStream& InRandom) const;
};

