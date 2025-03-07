//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskFinalize.h"

#include "Core/Utils/Debug/SceneDebugDataComponent.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraph.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowGrid.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Impl/CellFlowVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/Lib/CellFlowLog.h"

namespace DACellFlowStairUtil {
	void GetConnectedNodes(const UFlowAbstractNode* Node, const FFlowAbstractGraphQuery& GraphQuery, TArray<UFlowAbstractNode*>& OutConnectedNodes) {
		TArray<FGuid> ConnectedNodeIds = GraphQuery.GetConnectedNodes(Node->NodeId);
		for (const FGuid& ConnectedNodeId : ConnectedNodeIds) {
			UFlowAbstractNode* ConnectedNode = GraphQuery.GetNode(ConnectedNodeId);
			if (ConnectedNode->bActive) {
				OutConnectedNodes.Add(ConnectedNode);
			}
		}
	}

	void GetConnectedNodeHeights(const TArray<UFlowAbstractNode*>& ConnectedNodes, TArray<int>& OutConnectedHeights) {
		for (const UFlowAbstractNode* ConnectedNode : ConnectedNodes) {
			OutConnectedHeights.Add(FMath::RoundToInt(ConnectedNode->Coord.Z));
		}
	}
	
	/**
	 * Updates the node height depending on the surrounding
	 * Returns true if update, false otherwise
	 */
	bool UpdateNodeHeight(UFlowAbstractNode* Node, const FFlowAbstractGraphQuery& GraphQuery, const FRandomStream& Random, int MaxClimbHeight,
			const UCellFlowLayoutTaskFinalize::FHeightIncompatibilityState IncompatibilityState) {
		TArray<UFlowAbstractNode*> ConnectedNodes;
		GetConnectedNodes(Node, GraphQuery, ConnectedNodes);

		if (ConnectedNodes.Num() == 0) {
			return false;
		}

		auto IsNodeSetupIncompatible = [&IncompatibilityState](const UFlowAbstractNode* Node, const UFlowAbstractNode* ConnectedNode) {
			const int NodeZ = FMath::RoundToInt(Node->Coord.Z);
			const int ConnectedNodeZ = FMath::RoundToInt(ConnectedNode->Coord.Z);

			if (NodeZ == ConnectedNodeZ) {
				return false;	// Same height, assume compatible
			}
			else if (NodeZ < ConnectedNodeZ) {
				return IncompatibilityState.IsIncompatible(Node->NodeId, ConnectedNode->NodeId);
			}
			else {
				return IncompatibilityState.IsIncompatible(ConnectedNode->NodeId, Node->NodeId);
			}
		};

		{
			int Penalty = 0;
			const int NodeZ = FMath::RoundToInt(Node->Coord.Z);
			for (const UFlowAbstractNode* ConnectedNode : ConnectedNodes) {
				const int ConnectedNodeZ = FMath::RoundToInt(ConnectedNode->Coord.Z);
				const int HeightDiff = FMath::Abs(ConnectedNodeZ - NodeZ);

				const int AllowedClimbHeight = IsNodeSetupIncompatible(Node, ConnectedNode) ? 0 : MaxClimbHeight;
				Penalty += FMath::Max(0, HeightDiff - AllowedClimbHeight);
			}
			if (Penalty == 0) {
				return false;
			}
		}

		const int NodeZ = FMath::RoundToInt(Node->Coord.Z);
		int NewZ = NodeZ;
		for (const UFlowAbstractNode* ConnectedNode : ConnectedNodes) {
			const int ConnectedNodeZ = FMath::RoundToInt(ConnectedNode->Coord.Z);
			const int HeightDiff = FMath::Abs(ConnectedNodeZ - NodeZ);
			const int AllowedClimbHeight = IsNodeSetupIncompatible(Node, ConnectedNode) ? 0 : MaxClimbHeight;
			
			if (HeightDiff > AllowedClimbHeight) {
				const float MidZ = (NodeZ + ConnectedNodeZ) * 0.5f;
				const int MidZLo = FMath::FloorToInt(MidZ);
				const int MidZHi = FMath::CeilToInt(MidZ);
				if (NewZ == MidZLo) {
					NewZ = MidZHi;
				}
				else if (NewZ == MidZHi) {
					NewZ = MidZLo;
				}
				else {
					NewZ = Random.FRand() < 0.5f ? MidZLo : MidZHi;
				}

				if (NewZ != NodeZ) {
					break;
				}
			}
		}

		if (NewZ == NodeZ) {
			return false;
		}

		Node->Coord.Z = NewZ;
		return true;
	}
	
	bool FixCellHeightVariations(int NumIterations, const FFlowAbstractGraphQuery& GraphQuery, const FRandomStream& Random, int MaxClimbHeight,
			const UCellFlowLayoutTaskFinalize::FHeightIncompatibilityState IncompatibilityState)
	{
		UCellFlowLayoutGraph* Graph = GraphQuery.GetGraph<UCellFlowLayoutGraph>();
		int IterationIdx{};
		bool bSuccess{false};
		for (IterationIdx = 0; IterationIdx < NumIterations; IterationIdx++) {
			TArray<int> NodeIndices;
			FMathUtils::GetShuffledIndices(Graph->GraphNodes.Num(), Random, NodeIndices);
			bool bChangedThisFrame{};
			for (const int NodeIdx : NodeIndices) {
				UFlowAbstractNode* Node = Graph->GraphNodes[NodeIdx];
				if (!Node || !Node->bActive) continue;
				bChangedThisFrame |= UpdateNodeHeight(Node, GraphQuery, Random, MaxClimbHeight, IncompatibilityState);
			}

			if (!bChangedThisFrame) {
				// The heights were not changed this frame.
				bSuccess = true;
				break;
			}
		}

		UE_LOG(LogCellFlow, Log, TEXT("Fixed height variations in [%d] Iterations"), (IterationIdx + 1));
		return bSuccess;
	}
}

void UCellFlowLayoutTaskFinalize::FHeightIncompatibilityState::RegisterIncompatibleHeights(const FGuid& GroupLow, const FGuid& GroupHi) {
	FIncompatibleChunkHeights& Info = IncompatibleChunkHeights.FindOrAdd(GetKeyHash(GroupLow, GroupHi));
	Info.LowerLayoutNodeId = GroupLow;
	Info.HigherLayoutNodeId = GroupHi;
}

bool UCellFlowLayoutTaskFinalize::FHeightIncompatibilityState::IsIncompatible(const FGuid& GroupLow, const FGuid& GroupHi) const {
	return IncompatibleChunkHeights.Contains(GetKeyHash(GroupLow, GroupHi));
}

uint32 UCellFlowLayoutTaskFinalize::FHeightIncompatibilityState::GetKeyHash(const FGuid& GroupLow, const FGuid& GroupHi) {
	return HashCombine( GetTypeHash(GroupLow), GetTypeHash(GroupHi));
}

void UCellFlowLayoutTaskFinalize::InitializeCellGraph(UDAFlowCellGraph* InCellGraph) {
	InCellGraph->GridInfo = {};
	InCellGraph->DCELInfo = {};
}

