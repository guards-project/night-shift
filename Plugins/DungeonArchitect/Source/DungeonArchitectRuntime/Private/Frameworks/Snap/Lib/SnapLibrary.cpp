//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/SnapLibrary.h"

#include "Core/Utils/DungeonBoundingShapes.h"
#include "Core/Utils/MathUtils.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionConstants.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionInfo.h"
#include "Frameworks/Snap/Lib/Utils/SnapDiagnostics.h"

FORCEINLINE int32 PermuteCompareTo(const SnapLib::ISnapGraphNodePtr A, const SnapLib::ISnapGraphNodePtr B) {
    return A->Compare(B);
}

class UGrammarScriptGraph;

FBox SnapLib::FModuleNode::GetModuleBounds() const {
    return ModuleDBItem->GetBounds().TransformBy(WorldTransform);
}

FDABoundsShapeList SnapLib::FModuleNode::GetModuleBoundShapes() const {
    return ModuleDBItem->GetBoundShapes().TransformBy(WorldTransform);
}

TSoftObjectPtr<UWorld> SnapLib::IModuleDatabaseItem::GetThemedLevel(const FString& InThemeName) const {
    const TSoftObjectPtr<UWorld>* WorldPtr = GetThemedLevels().Find(InThemeName);
    return WorldPtr ? *WorldPtr : nullptr;
}

///////////////////////////// FSnapMapGraphGenerator /////////////////////////////
SnapLib::FSnapGraphGenerator::FSnapGraphGenerator(SnapLib::IModuleDatabasePtr InModuleDatabase, const FGrowthStaticState& InStaticState)
    : ModuleDatabase(InModuleDatabase)
    , StaticState(InStaticState)
{
}

SnapLib::FModuleNodePtr SnapLib::FSnapGraphGenerator::Generate(SnapLib::ISnapGraphNodePtr StartNode) {
    if (!StartNode) {
        return nullptr;
    }

    SharedState = FGrowthSharedState();
    
    SnapLib::FGrowthResult Result;
    GrowNode(StartNode, SnapLib::FGrowthInputState(), Result);

    if (Result.SuccessType != FGrowthResultType::Success) {
        return nullptr;
    }

    AssignNetworkNodeIds(Result.Node);
    return Result.Node;
}

