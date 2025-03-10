//$ Copyright 2015-23, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Meshing/Geometry/DungeonProceduralMesh.h"

#include "DynamicMeshBuilder.h"
#include "Engine/Engine.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PrimitiveSceneProxy.h"
#include "SceneInterface.h"
#include "StaticMeshResources.h"

namespace {
	FORCEINLINE int32 GetNumVertsPerPrimitive(int32 PrimitiveType) {
		if (PrimitiveType == PT_TriangleList) return 3;
		else if (PrimitiveType == PT_LineList) return 2;
		else if (PrimitiveType == PT_PointList) return 1;
		else return 3;
	}
}

/** Resource array to pass  */
class FDAProcMeshVertexResourceArray : public FResourceArrayInterface
{
public:
	FDAProcMeshVertexResourceArray(void* InData, uint32 InSize)
		: Data(InData)
		, Size(InSize)
	{
	}

	virtual const void* GetResourceData() const override { return Data; }
	virtual uint32 GetResourceDataSize() const override { return Size; }
	virtual void Discard() override { }
	virtual bool IsStatic() const override { return false; }
	virtual bool GetAllowCPUAccess() const override { return false; }
	virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override { }
	
private:
	void* Data;
	uint32 Size;
};

/** Class representing a single section of the proc mesh */
class FDAProcMeshProxySection
{
public:
	/** Material applied to this section */
	UMaterialInterface* Material;
	/** Vertex buffer for this section */
	FStaticMeshVertexBuffers VertexBuffers;
	/** Index buffer for this section */
	FDynamicMeshIndexBuffer32 IndexBuffer;
	/** Vertex factory for this section */
	FLocalVertexFactory VertexFactory;
	/** Whether this section is currently visible */
	bool bSectionVisible;

	int32 PrimitiveType = 0;

	FDAProcMeshProxySection(ERHIFeatureLevel::Type InFeatureLevel)
	: Material(NULL)
	, VertexFactory(InFeatureLevel, "FDAProcMeshProxySection")
	, bSectionVisible(true)
	{}
};

struct FDAProcMeshProxyLODData {
	TArray<FDAProcMeshProxySection*> Sections;
	float ScreenSize{};
};

/** 
 *	Struct used to send update to mesh data 
 *	Arrays may be empty, in which case no update is performed.
 */
class FDAProcMeshSectionUpdateData
{
public:
	int32 TargetLOD{};
	/** Section to update */
	int32 TargetSection;
	/** New vertex information */
	TArray<FDAProcMeshVertex> NewVertexBuffer;
};

static void ConvertDAProcMeshToDynMeshVertex(FDynamicMeshVertex& Vert, const FDAProcMeshVertex& ProcVert)
{
	Vert.Position = FVector3f(ProcVert.Position);
	Vert.Color = ProcVert.Color;
	Vert.TextureCoordinate[0] = FVector2f(ProcVert.UV0);
	Vert.TextureCoordinate[1] = FVector2f(ProcVert.UV1);
	//Vert.TextureCoordinate[2] = FVector2f(ProcVert.UV2);
	//Vert.TextureCoordinate[3] = FVector2f(ProcVert.UV3);
	Vert.TangentX = ProcVert.Tangent.TangentX;
	Vert.TangentZ = ProcVert.Normal;
	Vert.TangentZ.Vector.W = ProcVert.Tangent.bFlipTangentY ? -127 : 127;
}