bool UCellFlowLayoutTaskFinalize::AssignGridConnections(UDAFlowCellGraph* CellGraph, UCellFlowLayoutGraph* LayoutGraph, const FFlowAbstractGraphQuery& LayoutGraphQuery,
                                                        const FRandomStream& Random, FHeightIncompatibilityState& IncompatibilityState) const {
	/*
	 * Struct to hold all the cell tile info
	 * Struct to hold the edge info
	 * For N iterations:
	 *   Init Cell tilemap state info
	 *   Foreach Link in Layout Graph:
	 *     Get Shared edges between the two nodes of the link
	 *     Shuffle the edge list
	 *     Find the first valid edge and create a door and stair combo
	 *     Update the Tilemap with the new occupancy constraint
	 *     If cannot find it, throw away the result and try again with a new iteration
	 *   If all connections placed successfully, Return true
	 * If Iterations completed and no result found, Return false 
	 */

	TArray<FCellFlowGridEdgeInfo>& HalfEdges = CellGraph->GridInfo.HalfEdges;
	TMap<int32, FDAFlowCellGraphGridStairInfo>& Stairs = CellGraph->GridInfo.Stairs;
	
	DA::FCellGraphBuilder::GenerateEdgeList(CellGraph, HalfEdges, LayoutGraphQuery, false);
	
	TMap<int32, TArray<int>> GroupEdges;	// Maps the GroupId -> Edges Array Index
	for (int i = 0; i < HalfEdges.Num(); i++) {
		const FCellFlowGridEdgeInfo& Edge = HalfEdges[i];
		const FCellFlowGridEdgeInfo& TwinEdge = HalfEdges[Edge.TwinIndex];
		if (Edge.TileGroup != INDEX_NONE && Edge.TileGroup != TwinEdge.TileGroup) {
			TArray<int>& GroupEdgeList = GroupEdges.FindOrAdd(Edge.TileGroup);
			GroupEdgeList.Add(i);
		}
	} 

	TArray<const FDAFlowCellGroupNode*> ValidGroupNodes;
	for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
		if (GroupNode.IsActive() && GroupNode.LayoutNodeID.IsValid()) {
			const UFlowAbstractNode* LayoutGraphNode = LayoutGraphQuery.GetNode(GroupNode.LayoutNodeID);
			if (LayoutGraphNode && LayoutGraphNode->bActive) {
				ValidGroupNodes.Add(&GroupNode);
			}
		}
	}
	
	TMap<FGuid, int32> LayoutToGroupMap;
	for (const FDAFlowCellGroupNode* GroupNode : ValidGroupNodes) {
		if (GroupNode->IsActive() && GroupNode->LayoutNodeID.IsValid()) {
			UFlowAbstractNode* LayoutGraphNode = LayoutGraphQuery.GetNode(GroupNode->LayoutNodeID);
			if (!LayoutGraphNode || !LayoutGraphNode->bActive) {
				continue;
			}
			
			int& GroupIdRef = LayoutToGroupMap.FindOrAdd(GroupNode->LayoutNodeID);
			GroupIdRef = GroupNode->GroupId;
		}
	}
	
	TMap<FIntPoint, TArray<int>> LinkEdgeMap;	// Maps: (SrcGroupId, DstGroupId) -> List of edge indices in the Edges Array
	for (UFlowAbstractLink* GraphLink : LayoutGraph->GraphLinks) {
		if (GraphLink->Type == EFlowAbstractLinkType::Unconnected) {
			continue;
		}

		const int* SrcGroupIdPtr = LayoutToGroupMap.Find(GraphLink->Source);
		const int* DstGroupIdPtr = LayoutToGroupMap.Find(GraphLink->Destination);
		if (!SrcGroupIdPtr || !DstGroupIdPtr) {
			continue;
		}

		const int SrcGroupId = *SrcGroupIdPtr;
		const int DstGroupId = *DstGroupIdPtr;

		FIntPoint Key(SrcGroupId, DstGroupId);
		TArray<int>& LinkEdges = LinkEdgeMap.FindOrAdd(Key);

		const TArray<int>* SrcEdgesPtr = GroupEdges.Find(SrcGroupId);
		if (!SrcEdgesPtr) continue;
		
		const TArray<int>& SrcEdges = *SrcEdgesPtr;
		for (int SrcEdgeIdx : SrcEdges) {
			const FCellFlowGridEdgeInfo& Edge = HalfEdges[SrcEdgeIdx];
			check(Edge.TileGroup == SrcGroupId);
			check(Edge.TwinIndex != INDEX_NONE);

			const FCellFlowGridEdgeInfo& TwinEdge = HalfEdges[Edge.TwinIndex];
			
			if (TwinEdge.TileGroup == DstGroupId) {
				LinkEdges.Add(SrcEdgeIdx);
			}
		}
	}
	
	TMap<FIntPoint, int32> CellGroups;
	for (const FDAFlowCellGroupNode* GroupNode : ValidGroupNodes) {
		if (!GroupNode->IsActive()) continue;
		for (const int LeafNodeId : GroupNode->LeafNodes) {
			if (const UDAFlowCellLeafNodeGrid* GridLeafNode = Cast<UDAFlowCellLeafNodeGrid>(CellGraph->LeafNodes[LeafNodeId])) {
				for (int y = GridLeafNode->Location.Y; y < GridLeafNode->Location.Y + GridLeafNode->Size.Y; y++) {
					for (int x = GridLeafNode->Location.X; x < GridLeafNode->Location.X + GridLeafNode->Size.X; x++) {
						int32& GroupRef = CellGroups.FindOrAdd(FIntPoint(x, y));
						GroupRef = GroupNode->GroupId;
					}
				}
			}
		}
	}

	TSet<FIntPoint> OccupiedCells;	// The cells that have been occupied (by stairs)
	TSet<FIntPoint> ForceFreeCells;	// The cells that need to be free (e.g. opening of a door, exit of a stair)
	for (UFlowAbstractLink* GraphLink : LayoutGraph->GraphLinks) {
		if (GraphLink->Type == EFlowAbstractLinkType::Unconnected) {
			continue;
		}

		const int* SrcGroupIdPtr = LayoutToGroupMap.Find(GraphLink->Source);
		const int* DstGroupIdPtr = LayoutToGroupMap.Find(GraphLink->Destination);
		if (!SrcGroupIdPtr || !DstGroupIdPtr) {
			continue;
		}

		const int SrcGroupId = *SrcGroupIdPtr;
		const int DstGroupId = *DstGroupIdPtr;
		
		FIntPoint Key(SrcGroupId, DstGroupId);
		TArray<int>& LinkEdges = LinkEdgeMap.FindOrAdd(Key);
		if (LinkEdges.IsEmpty()) {
			continue;
		}
		
		// Randomize the link edges
		TArray<int> LinkEdgeIndices;
		FMathUtils::GetShuffledIndices(LinkEdges.Num(), Random, LinkEdgeIndices);
		bool bHandled{false};

		for (int i = 0; i < LinkEdges.Num(); i++) {
			const int LinkEdgeArrayIdx = LinkEdgeIndices[i];
			const int LinkEdgeIdx = LinkEdges[LinkEdgeArrayIdx];
			FCellFlowGridEdgeInfo& Edge = HalfEdges[LinkEdgeIdx];
			FCellFlowGridEdgeInfo& TwinEdge = HalfEdges[Edge.TwinIndex];
			
			// Make sure both the cells of the edge are not occupied
			if (OccupiedCells.Contains(Edge.Coord) || OccupiedCells.Contains(TwinEdge.Coord)) {
				continue;
			}

			if (Edge.HeightZ == TwinEdge.HeightZ) {
				// Same height and the space adjacent to the edge is not occupied. Make a connection here
				Edge.bConnection = true;
				TwinEdge.bConnection = true;

				// Make sure the tiles adjacent to the edge are not occupied at a later iteration (e.g. by a stair, blocking this door path way)
				ForceFreeCells.Add(Edge.Coord);
				ForceFreeCells.Add(TwinEdge.Coord);
				bHandled = true;
				break;
			}
			else {
				// Height different, Check if we can have a stair here
				auto HandleStair = [&OccupiedCells, &ForceFreeCells, &CellGroups, &Stairs](FCellFlowGridEdgeInfo& EdgeLo, FCellFlowGridEdgeInfo& EdgeHi) -> bool {
					check(EdgeLo.HeightZ < EdgeHi.HeightZ);
					// TODO: Handle stairs of height more than one

					// Make sure the adjacent stair location belongs to the same group
					const int EdgeLoGroup = CellGroups[FIntPoint(EdgeLo.Coord.X, EdgeLo.Coord.Y)];

					auto IsCellFreeInGroup = [&OccupiedCells, &CellGroups, EdgeLoGroup](const FIntPoint& InTileCoord) {
						if (OccupiedCells.Contains(InTileCoord)) {
							return false;
						}
						const int* TileGroupPtr = CellGroups.Find(InTileCoord);
						if (!TileGroupPtr) {
							return false;
						}
					
						const int TileGroup = *TileGroupPtr;
						if (EdgeLoGroup != TileGroup) {
							// They belong to different group, i.e. the stair leads down to a wall
							return false;
						}

						return true;
					};
					
					// The tile adjacent to the stair entrance. This needs to be free and tagged as such
					const FIntPoint StairDownDir = EdgeLo.Coord - EdgeHi.Coord;
					const FIntPoint DirRight {StairDownDir.Y, -StairDownDir.X};
					
					const FIntPoint StairEntranceForceFreeLoc = EdgeLo.Coord + FIntPoint(StairDownDir.X, StairDownDir.Y);

					/*
					 * Layout of the door, below which is the stair (C0).
					 * Case 1: The space in C1 needs to be empty, so it can lead up to the staircase
					 * Case 2: If L0 is empty, L1 needs to be empty, so it can access the staircase through C1
					 * Case 3: Similarly, if R0 is empty, R1 also needs to be empty, so it can access the staircase through C1
					 * 
					 *  -- DOOR --
					 *  L0  C0  R0
					 *  L1  C1  R1
					 */
					const FIntPoint C0 = EdgeLo.Coord;
					const FIntPoint C1 = StairEntranceForceFreeLoc;
					const FIntPoint L0 = C0 - DirRight;
					const FIntPoint L1 = C1 - DirRight;
					const FIntPoint R0 = C0 + DirRight;
					const FIntPoint R1 = C1 + DirRight;

					if (ForceFreeCells.Contains(C0)) {
						// We cannot occupy the stair cell as it needs to be free
						return false;
					}
					
					// Case 1
					if (!IsCellFreeInGroup(C0) || !IsCellFreeInGroup(C1)) {
						return false;
					}

					// Case 2
					if (IsCellFreeInGroup(L0) && !IsCellFreeInGroup(L1)) {
						return false;
					}

					// Case 3
					if (IsCellFreeInGroup(R0) && !IsCellFreeInGroup(R1)) {
						return false;
					}
					
					ForceFreeCells.Add(C1);
					ForceFreeCells.Add(EdgeHi.Coord);
					OccupiedCells.Add(C0);
					EdgeLo.bConnection = true;
					EdgeHi.bConnection = true;
					EdgeLo.bContainsStair = true;

					// Setup Stair info
					{
						const FIntPoint StairDir = EdgeLo.Coord - EdgeHi.Coord;
						float StairRotAngle{};
						if (StairDir.Y == -1) {
							StairRotAngle = 0;
						}
						else if (StairDir.Y == 1) {
							StairRotAngle = PI;
						}
						else if (StairDir.X == 1) {
							StairRotAngle = PI * 0.5f;
						}
						else {
							StairRotAngle = PI * 1.5f;
						}

						FDAFlowCellGraphGridStairInfo& Stair = Stairs.FindOrAdd(EdgeLo.EdgeIndex);
						Stair.EdgeIndex = EdgeLo.EdgeIndex;
						Stair.AngleRadians = StairRotAngle;
						Stair.Direction = FVector(StairDir.X, StairDir.Y, 0);
						Stair.LocalLocation = FVector(EdgeLo.Coord.X + 0.5F, EdgeLo.Coord.Y + 0.5F, EdgeLo.HeightZ);
					}
					
					return true;
				};
				
				if (Edge.HeightZ < TwinEdge.HeightZ) {
					if (HandleStair(Edge, TwinEdge)) {
						bHandled = true;
						break;
					}
				}
				else {
					if (HandleStair(TwinEdge, Edge)) {
						bHandled = true;
						break;
					}
				}
			}
		}

		if (!bHandled) {
			FCellFlowGridEdgeInfo& Edge = HalfEdges[0];
			FCellFlowGridEdgeInfo& TwinEdge = HalfEdges[Edge.TwinIndex];
			if (Edge.HeightZ != TwinEdge.HeightZ) {
				int GroupLo = Edge.HeightZ < TwinEdge.HeightZ ? Edge.TileGroup : TwinEdge.TileGroup;
				int GroupHi = Edge.HeightZ > TwinEdge.HeightZ ? Edge.TileGroup : TwinEdge.TileGroup;
				const FGuid& NodeLo = CellGraph->GroupNodes[GroupLo].LayoutNodeID;
				const FGuid& NodeHi = CellGraph->GroupNodes[GroupHi].LayoutNodeID;
				IncompatibilityState.RegisterIncompatibleHeights(NodeLo, NodeHi);
			}
			return false;
		}
	}
	
	return true;
}