void SnapLib::FSnapGraphGenerator::GrowNode(const SnapLib::ISnapGraphNodePtr& MissionNode, const SnapLib::FGrowthInputState& InputState, SnapLib::FGrowthResult& OutResult) {
#define DIAGNOSTIC_LOG(Func, ...) if (StaticState.Diagnostics.IsValid()) StaticState.Diagnostics->Log##Func(__VA_ARGS__);

    check (!InputState.VisitedNodes.Contains(MissionNode->GetNodeID()));
    
    DIAGNOSTIC_LOG(MoveToNode, MissionNode->GetNodeID());
    
    // Check if we've taken too much time to process this
    {
        // TODO: This will cause timeouts when debugging with breakpoints, find a better way
        double CurrentTimeSecs = FPlatformTime::Seconds();
        if (CurrentTimeSecs >= StaticState.StartTimeSecs + StaticState.MaxProcessingTimeSecs) {
            OutResult.SuccessType = FGrowthResultType::FailHalt;
            OutResult.Node = nullptr;
            OutResult.BranchOcclusion.Reset();
            OutResult.BranchVisited.Reset();

            DIAGNOSTIC_LOG(TimeoutHalt);
            return;
        }
    }

    FGuid IncomingNodeId = InputState.RemoteIncomingDoor.IsValid() ? InputState.RemoteIncomingDoor->Owner->ModuleInstanceId : FGuid();
    TArray<SnapLib::ISnapGraphNodePtr> OutgoingNodes = MissionNode->GetOutgoingNodes(IncomingNodeId);
    
    // TODO: Cache the sort in a memo
    OutgoingNodes.Sort([](const SnapLib::ISnapGraphNodePtr& A, const SnapLib::ISnapGraphNodePtr& B) -> bool {
        return A->Compare(B) < 0;
    });

    const FName ModuleCategory = MissionNode->GetCategory();
    const TArray<SnapLib::IModuleDatabaseItemPtr> PossibleModules = ModuleDatabase->GetCategoryModules(ModuleCategory);
    FGuid NodeId = MissionNode->GetNodeID();

    // Try to fit a module with the incoming door
    TArray<int32> ShuffledModuleIndices = FMathUtils::GetShuffledIndices(PossibleModules.Num(), StaticState.Random);
    for (int32 ModuleIdxRef = 0; ModuleIdxRef < PossibleModules.Num(); ModuleIdxRef++) {
        int32 ModuleIdx = ShuffledModuleIndices[ModuleIdxRef];
        SnapLib::IModuleDatabaseItemPtr Module = PossibleModules[ModuleIdx];
        SnapLib::FModuleNodePtr ModuleNode = Module->CreateModuleNode(NodeId);
        if (!ModuleNode.IsValid()) {
            continue;
        }

        // Cache the module node
        {
            FModuleNodeWeakPtr& NodePtrRef = SharedState.CachedModuleNodes.FindOrAdd(NodeId);
            NodePtrRef = ModuleNode;
        }
        
        struct FModuleGrowthFrame {
            int32 DoorIdx = -1;
            SnapLib::FModuleDoorPtr IncomingDoor;
            TArray<FTransform> DesiredTransforms;
        };

        SnapLib::FModuleDoorPtr RemoteIncomingDoor = InputState.RemoteIncomingDoor;
        TArray<FModuleGrowthFrame> GrowthFrames;
        if (RemoteIncomingDoor.IsValid()) {
            TArray<int32> ShuffledDoorIndices = FMathUtils::GetShuffledIndices(
            ModuleNode->Doors.Num(), StaticState.Random);
            for (int32 DoorIdxRef = 0; DoorIdxRef < ModuleNode->Doors.Num(); DoorIdxRef++) {
                int32 DoorIdx = ShuffledDoorIndices[DoorIdxRef];
                SnapLib::FModuleDoorPtr IncomingDoor = ModuleNode->Doors[DoorIdx];
                TArray<FTransform> NewModuleTransforms;
                if (GetDoorFitConfiguration(RemoteIncomingDoor, IncomingDoor, Module->ShouldAllowRotation(), NewModuleTransforms)) {
                    FModuleGrowthFrame& Frame = GrowthFrames.AddDefaulted_GetRef();
                    Frame.DesiredTransforms = NewModuleTransforms;
                    Frame.DoorIdx = DoorIdx;
                    Frame.IncomingDoor = IncomingDoor;
                }
            }
        }
        else {
            FModuleGrowthFrame& Frame = GrowthFrames.AddDefaulted_GetRef();
            Frame.DesiredTransforms = GetStartingNodeTransforms(ModuleNode, MissionNode);
            Frame.DoorIdx = -1;
            Frame.IncomingDoor = nullptr;   
        }
        
        for (const FModuleGrowthFrame& GrowthFrame : GrowthFrames) {
            for (const FTransform& GrowthFrameTransform : GrowthFrame.DesiredTransforms) {
                ModuleNode->WorldTransform = GrowthFrameTransform;
                // Add diagnostic information if available
                if (StaticState.Diagnostics.IsValid()) {
                    const FGuid& RemoteNodeId = RemoteIncomingDoor.IsValid()
                                                    ? RemoteIncomingDoor->Owner->ModuleInstanceId
                                                    : FGuid();
                    const FGuid& DoorId = GrowthFrame.IncomingDoor.IsValid() ? GrowthFrame.IncomingDoor->ConnectionId : FGuid();
                    const FGuid& RemoteDoorId = RemoteIncomingDoor.IsValid()
                                                    ? RemoteIncomingDoor->ConnectionId
                                                    : FGuid();
                    int32 RemoteDoorIndex = InputState.RemoteIncomingDoorIndex;
                    FTransform DoorWorldTransform = RemoteIncomingDoor.IsValid()
                                                        ? RemoteIncomingDoor->LocalTransform * RemoteIncomingDoor->Owner->WorldTransform
                                                        : FTransform::Identity;
                    FBox DoorLocalBounds = FBox(FVector(-200, -50, 0), FVector(200, 50, 400));
                    FBox DoorWorldBounds = DoorLocalBounds.TransformBy(DoorWorldTransform);
                
                    DIAGNOSTIC_LOG(AssignModule, Module->GetLevel(), Module->GetBounds(), ModuleNode->WorldTransform, GrowthFrame.DoorIdx, RemoteDoorIndex,
                                  DoorId, RemoteDoorId, NodeId, RemoteNodeId, DoorWorldBounds);
                }

                if (ModuleOccludes(ModuleNode, MissionNode, InputState.OcclusionStack)) {
                    DIAGNOSTIC_LOG(RejectModule, SnapLib::EModuleRejectReason::BoundsCollide);
                    continue;
                }

                // We have a module that fits without occluding
                // Start permutation of each door / outgoing node configuration and try to grow outward
                TArray<int32> OutgoingDoorIndices;
                for (int i = 0; i < ModuleNode->Doors.Num(); i++) {
                    if (RemoteIncomingDoor.IsValid() && i == GrowthFrame.DoorIdx) {
                        continue;
                    }
                    OutgoingDoorIndices.Add(i);
                }
                FMathUtils::Shuffle(OutgoingDoorIndices, StaticState.Random);

                if (OutgoingNodes.Num() > OutgoingDoorIndices.Num()) {
                    // Too few available doors in this module to grow 
                    DIAGNOSTIC_LOG(RejectModule, SnapLib::EModuleRejectReason::NotEnoughDoorsAvailable);
                    break;
                }

                FBox NodeBounds = ModuleNode->GetModuleBounds();
                FDABoundsShapeList NodeBoundShapes = ModuleNode->GetModuleBoundShapes();
            
                // Create permutation engine and iterate each permutation
                SnapLib::FBranchGrowthPermutations PermutationEngine(OutgoingDoorIndices, OutgoingNodes);
                while (PermutationEngine.CanRun()) {
                    bool bAllBranchesSuccessful = true;
                    TArray<FOcclusionEntry> BranchOcclusion;
                    BranchOcclusion.Add({
                        NodeBounds,
                        NodeBoundShapes
                    });

                    TSet<FGuid> BranchVisited;
                    BranchVisited.Add(MissionNode->GetNodeID());

                    TArray<int32> Doors;
                    TArray<SnapLib::ISnapGraphNodePtr> Nodes;
                    PermutationEngine.Execute(Doors, Nodes);
                    ModuleNode->Outgoing.Reset();
                    for (int i = 0; i < Doors.Num(); i++) {
                        int32 OutgoingDoorIdx = Doors[i];
                        SnapLib::FModuleDoorPtr OutgoingDoor = ModuleNode->Doors[OutgoingDoorIdx];
                        SnapLib::ISnapGraphNodePtr OutgoingNode = Nodes[i];

                        TSet<FGuid> VisitedAlongPath = InputState.VisitedNodes;
                        VisitedAlongPath.Append(BranchVisited);
                    
                        // Check if the outgoing node is not already visited
                        if (!VisitedAlongPath.Contains(OutgoingNode->GetNodeID())) {
                            SnapLib::FGrowthInputState ChildInputState = InputState;
                            ChildInputState.VisitedNodes.Append(BranchVisited);
                            ChildInputState.OcclusionStack.Append(BranchOcclusion);
                            ChildInputState.RemoteIncomingDoor = OutgoingDoor;
                            ChildInputState.RemoteIncomingDoorIndex = OutgoingDoorIdx;

                            SnapLib::FGrowthResult ChildResult;
                            GrowNode(OutgoingNode, ChildInputState, ChildResult);
                            if (ChildResult.SuccessType == FGrowthResultType::FailBranch) {
                                bAllBranchesSuccessful = false;
                                break;
                            }
                            if (ChildResult.SuccessType == FGrowthResultType::FailHalt) {
                                OutResult.SuccessType = FGrowthResultType::FailHalt;
                                return;
                            }
                        
                            OutgoingDoor->ConnectedDoor = ChildResult.IncomingDoor;
                            ChildResult.IncomingDoor->ConnectedDoor = OutgoingDoor;
                            BranchVisited.Append(ChildResult.BranchVisited);
                            BranchOcclusion.Append(ChildResult.BranchOcclusion);
                            ModuleNode->Outgoing.Add(OutgoingDoor);
                        }
                        else {
                            if (InputState.RemoteIncomingDoor.IsValid() && OutgoingNode->GetNodeID() != InputState.RemoteIncomingDoor->Owner->ModuleInstanceId) {
                                // Connecting to a pre-existing room. Make sure we can connect
                                FModuleNodePtr OutgoingModule = SharedState.CachedModuleNodes.FindOrAdd(OutgoingNode->GetNodeID()).Pin();
                                check(OutgoingModule.IsValid());
                        
                                FModuleDoorPtr ThisModuleDoor = OutgoingDoor;
                                FModuleDoorPtr OtherConnectedDoor = nullptr;
                                {
                                    FTransform ThisDoorTransform = ThisModuleDoor->LocalTransform * ThisModuleDoor->Owner->WorldTransform; 
                                    //FVector ThisDoorPosition = ThisModuleDoor->Owner->WorldTransform.TransformPosition(ThisModuleDoor->LocalTransform.GetLocation());
                                    FVector ThisDoorPosition = ThisDoorTransform.GetLocation();
                                    for (FModuleDoorPtr OtherModuleDoor : OutgoingModule->Doors) {
                                        FTransform OtherDoorTransform = OtherModuleDoor->LocalTransform * OtherModuleDoor->Owner->WorldTransform; 
                                        //FVector OtherDoorPosition = OtherModuleDoor->Owner->WorldTransform.TransformPosition(OtherModuleDoor->LocalTransform.GetLocation());
                                        FVector OtherDoorPosition = OtherDoorTransform.GetLocation();

                                        // Check if they are close together (i.e. connected)
                                        float Distance = (ThisDoorPosition - OtherDoorPosition).Size();
                                        if (Distance < StaticState.BoundsContraction)
                                        {
                                            OtherConnectedDoor = OtherModuleDoor;
                                            break;
                                        }
                                    }
                                }

                                if (!OtherConnectedDoor.IsValid()) {
                                    //OutResult.SuccessType = FGrowthResultType::FailHalt;
                                    //return;
                                    bAllBranchesSuccessful = false;
                                    break;
                                }
                        
                                ThisModuleDoor->ConnectedDoor = OtherConnectedDoor;
                                OtherConnectedDoor->ConnectedDoor = ThisModuleDoor;
                                ModuleNode->Outgoing.Add(ThisModuleDoor);
                            }
                        }
                    }

                    if (bAllBranchesSuccessful) {
                        ModuleNode->Incoming = GrowthFrame.IncomingDoor;

                        OutResult.SuccessType = FGrowthResultType::Success;
                        OutResult.Node = ModuleNode;
                        OutResult.IncomingDoor = GrowthFrame.IncomingDoor;
                        OutResult.BranchOcclusion = BranchOcclusion;
                        OutResult.BranchVisited = BranchVisited;
                        DIAGNOSTIC_LOG(BacktrackFromNode, true);
                        return;
                    }
                }

                DIAGNOSTIC_LOG(RejectModule, SnapLib::EModuleRejectReason::CannotBuildSubTree);
            }
        }
    }

    OutResult.SuccessType = FGrowthResultType::FailBranch;
    OutResult.Node = nullptr;
    OutResult.BranchOcclusion.Reset();
    OutResult.BranchVisited.Reset();

    DIAGNOSTIC_LOG(BacktrackFromNode, false);
    
#undef DIAGNOSTIC_LOG
}