/** Procedural mesh scene proxy */
class FDAProcMeshSceneProxy final : public FPrimitiveSceneProxy {
public:
	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}


	FDAProcMeshSceneProxy(UDAProcMeshComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, BodySetup(Component->GetBodySetup())
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	{
		bSelectable = Component->bSelectable;
		LODFactorScale = Component->LODFactorScale;

		// Copy each section
		const int32 NumLODs = Component->LODData.Num();
		LODData.SetNum(NumLODs);
		if (NumLODs > 0) {
			for (int LodIdx = 0; LodIdx < NumLODs; LodIdx++) {
				FDAProcMeshProxyLODData& LODInfo = LODData[LodIdx];
				LODInfo.ScreenSize = Component->LODData[LodIdx].ScreenSize;
				TArray<FDAProcMeshProxySection*>& Sections = LODInfo.Sections;
				const int32 NumSections = Component->LODData[LodIdx].MeshSections.Num();
				Sections.AddZeroed(NumSections);
				for (int SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
				{
					FDAProcMeshSection& SrcSection = Component->LODData[LodIdx].MeshSections[SectionIdx];
					if (SrcSection.ProcIndexBuffer.Num() > 0 && SrcSection.ProcVertexBuffer.Num() > 0)
					{
						FDAProcMeshProxySection* NewSection = new FDAProcMeshProxySection(GetScene().GetFeatureLevel());
						NewSection->PrimitiveType = SrcSection.PrimitiveType;

						// Copy data from vertex buffer
						const int32 NumVerts = SrcSection.ProcVertexBuffer.Num();

						// Allocate verts

						TArray<FDynamicMeshVertex> Vertices;
						Vertices.SetNumUninitialized(NumVerts);
						// Copy verts
						for (int VertIdx = 0; VertIdx < NumVerts; VertIdx++)
						{
							const FDAProcMeshVertex& ProcVert = SrcSection.ProcVertexBuffer[VertIdx];
							FDynamicMeshVertex& Vert = Vertices[VertIdx];
							ConvertDAProcMeshToDynMeshVertex(Vert, ProcVert);
						}

						// Copy index buffer
						NewSection->IndexBuffer.Indices = SrcSection.ProcIndexBuffer;

						NewSection->VertexBuffers.InitFromDynamicVertex(&NewSection->VertexFactory, Vertices, 4);

						// Enqueue initialization of render resource
						BeginInitResource(&NewSection->VertexBuffers.PositionVertexBuffer);
						BeginInitResource(&NewSection->VertexBuffers.StaticMeshVertexBuffer);
						BeginInitResource(&NewSection->VertexBuffers.ColorVertexBuffer);
						BeginInitResource(&NewSection->IndexBuffer);
						BeginInitResource(&NewSection->VertexFactory);

						// Grab material
						NewSection->Material = Component->GetMaterial(SectionIdx);
						if (NewSection->Material == NULL)
						{
							NewSection->Material = UMaterial::GetDefaultMaterial(MD_Surface);
						}

						// Copy visibility info
						NewSection->bSectionVisible = SrcSection.bSectionVisible;

						// Save ref to new section
						Sections[SectionIdx] = NewSection;
					}
				}
			}
		}
	}

	virtual ~FDAProcMeshSceneProxy() override {
		for (FDAProcMeshProxyLODData& LODInfo : LODData) {
			for (FDAProcMeshProxySection* Section : LODInfo.Sections) {
				if (Section != nullptr) {
					Section->VertexBuffers.PositionVertexBuffer.ReleaseResource();
					Section->VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
					Section->VertexBuffers.ColorVertexBuffer.ReleaseResource();
					Section->IndexBuffer.ReleaseResource();
					Section->VertexFactory.ReleaseResource();
					delete Section;
				}
			}
		}
	}
	
	virtual int32 GetLOD(const FSceneView* View) const override {
		const FBoxSphereBounds& ProxyBounds = GetBounds();
		const FVector4& Origin = ProxyBounds.Origin;
		const float SphereRadius = ProxyBounds.SphereRadius;
		
		const int32 NumLODs = LODData.Num();
		const FSceneView& LODView = GetLODView(*View);
		const float ScreenRadiusSquared = ComputeBoundsScreenRadiusSquared(Origin, SphereRadius, LODView);
		const float ScreenSizeScale = LODFactorScale * LODView.LODDistanceFactor;
		const int32 MinLOD = 0;

		// Walk backwards and return the first matching LOD
		for (int32 LODIndex = NumLODs - 1; LODIndex >= 0; --LODIndex) {
			const float MeshScreenSize = LODData[LODIndex].ScreenSize * ScreenSizeScale;
			if (FMath::Square(MeshScreenSize * 0.5f) > ScreenRadiusSquared) {
				return FMath::Max(LODIndex, MinLOD);
			}
		}

		return MinLOD;
	}
	
	/** Called on render thread to assign new dynamic data */
	void UpdateSection_RenderThread(FDAProcMeshSectionUpdateData* SectionData)
	{
		check(IsInRenderingThread());

		// Check we have data 
		if(	SectionData != nullptr) 			
		{
			// Check it references a valid section
			if (SectionData->TargetLOD < LODData.Num()
				&& SectionData->TargetSection < LODData[SectionData->TargetLOD].Sections.Num()
				&& LODData[SectionData->TargetLOD].Sections[SectionData->TargetSection] != nullptr)
			{
				TArray<FDAProcMeshProxySection*>& Sections = LODData[SectionData->TargetLOD].Sections;
				FDAProcMeshProxySection* Section = Sections[SectionData->TargetSection];

				// Lock vertex buffer
				const int32 NumVerts = SectionData->NewVertexBuffer.Num();
			
				// Iterate through vertex data, copying in new info
				for(int32 i=0; i<NumVerts; i++)
				{
					const FDAProcMeshVertex& ProcVert = SectionData->NewVertexBuffer[i];
					FDynamicMeshVertex Vertex;
					ConvertDAProcMeshToDynMeshVertex(Vertex, ProcVert);

					Section->VertexBuffers.PositionVertexBuffer.VertexPosition(i) = Vertex.Position;
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, Vertex.TangentX.ToFVector3f(), Vertex.GetTangentY(), Vertex.TangentZ.ToFVector3f());
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 0, Vertex.TextureCoordinate[0]);
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 1, Vertex.TextureCoordinate[1]);
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 2, Vertex.TextureCoordinate[2]);
					Section->VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i, 3, Vertex.TextureCoordinate[3]);
					Section->VertexBuffers.ColorVertexBuffer.VertexColor(i) = Vertex.Color;
				}

				{
					auto& VertexBuffer = Section->VertexBuffers.PositionVertexBuffer;
					
					void* VertexBufferData = RHILockBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
					RHIUnlockBuffer(VertexBuffer.VertexBufferRHI);
				}

				{
					auto& VertexBuffer = Section->VertexBuffers.ColorVertexBuffer;
					void* VertexBufferData = RHILockBuffer(VertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetNumVertices() * VertexBuffer.GetStride(), RLM_WriteOnly);
					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetVertexData(), VertexBuffer.GetNumVertices() * VertexBuffer.GetStride());
					RHIUnlockBuffer(VertexBuffer.VertexBufferRHI);
				}

				{
					auto& VertexBuffer = Section->VertexBuffers.StaticMeshVertexBuffer;
					void* VertexBufferData = RHILockBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTangentSize(), RLM_WriteOnly);
					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTangentData(), VertexBuffer.GetTangentSize());
					RHIUnlockBuffer(VertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
				}

				{
					auto& VertexBuffer = Section->VertexBuffers.StaticMeshVertexBuffer;
					void* VertexBufferData = RHILockBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI, 0, VertexBuffer.GetTexCoordSize(), RLM_WriteOnly);
					FMemory::Memcpy(VertexBufferData, VertexBuffer.GetTexCoordData(), VertexBuffer.GetTexCoordSize());
					RHIUnlockBuffer(VertexBuffer.TexCoordVertexBuffer.VertexBufferRHI);
				}
			}

			// Free data sent from game thread
			delete SectionData;
		}
	}

	void SetSectionVisibility_RenderThread(int32 LODIndex, int32 SectionIndex, bool bNewVisibility)
	{
		check(IsInRenderingThread());

		if( LODIndex < LODData.Num()
			&& SectionIndex < LODData[LODIndex].Sections.Num()
			&& LODData[LODIndex].Sections[SectionIndex] != nullptr)
		{
			LODData[LODIndex].Sections[SectionIndex]->bSectionVisible = bNewVisibility;
		}
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		// Set up wireframe material (if needed)
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;
		const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;
		
		FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
		if (bWireframe) {
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL,
				GetSelectionColor(FLinearColor(0, 0.5f, 1.f), !(GIsEditor && EngineShowFlags.Selection) || IsSelected(), IsHovered(), false)
				);

			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		// TODO: Get this from the view
		TArray<int> LODIndices = { 0 };

		// For each view..
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				const int32 LODIdx = GetLOD(View);
				//for (int LODIdx = 0; LODIdx < LODData.Num(); LODIdx++)
				if (LODData.IsValidIndex(LODIdx)) {
					const TArray<FDAProcMeshProxySection*>& Sections = LODData[LODIdx].Sections;
			
					// Iterate over sections
					for (const FDAProcMeshProxySection* Section : Sections)
					{
						if (Section != nullptr && Section->bSectionVisible)
						{
							FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Section->Material->GetRenderProxy();

							// Draw the mesh.
							FMeshBatch& Mesh = Collector.AllocateMesh();
							FMeshBatchElement& BatchElement = Mesh.Elements[0];
							BatchElement.IndexBuffer = &Section->IndexBuffer;

							/*
							BatchElement.MaxScreenSize = LODData[LODIdx].ScreenSize;
							BatchElement.MinScreenSize = 0;
							if (LODIdx + 1 < LODData.Num()) {
								BatchElement.MinScreenSize = LODData[LODIdx + 1].ScreenSize;
							}
							*/
							
							Mesh.bWireframe = bWireframe;
							Mesh.VertexFactory = &Section->VertexFactory;
							Mesh.MaterialRenderProxy = MaterialProxy;

							bool bHasPrecomputedVolumetricLightmap;
							FMatrix PreviousLocalToWorld;
							int32 SingleCaptureIndex;
							bool bOutputVelocity;
							GetScene().GetPrimitiveUniformShaderParameters_RenderThread(GetPrimitiveSceneInfo(), bHasPrecomputedVolumetricLightmap, PreviousLocalToWorld, SingleCaptureIndex, bOutputVelocity);

							FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
							DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), PreviousLocalToWorld, GetBounds(), GetLocalBounds(), true, bHasPrecomputedVolumetricLightmap, bOutputVelocity);
							BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

							int32 NumVertsPerPrimitive = GetNumVertsPerPrimitive(Section->PrimitiveType);
							//BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, DrawsVelocity());
							BatchElement.FirstIndex = 0;
							BatchElement.NumPrimitives = Section->IndexBuffer.Indices.Num() / NumVertsPerPrimitive;
							BatchElement.MinVertexIndex = 0;
							BatchElement.MaxVertexIndex = Section->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
							
							Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
							Mesh.Type = Section->PrimitiveType;
							Mesh.DepthPriorityGroup = SDPG_World;
							Mesh.bCanApplyViewModeOverrides = false;
							Mesh.bSelectable = bSelectable;
							Collector.AddMesh(ViewIndex, Mesh);
						}
					}
				}
			}
		}
		
		// Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				// Draw simple collision as wireframe if 'show collision', and collision is enabled, and we are not using the complex as the simple
				if (ViewFamily.EngineShowFlags.Collision && IsCollisionEnabled() && BodySetup->GetCollisionTraceFlag() != ECollisionTraceFlag::CTF_UseComplexAsSimple)
				{
					FTransform GeomTransform(GetLocalToWorld());
					BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255), IsSelected(), IsHovered()).ToFColor(true), NULL, false, false, DrawsVelocity(), ViewIndex, Collector);
				}

				// Render bounds
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
		}