bool UCellFlowLayoutTaskFinalize::AssignDCELConnections(UDAFlowCellGraph* InCellGraph, UCellFlowVoronoiGraph* InVoronoiData,
		UCellFlowLayoutGraph* InLayoutGraph, const FFlowAbstractGraphQuery& InLayoutGraphQuery, const FRandomStream& InRandom,
		FHeightIncompatibilityState& IncompatibilityState)
{
	if (!InVoronoiData) {
		// No connections to resolve
		return true;
	}
	
	TArray<DA::DCEL::FEdge*>& HalfEdges = InVoronoiData->DGraph.GetEdges();
	TMap<int, int> LeafToGroupMap;
	for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
		for (int LeafNodeId : GroupNode.LeafNodes) {
			int& GroupIdRef = LeafToGroupMap.FindOrAdd(LeafNodeId);
			GroupIdRef = GroupNode.GroupId;
		}
	}
	
	TMap<int32, TArray<int>> GroupEdges;	// Maps the GroupId -> Edges Array Index
	for (int i = 0; i < HalfEdges.Num(); i++) {
		const DA::DCEL::FEdge* Edge = HalfEdges[i];
		if (Edge->LeftFace && Edge->LeftFace->bValid) {
			const int* GroupIdPtr = LeafToGroupMap.Find(Edge->LeftFace->FaceId);
			if (!GroupIdPtr) continue;

			if (Edge->Twin->LeftFace) {
				// Make sure they don't belong to the same group
				const int* TwinGroupIdPtr = LeafToGroupMap.Find(Edge->Twin->LeftFace->FaceId);
				if (TwinGroupIdPtr && *TwinGroupIdPtr == *GroupIdPtr) {
					continue;
				}
			}
			
			TArray<int>& GroupEdgeList = GroupEdges.FindOrAdd(*GroupIdPtr);
			GroupEdgeList.Add(i);
		}
	}

	TArray<const FDAFlowCellGroupNode*> ValidGroupNodes;
	TMap<FGuid, int32> LayoutToGroupMap;
	for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
		if (GroupNode.IsActive() && GroupNode.LayoutNodeID.IsValid()) {
			LayoutToGroupMap.Add(GroupNode.LayoutNodeID, GroupNode.GroupId);
			const UFlowAbstractNode* LayoutGraphNode = InLayoutGraphQuery.GetNode(GroupNode.LayoutNodeID);
			if (LayoutGraphNode && LayoutGraphNode->bActive) {
				ValidGroupNodes.Add(&GroupNode);
			}
		}
	}
	
	TMap<FIntPoint, TArray<int>> LinkEdgeMap;	// Maps: (SrcGroupId, DstGroupId) -> List of edge indices in the Edges Array
	TMap<UFlowAbstractLink*, int> LinkEdgeCount;
	for (UFlowAbstractLink* GraphLink : InLayoutGraph->GraphLinks) {
		if (GraphLink->Type == EFlowAbstractLinkType::Unconnected) {
			continue;
		}

		const int* SrcGroupIdPtr = LayoutToGroupMap.Find(GraphLink->Source);
		const int* DstGroupIdPtr = LayoutToGroupMap.Find(GraphLink->Destination);
		if (!SrcGroupIdPtr || !DstGroupIdPtr) {
			continue;
		}

		const int SrcGroupId = *SrcGroupIdPtr;
		const int DstGroupId = *DstGroupIdPtr;

		FIntPoint Key(SrcGroupId, DstGroupId);
		TArray<int>& LinkEdges = LinkEdgeMap.FindOrAdd(Key);

		const TArray<int>* SrcEdgesPtr = GroupEdges.Find(SrcGroupId);
		if (!SrcEdgesPtr) continue;
		
		const TArray<int>& SrcEdges = *SrcEdgesPtr;
		for (int SrcEdgeIdx : SrcEdges) {
			const DA::DCEL::FEdge* Edge = HalfEdges[SrcEdgeIdx];
			if (!Edge->Twin || !Edge->Twin->LeftFace) continue;
			
			const DA::DCEL::FEdge* TwinEdge = Edge->Twin;

			if (const int* TwinGroupIdPtr = LeafToGroupMap.Find(TwinEdge->LeftFace->FaceId)) {
				const int TwinGroupId = *TwinGroupIdPtr;
				if (TwinGroupId == DstGroupId) {
					LinkEdges.Add(SrcEdgeIdx);
				}
			}
		}

		int& CountRef = LinkEdgeCount.FindOrAdd(GraphLink);
		CountRef = LinkEdges.Num();
	}

	TArray<int>& DoorEdges = InCellGraph->DCELInfo.DoorEdges;
	TMap<int32, FDAFlowCellGraphDCELStairInfo>& Stairs = InCellGraph->DCELInfo.Stairs;

	// Start processing the links with the least shared edges between the chunks first. This would give them more free space during placement testing as other stairs would not get in the way 
	TArray<UFlowAbstractLink*> GraphLinksToProcess = InLayoutGraph->GraphLinks;
	GraphLinksToProcess.Sort([&LinkEdgeCount](const UFlowAbstractLink& A, const UFlowAbstractLink& B) {
		return LinkEdgeCount[&A] < LinkEdgeCount[&B];
	});
	
	for (UFlowAbstractLink* GraphLink : GraphLinksToProcess) {
		if (GraphLink->Type == EFlowAbstractLinkType::Unconnected) {
			continue;
		}

		const int* SrcGroupIdPtr = LayoutToGroupMap.Find(GraphLink->Source);
		const int* DstGroupIdPtr = LayoutToGroupMap.Find(GraphLink->Destination);
		if (!SrcGroupIdPtr || !DstGroupIdPtr) {
			continue;
		}

		const int SrcGroupId = *SrcGroupIdPtr;
		const int DstGroupId = *DstGroupIdPtr;
		
		FIntPoint Key(SrcGroupId, DstGroupId);
		const TArray<int>* LinkEdgesPtr = LinkEdgeMap.Find(Key);
		if (!LinkEdgesPtr || LinkEdgesPtr->IsEmpty()) {
			continue;
		}
		
		TArray<int> LinkEdges = LinkEdgesPtr->FilterByPredicate([&HalfEdges](int EdgeIdx) -> bool {
			const DA::DCEL::FEdge* Edge = HalfEdges[EdgeIdx];
			const double EdgeWidth = (Edge->Origin->Location - Edge->Twin->Origin->Location).Size();
			return EdgeWidth > 0.5;
		});
		
		if (LinkEdges.IsEmpty()) {
			continue;
		}

		// Randomize the link edges
		struct FPenaltySensor {
			FVector2d Location{};
			double Radius{};
			double PenaltyMultiplier{};
		};

		auto GenerateEdgePenaltySensors = [](const DA::DCEL::FEdge* Edge) {
			TArray<FPenaltySensor> Sensors;

			const double EdgeWidth = (Edge->Twin->Origin->Location - Edge->Origin->Location).Size();
			const FVector2d EdgeDir = (Edge->Twin->Origin->Location - Edge->Origin->Location) / EdgeWidth;
			const FVector EdgeDir3D(EdgeDir.X, EdgeDir.Y, 0);
			const FVector StairDir3D = FQuat(FVector::UpVector, PI * 0.5f).RotateVector(EdgeDir3D);
			const FVector2d StairDir{StairDir3D.X, StairDir3D.Y};
			const FVector2d EdgeMid = (Edge->Twin->Origin->Location + Edge->Origin->Location) * 0.5f;
			const double EdgeLength = (Edge->Origin->Location - Edge->Twin->Origin->Location).Size();
			const double StairWidth = FMath::Min(1, EdgeLength);
			const double HalfStairWidth = StairWidth * 0.5; 

			// Sensor in the middle of the stair box
			{
				// The box size is 1x1
				FPenaltySensor& Sensor = Sensors.AddDefaulted_GetRef();
				Sensor.Radius = FMath::Min(1, EdgeWidth) * 0.65;
				Sensor.Location = EdgeMid + StairDir * (1 - Sensor.Radius);
				Sensor.PenaltyMultiplier = 6;
			}

			// Sensor on the Lower tips of the staircase
			{
				FPenaltySensor& Sensor = Sensors.AddDefaulted_GetRef();
				Sensor.Radius = FMath::Min(1, EdgeWidth) * 0.45;
				Sensor.Location = EdgeMid + StairDir - EdgeDir * HalfStairWidth;
				Sensor.PenaltyMultiplier = 1;
			}
			
			{
				FPenaltySensor& Sensor = Sensors.AddDefaulted_GetRef();
				Sensor.Radius = FMath::Min(1, EdgeWidth) * 0.45;
				Sensor.Location = EdgeMid + StairDir;
				Sensor.PenaltyMultiplier = 2;
			}
			
			{
				FPenaltySensor& Sensor = Sensors.AddDefaulted_GetRef();
				Sensor.Radius = FMath::Min(1, EdgeWidth) * 0.45;
				Sensor.Location = EdgeMid + StairDir + EdgeDir * HalfStairWidth;
				Sensor.PenaltyMultiplier = 1;
			}

			
			return Sensors;
		};

		auto CalculateStairPlacementPenalty = [](const TArray<FPenaltySensor>& InSensors, const DA::DCEL::FEdge* EdgeToTest) {
			double Penalty = 0;
			for (const FPenaltySensor& Sensor : InSensors) {
				const double DistanceToSensor = FMathUtils::GetLineToPointDist2D(EdgeToTest->Origin->Location, EdgeToTest->Twin->Origin->Location, Sensor.Location);
				if (DistanceToSensor < Sensor.Radius) {
					double SensorPenalty = (Sensor.Radius - DistanceToSensor) / Sensor.Radius;
					SensorPenalty = FMath::Pow(SensorPenalty, 2.0);
					SensorPenalty *= Sensor.PenaltyMultiplier;
					Penalty += SensorPenalty;
				}
			}

			return Penalty;
		};
		
		TMap<int, double> StairPlacementPenalties;
		for (int LinkEdgeIdx : LinkEdges) {
			const DA::DCEL::FEdge* Edge = HalfEdges[LinkEdgeIdx];
			check(Edge->Index == LinkEdgeIdx);
			
			if (!Edge->LeftFace || !Edge->Twin->LeftFace) continue;
			int LowerGroupId{};
			int EdgeZ = InCellGraph->LeafNodes[Edge->LeftFace->FaceId]->LogicalZ;
			int TwinEdgeZ = InCellGraph->LeafNodes[Edge->Twin->LeftFace->FaceId]->LogicalZ;
			if (TwinEdgeZ < EdgeZ) {
				Edge = Edge->Twin;
				FMathUtils::Swap(EdgeZ, TwinEdgeZ);
			}
			
			{
				const int* LowerEdgeGroupIdPtr = LeafToGroupMap.Find(EdgeZ < TwinEdgeZ
						? Edge->LeftFace->FaceId
						: Edge->Twin->LeftFace->FaceId);
				if (!LowerEdgeGroupIdPtr) {
					continue;
				}

				LowerGroupId = *LowerEdgeGroupIdPtr;
			}

			const TArray<int>* GroupEdgesPtr = GroupEdges.Find(LowerGroupId);
			if (!GroupEdgesPtr) {
				continue;
			}
			const TArray<int>& GroupEdgesToTest = *GroupEdgesPtr;
			
			TArray<FPenaltySensor> EdgePenaltySensors = GenerateEdgePenaltySensors(Edge);

			double TotalPenalty{};
			
			for (int LinkEdgeToTestIdx : GroupEdgesToTest) {
				if (LinkEdgeToTestIdx == Edge->Index || LinkEdgeToTestIdx == Edge->Twin->Index) {
					continue;
				}
				const DA::DCEL::FEdge* EdgeToTest = HalfEdges[LinkEdgeToTestIdx];
				double TestPenalty = CalculateStairPlacementPenalty(EdgePenaltySensors, EdgeToTest);
				TotalPenalty += TestPenalty;
			}

			const double EdgeLength = (Edge->Origin->Location - Edge->Twin->Origin->Location).Size();
			if (EdgeLength < 1) {
				TotalPenalty += FMath::Square(1 - EdgeLength);
			}
			
			{
				double& EdgePenaltyRef = StairPlacementPenalties.FindOrAdd(Edge->Index);
				EdgePenaltyRef = TotalPenalty;
				
				double& TwinEdgePenaltyRef = StairPlacementPenalties.FindOrAdd(Edge->Twin->Index);
				TwinEdgePenaltyRef = TotalPenalty;
			}

			// Insert debug data
			if (bDebugStairPlacement) {
				// Draw the placement penalty info
				FDASceneDebugData& DebugData = InCellGraph->SceneDebugData;
				{
					const FVector2d EdgeMid2D = (Edge->Origin->Location + Edge->Twin->Origin->Location) * 0.5;
					if (!Edge->Twin->LeftFace) continue;

					const UDAFlowCellLeafNode* LeafNode = InCellGraph->LeafNodes[Edge->Twin->LeftFace->FaceId];
					if (!LeafNode) continue;
					const int LogicalHeightZ = LeafNode->LogicalZ;

					const FVector2d EdgeDir2D = (Edge->Twin->Origin->Location - Edge->Origin->Location).GetSafeNormal();
					const FVector EdgeDir(EdgeDir2D.X, EdgeDir2D.Y, 0);
					const FVector StairDirection = FQuat(FVector::UpVector, PI * 0.5f).RotateVector(EdgeDir);
					const FVector Location = (FVector(EdgeMid2D.X, EdgeMid2D.Y, LogicalHeightZ) + StairDirection * 0.15);
					FString DebugText = FString::SanitizeFloat(TotalPenalty, 2);
					DebugData.TextEntries.Add({FText::FromString(DebugText), Location, FLinearColor::Red});
				}
				
				for (FPenaltySensor& Sensor : EdgePenaltySensors) {
					const FVector SensorLocation = FVector(FVector2D(Sensor.Location), FMath::Min(EdgeZ, TwinEdgeZ));
					DebugData.SphereEntries.Add({SensorLocation, Sensor.Radius, FLinearColor::Red});
				}
			}
		};

		FMathUtils::Shuffle(LinkEdges, InRandom);
		LinkEdges.Sort([&StairPlacementPenalties](int EdgeIdxA, int EdgeIdxB) {
			return StairPlacementPenalties[EdgeIdxA] < StairPlacementPenalties[EdgeIdxB];
		});
		
		bool bHandled{false};
		for (int LinkEdgeIdx : LinkEdges) {
			DA::DCEL::FEdge* Edge = HalfEdges[LinkEdgeIdx];
			
			const double LogicalEdgeWidth = (Edge->Origin->Location - Edge->Twin->Origin->Location).Size();
			if (LogicalEdgeWidth < 0.5f) {
				// The door should be at least half the grid size width. E.g. if the grid size is 400x400 units, the door opening should be at least 200 units
				continue;
			}
			const double DoorWidth = FMath::Min(LogicalEdgeWidth, 1.0f);
			
			UDAFlowCellLeafNode* LeafNode = InCellGraph->LeafNodes[Edge->LeftFace->FaceId];
			UDAFlowCellLeafNode* TwinLeafNode = InCellGraph->LeafNodes[Edge->Twin->LeftFace->FaceId];
			if (!LeafNode || !TwinLeafNode) continue;

			DoorEdges.Add(Edge->Index);
			DoorEdges.Add(Edge->Twin->Index);
			bHandled = true;

			if (LeafNode->LogicalZ != TwinLeafNode->LogicalZ) {
				DA::DCEL::FEdge* StairEdge = (LeafNode->LogicalZ < TwinLeafNode->LogicalZ) ? Edge : Edge->Twin;
				UDAFlowCellLeafNode* StairLeafNode = (LeafNode->LogicalZ < TwinLeafNode->LogicalZ) ? LeafNode : TwinLeafNode;
				
				FDAFlowCellGraphDCELStairInfo& Stair = Stairs.FindOrAdd(StairEdge->Index);
				Stair.EdgeIndex = StairEdge->Index;
				Stair.LogicalWidth = DoorWidth;

				FVector2D BaseLoc2D = (StairEdge->Twin->Origin->Location + StairEdge->Origin->Location) * 0.5;
				Stair.LogicalLocation = FVector(BaseLoc2D, StairLeafNode->LogicalZ);
				
				FVector2d EdgeDir2D = (StairEdge->Twin->Origin->Location - StairEdge->Origin->Location).GetSafeNormal();
				FVector EdgeDir(EdgeDir2D.X, EdgeDir2D.Y, 0);
				Stair.Direction = FQuat(FVector::UpVector, PI * 0.5f).RotateVector(EdgeDir);
			}
			
			break;
		}

		if (!bHandled) {
			return false;
		}
	}
	
	return true;
}