TArray<FTransform> SnapLib::FSnapGraphGenerator::GetStartingNodeTransforms(const SnapLib::FModuleNodePtr& ModuleNode, const SnapLib::ISnapGraphNodePtr& MissionNode) {
    return { StaticState.DungeonBaseTransform };
}

void SnapLib::FSnapGraphGenerator::AssignNetworkNodeIds(const SnapLib::FModuleNodePtr& Node) {
    uint32 IdCounter = 0;
    TSet<SnapLib::FModuleNodePtr> Visited;
    TArray<SnapLib::FModuleNodePtr> Stack;
    Stack.Push(Node);
    while (Stack.Num() > 0) {
        SnapLib::FModuleNodePtr Top = Stack.Pop();
        if (Visited.Contains(Top)) {
            continue;
        }
        Visited.Add(Top);

        Top->NetworkId = IdCounter;
        IdCounter++;

        for (SnapLib::FModuleDoorPtr Door : Top->Outgoing) {
            if (Door.IsValid() && Door->ConnectedDoor.IsValid() && Door->ConnectedDoor->Owner.IsValid()) {
                SnapLib::FModuleNodePtr Child = Door->ConnectedDoor->Owner;
                Stack.Push(Child);
            }
        }
    }
}

SnapLib::FModuleNodePtr SnapLib::FSnapGraphGenerator::GetConnectedModule(const SnapLib::FModuleDoorPtr& Door) {
    if (Door.IsValid() && Door->ConnectedDoor.IsValid()) {
        SnapLib::FModuleDoorPtr OtherDoor = Door->ConnectedDoor;
        if (OtherDoor->Owner.IsValid()) {
            return OtherDoor->Owner;
        }
    }
    return nullptr;
}