#endif
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const override
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}

	void SetWireframeColor(const FLinearColor& InColor) {
		WireframeColor = InColor;
	}

private:
	TArray<FDAProcMeshProxyLODData> LODData;
	float LODFactorScale{1.0f};

	UBodySetup* BodySetup;

	FMaterialRelevance MaterialRelevance;

	bool bSelectable = true;
	FLinearColor WireframeColor{0, 0.5f, 1.f}; 
};

//////////////////////////////////////////////////////////////////////////


UDAProcMeshComponent::UDAProcMeshComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bUseComplexAsSimpleCollision = true;
}

void UDAProcMeshComponent::PostLoad()
{
	Super::PostLoad();

	if (MeshBodySetup && IsTemplate())
	{
		MeshBodySetup->SetFlags(RF_Public);
	}
}

FDAProcMeshSection::FDAProcMeshSection(): SectionLocalBox(ForceInit)
                                          , bEnableCollision(false)
                                          , bSectionVisible(true)
                                          , PrimitiveType(PT_TriangleList) {
	
}

void FDAProcMeshSection::Reset() {
	ProcVertexBuffer.Empty();
	ProcIndexBuffer.Empty();
	SectionLocalBox.Init();
	bEnableCollision = false;
	bSectionVisible = true;
}

void UDAProcMeshComponent::CreateMeshSection(int LODIndex, int32 SectionIndex, const TArray<FDAProcMeshVertex>& Vertices, const TArray<int32>& Indices, int32 PrimitiveType, bool bCreateCollision)
{
	if (LODIndex >= LODData.Num()) {
		LODData.SetNum(LODIndex + 1, false);
	}

	TArray<FDAProcMeshSection>& MeshSections = LODData[LODIndex].MeshSections;
	// Ensure sections array is long enough
	if (SectionIndex >= MeshSections.Num())
	{
		MeshSections.SetNum(SectionIndex + 1, false);
	}

	// Reset this section (in case it already existed)
	FDAProcMeshSection& NewSection = MeshSections[SectionIndex];
	NewSection.Reset();

	const int32 NumVerts = Vertices.Num();
	NewSection.ProcVertexBuffer = Vertices;

	// Update bounding box
	for (int32 VertIdx = 0; VertIdx < NumVerts; VertIdx++) {
		NewSection.SectionLocalBox += Vertices[VertIdx].Position;
	}

	// Copy index buffer (clamping to vertex range)
	int32 NumVerticesPerPrimitive = GetNumVertsPerPrimitive(PrimitiveType);
	int32 NumTriIndices = Indices.Num();
	NumTriIndices = (NumTriIndices / NumVerticesPerPrimitive) * NumVerticesPerPrimitive; // Ensure we have exact number of triangles (array is multiple of 3 long)

	NewSection.ProcIndexBuffer.Reset();
	NewSection.ProcIndexBuffer.AddUninitialized(NumTriIndices);
	for (int32 IndexIdx = 0; IndexIdx < NumTriIndices; IndexIdx++)
	{
		NewSection.ProcIndexBuffer[IndexIdx] = FMath::Min(Indices[IndexIdx], NumVerts - 1);
	}

	NewSection.PrimitiveType = PrimitiveType;
	NewSection.bEnableCollision = bCreateCollision && PrimitiveType == PT_TriangleList;

	UpdateLocalBounds(); // Update overall bounds
	UpdateScreenSizes();
	UpdateCollision(); // Mark collision as dirty
	MarkRenderStateDirty(); // New section requires recreating scene proxy
}