void UCellFlowLayoutTaskFinalize::Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) {
	Super::Execute(Input, InExecSettings, Output);
	
	UDAFlowCellGraph* CellGraph = Output.State->GetState<UDAFlowCellGraph>(UDAFlowCellGraph::StateTypeID);
    UCellFlowLayoutGraph* LayoutGraph = Output.State->GetState<UCellFlowLayoutGraph>(UFlowAbstractGraphBase::StateTypeID);
	UCellFlowVoronoiGraph* VoronoiData = Output.State->GetState<UCellFlowVoronoiGraph>(UCellFlowVoronoiGraph::StateTypeID);
	const FFlowAbstractGraphQuery GraphQuery(LayoutGraph);
	
#if WITH_EDITORONLY_DATA
	CellGraph->bRenderInactiveGroups = false;
#endif // WITH_EDITORONLY_DATA
	
	const FRandomStream& Random = *Input.Random;

	// Randomize the height of the cells
	for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
		if (UFlowAbstractNode* LayoutNode = GraphQuery.GetNode(GroupNode.LayoutNodeID)) {
			LayoutNode->Coord.Z = Random.RandRange(MinHeight, MaxHeight);
		}
	}

	FHeightIncompatibilityState IncompatibilityState;
	auto ResolveHeights = [&]() -> bool {
		// Iteratively fix the height incompatibilities
		{
			const int Iterations = FMath::Max(1, HeightResolveIterations);
			const bool bSuccess = DACellFlowStairUtil::FixCellHeightVariations(Iterations, GraphQuery, Random, MaxClimbHeight, IncompatibilityState);
			if (!bSuccess) {
				return false;
			}
		}

		// Assign the heights to the group leaf nodes.
		{
			for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
				if (const UFlowAbstractNode* LayoutNode = GraphQuery.GetNode(GroupNode.LayoutNodeID)) {
					const int Height = FMath::RoundToInt(LayoutNode->Coord.Z);
					for (const int LeafNodeId : GroupNode.LeafNodes) {
						if (UDAFlowCellLeafNode* LeafNode = Cast<UDAFlowCellLeafNode>(CellGraph->LeafNodes[LeafNodeId])) {
							LeafNode->LogicalZ = Height;
						}
					}
				}
			}
		}
		// TODO: Assign further height variations along the group leaf nodes
		
		return true;
	};

	auto ResolveStairs = [&]() {
		bool bSuccess = AssignGridConnections(CellGraph, LayoutGraph, GraphQuery, Random, IncompatibilityState);		// TODO: Merge Grid with DCEL in the future
		if (bSuccess) {
			bSuccess |= AssignDCELConnections(CellGraph, VoronoiData, LayoutGraph, GraphQuery, Random, IncompatibilityState);
		}
		return bSuccess;
	};

	// Spawn Items
	auto ResolveSpawns = [&]() {
		bool bSuccess = SpawnItemsGrid(CellGraph, GraphQuery, Random);
		if (bSuccess) {
			bSuccess = SpawnItemsVoronoi(CellGraph, VoronoiData, GraphQuery, Random);
		}
		return bSuccess;
	};
	

	int StairResolveIterIdx = 0;
	bool bResolveStairsSuccess{}, bResolveSpawnSuccess{};
	while ((!bResolveStairsSuccess || !bResolveSpawnSuccess) && StairResolveIterIdx < FMath::Max(StairResolveIterations, 1)) {
		InitializeCellGraph(CellGraph);
		if (!ResolveHeights()) {
			Output.ErrorMessage = "Cannot Resolve Heights";
			Output.ExecutionResult = EFlowTaskExecutionResult::FailRetry;
			return;
		}

		bResolveStairsSuccess = ResolveStairs();
		if (bResolveStairsSuccess) {
			bResolveSpawnSuccess = ResolveSpawns();
		}
		
		StairResolveIterIdx++;
	}

	if (!bResolveStairsSuccess) {
		Output.ErrorMessage = "Cannot Resolve Stairs";
		Output.ExecutionResult = EFlowTaskExecutionResult::FailRetry;
		return;
	}

	if (!bResolveSpawnSuccess) {
		Output.ErrorMessage = "Not enough space to spawn items";
		Output.ExecutionResult = EFlowTaskExecutionResult::FailRetry;
		return;
	}

	Output.ExecutionResult = EFlowTaskExecutionResult::Success;
}

