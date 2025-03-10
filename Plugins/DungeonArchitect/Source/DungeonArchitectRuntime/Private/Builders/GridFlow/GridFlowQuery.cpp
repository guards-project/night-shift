//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/GridFlow/GridFlowQuery.h"

#include "Builders/GridFlow/GridFlowConfig.h"
#include "Builders/GridFlow/GridFlowModel.h"
#include "Core/Dungeon.h"
#include "Core/DungeonBuilder.h"
#include "Core/DungeonProp.h"
#include "Core/Utils/MathUtils.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractNode.h"
#include "Frameworks/FlowImpl/GridFlow/LayoutGraph/GridFlowAbstractGraph.h"

#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridFlowQuery, Log, All);

void UGridFlowQuery::InitializeImpl(UDungeonConfig* InConfig, UDungeonModel* InModel) {
    Config = Cast<UGridFlowConfig>(InConfig);
    Model = Cast<UGridFlowModel>(InModel);

    if (!Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid config passed to grid flow query object"));
    }
    if (!Model) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid model passed to grid flow query object"));
    }
}

FVector UGridFlowQuery::ConvertTileToWorldCoord(const FVector& TileCoord) {
    if (!Model || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        return FVector::ZeroVector;
    }

    const FIntPoint Offset = Model->TilemapBuildSetup.TilemapOffset;

    const float TileX = TileCoord.X - Offset.X;
    const float TileY = TileCoord.Y - Offset.Y;
    const float TileZ = TileCoord.Z;

    FVector TileCenter = FVector(TileX + 0.5f, TileY + 0.5f, TileZ) * Config->GridSize;

    AActor* HostActor = Cast<AActor>(GetOuter());
    if (HostActor) {
        TileCenter = HostActor->GetTransform().TransformPosition(TileCenter);
    }
    return TileCenter;
}

FVector UGridFlowQuery::ConvertWorldToTileCoord(const FVector& InWorldCoords) {
    if (!Model || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        return FVector::ZeroVector;
    }

    FVector LocalCoords = InWorldCoords;
    AActor* HostActor = Cast<AActor>(GetOuter());
    if (HostActor) {
        LocalCoords = HostActor->GetTransform().InverseTransformPosition(LocalCoords);
    }

    const FVector TileCoordsF = LocalCoords / Config->GridSize;

    int32 TileX = FMath::FloorToInt(TileCoordsF.X);
    int32 TileY = FMath::FloorToInt(TileCoordsF.Y);
    const int32 TileZ = FMath::FloorToInt(TileCoordsF.Z);

    //int32 TileX = TileCoordsF.X;
    //int32 TileY = TileCoordsF.Y;
    //int32 TileZ = TileCoordsF.Z;

    TileX += Model->TilemapBuildSetup.TilemapOffset.X;
    TileY += Model->TilemapBuildSetup.TilemapOffset.Y;
    return FVector(TileX, TileY, TileZ);
}

bool UGridFlowQuery::GetCellAtTileCoord(const FVector& TileCoord, FFlowTilemapCell& OutCell) {
    if (!Model || !Model->Tilemap || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        return false;
    }

    const int32 IX = FMath::FloorToInt(TileCoord.X);
    const int32 IY = FMath::FloorToInt(TileCoord.Y);
    const FFlowTilemapCell* CellPtr = Model->Tilemap->GetSafe(IX, IY);
    if (CellPtr) {
        OutCell = *CellPtr;
        return true;
    }
    return false;
}

bool UGridFlowQuery::GetCellAtWorldCoord(const FVector& InWorldCoord, FFlowTilemapCell& OutCell) {
    if (!Model || !Model->Tilemap || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        return false;
    }
    const FVector TileCoord = ConvertWorldToTileCoord(InWorldCoord);
    return GetCellAtTileCoord(TileCoord, OutCell);
}

bool UGridFlowQuery::GetCellRoomType(const FVector& WorldCoord, EGridFlowAbstractNodeRoomType& OutRoomType) {
    if (!Model || !Model->Tilemap || !Model->AbstractGraph || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        return false;
    }
    const FVector TileCoords = ConvertWorldToTileCoord(WorldCoord);
    const FFlowTilemapCell* CellPtr = Model->Tilemap->GetSafe(TileCoords.X, TileCoords.Y);
    if (CellPtr) {
        const FVector NodeCoord = CellPtr->ChunkCoord;
        for (const UFlowAbstractNode* Node : Model->AbstractGraph->GraphNodes) {
            if (!Node) continue;
            if (NodeCoord.Equals(Node->Coord)) {
                UFANodeTilemapDomainData* TilemapDomainData = Node->FindDomainData<UFANodeTilemapDomainData>();
                if (TilemapDomainData) {
                    OutRoomType = TilemapDomainData->RoomType;
                    return true;
                }
            }
        }
    }
    return false;
}