void UDAProcMeshComponent::UpdateScreenSizes() {
	// Calculate the LOD screen sizes
	// Possible model for flexible LODs
	const float MaxDeviation = 10000.0f; // specify
	const float PixelError = UE_SMALL_NUMBER;
	const float ViewDistance = 1000; //(MaxDeviation * 960.0f) / PixelError;

	// Generate a projection matrix.
	const float HalfFOV = UE_PI * 0.25f;
	const float ScreenWidth = 1920.0f;
	const float ScreenHeight = 1080.0f;
	const FPerspectiveMatrix ProjMatrix(HalfFOV, ScreenWidth, ScreenHeight, 1.0f);

	const FBoxSphereBounds& ProxyBounds = GetLocalBounds();
	const float SphereRadius = ProxyBounds.SphereRadius;
	constexpr float ScreenSizeMult = 1.f / 1.125f;
	for (int32 LOD = 0; LOD < LODData.Num(); ++LOD) {
		if (LOD == 0) {
			LODData[LOD].ScreenSize = ComputeBoundsScreenSize(FVector::ZeroVector, SphereRadius, FVector(0.0f, 0.0f, ViewDistance + SphereRadius), ProjMatrix);
		}
		else {
			LODData[LOD].ScreenSize = LODData[LOD - 1].ScreenSize * ScreenSizeMult;
		}
	}
}