bool UCellFlowLayoutTaskFinalize::SpawnItemsGrid(UDAFlowCellGraph* CellGraph, const FFlowAbstractGraphQuery& GraphQuery, const FRandomStream& Random) const {
	TArray<FCellFlowGridEdgeInfo>& HalfEdges = CellGraph->GridInfo.HalfEdges;
	TSet<FIntPoint> UnavailableTileCoords;
	TSet<FIntPoint> DoorAdjacentCoords;
	for (int i = 0; i < HalfEdges.Num(); i++) {
		const FCellFlowGridEdgeInfo& Edge = HalfEdges[i];
		const FCellFlowGridEdgeInfo& TwinEdge = HalfEdges[Edge.TwinIndex];
		if (Edge.bConnection) {
			UnavailableTileCoords.Add(Edge.Coord);
			if (Edge.bContainsStair) {
				const FIntPoint StairDownDir = Edge.Coord - TwinEdge.Coord;
				const FIntPoint DirRight {StairDownDir.Y, -StairDownDir.X};
				const FIntPoint StairEntrance = Edge.Coord + FIntPoint(StairDownDir.X, StairDownDir.Y);
				UnavailableTileCoords.Add(StairEntrance);
			}
		}
	}
	
	for (const FDAFlowCellGroupNode& GroupNode : CellGraph->GroupNodes) {
		UFlowAbstractNode* LayoutNode = GraphQuery.GetNode(GroupNode.LayoutNodeID);
		if (!LayoutNode || LayoutNode->NodeItems.Num() == 0 || GroupNode.LeafNodes.Num() == 0) {
			continue;
		}

		int Z{};
		
		// Gather the valid cells
		TArray<FIntPoint> AvailableCoords;
		bool bGridChunk{};
		for (const int LeafNodeIdx : GroupNode.LeafNodes) {
			if (!CellGraph->LeafNodes.IsValidIndex(LeafNodeIdx)) {
				continue;
			}

			if (const UDAFlowCellLeafNodeGrid* LeafNode = Cast<UDAFlowCellLeafNodeGrid>(CellGraph->LeafNodes[LeafNodeIdx])) {
				bGridChunk = true;
				Z = LeafNode->LogicalZ;
				for (int Y = LeafNode->Location.Y; Y < LeafNode->Location.Y + LeafNode->Size.Y; Y++) {
					for (int X = LeafNode->Location.X; X < LeafNode->Location.X + LeafNode->Size.X; X++) {
						FIntPoint Coord{X, Y};
						if (!UnavailableTileCoords.Contains(Coord)) {
							AvailableCoords.Add(Coord);
						}
					}
				}
			}
		}

		if (!bGridChunk) {
			continue;
		}

		if (AvailableCoords.Num() < LayoutNode->NodeItems.Num()) {
			return false;
		}

		TArray<FDAFlowCellGraphSpawnInfo>& SpawnList = CellGraph->GridInfo.SpawnInfo;
		for (const UFlowGraphItem* NodeItem : LayoutNode->NodeItems) {
			FIntPoint Coord2D;
			{
				const int AvailableTileIdx = Random.RandRange(0, AvailableCoords.Num() - 1);
				Coord2D = AvailableCoords[AvailableTileIdx];
				AvailableCoords.RemoveAtSwap(AvailableTileIdx);
			}
			
			FDAFlowCellGraphSpawnInfo& SpawnInfo = SpawnList.AddDefaulted_GetRef();
			SpawnInfo.Coord = FVector(Coord2D.X, Coord2D.Y, Z);
			SpawnInfo.Item = NodeItem;
			SpawnInfo.GroupId = GroupNode.GroupId;
		}
	}

	return true;
}