void UGridFlowQuery::GetFreeTileLocation(TArray<EGridFlowAbstractNodeRoomType> AllowedRoomTypes,
                                         const FRandomStream& Random, bool& OutSuccess, FVector& OutWorldCoord) {
    if (!Model || !Model->Tilemap || !Model->AbstractGraph || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        OutSuccess = false;
        OutWorldCoord = FVector::ZeroVector;
        return;
    }

    const TSet<EGridFlowAbstractNodeRoomType> ValidRoomTypes(AllowedRoomTypes);
    TMap<FVector, EGridFlowAbstractNodeRoomType> RoomTypesByNodeCoord;
    for (const UFlowAbstractNode* Node : Model->AbstractGraph->GraphNodes) {
        if (!Node) continue;
        EGridFlowAbstractNodeRoomType& RoomTypeRef = RoomTypesByNodeCoord.FindOrAdd(Node->Coord);
        UFANodeTilemapDomainData* TilemapDomainData = Node->FindDomainData<UFANodeTilemapDomainData>();
        if (TilemapDomainData) {
            RoomTypeRef = TilemapDomainData->RoomType;
        }
    }

    TArray<FIntVector> AllFloorTiles;
    TArray<FIntVector> ValidFloorTiles;
    for (const FFlowTilemapCell& Cell : Model->Tilemap->GetCells()) {
        if (Cell.bLayoutCell && Cell.CellType == EFlowTilemapCellType::Floor) {
            if (Cell.bHasItem) continue;
            if (Cell.bHasOverlay && Cell.Overlay.bTileBlockingOverlay) continue;

            EGridFlowAbstractNodeRoomType* RoomTypePtr = RoomTypesByNodeCoord.Find(Cell.ChunkCoord);
            if (RoomTypePtr) {
                const EGridFlowAbstractNodeRoomType RoomType = *RoomTypePtr;
                FIntVector TileCoord(Cell.TileCoord.X, Cell.TileCoord.Y, Cell.Height);
                if (ValidRoomTypes.Contains(RoomType)) {
                    ValidFloorTiles.Add(TileCoord);
                }
                AllFloorTiles.Add(TileCoord);
            }
        }
    }

    if (ValidFloorTiles.Num() == 0 && AllFloorTiles.Num() == 0) {
        OutSuccess = false;
        OutWorldCoord = FVector::ZeroVector;
        return;
    }

    TArray<FIntVector>& ValidTiles = ValidFloorTiles.Num() > 0 ? ValidFloorTiles : AllFloorTiles;
    const int32 Index = Random.RandRange(0, ValidTiles.Num() - 1);
    const FIntVector TargetTileCoord = ValidTiles[Index];
    const FVector TargetWorldCoord = ConvertTileToWorldCoord(FMathUtils::ToVector(TargetTileCoord));

    OutWorldCoord = TargetWorldCoord;
    OutSuccess = true;
}

bool UGridFlowQuery::GetChunkAtWorldCoord(const FVector& InWorldCoord, FGridFlowChunkQueryResult& Result) {
    if (!Model || !Model->Tilemap || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        return false;
    }
    FVector TileCoord = ConvertWorldToTileCoord(InWorldCoord);
    const FFlowTilemapCell* CellPtr = Model->Tilemap->GetSafe(TileCoord.X, TileCoord.Y);
    if (CellPtr) {
        if (CellPtr->CellType != EFlowTilemapCellType::Empty && CellPtr->CellType != EFlowTilemapCellType::Custom) {
            return GetChunkAtLayoutNodeCoord(CellPtr->ChunkCoord, Result);
        }
    }
    return false;
}

bool UGridFlowQuery::GetChunkAtLayoutNodeCoord(const FVector& InLayoutNodeCoord, FGridFlowChunkQueryResult& Result) const {
    if (!Model || !Model->AbstractGraph || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        return false;
    }
    
    for (UFlowAbstractNode* Node : Model->AbstractGraph->GraphNodes) {
        if (!Node) continue;
        if (Node->Coord.Equals(InLayoutNodeCoord)) {
            GetChunkAtLayoutNode(Node, Result);
            return true;
        }
    }
    return false;
}