bool SnapLib::FSnapGraphGenerator::CanConnectDoors(const SnapLib::FModuleDoorPtr& A, const SnapLib::FModuleDoorPtr& B, bool bAllowRotation) const {
    if (!A.IsValid() || !B.IsValid() || A->Owner == B->Owner) return false;

    if (!A->ConnectionInfo || !B->ConnectionInfo) {
        // invalid asset reference
        return false;
    }
    
    if (A->ConnectionInfo->ConnectionCategory != B->ConnectionInfo->ConnectionCategory) {
        // Different door assets categories
        return false;
    }

    const bool bHasMale = A->ConnectionConstraint == ESnapConnectionConstraint::PlugMale
        || B->ConnectionConstraint == ESnapConnectionConstraint::PlugMale;

    const bool bHasFemale = A->ConnectionConstraint == ESnapConnectionConstraint::PlugFemale
        || B->ConnectionConstraint == ESnapConnectionConstraint::PlugFemale;

    if ((bHasFemale && !bHasMale) || (bHasMale && !bHasFemale)) {
        return false;
    }

    if (A->ConnectionConstraint == ESnapConnectionConstraint::RoomEntry) {
        // Door A should be an exit door, to satisfy the flow graph's link direction
        return false;
    }

    if (B->ConnectionConstraint == ESnapConnectionConstraint::RoomExit) {
        // Door B should an entry door, to satisfy the flow graph's link direction
        return false;
    }
    
    if (!bAllowRotation) {
        // Make sure the doors are facing the opposite directions
        const FVector DoorForwardA = A->LocalTransform.GetRotation().GetForwardVector();
        const FVector DoorForwardB = B->LocalTransform.GetRotation().GetForwardVector();
        const float Dot = FVector::DotProduct(DoorForwardA, DoorForwardB);
        if (!FMath::IsNearlyEqual(Dot, -1, 1e-4f)) {
            return false;
        }
    }

    return true;
}