void UDAProcMeshComponent::ClearMeshSection(int LODIndex, int32 SectionIndex)
{
	if (LODIndex < LODData.Num() && SectionIndex < LODData[LODIndex].MeshSections.Num())
	{
		LODData[LODIndex].MeshSections[SectionIndex].Reset();
		UpdateLocalBounds();
		UpdateCollision();
		UpdateScreenSizes();
		MarkRenderStateDirty();
	}
}

void UDAProcMeshComponent::ClearAllMeshSections()
{
	LODData.Empty();
	UpdateLocalBounds();
	UpdateCollision();
	MarkRenderStateDirty();
}

void UDAProcMeshComponent::SetMeshSectionVisible(int LODIndex, int32 SectionIndex, bool bNewVisibility)
{
	if(LODIndex < LODData.Num() && SectionIndex < LODData[LODIndex].MeshSections.Num())
	{
		// Set game thread state
		LODData[LODIndex].MeshSections[SectionIndex].bSectionVisible = bNewVisibility;

		if (SceneProxy)
		{
			FDAProcMeshSceneProxy* MeshProxy = (FDAProcMeshSceneProxy*)SceneProxy;
			ENQUEUE_RENDER_COMMAND(FDAMeshSectionUpdate)
			([MeshProxy, SectionIndex, bNewVisibility, LODIndex](FRHICommandListImmediate& RHICmdList) {
				MeshProxy->SetSectionVisibility_RenderThread(LODIndex, SectionIndex, bNewVisibility);
			});

		}
	}
}