bool UCellFlowLayoutTaskFinalize::SpawnItemsVoronoi(UDAFlowCellGraph* InCellGraph, UCellFlowVoronoiGraph* InVoronoiData, const FFlowAbstractGraphQuery& InGraphQuery, const FRandomStream& InRandom) const {
	const TArray<DA::DCEL::FEdge*>& HalfEdges = InVoronoiData->DGraph.GetEdges();
	for (const FDAFlowCellGroupNode& GroupNode : InCellGraph->GroupNodes) {
		if (!GroupNode.IsActive()) {
			continue;
		}

		// Check if this group maps to a valid layout node
		if (!GroupNode.LayoutNodeID.IsValid()) {
			continue;
		}

		const UFlowAbstractNode* LayoutNode = InGraphQuery.GetNode(GroupNode.LayoutNodeID);
		if (!LayoutNode || !LayoutNode->bActive) {
			continue;
		}

		if (LayoutNode->NodeItems.Num() == 0) {
			// Nothing to spawn here
			continue;
		}

		int LogicalZ{};
		{
			// Check if this is a DCEL group node
			const int FirstLeafNodeId = *GroupNode.LeafNodes.begin();
			if (!InCellGraph->LeafNodes.IsValidIndex(FirstLeafNodeId)) {
				continue;
			}

			const UDAFlowCellLeafNodeVoronoi* FirstVoronoiLeafNode = Cast<UDAFlowCellLeafNodeVoronoi>(InCellGraph->LeafNodes[FirstLeafNodeId]);
			if (!FirstVoronoiLeafNode) {
				// Not a voronoi group node
				continue;
			}
			LogicalZ = FirstVoronoiLeafNode->LogicalZ;
		}
		
		TMap<int, const UDAFlowCellLeafNodeVoronoi*> SiteToLeafMappings;
		TArray<const DA::DCEL::FEdge*> LeafCellEdges;
        for (const int LeafNodeId : GroupNode.LeafNodes) {
            if (const UDAFlowCellLeafNodeVoronoi* VoronoiLeafNode = Cast<UDAFlowCellLeafNodeVoronoi>(InCellGraph->LeafNodes[LeafNodeId])) {
            	const UDAFlowCellLeafNodeVoronoi*& LeafRef = SiteToLeafMappings.FindOrAdd(VoronoiLeafNode->SiteIndex);
            	LeafRef = VoronoiLeafNode;
            }
        }
        
        for (const DA::DCEL::FEdge* Edge : HalfEdges) {
            if (!Edge->LeftFace || !Edge->LeftFace->bValid) {
                continue;
            }
            if (SiteToLeafMappings.Contains(Edge->LeftFace->FaceId)) {
                LeafCellEdges.Add(Edge);
            }
        }
		
		if (LeafCellEdges.Num() == 0) {
			// Cannot find any edges that belong to this group
			return false;
		}

		// Find spawn locations within the group cells
		TArray<FVector> PlacementCoords;
		for (int i = 0; i < LayoutNode->NodeItems.Num(); i++) {
			// Randomly pick edges in the DCEL leaf nodes
			const int EdgeIdx = InRandom.RandRange(0, LeafCellEdges.Num() - 1);
			const DA::DCEL::FEdge* Edge = LeafCellEdges[EdgeIdx];
			
			const FVector2d EdgeStart2D = Edge->Origin->Location;
			const FVector2d EdgeEnd2D = Edge->Twin->Origin->Location;
			
			const FVector EdgeStart = FVector(EdgeStart2D, LogicalZ);
			const FVector EdgeEnd = FVector(EdgeEnd2D, LogicalZ);

			const int32 SiteIndex = Edge->LeftFace->FaceId;
			const UDAFlowCellLeafNodeVoronoi** LeafPtr = SiteToLeafMappings.Find(SiteIndex);
			check(LeafPtr);

			const UDAFlowCellLeafNodeVoronoi* LeafNode = *LeafPtr;
			FVector SiteCenter(LeafNode->CenterX, LeafNode->CenterY, LogicalZ);

			FVector& PlacementCoord = PlacementCoords.AddDefaulted_GetRef();
			PlacementCoord = FRandomUtils::GetPointOnTriangle(InRandom, EdgeStart, EdgeEnd, SiteCenter);
		}

		struct FStairInfo {
			FTransform Transform{FTransform::Identity};
			int BoundaryLineIndex{INDEX_NONE};
		};
		TArray<FStairInfo> ChunkStairs;
		const TMap<int, FDAFlowCellGraphDCELStairInfo>& Stairs = InCellGraph->DCELInfo.Stairs;
		
		// Iteratively separate the items
		{
			struct FLineInfo {
				FVector Start{};
				FVector End{};
				FVector2D Dir{};
				FVector2D DirLeft{};
			};
			TArray<FLineInfo> BoundaryLines;
			for (const DA::DCEL::FEdge* Edge : LeafCellEdges) {
				if (Edge && Edge->Twin && Edge->LeftFace && Edge->Twin->LeftFace) {
					const int SiteA = Edge->LeftFace->FaceId;
					const int SiteB = Edge->Twin->LeftFace->FaceId;
					const UDAFlowCellLeafNodeVoronoi** LeafAPtr = SiteToLeafMappings.Find(SiteA);
					const UDAFlowCellLeafNodeVoronoi** LeafBPtr = SiteToLeafMappings.Find(SiteB);
					if (LeafAPtr && LeafBPtr) {
						const UDAFlowCellLeafNodeVoronoi* LeafA = *LeafAPtr;
						const UDAFlowCellLeafNodeVoronoi* LeafB = *LeafBPtr;
						if (GroupNode.LeafNodes.Contains(LeafA->CellId) && GroupNode.LeafNodes.Contains(LeafB->CellId)) {
							// Edge shared by the leaf nodes of the same group
							continue;
						}
					}
				}
				const FVector2d EdgeStart2D = Edge->Origin->Location;
				const FVector2d EdgeEnd2D = Edge->Twin->Origin->Location;
				const FVector2d EdgeDir2D = (EdgeEnd2D - EdgeStart2D).GetSafeNormal();
				const FVector EdgeDir(EdgeDir2D.X, EdgeDir2D.Y, 0);
				const FVector LeftDir = FQuat(FVector::UpVector, PI * 0.5f).RotateVector(EdgeDir);

				FLineInfo& LineInfo = BoundaryLines.AddDefaulted_GetRef(); 
				LineInfo.Start = FVector(EdgeStart2D, LogicalZ);
				LineInfo.End = FVector(EdgeEnd2D, LogicalZ);
				LineInfo.Dir = EdgeDir2D;
				LineInfo.DirLeft = FVector2D(LeftDir.X, LeftDir.Y);

				// Check if this edge contains a stair
				if (const FDAFlowCellGraphDCELStairInfo* StairInfo = Stairs.Find(Edge->Index)) {
					FStairInfo& ChunkStairInfo = ChunkStairs.AddDefaulted_GetRef();
					{
						FVector StairScale = FVector(StairInfo->LogicalWidth, 1, 1);
						FQuat StairRotation = FQuat::FindBetweenVectors({0, -1, 0}, StairInfo->Direction);
						FVector StairLocation = StairInfo->LogicalLocation + StairInfo->Direction * 0.5f;
						ChunkStairInfo.Transform = FTransform(StairRotation, StairLocation, StairScale);
						ChunkStairInfo.BoundaryLineIndex = BoundaryLines.Num() - 1;
					}

					static const TArray<FVector> LocalStairCoords = {
						{-.5f, -.5f, 0},
						{-.5f, .5f, 0},
						{.5f, .5f, 0},
						{.5f, -.5f, 0},
					};

					TArray<FVector> WorldStairCoords;
					for (const FVector& LocalStairCoord : LocalStairCoords) {
						WorldStairCoords.Add(ChunkStairInfo.Transform.TransformPosition(LocalStairCoord));
					}
					
					// Transform the points and make new edge lists
					auto MakeStairEdge = [](const FVector& Start, const FVector& End) -> FLineInfo {
						FLineInfo LineInfo;
						LineInfo.Start = Start;
						LineInfo.End = End;
					
						const FVector Dir = (End - Start).GetSafeNormal2D();
						LineInfo.Dir = FVector2D(Dir.X, Dir.Y);
					
						const FVector LeftDir = FQuat(FVector::UpVector, PI * 0.5f).RotateVector(Dir);
						LineInfo.DirLeft = FVector2D(LeftDir.X, LeftDir.Y);

						return LineInfo;
					};
					
					// Add the stairs boundary lines so they don't spawn inside them
					BoundaryLines.Add(MakeStairEdge(WorldStairCoords[0], WorldStairCoords[1]));
					BoundaryLines.Add(MakeStairEdge(WorldStairCoords[2], WorldStairCoords[3]));
					BoundaryLines.Add(MakeStairEdge(WorldStairCoords[3], WorldStairCoords[0]));
				}
			}

			// Get the list of edges
			for (int IterationIdx = 0; IterationIdx < SpawnSeparationIterations; IterationIdx++) {
				bool bPushed{};
				TArray<FVector2D> PushVectors;
				for (int i = 0; i < PlacementCoords.Num(); i++) {
					FVector& PlacementCoord = PlacementCoords[i];

					// Check if we are inside a stair case bounds
					for (const FStairInfo& ChunkStair : ChunkStairs) {
						// Get the local point relative to the stairs and check the bounds in local space
						FVector LS = ChunkStair.Transform.InverseTransformPosition(PlacementCoord);
						if (LS.X > -0.5f && LS.X < 0.5f && LS.Y > -0.5f && LS.Y < 0.5f) {
							// Inside the stair bounds
							const float X = InRandom.FRand() < 0.5f ? -0.5f : 0.5f;
							LS = FVector(X, 0.52f, 0) + InRandom.GetUnitVector() * FVector(0.02, 0.02, 0);
							PlacementCoord = ChunkStair.Transform.TransformPosition(LS);
						}
					} 
					
					FVector2D PushVector{FVector2D::Zero()};

					// Separate them from the other items`
					// Move them with the separation distance along both sides, hence multiply by 2,
					const float ItemSeparationDist = SpawnSeparationDistance * 2;  
					for (int j = 0; j < PlacementCoords.Num(); j++) {
						if (i == j) continue;
						FVector& OtherPlacementCoord = PlacementCoords[j];
						float Distance = (PlacementCoord - OtherPlacementCoord).Size2D();
						if (Distance < ItemSeparationDist) {
							float DistanceToPush = ItemSeparationDist - Distance;
							FVector2D DirToOtherCoord = FVector2D(OtherPlacementCoord - PlacementCoord).GetSafeNormal();
							PushVector -= (DirToOtherCoord * DistanceToPush) * SpawnItemSeparationSensitivity;
							bPushed = true;
						}
					}
					
					int ClosestPointIndex = INDEX_NONE;
					float ClosestPointDistance = MAX_flt;
					
					// Separate them from the boundary edges
					for (int BoundaryIdx = 0; BoundaryIdx < BoundaryLines.Num(); BoundaryIdx++) {
						const FLineInfo& BoundaryLine = BoundaryLines[BoundaryIdx];
						float DistanceToLine = FMathUtils::GetLineToPointDist2D(
							FVector2d{BoundaryLine.Start.X, BoundaryLine.Start.Y},
							FVector2d{BoundaryLine.End.X, BoundaryLine.End.Y},
							FVector2d{PlacementCoord.X, PlacementCoord.Y}
							);

						if (DistanceToLine < SpawnSeparationDistance) {
							float DistanceToPush = SpawnSeparationDistance - DistanceToLine;
							PushVector += BoundaryLine.DirLeft * DistanceToPush * SpawnEdgeSeparationSensitivity;
							bPushed = true;
						}

						if (ClosestPointDistance > DistanceToLine) {
							ClosestPointIndex = BoundaryIdx;
							ClosestPointDistance = DistanceToLine;
						}
					}

					// Check if the point lies to the left of the closest point (this ensures the point is inside the poly)
					if (ClosestPointIndex != INDEX_NONE) {
						const FLineInfo& ClosestEdge = BoundaryLines[ClosestPointIndex];
						const FVector NewLocation = PlacementCoord + FVector(PushVector, 0);
						FVector2D DirToPoint = FVector2D(NewLocation - ClosestEdge.Start);
						FVector Cross = FVector::CrossProduct(FVector(ClosestEdge.Dir, 0), FVector(DirToPoint, 0));
						if (Cross.Z < 0) {
							PushVector = ClosestEdge.DirLeft * (ClosestPointDistance + SpawnSeparationDistance);
						}
					}

					PushVectors.Add(PushVector);
				}
				
				if (!bPushed) {
					break;
				}
				
				for (int i = 0; i < PlacementCoords.Num(); i++) {
					PlacementCoords[i] += FVector(PushVectors[i], 0);
				}
			}
		}

		// Save the placement locations
		for (int i = 0; i < PlacementCoords.Num(); i++) {
			FDAFlowCellGraphSpawnInfo& SpawnInfo = InCellGraph->DCELInfo.SpawnInfo.AddDefaulted_GetRef();
			SpawnInfo.Coord = PlacementCoords[i];
			SpawnInfo.Item = LayoutNode->NodeItems[i];
			SpawnInfo.GroupId = GroupNode.GroupId;
		}
	}

	return true;
}