bool SnapLib::FSnapGraphGenerator::Intersects(const FOcclusionEntry& A, const FOcclusionEntry& B, float Tolerance) {
    // Check if the AABB bounds intersect.   This allows for fast bounds intersection. If they do, we check the more costlier options
    const bool bFastIntersectionCheck = A.Bounds.ExpandBy(-Tolerance).Intersect(B.Bounds);
    if (!bFastIntersectionCheck) {
        // Do not intersect
        return false;
    }
    
    if (A.BoundsShapes.GetTotalCustomShapes() == 0 && B.BoundsShapes.GetTotalCustomShapes() == 0) {
        // return the result of the AABB bounds intersection test
        return bFastIntersectionCheck;
    }
    else if (A.BoundsShapes.GetTotalCustomShapes() == 0 && B.BoundsShapes.GetTotalCustomShapes() > 0) {
        return FDABoundsShapeCollision::Intersects(A.Bounds, B.BoundsShapes, Tolerance);
    }
    else if (A.BoundsShapes.GetTotalCustomShapes() > 0 && B.BoundsShapes.GetTotalCustomShapes() == 0) {
        return FDABoundsShapeCollision::Intersects(B.Bounds, A.BoundsShapes, Tolerance);
    }

    return FDABoundsShapeCollision::Intersects(A.BoundsShapes, B.BoundsShapes, Tolerance);
}