bool UDAProcMeshComponent::IsMeshSectionVisible(int LODIndex, int32 SectionIndex) const
{
	return (LODIndex < LODData.Num() && SectionIndex < LODData[LODIndex].MeshSections.Num())
		? LODData[LODIndex].MeshSections[SectionIndex].bSectionVisible
		: false;
}

int32 UDAProcMeshComponent::GetNumSections(int LODIndex) const
{
	return LODIndex < LODData.Num() ? LODData[LODIndex].MeshSections.Num() : 0;
}

void UDAProcMeshComponent::AddCollisionConvexMesh(TArray<FVector> ConvexVerts)
{
	if(ConvexVerts.Num() >= 4)
	{ 
		// New element
		FKConvexElem NewConvexElem;
		// Copy in vertex info
		NewConvexElem.VertexData = ConvexVerts;
		// Update bounding box
		NewConvexElem.ElemBox = FBox(NewConvexElem.VertexData);
		// Add to array of convex elements
		CollisionConvexElems.Add(NewConvexElem);
		// Refresh collision
		UpdateCollision();
	}
}

void UDAProcMeshComponent::ClearCollisionConvexMeshes()
{
	// Empty simple collision info
	CollisionConvexElems.Empty();
	// Refresh collision
	UpdateCollision();
}

void UDAProcMeshComponent::SetCollisionConvexMeshes(const TArray< TArray<FVector> >& ConvexMeshes)
{
	CollisionConvexElems.Reset();

	// Create element for each convex mesh
	for (int32 ConvexIndex = 0; ConvexIndex < ConvexMeshes.Num(); ConvexIndex++)
	{
		FKConvexElem NewConvexElem;
		NewConvexElem.VertexData = ConvexMeshes[ConvexIndex];
		NewConvexElem.ElemBox = FBox(NewConvexElem.VertexData);

		CollisionConvexElems.Add(NewConvexElem);
	}

	UpdateCollision();
}


void UDAProcMeshComponent::UpdateLocalBounds()
{
	FBox LocalBox(ForceInit);

	for (const FDAProcMeshLODData& LODInfo : LODData) {
		for (const FDAProcMeshSection& Section : LODInfo.MeshSections)
		{
			LocalBox += Section.SectionLocalBox;
		}
	}

	LocalBounds = LocalBox.IsValid ? FBoxSphereBounds(LocalBox) : FBoxSphereBounds(FVector(0, 0, 0), FVector(0, 0, 0), 0); // fallback to reset box sphere bounds

	// Update global bounds
	UpdateBounds();
	// Need to send to render thread
	MarkRenderTransformDirty();
}

FPrimitiveSceneProxy* UDAProcMeshComponent::CreateSceneProxy()
{
	return new FDAProcMeshSceneProxy(this);
}

int32 UDAProcMeshComponent::GetNumMaterials() const
{
	return LODData.Num() > 0 ? LODData[0].MeshSections.Num() : 0;
}