void UGridFlowQuery::GetConnectedLayoutNodeCoords(const FVector& InLayoutNodeCoord, TArray<FVector>& ConnectedCoords) const {
    const FFlowAbstractGraphQuery Query(Model->AbstractGraph);
    const FGuid NodeId = Query.GetNodeAtCoord(InLayoutNodeCoord);
    const TArray<FGuid> ConnectedNodes = Query.GetConnectedNodes(NodeId);
    for (const FGuid& ConnectedNodeId : ConnectedNodes) {
        if (const UFlowAbstractNode* ConnectedNode = Query.GetNode(ConnectedNodeId)) {
            ConnectedCoords.Add(ConnectedNode->Coord);
        }
    }
}

void UGridFlowQuery::GetChunkAtLayoutNode(const UFlowAbstractNode* InLayoutNode, FGridFlowChunkQueryResult& Result) {
    UFANodeTilemapDomainData* TilemapDomainData = InLayoutNode->FindDomainData<UFANodeTilemapDomainData>();
    if (TilemapDomainData) {
        const FGridFlowAbstractNodeTilemapMetadata& TileData = TilemapDomainData->TilemapMetadata;
        Result.LayoutNodeCoord = InLayoutNode->Coord;
        Result.TilemapCoordStart = FVector(TileData.TileCoordStart.X, TileData.TileCoordStart.Y, 0);
        Result.TilemapCoordEnd = FVector(TileData.TileCoordEnd.X, TileData.TileCoordEnd.Y, 0);
        Result.TileCoords.Empty(TileData.Tiles.Num());
        for (const FIntPoint& Coord : TileData.Tiles) {
            Result.TileCoords.Add(FVector(Coord.X, Coord.Y, 0));
        }
        Result.RoomType = TilemapDomainData->RoomType;
        Result.PathName = InLayoutNode->PathName;
        Result.PathIndex = InLayoutNode->PathIndex;
        Result.PathLength = InLayoutNode->PathLength;
    }
}

void UGridFlowQuery::GetAllChunksOfType(EGridFlowAbstractNodeRoomType InRoomType,
                                        TArray<FGridFlowChunkQueryResult>& OutChunks) const {
    OutChunks.Reset();
    if (!Model || !Model->AbstractGraph) return;
    
    for (const UFlowAbstractNode* Node : Model->AbstractGraph->GraphNodes) {
        if (!Node) continue;
        
        UFANodeTilemapDomainData* TilemapDomainData = Node->FindDomainData<UFANodeTilemapDomainData>();
        if (TilemapDomainData && TilemapDomainData->RoomType == InRoomType) {
            FGridFlowChunkQueryResult& ChunkInfo = OutChunks.AddDefaulted_GetRef();
            GetChunkAtLayoutNode(Node, ChunkInfo);
        }
    }
}

void UGridFlowQuery::GetDungeonBounds(FVector& BoundsCenter, FVector& BoundsExtent) const {
    if (!Model || !Model->Tilemap || !Config) {
        UE_LOG(LogGridFlowQuery, Error, TEXT("Invalid grid flow query object state"));
        BoundsCenter = FVector::ZeroVector;
        BoundsExtent = FVector::ZeroVector;
        return;
    }

    // Iterate through all the tiles in the tilemap and grab the bounds
    const FIntPoint Offset = Model->TilemapBuildSetup.TilemapOffset;
    FBox Bounds;
    const FVector& GridSize = Config->GridSize;
    for (const FFlowTilemapCell& Cell : Model->Tilemap->GetCells()) {
        if (Cell.bLayoutCell) {
            const float TileX = Cell.TileCoord.X - Offset.X;
            const float TileY = Cell.TileCoord.Y - Offset.Y;
            const float TileZ = Cell.Height;
            
            FVector TileCenter = FVector(TileX + 0.5f, TileY + 0.5f, TileZ) * GridSize;
            Bounds += DungeonTransform.TransformPosition(TileCenter);
        } 
    }

    Bounds = Bounds.ExpandBy(FVector(GridSize * 0.5f));
    BoundsCenter = Bounds.GetCenter();
    BoundsExtent = Bounds.GetExtent();
}

void UGridFlowQuery::IsNearMarker(const FTransform& CurrentMarkerTransform, const FString& NearbyMarkerName,
                                  float NearbyDistance, UDungeonBuilder* Builder, bool& bIsNear, int32& NumFound) {
    NumFound = 0;
    if (Builder) {
        const float QueryDistanceSq = NearbyDistance * NearbyDistance;
        const FVector CurrentLocation = CurrentMarkerTransform.GetLocation();

        for (const FDAMarkerInfo& Socket : Builder->GetMarkers()) {
            const float DistanceSq = (Socket.Transform.GetLocation() - CurrentLocation).SizeSquared();
            if (DistanceSq < QueryDistanceSq && Socket.MarkerName == NearbyMarkerName) {
                NumFound++;
            }
        }
    }
    bIsNear = NumFound > 0;
}