bool SnapLib::FSnapGraphGenerator::IsInside(const FBox& OwningBox, const FOcclusionEntry& TestInside, float Tolerance) {
    if (TestInside.BoundsShapes.GetTotalCustomShapes() == 0) {
        return OwningBox.IsInside(TestInside.Bounds.ExpandBy(-Tolerance));
    }
    
    const bool bFastCheckIntersect = OwningBox.Intersect(TestInside.Bounds.ExpandBy(-Tolerance));
    if (!bFastCheckIntersect) {
        // Do not intersect, cannot be inside
        return false;
    }
    
    for (const FDABoundsShapeCircle& Circle : TestInside.BoundsShapes.Circles) {
        const FVector BaseLocation = Circle.Transform.GetLocation();
        if (BaseLocation.X - Circle.Radius < OwningBox.Min.X
            || BaseLocation.X + Circle.Radius > OwningBox.Max.X
            || BaseLocation.Y - Circle.Radius < OwningBox.Min.Y
            || BaseLocation.Y + Circle.Radius > OwningBox.Max.Y
            || BaseLocation.Z < OwningBox.Min.Z
            || BaseLocation.Z + Circle.Height > OwningBox.Max.Z)
        {
            return false;
        }
    }

    for (const FDABoundsShapeConvexPoly& ConvexHull : TestInside.BoundsShapes.ConvexPolys) {
        for (const FVector2D& LocalHullPoint : ConvexHull.Points) {
            const FVector PointLo = ConvexHull.Transform.TransformPosition(FVector(LocalHullPoint, 0));
            if (!OwningBox.IsInside(PointLo)) return false;
            
            const FVector PointHi = ConvexHull.Transform.TransformPosition(FVector(LocalHullPoint, ConvexHull.Height));
            if (!OwningBox.IsInside(PointHi)) return false;
        }
    }

    return true;
}

void SnapLib::TraverseModuleGraph(FModuleNodePtr StartNode, TFunction<void(FModuleNodePtr Node)> Visit) {
    TSet<FModuleNodePtr> Visited;
    TArray<FModuleNodePtr> Stack;
    Stack.Add(StartNode);
    Visited.Add(StartNode);
    
    while (Stack.Num() > 0) {
        const FModuleNodePtr Top = Stack.Pop();

        Visit(Top);
        
        TSet<FModuleDoorPtr> ConnectedNodes;
        ConnectedNodes.Add(Top->Incoming);
        ConnectedNodes.Append(Top->Outgoing);
        
        for (FModuleDoorPtr Door : ConnectedNodes) {
            if (Door.IsValid() && Door->ConnectedDoor.IsValid() && Door->ConnectedDoor->Owner.IsValid()) {
                SnapLib::FModuleNodePtr Child = Door->ConnectedDoor->Owner;
                if (!Visited.Contains(Child)) {
                    Stack.Push(Child);
                    Visited.Add(Child);
                }
            }
        }
    }
}

bool SnapLib::FSnapGraphGenerator::GetDoorFitConfiguration(SnapLib::FModuleDoorPtr RemoteDoor,
                                                           SnapLib::FModuleDoorPtr DoorToFit, bool bAllowModuleRotation,
                                                           TArray<FTransform>& OutNewTransforms) {
    if (!RemoteDoor.IsValid()) {
        // No incoming door. This is the first node in the graph
        OutNewTransforms = { FTransform::Identity };
        return true;
    }

    if (!CanConnectDoors(RemoteDoor, DoorToFit, bAllowModuleRotation)) {
        return false;
    }

    if (!RemoteDoor->Owner.IsValid()) {
        return false;
    }

    if (!RemoteDoor->ConnectionInfo || !DoorToFit->ConnectionInfo) {
        return false;
    }
    
    bool bVerticalConnection = false;
    {
        bool bVerticalA = RemoteDoor->ConnectionInfo->bVerticalDoor;
        bool bVerticalB = DoorToFit->ConnectionInfo->bVerticalDoor;
        bVerticalConnection = bVerticalA && bVerticalB;
    }

    if (bVerticalConnection) {        
        FTransform DesiredDoorTransform = RemoteDoor->LocalTransform * RemoteDoor->Owner->WorldTransform;
            
        // Calculate the rotation
        FQuat DesiredRotation;
        {
            FVector TargetVector = DesiredDoorTransform.GetRotation().Vector();
            FVector SourceVector = DoorToFit->LocalTransform.GetRotation().Vector();
            DesiredRotation = FQuat::FindBetween(SourceVector, TargetVector);
        }

        // Calculate the translation
        FVector DesiredOffset;
        {
            FVector TargetOffset = DesiredDoorTransform.GetLocation();
            FVector SourceOffset = DoorToFit->LocalTransform.GetLocation();
            SourceOffset = DesiredRotation * SourceOffset;
            DesiredOffset = TargetOffset - SourceOffset;
        }

        // Calculate the translation
        OutNewTransforms = { FTransform(DesiredRotation, DesiredOffset) };
    }
    else {
        // Non vertical connection path
        FTransform AttachmentDoorTransform = DoorToFit->LocalTransform;
        if (bAllowModuleRotation) {
            FTransform DesiredDoorTransform = FTransform(FQuat::MakeFromEuler(FVector(0, 0, 180))) *
                    RemoteDoor->LocalTransform * RemoteDoor->Owner->WorldTransform;
            
            // Calculate the rotation
            FQuat DesiredRotation;
            {
                FVector TargetVector = DesiredDoorTransform.GetRotation().Vector();
                FVector SourceVector = AttachmentDoorTransform.GetRotation().Vector();
                DesiredRotation = FQuat::FindBetween(SourceVector, TargetVector);
            }

            // Calculate the translation
            FVector DesiredOffset;
            {
                FVector TargetOffset = DesiredDoorTransform.GetLocation();
                FVector SourceOffset = AttachmentDoorTransform.GetLocation();
                SourceOffset = DesiredRotation * SourceOffset;
                DesiredOffset = TargetOffset - SourceOffset;
            }

            OutNewTransforms = { FTransform(DesiredRotation, DesiredOffset) };
        }
        else {

            FTransform DesiredDoorTransform = RemoteDoor->LocalTransform * RemoteDoor->Owner->WorldTransform;

            // Calculate the translation
            FVector DesiredOffset;
            {
                FVector TargetOffset = DesiredDoorTransform.GetLocation();
                FVector SourceOffset = AttachmentDoorTransform.GetLocation();
                DesiredOffset = TargetOffset - SourceOffset;
            }

            OutNewTransforms = { FTransform(FQuat::Identity, DesiredOffset) };
        }
    }
    
    return true;
}