FDAProcMeshSection* UDAProcMeshComponent::GetMeshSection(int LODIndex, int32 SectionIndex)
{
	if (LODIndex < LODData.Num() && SectionIndex < LODData[LODIndex].MeshSections.Num())
	{
		return &LODData[LODIndex].MeshSections[SectionIndex];
	}
	else
	{
		return nullptr;
	}
}


void UDAProcMeshComponent::SetMeshSection(int LODIndex, int32 SectionIndex, const FDAProcMeshSection& Section)
{
	// Ensure lod array is long enough
	if (LODIndex >= LODData.Num()) {
		LODData.SetNum(LODIndex + 1, false);
	}
	TArray<FDAProcMeshSection>& MeshSections = LODData[LODIndex].MeshSections;
	
	// Ensure sections array is long enough
	if (SectionIndex >= MeshSections.Num())
	{
		MeshSections.SetNum(SectionIndex + 1, false);
	}

	MeshSections[SectionIndex] = Section;

	UpdateLocalBounds(); // Update overall bounds
	UpdateCollision(); // Mark collision as dirty
	UpdateScreenSizes();
	MarkRenderStateDirty(); // New section requires recreating scene proxy
}

FBoxSphereBounds UDAProcMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds Ret(LocalBounds.TransformBy(LocalToWorld));

	Ret.BoxExtent *= BoundsScale;
	Ret.SphereRadius *= BoundsScale;

	return Ret;
}

bool UDAProcMeshComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	int32 VertexBase = 0; // Base vertex index for current section

	// See if we should copy UVs
	bool bCopyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults; 
	if (bCopyUVs)
	{
		CollisionData->UVs.AddZeroed(1); // only one UV channel
	}

	for (int32 LODIdx = 0; LODIdx < LODData.Num(); LODIdx++) {
		TArray<FDAProcMeshSection>& MeshSections = LODData[LODIdx].MeshSections;

		for (int32 SectionIdx = 0; SectionIdx < MeshSections.Num(); SectionIdx++)
		{
			FDAProcMeshSection& Section = MeshSections[SectionIdx];
			// Do we have collision enabled?
			if (Section.bEnableCollision && Section.PrimitiveType == PT_TriangleList)
			{
				// Copy vert data
				for (int32 VertIdx = 0; VertIdx < Section.ProcVertexBuffer.Num(); VertIdx++)
				{
					CollisionData->Vertices.Add(FVector3f(Section.ProcVertexBuffer[VertIdx].Position));

					// Copy UV if desired
					if (bCopyUVs)
					{
						CollisionData->UVs[0].Add(Section.ProcVertexBuffer[VertIdx].UV0);
					}
				}

				// Copy triangle data
				const int32 NumTriangles = Section.ProcIndexBuffer.Num() / 3;
				for (int32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++)
				{
					// Need to add base offset for indices
					FTriIndices Triangle;
					Triangle.v0 = Section.ProcIndexBuffer[(TriIdx * 3) + 0] + VertexBase;
					Triangle.v1 = Section.ProcIndexBuffer[(TriIdx * 3) + 1] + VertexBase;
					Triangle.v2 = Section.ProcIndexBuffer[(TriIdx * 3) + 2] + VertexBase;
					CollisionData->Indices.Add(Triangle);

					// Also store material info
					CollisionData->MaterialIndices.Add(SectionIdx);
				}

				// Remember the base index that new verts will be added from in next section
				VertexBase = CollisionData->Vertices.Num();
			}
		}
	}

	CollisionData->bFlipNormals = true;
	CollisionData->bDeformableMesh = true;
	CollisionData->bFastCook = true;

	return true;
}

bool UDAProcMeshComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	for (int32 LODIdx = 0; LODIdx < LODData.Num(); LODIdx++) {
		const TArray<FDAProcMeshSection>& MeshSections = LODData[LODIdx].MeshSections;
		for (const FDAProcMeshSection& Section : MeshSections) {
			if (Section.ProcIndexBuffer.Num() >= 3 && Section.bEnableCollision && Section.PrimitiveType == PT_TriangleList) {
				return true;
			}
		}
	}

	return false;
}

void UDAProcMeshComponent::Serialize(FArchive& Ar) {
	Super::Serialize(Ar);

	// TODO: Implement Versioning
	Ar << LODData;
}