bool SnapLib::FSnapGraphGenerator::ModuleOccludes(const FModuleNodePtr& ModuleNode, const SnapLib::ISnapGraphNodePtr& MissionNode, const TArray<FOcclusionEntry>& OcclusionList) {
    const FOcclusionEntry ModuleOcclusion {
        ModuleNode->GetModuleBounds(),
        ModuleNode->GetModuleBoundShapes()
    };

    const float CollisionTolerance = StaticState.BoundsContraction;
    // Make sure the negation volume constraints are satisfied
    for (const SnapLib::FSnapNegationVolumeState& NegationVolume : StaticState.NegationVolumes) {
        if (NegationVolume.bInverse) {
            // The module should be completely inside this
            if (!IsInside(NegationVolume.Bounds, ModuleOcclusion, CollisionTolerance)) {
                return true;
            }
        }
        else {
            // We should not intersect with the negation volume
            FOcclusionEntry NegationVolumeOcclusion { NegationVolume.Bounds, {} };
            if (Intersects(NegationVolumeOcclusion, ModuleOcclusion, CollisionTolerance)) {
                return true;
            }
        }
    }

    for (const FOcclusionEntry& Occlusion : OcclusionList) {
        if (Intersects(ModuleOcclusion, Occlusion, CollisionTolerance)) {
            return true;
        }
    }
    return false;
}

///////////////////////////// FBranchGrowthPermutations /////////////////////////////
SnapLib::FBranchGrowthPermutations::FBranchGrowthPermutations(const TArray<int32>& InOutgoingDoors,
                                                     const TArray<ISnapGraphNodePtr> InOutgoingNodes)
    : OutgoingDoors(InOutgoingDoors)
    , OutgoingNodes(InOutgoingNodes)
{
    Permutation = MakeShareable(new FPermutation<ISnapGraphNodePtr>(OutgoingNodes));
    Combination = MakeShareable(new FCombination<int32>(OutgoingDoors, OutgoingNodes.Num()));

    CurrentSelection = Combination->Execute();
}

void SnapLib::FBranchGrowthPermutations::Execute(TArray<int32>& OutDoors, TArray<ISnapGraphNodePtr>& OutNodes) {
    if (!CanRun()) {
        return;
    }

    if (!Permutation->CanPermute()) {
        CurrentSelection = Combination->Execute();
        Permutation = MakeShareable(new FPermutation<ISnapGraphNodePtr>(OutgoingNodes));
    }

    Permutation->Permutate();

    OutDoors = CurrentSelection;
    OutNodes = Permutation->Data;
}