UBodySetup* UDAProcMeshComponent::CreateBodySetupHelper()
{
	// The body setup in a template needs to be public since the property is Tnstanced and thus is the archetype of the instance meaning there is a direct reference
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	NewBodySetup->bGenerateMirroredCollision = false;
	NewBodySetup->bDoubleSidedGeometry = true;
	NewBodySetup->CollisionTraceFlag = bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;

	return NewBodySetup;
}

void UDAProcMeshComponent::CreateMeshBodySetup()
{
	if (MeshBodySetup == nullptr)
	{
		MeshBodySetup = CreateBodySetupHelper();
	}
}

void UDAProcMeshComponent::UpdateCollision()
{
	UWorld* World = GetWorld();
	const bool bUseAsyncCook = World && World->IsGameWorld() && bUseAsyncCooking;

	if(bUseAsyncCook)
	{
		AsyncBodySetupQueue.Add(CreateBodySetupHelper());
	}
	else
	{
		AsyncBodySetupQueue.Empty();	//If for some reason we modified the async at runtime, just clear any pending async body setups
		CreateMeshBodySetup();
	}
	
	UBodySetup* UseBodySetup = bUseAsyncCook ? AsyncBodySetupQueue.Last() : MeshBodySetup;

	// Fill in simple collision convex elements
	UseBodySetup->AggGeom.ConvexElems = CollisionConvexElems;

	// Set trace flag
	UseBodySetup->CollisionTraceFlag = bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;

	if(bUseAsyncCook)
	{
		UseBodySetup->CreatePhysicsMeshesAsync(FOnAsyncPhysicsCookFinished::CreateUObject(this, &UDAProcMeshComponent::FinishPhysicsAsyncCook, UseBodySetup));
	}
	else
	{
		// New GUID as collision has changed
		UseBodySetup->BodySetupGuid = FGuid::NewGuid();
		// Also we want cooked data for this
		UseBodySetup->bHasCookedCollisionData = true;
		UseBodySetup->InvalidatePhysicsData();
		UseBodySetup->CreatePhysicsMeshes();
		RecreatePhysicsState();
	}
}

void UDAProcMeshComponent::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
	TArray<UBodySetup*> NewQueue;
	NewQueue.Reserve(AsyncBodySetupQueue.Num());

	int32 FoundIdx;
	if(AsyncBodySetupQueue.Find(FinishedBodySetup, FoundIdx))
	{
		if (bSuccess) {
			//The new body was found in the array meaning it's newer so use it
			MeshBodySetup = FinishedBodySetup;
			RecreatePhysicsState();

			//remove any async body setups that were requested before this one
			for (int32 AsyncIdx = FoundIdx + 1; AsyncIdx < AsyncBodySetupQueue.Num(); ++AsyncIdx)
			{
				NewQueue.Add(AsyncBodySetupQueue[AsyncIdx]);
			}

			AsyncBodySetupQueue = NewQueue;
		}
		else {
			AsyncBodySetupQueue.RemoveAt(FoundIdx);
		}
	}
}

UBodySetup* UDAProcMeshComponent::GetBodySetup()
{
	CreateMeshBodySetup();
	return MeshBodySetup;
}

UMaterialInterface* UDAProcMeshComponent::GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const
{
	UMaterialInterface* Result = nullptr;
	SectionIndex = 0;

	constexpr int32 LODIndex = 0;
	if (FaceIndex >= 0 && LODData.IsValidIndex(LODIndex))
	{
		const TArray<FDAProcMeshSection>& MeshSections = LODData[LODIndex].MeshSections;
		
		// Look for element that corresponds to the supplied face
		int32 TotalFaceCount = 0;
		for (int32 SectionIdx = 0; SectionIdx < MeshSections.Num(); SectionIdx++)
		{
			const FDAProcMeshSection& Section = MeshSections[SectionIdx];
			const int32 NumFaces = Section.ProcIndexBuffer.Num() / 3;
			TotalFaceCount += NumFaces;

			if (FaceIndex < TotalFaceCount)
			{
				// Grab the material
				Result = GetMaterial(SectionIdx);
				SectionIndex = SectionIdx;
				break;
			}
		}
	}

	return Result;
}



