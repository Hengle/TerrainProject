/*
* Copyright (c) <2021> Side Effects Software Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. The name of Side Effects Software may not be used to endorse or
*    promote products derived from this software without specific prior
*    written permission.
*
* THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE "AS IS" AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
* NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "HoudiniMeshTranslator.h"

#include "HoudiniApi.h"
#include "HoudiniEngine.h"
#include "HoudiniOutput.h"
#include "HoudiniGenericAttribute.h"
#include "HoudiniGeoPartObject.h"
#include "HoudiniGenericAttribute.h"
#include "HoudiniEngineUtils.h"
#include "HoudiniEnginePrivatePCH.h"
#include "HoudiniMaterialTranslator.h"
#include "HoudiniAssetActor.h"

#include "HoudiniStaticMesh.h"
#include "HoudiniStaticMeshComponent.h"
#include "Engine/StaticMeshSocket.h"

#include "PhysicsEngine/BodySetup.h"
#include "Engine/StaticMesh.h"
#include "PackageTools.h"
#include "RawMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "MeshDescriptionOperations.h"

#include "BSPOps.h"
#include "Model.h"
#include "Engine/Polys.h"
#include "AssetRegistryModule.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatformManagerModule.h"
#include "AI/Navigation/NavCollisionBase.h"
#include "ObjectTools.h"

// #include "Async/ParallelFor.h"

#include "ProfilingDebugging/CpuProfilerTrace.h"

#include "EditorSupportDelegates.h"

#if WITH_EDITOR
	#include "UnrealEd/Private/ConvexDecompTool.h"
	#include "Editor/UnrealEd/Private/GeomFitUtils.h"
	#include "LevelEditorViewport.h"
	#include "FileHelpers.h"
#endif

#define LOCTEXT_NAMESPACE HOUDINI_LOCTEXT_NAMESPACE

// 
bool
FHoudiniMeshTranslator::CreateAllMeshesAndComponentsFromHoudiniOutput(
	UHoudiniOutput* InOutput, 
	const FHoudiniPackageParams& InPackageParams,
	const EHoudiniStaticMeshMethod& InStaticMeshMethod,
	const FHoudiniStaticMeshGenerationProperties& InSMGenerationProperties,
	UObject* InOuterComponent,
	bool bInTreatExistingMaterialsAsUpToDate,
	bool bInDestroyProxies)
{
	if (!InOutput || InOutput->IsPendingKill())
		return false;

	if (!InPackageParams.OuterPackage || InPackageParams.OuterPackage->IsPendingKill())
		return false;

	if (!InOuterComponent || InOuterComponent->IsPendingKill())
		return false;

	TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject> NewOutputObjects;
	TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject> OldOutputObjects = InOutput->GetOutputObjects();
	TMap<FString, UMaterialInterface*>& AssignementMaterials = InOutput->GetAssignementMaterials();
	TMap<FString, UMaterialInterface*>& ReplacementMaterials = InOutput->GetReplacementMaterials();

	bool InForceRebuild = false; 
	if (InOutput->HasAnyCurrentProxy() && InStaticMeshMethod != EHoudiniStaticMeshMethod::UHoudiniStaticMesh)
	{
		// Make sure we're not preventing refinement
		InForceRebuild = true;
	}

	// Iterate on all of the output's HGPO, creating meshes as we go
	for (const FHoudiniGeoPartObject& CurHGPO : InOutput->HoudiniGeoPartObjects)
	{
		// Not a mesh, skip
		if (CurHGPO.Type != EHoudiniPartType::Mesh)
			continue;

		// See if we have some uproperty attributes to update on 
		// the outer component (in most case, the HAC)
		TArray<FHoudiniGenericAttribute> PropertyAttributes;
		if (GetGenericPropertiesAttributes(
			CurHGPO.GeoId, CurHGPO.PartId,
			0, 0,
			PropertyAttributes))
		{
			UpdateGenericPropertiesAttributes(
				InOuterComponent, PropertyAttributes);
		}

		CreateStaticMeshFromHoudiniGeoPartObject(
			CurHGPO,
			InPackageParams,
			OldOutputObjects,
			NewOutputObjects,
			AssignementMaterials,
			ReplacementMaterials,
			InForceRebuild,
			InStaticMeshMethod,
			InSMGenerationProperties,
			bInTreatExistingMaterialsAsUpToDate);
	}

	return FHoudiniMeshTranslator::CreateOrUpdateAllComponents(
		InOutput,
		InOuterComponent,
		NewOutputObjects,
		bInDestroyProxies);
}

bool
FHoudiniMeshTranslator::CreateOrUpdateAllComponents(
	UHoudiniOutput* InOutput,
	UObject* InOuterComponent,
	TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject>& InNewOutputObjects,
	bool bInDestroyProxies,
	bool bInApplyGenericProperties)
{
	if (!InOutput || InOutput->IsPendingKill())
		return false;

	if (!InOuterComponent || InOuterComponent->IsPendingKill())
		return false;

	TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject> OldOutputObjects = InOutput->GetOutputObjects();

	// Remove Static Meshes and their components from the old map 
	// to avoid their deletion if new proxies were created for them
	for (auto& NewOutputObj : InNewOutputObjects)
	{
		FHoudiniOutputObjectIdentifier OutputIdentifier = NewOutputObj.Key;

		// See if we already had that pair in the old map of static mesh
		FHoudiniOutputObject* FoundOldOutputObj = OldOutputObjects.Find(NewOutputObj.Key);
		if (!FoundOldOutputObj)
			continue;
		
		UObject* NewStaticMesh = NewOutputObj.Value.OutputObject;
		UObject* NewProxyMesh = NewOutputObj.Value.ProxyObject;

		UObject* OldStaticMesh = FoundOldOutputObj->OutputObject;
		if (OldStaticMesh && !OldStaticMesh->IsPendingKill())
		{
			// If a proxy was created for an existing static mesh, keep the existing static
			// mesh (will be hidden)
			if (NewProxyMesh && NewOutputObj.Value.bProxyIsCurrent)
			{
				// Remove it from the old map to avoid its destruction
				OldOutputObjects.Remove(OutputIdentifier);
			}
			else if (NewStaticMesh && NewStaticMesh == OldStaticMesh)
			{
				// Remove it from the old map to avoid its destruction
				OldOutputObjects.Remove(OutputIdentifier);
			}
		}
		
		UObject* OldProxyMesh = FoundOldOutputObj->ProxyObject;
		if (OldProxyMesh && !OldProxyMesh->IsPendingKill())
		{
			// If a new static mesh was created for a proxy, keep the proxy (will be hidden)
			// ... unless we want to explicitly destroy proxies
			if (NewStaticMesh && !bInDestroyProxies)
			{
				// Remove it from the old map to avoid its destruction
				OldOutputObjects.Remove(OutputIdentifier);
			}
			else if (NewProxyMesh && (NewProxyMesh == OldProxyMesh))
			{
				// Remove it from the old map to avoid its destruction
				OldOutputObjects.Remove(OutputIdentifier);
			}
		}
	}	

	// The old map now only contains unused/stale Meshes/Components, delete them
	for (auto& OldPair : OldOutputObjects)
	{
		// Get the old Identifier / StaticMesh
		FHoudiniOutputObjectIdentifier& OutputIdentifier = OldPair.Key;
		FHoudiniOutputObject& OldOutputObject = OldPair.Value;

		// Remove the old component from the map
		RemoveAndDestroyComponent(OldOutputObject.OutputComponent);
		OldOutputObject.OutputComponent = nullptr;
		// Remove the old proxy component from the map
		RemoveAndDestroyComponent(OldOutputObject.ProxyComponent);
		OldOutputObject.ProxyComponent = nullptr;

		if (OldOutputObject.OutputObject && !OldOutputObject.OutputObject->IsPendingKill())
		{
			OldOutputObject.OutputObject->MarkPendingKill();
		}

		if (OldOutputObject.ProxyObject && !OldOutputObject.ProxyObject->IsPendingKill())
		{
			OldOutputObject.ProxyObject->MarkPendingKill();
		}		
	}
	OldOutputObjects.Empty();

	/*
	// Remove any stale components, these are components with OutputIdentifiers that are not 
	// in NewOutputObjects. This seems to happen mostly with the first or second cook after a
	// "Rebuild Asset"
	if (OutputComponents.Num() > 0 || OutputProxyComponents.Num() > 0)
	{
		TArray<TPair<FHoudiniOutputObjectIdentifier, UObject*>> StaleComponents;
		const uint32 MaxNumStale = FMath::Max(OutputComponents.Num(), OutputProxyComponents.Num());
		StaleComponents.Reserve(MaxNumStale);
		for (auto& ComponentPair : OutputComponents)
		{
			if (!NewOutputObjects.Contains(ComponentPair.Key) && !OldOutputObjectsReplacedByProxy.Contains(ComponentPair.Key))
			{
				StaleComponents.Add(ComponentPair);
			}
		}
		for (auto& ComponentPair : StaleComponents)
		{
			RemoveAndDestroyComponent(ComponentPair.Key, OutputComponents);
		}
		StaleComponents.Empty(MaxNumStale);

		for (auto& ComponentPair : OutputProxyComponents)
		{
			if (!NewOutputProxyObjects.Contains(ComponentPair.Key) && !OldOutputProxyObjectsReplacedByStaticMesh.Contains(ComponentPair.Key))
			{
				StaleComponents.Add(ComponentPair);
			}
		}
		for (auto& ComponentPair : StaleComponents)
		{
			RemoveAndDestroyComponent(ComponentPair.Key, OutputProxyComponents);
		}
		StaleComponents.Empty();
	}
	*/

	// Now create/update the new static mesh components
	for (auto& NewPair : InNewOutputObjects)
	{
		// Get the old Identifier / StaticMesh
		const FHoudiniOutputObjectIdentifier& OutputIdentifier = NewPair.Key;
		FHoudiniOutputObject& OutputObject = NewPair.Value;

		// Check if we should create a Proxy/SMC
		if (OutputObject.bProxyIsCurrent)
		{
			UObject *Mesh = OutputObject.ProxyObject;
			if (!Mesh || Mesh->IsPendingKill() || !Mesh->IsA<UHoudiniStaticMesh>())
			{
				HOUDINI_LOG_ERROR(TEXT("Proxy Mesh is invalid (wrong type or pending kill)..."));
				continue;
			}

			// Create or update a new proxy component
			TSubclassOf<UMeshComponent> ComponentType = UHoudiniStaticMeshComponent::StaticClass();
			const FHoudiniGeoPartObject *FoundHGPO = nullptr;
			bool bCreated = false;
			UMeshComponent *MeshComponent = CreateOrUpdateMeshComponent(InOutput, InOuterComponent, OutputIdentifier, ComponentType, OutputObject, FoundHGPO, bCreated);
			if (MeshComponent)
			{
				UHoudiniStaticMeshComponent *HSMC = Cast<UHoudiniStaticMeshComponent>(MeshComponent);

				if (bCreated)
				{
					PostCreateHoudiniStaticMeshComponent(HSMC, Mesh);
				}
				else if (HSMC && !HSMC->IsPendingKill() && HSMC->GetMesh() != Mesh)
				{
					// We need to reassign the HSM to the component
					UHoudiniStaticMesh* HSM = Cast<UHoudiniStaticMesh>(Mesh);
					HSMC->SetMesh(HSM);
				}

				UpdateMeshComponent(
					MeshComponent, 
					OutputIdentifier, 
					FoundHGPO, 
					InOutput->HoudiniCreatedSocketActors, 
					InOutput->HoudiniAttachedSocketActors,
					bInApplyGenericProperties);

				if (!bCreated)
				{
					// For proxy meshes: notify that the mesh has been updated
					HSMC->NotifyMeshUpdated();
					HSMC->SetHoudiniIconVisible(true);
				}
			}

			// Now, ensure that meshes replaced by proxies are still kept but hidden
			USceneComponent* SceneComponent = Cast<USceneComponent>(OutputObject.OutputComponent);
			if (SceneComponent)
			{
				SceneComponent->SetVisibility(false);
				SceneComponent->SetHiddenInGame(true);
			}

			// If the proxy mesh we just created is templated, hide it in game
			if (FoundHGPO->bIsTemplated)
			{
				MeshComponent->SetHiddenInGame(true);
			}
		}
		else
		{
			// Create a new SMC if needed
			UObject* Mesh = OutputObject.OutputObject;
			if (!Mesh || Mesh->IsPendingKill() || !Mesh->IsA<UStaticMesh>())
			{
				HOUDINI_LOG_ERROR(TEXT("Mesh is invalid (wrong type or pending kill)..."));
				continue;
			}

			TSubclassOf<UMeshComponent> ComponentType = UStaticMeshComponent::StaticClass();
			const FHoudiniGeoPartObject *FoundHGPO = nullptr;
			bool bCreated = false;
			UMeshComponent *MeshComponent = CreateOrUpdateMeshComponent(InOutput, InOuterComponent, OutputIdentifier, ComponentType, OutputObject, FoundHGPO, bCreated);
			if (MeshComponent)
			{
				if (bCreated)
				{
					PostCreateStaticMeshComponent(Cast<UStaticMeshComponent>(MeshComponent), Mesh);
				}
				UpdateMeshComponent(
					MeshComponent, 
					OutputIdentifier, 
					FoundHGPO, 
					InOutput->HoudiniCreatedSocketActors, 
					InOutput->HoudiniAttachedSocketActors,
					bInApplyGenericProperties);
			}

			// Now, ensure that proxies replaced by meshes are still kept but hidden
			UHoudiniStaticMeshComponent *HSMC = Cast<UHoudiniStaticMeshComponent>(OutputObject.ProxyComponent);
			if (HSMC)
			{
				HSMC->SetVisibility(false);
				HSMC->SetHiddenInGame(true);
				HSMC->SetHoudiniIconVisible(false);
			}

			// If the mesh we just created is templated, hide it in game
			if (FoundHGPO->bIsTemplated)
			{
				MeshComponent->SetHiddenInGame(true);
			}
		}
	}

	// Assign the new output objects to the output
	InOutput->SetOutputObjects(InNewOutputObjects);

	return true;
}

void
FHoudiniMeshTranslator::UpdateMeshComponent(UMeshComponent *InMeshComponent, const FHoudiniOutputObjectIdentifier &InOutputIdentifier, 
	const FHoudiniGeoPartObject *InHGPO, TArray<AActor*> &HoudiniCreatedSocketActors, TArray<AActor*> &HoudiniAttachedSocketActors,
	bool bInApplyGenericProperties)
{
	// Update collision/visibility
	EHoudiniSplitType SplitType = GetSplitTypeFromSplitName(InOutputIdentifier.SplitIdentifier);
	if (SplitType == EHoudiniSplitType::InvisibleComplexCollider)
	{
		// Invisible complex collider should not be seen
		InMeshComponent->SetVisibility(false);
		InMeshComponent->SetHiddenInGame(true);
		InMeshComponent->SetCollisionProfileName(FName(TEXT("InvisibleWall")));
		InMeshComponent->SetCastShadow(false);
	}
	else
	{
		// Update visiblity
		bool bVisible = InHGPO ? InHGPO->bIsVisible : true;
		InMeshComponent->SetVisibility(bVisible);
		InMeshComponent->SetHiddenInGame(!bVisible);
	}

	// TODO:
	// Update navmesh?

	// Transform the component by transformation provided by HAPI.
	InMeshComponent->SetRelativeTransform(InHGPO->TransformMatrix);

	// If the static mesh had sockets, we can assign the desired actor to them now
	UStaticMeshComponent * StaticMeshComponent = Cast<UStaticMeshComponent>(InMeshComponent);
	UStaticMesh * StaticMesh = nullptr;
	if (StaticMeshComponent && !StaticMeshComponent->IsPendingKill())
		StaticMesh = StaticMeshComponent->GetStaticMesh();

	if (StaticMesh && !StaticMesh->IsPendingKill()) 
	{
		int32 NumberOfSockets = StaticMesh == nullptr ? 0 : StaticMesh->Sockets.Num();
		for (int32 nSocket = 0; nSocket < NumberOfSockets; nSocket++)
		{
			UStaticMeshSocket* MeshSocket = StaticMesh->Sockets[nSocket];
			if (MeshSocket && !MeshSocket->IsPendingKill() && (MeshSocket->Tag.IsEmpty()))
				continue;

			AddActorsToMeshSocket(StaticMesh->Sockets[nSocket], StaticMeshComponent, HoudiniCreatedSocketActors, HoudiniAttachedSocketActors);
		}

		// Iterate all remaining created socket actors, destroy the ones that are not assigned to socket after re-cook
		{
			for (int32 Idx = HoudiniCreatedSocketActors.Num() - 1; Idx >= 0; --Idx) 
			{
				AActor * CurActor = HoudiniCreatedSocketActors[Idx];

				if (!CurActor || CurActor->IsPendingKill())
				{
					HoudiniCreatedSocketActors.RemoveAt(Idx);
					continue;
				}

				bool bFoundSocket = false;
				for (auto & CurSocket : StaticMesh->Sockets)
				{
					if (CurSocket->SocketName == CurActor->GetAttachParentSocketName())
					{
						bFoundSocket = true;
						break;
					}
				}
				// cur actor's attaching socket is found, skip
				if (bFoundSocket)
					continue;

				// Destroy the previous created socket actor if not found
				HoudiniCreatedSocketActors.RemoveAt(Idx);
				CurActor->Destroy();
			}
		}

		// Detach the in level actors which is not attached to any socket now
		{
			for (int32 Idx = HoudiniAttachedSocketActors.Num() - 1; Idx >= 0; --Idx) 
			{
				AActor* CurActor = HoudiniAttachedSocketActors[Idx];
				if (!CurActor || CurActor->IsPendingKill()) 
				{
					HoudiniAttachedSocketActors.RemoveAt(Idx);
					continue;
				}

				bool bFoundSocket = false;
				for (auto & CurSocket : StaticMesh->Sockets)
				{
					if (CurSocket->SocketName == CurActor->GetAttachParentSocketName())
					{
						bFoundSocket = true;
						break;
					}
				}

				if (bFoundSocket)
					continue;

				// If the attached socket name is not found in current socket, detach it and remove from the array
				CurActor->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
				HoudiniAttachedSocketActors.RemoveAt(Idx);			
			}
		}

	}

	if (bInApplyGenericProperties)
	{
		// Clear the component tags as generic properties only add them
		InMeshComponent->ComponentTags.Empty();
		// Update the property attributes on the component
		TArray<FHoudiniGenericAttribute> PropertyAttributes;
		if (GetGenericPropertiesAttributes(
			InOutputIdentifier.GeoId, InOutputIdentifier.PartId,
			InOutputIdentifier.PointIndex, InOutputIdentifier.PrimitiveIndex,
			PropertyAttributes))
		{
			UpdateGenericPropertiesAttributes(InMeshComponent, PropertyAttributes);
		}
	}
}

bool
FHoudiniMeshTranslator::CreateStaticMeshFromHoudiniGeoPartObject(
	const FHoudiniGeoPartObject& InHGPO,
	const FHoudiniPackageParams& InPackageParams,
	const TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject>& InOutputObjects,
	TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject>& OutOutputObjects,
	TMap<FString, UMaterialInterface*>& AssignmentMaterialMap,
	TMap<FString, UMaterialInterface*>& ReplacementMaterialMap,
	const bool& InForceRebuild,
	const EHoudiniStaticMeshMethod& InStaticMeshMethod,
	const FHoudiniStaticMeshGenerationProperties& InSMGenerationProperties,
	bool bInTreatExistingMaterialsAsUpToDate)
{
	// If we're not forcing the rebuild
	// No need to recreate something that hasn't changed
	if (!InForceRebuild && (!InHGPO.bHasGeoChanged || !InHGPO.bHasPartChanged) && InOutputObjects.Num() > 0)
	{
		// Simply reuse the existing meshes
		OutOutputObjects = InOutputObjects;
		return true;
	}
	
	FHoudiniMeshTranslator CurrentTranslator;
	CurrentTranslator.ForceRebuild = InForceRebuild;
	CurrentTranslator.SetHoudiniGeoPartObject(InHGPO);
	CurrentTranslator.SetInputObjects(InOutputObjects);
	CurrentTranslator.SetOutputObjects(OutOutputObjects);
	CurrentTranslator.SetInputAssignmentMaterials(AssignmentMaterialMap);
	CurrentTranslator.SetReplacementMaterials(ReplacementMaterialMap);
	CurrentTranslator.SetPackageParams(InPackageParams, true);
	CurrentTranslator.SetTreatExistingMaterialsAsUpToDate(bInTreatExistingMaterialsAsUpToDate);
	CurrentTranslator.SetStaticMeshGenerationProperties(InSMGenerationProperties);

	// TODO: Fetch from settings/HAC
	CurrentTranslator.DefaultMeshSmoothing = 1;
	if (false)
		CurrentTranslator.DefaultMeshSmoothing = 0;

	// TODO: mechanism to determine when to use dynamic mesh for fast updates, and when to switch to
	// baking the full static mesh
	switch (InStaticMeshMethod)
	{
		case EHoudiniStaticMeshMethod::RawMesh:
			CurrentTranslator.CreateStaticMesh_RawMesh();
			break;
		case EHoudiniStaticMeshMethod::FMeshDescription:
			CurrentTranslator.CreateStaticMesh_MeshDescription();
			break;
		case EHoudiniStaticMeshMethod::UHoudiniStaticMesh:
			CurrentTranslator.CreateHoudiniStaticMesh();
			break;
	}

	// Copy the output objects/materials
	OutOutputObjects = CurrentTranslator.OutputObjects;
	AssignmentMaterialMap = CurrentTranslator.OutputAssignmentMaterials;

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartVertexList()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartVertexList"));

	if (HGPO.PartInfo.VertexCount <= 0)
		return false;

	// Get the vertex List
	PartVertexList.SetNumUninitialized(HGPO.PartInfo.VertexCount);

	if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetVertexList(
		FHoudiniEngine::Get().GetSession(),
		HGPO.GeoId, HGPO.PartId, &PartVertexList[0], 0, HGPO.PartInfo.VertexCount))
	{
		// Error getting the vertex list.
		HOUDINI_LOG_MESSAGE(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s] unable to retrieve vertex list - skipping."),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);

		return false;
	}

	return true;
}

void
FHoudiniMeshTranslator::SortSplitGroups()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::SortSplitGroups"));

	// Sort the splits in the order that we want to process them:
	// Simple/Convex invisible colliders should be treated first as they will need to be attached to the visible meshes
	TArray<FString> First;
	
	// The main geo and its LODs should be created after.
	TArray<FString> Main;
	TArray<FString> LODs;

	// Finally, visible colliders and invisible complex colliders as they need their own static mesh
	TArray<FString> Last;

	for (auto& curSplit : HGPO.SplitGroups)
	{
		EHoudiniSplitType curSplitType = GetSplitTypeFromSplitName(curSplit);
		switch (curSplitType)
		{
			case EHoudiniSplitType::InvisibleSimpleCollider:
			case EHoudiniSplitType::InvisibleUCXCollider:
				First.Add(curSplit);
				break;

			case EHoudiniSplitType::Normal:
				Main.Add(curSplit);
				break;

			case EHoudiniSplitType::LOD:
				LODs.Add(curSplit);
				break;

			case EHoudiniSplitType::RenderedSimpleCollider:
			case EHoudiniSplitType::RenderedUCXCollider:
			case EHoudiniSplitType::RenderedComplexCollider:
			case EHoudiniSplitType::InvisibleComplexCollider:
				Last.Add(curSplit);
				break;
		}
	}

	// Make sure LODs are order by name
	LODs.Sort();

	// Copy the split names in order
	AllSplitGroups.Empty();
	for (auto& splitName : First)
		AllSplitGroups.Add(splitName);

	for (auto& splitName : Main)
		AllSplitGroups.Add(splitName);

	for (auto& splitName : LODs)
		AllSplitGroups.Add(splitName);

	for (auto& splitName : Last)
		AllSplitGroups.Add(splitName);
}

bool
FHoudiniMeshTranslator::UpdateSplitsFacesAndIndices()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdateSplitsFacesAndIndices"));

	// Reset the splits faces/indices arrays
	AllSplitVertexLists.Empty();
	AllSplitVertexCounts.Empty();
	AllSplitFaceIndices.Empty();
	AllSplitFirstValidVertexIndex.Empty();
	AllSplitFirstValidPrimIndex.Empty();

	bool bHasSplit = AllSplitGroups.Num() > 0;
	if (bHasSplit)
	{
		HAPI_PartInfo PartInfo = FHoudiniEngineUtils::ToHAPIPartInfo(HGPO.PartInfo);

		// Buffer for all vertex indices used for split groups.
		// We need this to figure out all vertex indices that are not part of them. 
		TArray<int32> AllVertexList;
		AllVertexList.SetNumZeroed(PartVertexList.Num());

		// Buffer for all face indices used for split groups.
		// We need this to figure out all face indices that are not part of them.
		TArray<int32> AllGroupFaceIndices;
		AllGroupFaceIndices.SetNumZeroed(HGPO.PartInfo.FaceCount);

		// Some of the groups may contain invalid geometry 
		// Store them here so we can remove them afterwards
		TArray<int32> InvalidGroupNameIndices;

		// Extract the vertices/faces for each of the split groups
		for (int32 SplitIdx = 0; SplitIdx < AllSplitGroups.Num(); SplitIdx++)
		{
			const FString& GroupName = AllSplitGroups[SplitIdx];

			// New vertex list just for this group.
			TArray< int32 > GroupVertexList;
			TArray< int32 > AllFaceList;
			
			int32 FirstValidPrimIndex = 0;
			int32 FirstValidVertexIndex = 0;
			// Extract vertex indices for this split.
			int32 GroupVertexListCount = FHoudiniEngineUtils::HapiGetVertexListForGroup(
				HGPO.GeoId, PartInfo, GroupName,
				PartVertexList, GroupVertexList,
				AllVertexList, AllFaceList, AllGroupFaceIndices,
				FirstValidVertexIndex, FirstValidPrimIndex,
				HGPO.PartInfo.bIsInstanced);

			if (GroupVertexListCount <= 0)
			{
				// This group doesn't have vertices/faces, mark it as invalid
				InvalidGroupNameIndices.Add(SplitIdx);

				// Error getting the vertex list.
				HOUDINI_LOG_MESSAGE(
					TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s] unable to retrieve vertex list for group %s - skipping."),
					HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, *GroupName);

				continue;
			}

			// If list is not empty, we store it for this group - this will define new mesh.
			AllSplitVertexLists.Add(GroupName, GroupVertexList);
			AllSplitVertexCounts.Add(GroupName, GroupVertexListCount);
			AllSplitFaceIndices.Add(GroupName, AllFaceList);
			AllSplitFirstValidVertexIndex.Add(GroupName, FirstValidVertexIndex);
			AllSplitFirstValidPrimIndex.Add(GroupName, FirstValidPrimIndex);
		}

		if (InvalidGroupNameIndices.Num() > 0)
		{
			// Remove all invalid split groups
			for (int32 InvalIdx = InvalidGroupNameIndices.Num() - 1; InvalIdx >= 0; InvalIdx--)
			{
				int32 Index = InvalidGroupNameIndices[InvalIdx];
				AllSplitGroups.RemoveAt(Index);
			}
		}

		// We also need to figure out / construct the vertex list for everything that's not in a split group
		TArray<int32> GroupSplitFacesRemaining;
		GroupSplitFacesRemaining.Init(-1, PartVertexList.Num());

		int32 GroupVertexListCount = 0;
		bool bHasMainSplitGroup = false;
		TArray< int32 > GroupSplitFaceIndicesRemaining;
		int32 FistUnusedVertexIndex = -1;		
		for (int32 SplitVertexIdx = 0; SplitVertexIdx < AllVertexList.Num(); SplitVertexIdx++)
		{
			if (AllVertexList[SplitVertexIdx] == 0)
			{
				// This is an unused index, we need to add it to unused vertex list.
				FistUnusedVertexIndex = SplitVertexIdx;
				GroupSplitFacesRemaining[SplitVertexIdx] = PartVertexList[SplitVertexIdx];
				bHasMainSplitGroup = true;
				GroupVertexListCount++;
			}
		}

		int32 FistUnusedPrimIndex = -1;
		for (int32 SplitFaceIdx = 0; SplitFaceIdx < AllGroupFaceIndices.Num(); SplitFaceIdx++)
		{
			if (AllGroupFaceIndices[SplitFaceIdx] == 0)
			{
				// This is unused face, we need to add it to unused faces list.
				GroupSplitFaceIndicesRemaining.Add(SplitFaceIdx);
				FistUnusedPrimIndex = SplitFaceIdx;
			}
		}

		// We store the remaining geo vertex list as a special split named "main geo"
		// and make sure its treated before the collider meshes
		if (bHasMainSplitGroup)
		{
			static const FString RemainingGroupName = HAPI_UNREAL_GROUP_GEOMETRY_NOT_COLLISION;
			AllSplitGroups.Add(RemainingGroupName);
			AllSplitVertexLists.Add(RemainingGroupName, GroupSplitFacesRemaining);
			AllSplitVertexCounts.Add(RemainingGroupName, GroupVertexListCount);
			AllSplitFaceIndices.Add(RemainingGroupName, GroupSplitFaceIndicesRemaining);
			AllSplitFirstValidPrimIndex.Add(RemainingGroupName, FistUnusedPrimIndex);
			AllSplitFirstValidVertexIndex.Add(RemainingGroupName, FistUnusedVertexIndex);
		}
	}
	else
	{
		// No splitting required
		// Mark everything as the main geo group
		static const FString RemainingGroupName = HAPI_UNREAL_GROUP_GEOMETRY_NOT_COLLISION;
		AllSplitGroups.Add(RemainingGroupName);
		AllSplitVertexLists.Add(RemainingGroupName, PartVertexList);
		AllSplitVertexCounts.Add(RemainingGroupName, PartVertexList.Num());
		AllSplitFirstValidPrimIndex.Add(RemainingGroupName, 0);
		AllSplitFirstValidVertexIndex.Add(RemainingGroupName, 0);

		TArray<int32> AllFaces;
		for (int32 FaceIdx = 0; FaceIdx < HGPO.PartInfo.FaceCount; ++FaceIdx)
			AllFaces.Add(FaceIdx);

		AllSplitFaceIndices.Add(RemainingGroupName, AllFaces);
	}

	return true;
}

void
FHoudiniMeshTranslator::ResetPartCache()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::ResetPartCache"));

	// Vertex Positions
	PartPositions.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoPositions);

	// Vertex Normals
	PartNormals.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoNormals);

	// Vertex TangentU
	PartTangentU.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoTangentU);

	// Vertex TangentV
	PartTangentV.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoTangentV);

	// Vertex Colors
	PartColors.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoColors);

	// Vertex Alpha values
	PartAlphas.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoAlpha);

	// FaceSmoothing values
	PartFaceSmoothingMasks.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoFaceSmoothingMasks);

	// UVs
	PartUVSets.Empty();
	AttribInfoUVSets.Empty();

	// UVs
	PartLightMapResolutions.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoLightmapResolution);

	// Material IDs per face
	PartFaceMaterialIds.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoFaceMaterialIds);
	// Unique material IDs
	PartUniqueMaterialIds.Empty();
	// Material infos for each unique Material
	PartUniqueMaterialInfos.Empty();
	//
	bOnlyOneFaceMaterial = false;

	// Face Materials override
	PartFaceMaterialOverrides.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoFaceMaterialOverrides);
	bMaterialOverrideNeedsCreateInstance = false;

	// LOD Screensize
	PartLODScreensize.Empty();
	FHoudiniApi::AttributeInfo_Init(&AttribInfoLODScreensize);
}

bool
FHoudiniMeshTranslator::UpdatePartPositionIfNeeded()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartPositionIfNeeded"));

	// Only Retrieve the vertices positions if necessary
	if (PartPositions.Num() > 0)
		return true;

	if (!FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
		HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
		HAPI_UNREAL_ATTRIB_POSITION, AttribInfoPositions, PartPositions))
	{
		// Error retrieving positions.
		HOUDINI_LOG_WARNING(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve position data")
			TEXT("- skipping."),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		return false;
	}
	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartNormalsIfNeeded()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartNormalsIfNeeded"));

	// No need to read the normals if we want unreal to recompute them after
	const UHoudiniRuntimeSettings* HoudiniRuntimeSettings = GetDefault<UHoudiniRuntimeSettings>();
	bool bReadNormals = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->RecomputeNormalsFlag != EHoudiniRuntimeSettingsRecomputeFlag::HRSRF_Always : true;
	if (!bReadNormals)
		return true;

	// Only Retrieve the normals if we haven't already
	if (PartNormals.Num() > 0)
		return true;

	// Retrieve normal data for this part
	bool Success = FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
		HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
		HAPI_UNREAL_ATTRIB_NORMAL, AttribInfoNormals, PartNormals);

	// There is no normals to fetch
	if (!AttribInfoNormals.exists)
		return true;

	if  (!Success && AttribInfoNormals.exists)
	{
		// Error retrieving normals.
		HOUDINI_LOG_WARNING(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve normal data"),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		return false;
	}

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartTangentsIfNeeded()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartTangentsIfNeeded"))

	bool bReturn = true;
	if (PartTangentU.Num() <= 0)
	{
		// Retrieve TangentU data for this part
		bool Success = FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
			HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
			HAPI_UNREAL_ATTRIB_TANGENTU, AttribInfoTangentU, PartTangentU);
		
		if (!Success && AttribInfoTangentU.exists)
		{
			// Error retrieving tangent.
			HOUDINI_LOG_WARNING(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve tangentU data"),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
			bReturn = false;
		}
	}

	if (PartTangentV.Num() <= 0)
	{
		// Retrieve TangentV data for this part
		bool Success = FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
			HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
			HAPI_UNREAL_ATTRIB_TANGENTV, AttribInfoTangentV, PartTangentV);

		if (!Success && AttribInfoTangentV.exists)
		{
			// Error retrieving tangent.
			HOUDINI_LOG_WARNING(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve tangentV data"),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
			bReturn = false;
		}
	}

	return bReturn;
}

bool
FHoudiniMeshTranslator::UpdatePartColorsIfNeeded()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartColorsIfNeeded"));

	// Only Retrieve the vertices colors if necessary
	if (PartColors.Num() > 0)
		return true;

	bool Success = FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
		HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
		HAPI_UNREAL_ATTRIB_COLOR, AttribInfoColors, PartColors);

	if (!Success && AttribInfoColors.exists)
	{
		// Error retrieving colors.
		HOUDINI_LOG_WARNING(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve color data"),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		return false;
	}

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartAlphasIfNeeded()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartAlphasIfNeeded"));

	// Only Retrieve the vertices alphas if necessary
	if (PartAlphas.Num() > 0)
		return true;

	bool Success = FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
		HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
		HAPI_UNREAL_ATTRIB_ALPHA, AttribInfoAlpha, PartAlphas);

	if (!Success && AttribInfoAlpha.exists)
	{
		// Error retrieving alpha values.
		HOUDINI_LOG_WARNING(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve alpha data"),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		return false;
	}

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartFaceSmoothingIfNeeded()
{
	// Only Retrieve the vertices FaceSmoothing if necessary
	if (PartFaceSmoothingMasks.Num() > 0)
		return true;

	bool Success = FHoudiniEngineUtils::HapiGetAttributeDataAsInteger(
		HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
		HAPI_UNREAL_ATTRIB_FACE_SMOOTHING_MASK,
		AttribInfoFaceSmoothingMasks, PartFaceSmoothingMasks);

	if (!Success && AttribInfoFaceSmoothingMasks.exists)
	{
		// Error retrieving FaceSmoothing values.
		HOUDINI_LOG_WARNING(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve FaceSmoothing data"),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		return false;
	}

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartUVSetsIfNeeded(const bool& bRemoveUnused)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartUVSetsIfNeeded"));

	// Only Retrieve uvs if necessary
	if (PartUVSets.Num() > 0)
		return true;

	PartUVSets.SetNum(MAX_STATIC_TEXCOORDS);
	AttribInfoUVSets.SetNum(MAX_STATIC_TEXCOORDS);

	// The second UV set should be called uv2, but we will still check if need to look for a uv1 set.
	// If uv1 exists, we'll look for uv, uv1, uv2 etc.. if not we'll look for uv, uv2, uv3 etc..
	bool bUV1Exists = FHoudiniEngineUtils::HapiCheckAttributeExists(HGPO.GeoId, HGPO.PartId, "uv1");

	// Retrieve UVs.
	for (int32 TexCoordIdx = 0; TexCoordIdx < MAX_STATIC_TEXCOORDS; ++TexCoordIdx)
	{
		FString UVAttributeName = HAPI_UNREAL_ATTRIB_UV;
		if (TexCoordIdx > 0)
			UVAttributeName += FString::Printf(TEXT("%d"), bUV1Exists ? TexCoordIdx : TexCoordIdx + 1);

		FHoudiniApi::AttributeInfo_Init(&AttribInfoUVSets[TexCoordIdx]);
		FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
			HGPO.GeoId, HGPO.PartId, TCHAR_TO_ANSI(*UVAttributeName),
			AttribInfoUVSets[TexCoordIdx], PartUVSets[TexCoordIdx], 2);
	}

	// Also look for 16.5 uvs (attributes with a Texture type) 
	// For that, we'll have to iterate through ALL the attributes and check their types
	TArray< FString > FoundAttributeNames; 
	TArray< HAPI_AttributeInfo > FoundAttributeInfos;
		
	for (int32 AttrIdx = 0; AttrIdx < HAPI_ATTROWNER_MAX; ++AttrIdx)
	{
		FHoudiniEngineUtils::HapiGetAttributeOfType(
			HGPO.GeoId, HGPO.PartId, (HAPI_AttributeOwner)AttrIdx, 
			HAPI_ATTRIBUTE_TYPE_TEXTURE, FoundAttributeInfos, FoundAttributeNames);
	}

	if (FoundAttributeInfos.Num() <= 0)
		return true;

	// We found some additionnal uv attributes
	int32 AvailableIdx = 0;
	for (int32 attrIdx = 0; attrIdx < FoundAttributeInfos.Num(); attrIdx++)
	{
		// Ignore the old uvs
		if (FoundAttributeNames[attrIdx] == TEXT("uv")
			|| FoundAttributeNames[attrIdx] == TEXT("uv1")
			|| FoundAttributeNames[attrIdx] == TEXT("uv2")
			|| FoundAttributeNames[attrIdx] == TEXT("uv3")
			|| FoundAttributeNames[attrIdx] == TEXT("uv4")
			|| FoundAttributeNames[attrIdx] == TEXT("uv5")
			|| FoundAttributeNames[attrIdx] == TEXT("uv6")
			|| FoundAttributeNames[attrIdx] == TEXT("uv7")
			|| FoundAttributeNames[attrIdx] == TEXT("uv8"))
			continue;

		HAPI_AttributeInfo CurrentAttrInfo = FoundAttributeInfos[attrIdx];
		if (!CurrentAttrInfo.exists)
			continue;

		// Look for the next available index in the return arrays
		for (; AvailableIdx < AttribInfoUVSets.Num(); AvailableIdx++)
		{
			if (!AttribInfoUVSets[AvailableIdx].exists)
				break;
		}

		// We are limited to MAX_STATIC_TEXCOORDS uv sets!
		// If we already have too many uv sets, skip the rest
		if ((AvailableIdx >= MAX_STATIC_TEXCOORDS) || (AvailableIdx >= AttribInfoUVSets.Num()))
		{
			HOUDINI_LOG_WARNING(TEXT("Too many UV sets found. Unreal only supports %d , skipping the remaining uv sets."), (int32)MAX_STATIC_TEXCOORDS);
			break;
		}

		// Force the tuple size to 2 ?
		CurrentAttrInfo.tupleSize = 2;

		// Add the attribute infos we found
		AttribInfoUVSets[AvailableIdx] = CurrentAttrInfo;

		// Allocate sufficient buffer for the attribute's data.
		PartUVSets[AvailableIdx].SetNumUninitialized(CurrentAttrInfo.count * CurrentAttrInfo.tupleSize);

		// Get the texture coordinates
		if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetAttributeFloatData(
			FHoudiniEngine::Get().GetSession(),
			HGPO.GeoId, HGPO.PartId, TCHAR_TO_UTF8(*(FoundAttributeNames[attrIdx])),
			&AttribInfoUVSets[AvailableIdx], -1,
			&PartUVSets[AvailableIdx][0], 0, CurrentAttrInfo.count))
		{
			// Something went wrong when trying to access the uv values, invalidate this set
			AttribInfoUVSets[AvailableIdx].exists = false;
		}
	}

	// Remove unused UV sets
	if (bRemoveUnused)
	{
		for (int32 Idx = PartUVSets.Num() - 1; Idx >= 0; Idx--)
		{
			if (PartUVSets[Idx].Num() > 0)
				continue;

			PartUVSets.RemoveAt(Idx);
		}
	}

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartLightmapResolutionsIfNeeded()
{
	// Only Retrieve the vertices lightmap resolution if necessary
	if (PartLightMapResolutions.Num() > 0)
		return true;

	// Get lightmap resolution (if present).
	bool Success = FHoudiniEngineUtils::HapiGetAttributeDataAsInteger(
		HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
		HAPI_UNREAL_ATTRIB_LIGHTMAP_RESOLUTION, 
		AttribInfoLightmapResolution, PartLightMapResolutions);

	if (!Success && AttribInfoLightmapResolution.exists)
	{
		// Error retrieving lightmap resolution values.
		HOUDINI_LOG_WARNING(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve lightmap resolution data"),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		return false;
	}

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartFaceMaterialIDsIfNeeded()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartFaceMaterialIDsIfNeeded"));

	// Only Retrieve the material IDs if necessary
	if (PartFaceMaterialIds.Num() > 0)
		return true;

	int32 NumFaces = HGPO.PartInfo.FaceCount;
	if (NumFaces <= 0)
		return true;

	PartFaceMaterialIds.SetNum(NumFaces);

	// Get the materials IDs per face
	HAPI_Bool bSingleFaceMaterial = false;
	if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetMaterialNodeIdsOnFaces(
		FHoudiniEngine::Get().GetSession(),
		HGPO.GeoId, HGPO.PartId, &bSingleFaceMaterial,
		&PartFaceMaterialIds[0], 0, NumFaces))
	{
		// Error retrieving material face assignments.
		HOUDINI_LOG_MESSAGE(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s] unable to retrieve material face assignments"),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		return false;
	}

	bOnlyOneFaceMaterial = bSingleFaceMaterial;

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartFaceMaterialOverridesIfNeeded()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartFaceMaterialOverridesIfNeeded"));

	// Only Retrieve the material overrides if necessary
	if (PartFaceMaterialOverrides.Num() > 0)
		return true;

	bMaterialOverrideNeedsCreateInstance = false;

	FHoudiniEngineUtils::HapiGetAttributeDataAsString(
		HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
		HAPI_UNREAL_ATTRIB_MATERIAL,
		AttribInfoFaceMaterialOverrides, PartFaceMaterialOverrides);

	// If material attribute was not found, check fallback compatibility attribute.
	if (!AttribInfoFaceMaterialOverrides.exists)
	{
		PartFaceMaterialOverrides.Empty();
		FHoudiniEngineUtils::HapiGetAttributeDataAsString(
			HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
			HAPI_UNREAL_ATTRIB_MATERIAL_FALLBACK,
			AttribInfoFaceMaterialOverrides, PartFaceMaterialOverrides);
	}

	// If material attribute and fallbacks were not found, check the material instance attribute.
	if (!AttribInfoFaceMaterialOverrides.exists)
	{
		PartFaceMaterialOverrides.Empty();
		FHoudiniEngineUtils::HapiGetAttributeDataAsString(
			HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
			HAPI_UNREAL_ATTRIB_MATERIAL_INSTANCE,
			AttribInfoFaceMaterialOverrides, PartFaceMaterialOverrides);
		
		// We will we need to create material instances from the override attributes
		bMaterialOverrideNeedsCreateInstance = AttribInfoFaceMaterialOverrides.exists;
	}

	if (AttribInfoFaceMaterialOverrides.exists
		&& AttribInfoFaceMaterialOverrides.owner != HAPI_ATTROWNER_PRIM
		&& AttribInfoFaceMaterialOverrides.owner != HAPI_ATTROWNER_DETAIL)
	{
		HOUDINI_LOG_WARNING(TEXT("Static Mesh [%d %s], Geo [%d], Part [%d %s]: unreal_material must be a primitive or detail attribute, ignoring attribute."),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		AttribInfoFaceMaterialOverrides.exists = false;
		bMaterialOverrideNeedsCreateInstance = false;
		PartFaceMaterialOverrides.Empty();
		return false;
	}

	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartNeededMaterials()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartNeededMaterials"));

	// Update the per face material IDs
	UpdatePartFaceMaterialIDsIfNeeded();

	// See if we have some material overides
	UpdatePartFaceMaterialOverridesIfNeeded();

	// If we have houdini materials AND overrides:
	// We want to only create the Houdini materials that are not "covered" by overrides
	// If we have material instance attributes, create all the houdini material anyway
	// as their textures could be referenced by the material instance parameters
	if (PartFaceMaterialOverrides.Num() > 0 && !bMaterialOverrideNeedsCreateInstance)
	{
		// If the material override was set on the detail, no need to look for houdini material IDs, as only the override will be used
		if (AttribInfoFaceMaterialOverrides.exists && AttribInfoFaceMaterialOverrides.owner == HAPI_ATTROWNER_PRIM)
		{
			for (int32 MaterialIdx = 0; MaterialIdx < PartFaceMaterialIds.Num(); ++MaterialIdx)
			{
				// Add a material ID to the unique array only if that face is not using the override
				if (PartFaceMaterialOverrides[MaterialIdx].IsEmpty())
					PartUniqueMaterialIds.AddUnique(PartFaceMaterialIds[MaterialIdx]);
			}
		}
	}
	else
	{
		// No material overrides, simply update the unique material array
		for (int32 MaterialIdx = 0; MaterialIdx < PartFaceMaterialIds.Num(); ++MaterialIdx)
			PartUniqueMaterialIds.AddUnique(PartFaceMaterialIds[MaterialIdx]);
	}

	// Remove the invalid material ID from the unique array
	PartUniqueMaterialIds.RemoveSingle(-1);

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::UpdatePartNeededMaterials - Get the unique material infos"));
		// Get the unique material infos
		PartUniqueMaterialInfos.SetNum(PartUniqueMaterialIds.Num());
		for (int32 MaterialIdx = 0; MaterialIdx < PartUniqueMaterialIds.Num(); MaterialIdx++)
		{

			FHoudiniApi::MaterialInfo_Init(&PartUniqueMaterialInfos[MaterialIdx]);
			if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetMaterialInfo(
				FHoudiniEngine::Get().GetSession(),
				PartUniqueMaterialIds[MaterialIdx],
				&PartUniqueMaterialInfos[MaterialIdx]))
			{
				// Error retrieving material face assignments.
				HOUDINI_LOG_MESSAGE(
					TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s] unable to retrieve material info for material %d"),
					HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, PartUniqueMaterialIds[MaterialIdx]);
				continue;
			}
		}
	}
	return true;
}

bool
FHoudiniMeshTranslator::UpdatePartLODScreensizeIfNeeded()
{
	// Only retrieve LOD screensizes if necessary
	if (PartLODScreensize.Num() > 0)
		return true;

	bool Success = FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
		HGPO.GeoInfo.NodeId, HGPO.PartInfo.PartId,
		HAPI_UNREAL_ATTRIB_LOD_SCREENSIZE,
		AttribInfoLODScreensize, PartLODScreensize);

	if (!Success && AttribInfoLODScreensize.exists)
	{
		// Error retrieving FaceSmoothing values.
		HOUDINI_LOG_WARNING(
			TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], unable to retrieve LOD screensizes"),
			HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName);
		return false;
	}

	return true;
}


UStaticMesh*
FHoudiniMeshTranslator::CreateNewStaticMesh(const FString& InSplitIdentifier)
{
	// Update the current Obj/Geo/Part/Split IDs
	PackageParams.ObjectId = HGPO.ObjectId;
	PackageParams.GeoId = HGPO.GeoId;
	PackageParams.PartId = HGPO.PartId;
	PackageParams.SplitStr = InSplitIdentifier;

	UStaticMesh * NewStaticMesh = PackageParams.CreateObjectAndPackage<UStaticMesh>();
	if (!NewStaticMesh || NewStaticMesh->IsPendingKill())
		return nullptr;

	return NewStaticMesh;
}

UHoudiniStaticMesh*
FHoudiniMeshTranslator::CreateNewHoudiniStaticMesh(const FString& InSplitIdentifier)
{
	// Update the current Obj/Geo/Part/Split IDs
	PackageParams.ObjectId = HGPO.ObjectId;
	PackageParams.GeoId = HGPO.GeoId;
	PackageParams.PartId = HGPO.PartId;
	// Add _HSM suffix to the split str, to distinguish the temporary HoudiniStaticMesh
	// from the UStaticMesh
	PackageParams.SplitStr = InSplitIdentifier + "_HSM";

	UHoudiniStaticMesh * NewStaticMesh = PackageParams.CreateObjectAndPackage<UHoudiniStaticMesh>();
	if (!NewStaticMesh || NewStaticMesh->IsPendingKill())
		return nullptr;

	return NewStaticMesh;
}

bool
FHoudiniMeshTranslator::CreateStaticMesh_RawMesh()
{
	double time_start = FPlatformTime::Seconds();

	// Start by updating the vertex list
	if (!UpdatePartVertexList())
		return false;

	// Sort the split groups
	SortSplitGroups();

	// Handles the split groups found in the part
	// and builds the corresponding faces and indices arrays
	if (!UpdateSplitsFacesAndIndices())
		return true;

	// Resets the containers used for the raw data extraction.
	ResetPartCache();

	// Prepare the object that will store UCX and simple colliders
	AllAggregateCollisions.Empty();

	// We need to know the number of LODs that will be needed for this part
	int32 NumberOfLODs = 0;
	bool bHasMainGeo = false;
	for (auto& curSplit : AllSplitGroups)
	{
		if (GetSplitTypeFromSplitName(curSplit) == EHoudiniSplitType::LOD)
			NumberOfLODs++;
		else if (GetSplitTypeFromSplitName(curSplit) == EHoudiniSplitType::Normal)
			bHasMainGeo = true;
	}

	// Update the part's material's IDS and info now
	CreateNeededMaterials();

	// Check now if they were updated
	bool bMaterialHasChanged = false;
	for (const auto& MatInfo : PartUniqueMaterialInfos)
	{
		if (MatInfo.hasChanged)
		{
			bMaterialHasChanged = true;
			break;
		}
	}

	// Get the current target platform for default lod policies
	ITargetPlatform * CurrentPlatform = GetTargetPlatformManagerRef().GetRunningTargetPlatform();
	check(CurrentPlatform);

	// New mesh list
	TMap<FHoudiniOutputObjectIdentifier, UStaticMesh*> StaticMeshToBuild;

	// Map of Houdini Material IDs to Unreal Material Indices
	TMap<HAPI_NodeId, int32> MapHoudiniMatIdToUnrealIndex;
	// Map of Houdini Material Attributes to Unreal Material Indices
	TMap<FString, int32> MapHoudiniMatAttributesToUnrealIndex;

	bool MeshMaterialsHaveBeenReset = false;

	// Mesh Socket array
	TArray<FHoudiniMeshSocket> AllSockets;
	FHoudiniEngineUtils::AddMeshSocketsToArray_DetailAttribute(
		HGPO.GeoId, HGPO.PartId, AllSockets, HGPO.PartInfo.bIsInstanced);
	FHoudiniEngineUtils::AddMeshSocketsToArray_Group(
		HGPO.GeoId, HGPO.PartId, AllSockets, HGPO.PartInfo.bIsInstanced);

	// Iterate through all detected split groups we care about and split geometry.
	// The split are ordered in the following way:
	// Invisible Simple/Convex Colliders > LODs > MainGeo > Visible Colliders > Invisible Colliders
	for (int32 SplitId = 0; SplitId < AllSplitGroups.Num(); SplitId++)
	{
		// Get split group name
		const FString& SplitGroupName = AllSplitGroups[SplitId];

		// Get the vertex indices for this group
		TArray<int32>& SplitVertexList = AllSplitVertexLists[SplitGroupName];

		// Get valid count of vertex indices for this split.
		const int32& SplitVertexCount = AllSplitVertexCounts[SplitGroupName];

		// Make sure we have a  valid vertex count for this split
		if (SplitVertexCount % 3 != 0 || SplitVertexList.Num() % 3 != 0)
		{
			// Invalid vertex count, skip this split or we'd crash trying to create a mesh for it.
			HOUDINI_LOG_WARNING(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] invalid vertex count.")
				TEXT("- skipping."),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);

			continue;
		}

		// Get the current split type
		EHoudiniSplitType SplitType = GetSplitTypeFromSplitName(SplitGroupName);
		if (SplitType == EHoudiniSplitType::Invalid)
		{
			// Invalid split, skip
			HOUDINI_LOG_WARNING(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] unknown split type.")
				TEXT("- skipping."),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
			continue;
		}

		// Get the output identifer for this split
		FHoudiniOutputObjectIdentifier OutputObjectIdentifier(
			HGPO.ObjectId, HGPO.GeoId, HGPO.PartId, GetMeshIdentifierFromSplit(SplitGroupName, SplitType));
		OutputObjectIdentifier.PartName = HGPO.PartName;

		// Get/Create the Aggregate Collisions for this mesh identifier
		FKAggregateGeom& AggregateCollisions = AllAggregateCollisions.FindOrAdd(OutputObjectIdentifier);

		// Handle UCX / Convex Hull colliders
		if (SplitType == EHoudiniSplitType::InvisibleUCXCollider || SplitType == EHoudiniSplitType::RenderedUCXCollider)
		{
			// Get the part position if needed
			UpdatePartPositionIfNeeded();

			// Create the convex hull colliders and add them to the Aggregate
			if (!AddConvexCollisionToAggregate(SplitGroupName, AggregateCollisions))
			{
				// Failed to generate a convex collider
				HOUDINI_LOG_WARNING(
					TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] failed to create convex collider."),
					HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
			}

			// If the collider is not visible, stop here
			if (SplitType == EHoudiniSplitType::InvisibleUCXCollider)
				continue;
		}
		else if (SplitType == EHoudiniSplitType::InvisibleSimpleCollider || SplitType == EHoudiniSplitType::RenderedSimpleCollider)
		{
			// Get the part position if needed
			UpdatePartPositionIfNeeded();

			// Create the simple colliders and add them to the aggregate
			if (!AddSimpleCollisionToAggregate(SplitGroupName, AggregateCollisions))
			{
				// Failed to generate a convex collider
				HOUDINI_LOG_WARNING(
					TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] failed to create simple collider."),
					HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
			}

			// If the collider is not visible, stop here
			if (SplitType == EHoudiniSplitType::InvisibleSimpleCollider)
				continue;
		}

		// Try to find existing properties for this identifier
		FHoudiniOutputObject* FoundOutputObject = InputObjects.Find(OutputObjectIdentifier);
		// Try to find an existing SM from a previous cook
		UStaticMesh* FoundStaticMesh = FindExistingStaticMesh(OutputObjectIdentifier);

		// Flag whether or not we need to rebuild the mesh
		bool bRebuildStaticMesh = false;
		if (HGPO.GeoInfo.bHasGeoChanged || HGPO.PartInfo.bHasChanged || ForceRebuild || !FoundStaticMesh || !FoundOutputObject)
			bRebuildStaticMesh = true;

		// TODO: Handle materials
		if (!bRebuildStaticMesh && !bMaterialHasChanged)
		{
			// We can simply reuse the found static mesh
			OutputObjects.Add(OutputObjectIdentifier, *FoundOutputObject);
			continue;
		}

		// Prepare LOD Group data for this static mesh
		FStaticMeshLODGroup LODGroup;

		bool bNewStaticMeshCreated = false;
		if (!FoundStaticMesh)
		{
			// If we couldn't find a valid existing static mesh, create a new one
			FoundStaticMesh = CreateNewStaticMesh(OutputObjectIdentifier.SplitIdentifier);
			if (!FoundStaticMesh || FoundStaticMesh->IsPendingKill())
				continue;

			bNewStaticMeshCreated = true;

			// Use the platform's default LODGroup policy
			// TODO? Add setting for default LOD Group?
			LODGroup = CurrentPlatform->GetStaticMeshLODSettings().GetLODGroup(NAME_None);
		}
		else
		{
			// Try to reuse the existing SM's LOD group instead of the default one
			LODGroup = CurrentPlatform->GetStaticMeshLODSettings().GetLODGroup(FoundStaticMesh->LODGroup);
		}

		if (!FoundOutputObject)
		{
			FHoudiniOutputObject NewOutputObject;
			FoundOutputObject = &OutputObjects.Add(OutputObjectIdentifier, NewOutputObject);
			InputObjects.Remove(OutputObjectIdentifier);
		}
		else
		{
			// If this is not a new output object we have to clear the CachedAttributes and CachedTokens before
			// setting the new values (so that we do not re-use any values from the previous cook)
			FoundOutputObject->CachedAttributes.Empty();
			FoundOutputObject->CachedTokens.Empty();
		}
		FoundOutputObject->bProxyIsCurrent = false;

		// TODO: Needed?
		// Free any RHI resources for existing mesh before we re-create in place.
		FoundStaticMesh->PreEditChange(NULL);

		// Check that the Static Mesh we found has the appropriate number of Source models/LODs		
		int32 NeededNumberOfLODs = FMath::Max(NumberOfLODs + (bHasMainGeo ? 1 : 0), LODGroup.GetDefaultNumLODs());

		// LODs are only for the "main" mesh, not for complex colliders!
		if (SplitType == EHoudiniSplitType::InvisibleComplexCollider || SplitType == EHoudiniSplitType::RenderedComplexCollider)
			NeededNumberOfLODs = FMath::Max(1, LODGroup.GetDefaultNumLODs());

		if (FoundStaticMesh->GetNumSourceModels() != NeededNumberOfLODs)
		{
			while (FoundStaticMesh->GetNumSourceModels() < NeededNumberOfLODs)
				FoundStaticMesh->AddSourceModel();

			// We may have to remove excessive LOD levels
			if (FoundStaticMesh->GetNumSourceModels() > NeededNumberOfLODs)
				FoundStaticMesh->SetNumSourceModels(NeededNumberOfLODs);

			// Initialize their default reduction setting
			for (int32 ModelLODIndex = 0; ModelLODIndex < NeededNumberOfLODs; ModelLODIndex++)
			{
				FoundStaticMesh->GetSourceModel(ModelLODIndex).ReductionSettings = LODGroup.GetDefaultSettings(ModelLODIndex);
			}
			FoundStaticMesh->LightMapResolution = LODGroup.GetDefaultLightMapResolution();
		}

		// By default, always work on the first source model, unless we're a LOD
		int32 SrcModelIndex = 0;
		int32 LODIndex = 0;
		if (SplitType == EHoudiniSplitType::LOD)
		{
			for (auto& curSplit : AllSplitGroups)
			{
				EHoudiniSplitType CurrentSplitType = GetSplitTypeFromSplitName(curSplit);
				if (CurrentSplitType == EHoudiniSplitType::LOD
					|| CurrentSplitType == EHoudiniSplitType::Normal)
				{
					LODIndex++;
				}

				if (curSplit == SplitGroupName)
					break;
			}

			// Fix for the case where we don't have a main geo
			if(!bHasMainGeo)
				LODIndex--;
		}

		// Grab the appropriate SourceModel
		FStaticMeshSourceModel* SrcModel = (FoundStaticMesh->IsSourceModelValid(LODIndex)) ? &(FoundStaticMesh->GetSourceModel(LODIndex)) : nullptr;
		if (!SrcModel)
		{
			HOUDINI_LOG_ERROR(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d, %s] Could not access SourceModel for the LOD %d - skipping."),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName, LODIndex);
			continue;
		}

		// Load existing raw model. This will be empty as we are constructing a new mesh.
		FRawMesh RawMesh;
		if (!bRebuildStaticMesh)
		{
			// We dont need to rebuild the mesh itself:
			// the geometry hasn't changed, but the materials have.
			// We can just load the old data into the Raw mesh and reuse it.
			SrcModel->LoadRawMesh(RawMesh);
		}
		else
		{
			//--------------------------------------------------------------------------------------------------------------------- 
			// NORMALS 
			//--------------------------------------------------------------------------------------------------------------------- 

			// Extract this part's normal if needed
			UpdatePartNormalsIfNeeded();

			// Get the normals for this split
			TArray<float> SplitNormals;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoNormals, PartNormals, SplitNormals);

			// Check that the number of normal we retrieved is correct
			int32 WedgeNormalCount = SplitNormals.Num() / 3;
			if (SplitNormals.Num() < 0 || !SplitNormals.IsValidIndex((WedgeNormalCount - 1) * 3 + 2))
			{
				// Ignore normals
				WedgeNormalCount = 0;
				HOUDINI_LOG_WARNING(TEXT("Invalid normal count detected - Skipping normals."));
			}

			// Transfer the normals to the raw mesh 
			RawMesh.WedgeTangentZ.SetNumZeroed(WedgeNormalCount);
			for (int32 WedgeTangentZIdx = 0; WedgeTangentZIdx < WedgeNormalCount; ++WedgeTangentZIdx)
			{
				// Swap Y/Z for Coordinates conversion
				RawMesh.WedgeTangentZ[WedgeTangentZIdx].X = SplitNormals[WedgeTangentZIdx * 3 + 0];
				RawMesh.WedgeTangentZ[WedgeTangentZIdx].Y = SplitNormals[WedgeTangentZIdx * 3 + 2];
				RawMesh.WedgeTangentZ[WedgeTangentZIdx].Z = SplitNormals[WedgeTangentZIdx * 3 + 1];
			}


			//--------------------------------------------------------------------------------------------------------------------- 
			// TANGENTS
			//--------------------------------------------------------------------------------------------------------------------- 

			// No need to read the tangents if we want unreal to recompute them after					
			const UHoudiniRuntimeSettings* HoudiniRuntimeSettings = GetDefault<UHoudiniRuntimeSettings>();
			bool bReadTangents = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->RecomputeTangentsFlag != EHoudiniRuntimeSettingsRecomputeFlag::HRSRF_Always : true;
			if (bReadTangents)
			{
				// Extract this part's Tangents if needed
				UpdatePartTangentsIfNeeded();

				// Get the Tangents for this split
				TArray< float > SplitTangentU;
				FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
					SplitVertexList, AttribInfoTangentU, PartTangentU, SplitTangentU);

				// Get the binormals for this split
				TArray< float > SplitTangentV;
				FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
					SplitVertexList, AttribInfoTangentV, PartTangentV, SplitTangentV);

				// We need to manually generate tangents if:
				// - we have normals but dont have tangentu or tangentv attributes
				// - we have not specified that we wanted unreal to generate them
				bool bGenerateTangents = (SplitNormals.Num() > 0) && (SplitTangentU.Num() <= 0 || SplitTangentV.Num() <= 0);

				// Check that the number of tangents read matches the number of normals
				int32 WedgeTangentUCount = SplitTangentU.Num() / 3;
				int32 WedgeTangentVCount = SplitTangentV.Num() / 3;
				if (WedgeTangentUCount != WedgeNormalCount || WedgeTangentVCount != WedgeNormalCount)
					bGenerateTangents = true;

				if (bGenerateTangents && (HoudiniRuntimeSettings->RecomputeTangentsFlag == EHoudiniRuntimeSettingsRecomputeFlag::HRSRF_Always))
				{
					// No need to generate tangents if we want unreal to recompute them after
					bGenerateTangents = false;
				}

				// Generate the tangents if needed
				if (bGenerateTangents)
				{
					RawMesh.WedgeTangentX.SetNumZeroed(WedgeNormalCount);
					RawMesh.WedgeTangentY.SetNumZeroed(WedgeNormalCount);
					for (int32 WedgeTangentZIdx = 0; WedgeTangentZIdx < WedgeNormalCount; ++WedgeTangentZIdx)
					{
						FVector TangentX, TangentY;
						RawMesh.WedgeTangentZ[WedgeTangentZIdx].FindBestAxisVectors(TangentX, TangentY);

						RawMesh.WedgeTangentX[WedgeTangentZIdx] = TangentX;
						RawMesh.WedgeTangentY[WedgeTangentZIdx] = TangentY;
					}
				}
				else
				{
					// Transfer the tangents we have read them and they're valid
					RawMesh.WedgeTangentX.SetNumZeroed(WedgeTangentUCount);
					for (int32 WedgeTangentUIdx = 0; WedgeTangentUIdx < WedgeTangentUCount; ++WedgeTangentUIdx)
					{
						// We need to flip Z and Y
						RawMesh.WedgeTangentX[WedgeTangentUIdx].X = SplitTangentU[WedgeTangentUIdx * 3 + 0];
						RawMesh.WedgeTangentX[WedgeTangentUIdx].Y = SplitTangentU[WedgeTangentUIdx * 3 + 2];
						RawMesh.WedgeTangentX[WedgeTangentUIdx].Z = SplitTangentU[WedgeTangentUIdx * 3 + 1];
					}

					RawMesh.WedgeTangentY.SetNumZeroed(WedgeTangentVCount);
					for (int32 WedgeTangentVIdx = 0; WedgeTangentVIdx < WedgeTangentVCount; ++WedgeTangentVIdx)
					{
						// We need to flip Z and Y
						RawMesh.WedgeTangentY[WedgeTangentVIdx].X = SplitTangentV[WedgeTangentVIdx * 3 + 0];
						RawMesh.WedgeTangentY[WedgeTangentVIdx].Y = SplitTangentV[WedgeTangentVIdx * 3 + 2];
						RawMesh.WedgeTangentY[WedgeTangentVIdx].Z = SplitTangentV[WedgeTangentVIdx * 3 + 1];
					}
				}
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			//  VERTEX COLORS AND ALPHAS
			//---------------------------------------------------------------------------------------------------------------------

			// Extract this part's colors if needed
			UpdatePartColorsIfNeeded();

			// Get the colors values for this split
			TArray<float> SplitColors;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoColors, PartColors, SplitColors);

			// Extract this part's alpha values if needed
			UpdatePartAlphasIfNeeded();

			// Get the colors values for this split
			TArray<float> SplitAlphas;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoAlpha, PartAlphas, SplitAlphas);

			// Transfer colors and alphas if possible
			int32 WedgeColorsCount = AttribInfoColors.exists ? SplitColors.Num() / AttribInfoColors.tupleSize : 0;
			bool bSplitColorValid = AttribInfoColors.exists && (AttribInfoColors.tupleSize >= 3) && WedgeColorsCount > 0;
			bool bSplitAlphaValid = AttribInfoAlpha.exists && (SplitAlphas.Num() == WedgeColorsCount);
			if (bSplitColorValid)
			{
				RawMesh.WedgeColors.SetNumZeroed(WedgeColorsCount);
				for (int32 WedgeColorIdx = 0; WedgeColorIdx < WedgeColorsCount; WedgeColorIdx++)
				{
					FLinearColor WedgeColor;
					WedgeColor.R = FMath::Clamp(
						SplitColors[WedgeColorIdx * AttribInfoColors.tupleSize + 0], 0.0f, 1.0f);
					WedgeColor.G = FMath::Clamp(
						SplitColors[WedgeColorIdx * AttribInfoColors.tupleSize + 1], 0.0f, 1.0f);
					WedgeColor.B = FMath::Clamp(
						SplitColors[WedgeColorIdx * AttribInfoColors.tupleSize + 2], 0.0f, 1.0f);

					if (bSplitAlphaValid)
					{
						// Use the Alpha attribute value
						WedgeColor.A = FMath::Clamp(SplitAlphas[WedgeColorIdx], 0.0f, 1.0f);
					}
					else if (AttribInfoColors.tupleSize >= 4)
					{
						// Use the alpha value from the color attribute
						WedgeColor.A = FMath::Clamp(
							SplitColors[WedgeColorIdx * AttribInfoColors.tupleSize + 3], 0.0f, 1.0f);
					}
					else
					{
						WedgeColor.A = 1.0f;
					}

					// Convert linear color to fixed color.
					RawMesh.WedgeColors[WedgeColorIdx] = WedgeColor.ToFColor(false);
				}
			}
			else
			{
				// TODO? Needed? New meshes wont have WedgeIndices yet!?
				// No Colors or Alphas, init colors to White
				FColor DefaultWedgeColor = FLinearColor::White.ToFColor(false);
				WedgeColorsCount = RawMesh.WedgeIndices.Num();
				if (WedgeColorsCount > 0)
					RawMesh.WedgeColors.Init(DefaultWedgeColor, WedgeColorsCount);
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			//  FACE SMOOTHING
			//---------------------------------------------------------------------------------------------------------------------

			// Extract this part's FaceSmoothing values if needed
			UpdatePartFaceSmoothingIfNeeded();

			// Get the FaceSmoothing values for this split
			TArray<int32> SplitFaceSmoothingMasks;
			FHoudiniMeshTranslator::TransferPartAttributesToSplit<int32>(
				SplitVertexList, AttribInfoFaceSmoothingMasks, PartFaceSmoothingMasks, SplitFaceSmoothingMasks);

			// FaceSmoothing masks must be initialized even if we don't have a value from Houdini!
			RawMesh.FaceSmoothingMasks.Init(DefaultMeshSmoothing, SplitVertexCount / 3);

			// Check that the number of face smoothing values we retrieved is correct
			int32 WedgeFaceSmoothCount = SplitFaceSmoothingMasks.Num() / 3;
			if (SplitFaceSmoothingMasks.Num() != 0 && !SplitFaceSmoothingMasks.IsValidIndex((WedgeFaceSmoothCount - 1) * 3 + 2))
			{
				// Ignore our face smoothing values
				WedgeFaceSmoothCount = 0;
				HOUDINI_LOG_WARNING(TEXT("Invalid face smoothing mask count detected - Skipping them."));
			}

			// Transfer the face smoothing masks to the raw mesh if we have any
			for (int32 WedgeFaceSmoothIdx = 0; WedgeFaceSmoothIdx < WedgeFaceSmoothCount; WedgeFaceSmoothIdx += 3)
			{
				RawMesh.FaceSmoothingMasks[WedgeFaceSmoothIdx] = SplitFaceSmoothingMasks[WedgeFaceSmoothIdx * 3];
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			//  UVS
			//--------------------------------------------------------------------------------------------------------------------- 

			// Extract this part's UV sets if needed
			UpdatePartUVSetsIfNeeded();

			// See if we need to transfer uv point attributes to vertex attributes.
			TArray<TArray<float>> SplitUVSets;
			SplitUVSets.SetNum(MAX_STATIC_TEXCOORDS);
			for (int32 TexCoordIdx = 0; TexCoordIdx < MAX_STATIC_TEXCOORDS; ++TexCoordIdx)
			{
				FHoudiniMeshTranslator::TransferPartAttributesToSplit<float>(
					SplitVertexList, AttribInfoUVSets[TexCoordIdx], PartUVSets[TexCoordIdx], SplitUVSets[TexCoordIdx]);
			}

			// Transfer UVs to the Raw Mesh
			int32 UVChannelCount = 0;
			int32 LightMapUVChannel = 0;
			for (int32 TexCoordIdx = 0; TexCoordIdx < MAX_STATIC_TEXCOORDS; ++TexCoordIdx)
			{
				const TArray<float>& SplitUVs = SplitUVSets[TexCoordIdx];

				int32 WedgeUVCount = SplitUVs.Num() / 2;
				if (SplitUVs.Num() > 0 && SplitUVs.IsValidIndex((WedgeUVCount - 1) * 2 + 1))
				{
					RawMesh.WedgeTexCoords[TexCoordIdx].SetNumZeroed(WedgeUVCount);
					for (int32 WedgeUVIdx = 0; WedgeUVIdx < WedgeUVCount; ++WedgeUVIdx)
					{
						// We need to flip V coordinate when it's coming from HAPI.
						RawMesh.WedgeTexCoords[TexCoordIdx][WedgeUVIdx].X = SplitUVs[WedgeUVIdx * 2 + 0];
						RawMesh.WedgeTexCoords[TexCoordIdx][WedgeUVIdx].Y = 1.0f - SplitUVs[WedgeUVIdx * 2 + 1];
					}

					UVChannelCount++;
					if (UVChannelCount <= 2)
						LightMapUVChannel = TexCoordIdx;
				}
				else
				{
					RawMesh.WedgeTexCoords[TexCoordIdx].Empty();
				}
			}

			// We must have at least one UV channel. If there's none, create one filled with zero data.
			if (UVChannelCount == 0)
				RawMesh.WedgeTexCoords[0].SetNumZeroed(SplitVertexCount);

			// Set the lightmap Coordinate Index
			// If we have more than one UV set, the 2nd valid set is used for lightmaps by convention
			// If not, the first UV set will be used
			FoundStaticMesh->LightMapCoordinateIndex = LightMapUVChannel;

			//--------------------------------------------------------------------------------------------------------------------- 
			// LIGHTMAP RESOLUTION
			//--------------------------------------------------------------------------------------------------------------------- 

			// Extract this part's LightmapResolution values if needed
			UpdatePartLightmapResolutionsIfNeeded();

			// make sure the mesh has a new lighting guid
			FoundStaticMesh->LightingGuid = FGuid::NewGuid();

			//--------------------------------------------------------------------------------------------------------------------- 
			//  INDICES
			//--------------------------------------------------------------------------------------------------------------------- 

			//
			// Because of the splits, we don't need to declare all the vertices in the Part, 
			// but only the one that are currently used by the split's faces.
			// The indicesMapper array is used to map those indices from Part Vertices to Split Vertices.
			// We also keep track of the needed vertices index to declare them easily afterwards.
			//

			// IndicesMapper:
			// Maps index values for all vertices in the Part:
			// - Vertices unused by the split will be set to -1
			// - Used vertices will have their value set to the "NewIndex"
			// So that IndicesMapper[ oldIndex ] => newIndex
			TArray<int32> IndicesMapper;
			IndicesMapper.Init(-1, SplitVertexList.Num());
			int32 CurrentMapperIndex = 0;

			// NeededVertices:
			// Array containing the old index of the needed vertices for the current split
			// NeededVertices[ newIndex ] => oldIndex
			TArray< int32 > NeededVertices;
			RawMesh.WedgeIndices.SetNumZeroed(SplitVertexCount);

			int32 ValidVertexId = 0;
			for (int32 VertexIdx = 0; VertexIdx < SplitVertexList.Num(); VertexIdx += 3)
			{
				int32 WedgeCheck = SplitVertexList[VertexIdx + 0];
				if (WedgeCheck == -1)
					continue;

				int32 WedgeIndices[3] =
				{
					SplitVertexList[VertexIdx + 0],
					SplitVertexList[VertexIdx + 1],
					SplitVertexList[VertexIdx + 2]
				};

				// Ensure the indices are valid
				if (!IndicesMapper.IsValidIndex(WedgeIndices[0])
					|| !IndicesMapper.IsValidIndex(WedgeIndices[1])
					|| !IndicesMapper.IsValidIndex(WedgeIndices[2]))
				{
					// Invalid face index.
					HOUDINI_LOG_MESSAGE(
						TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] has some invalid face indices"),
						HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
					continue;
				}

				// Converting Old (Part) Indices to New (Split) Indices:
				for (int32 i = 0; i < 3; i++)
				{
					if (IndicesMapper[WedgeIndices[i]] < 0)
					{
						// This old index has not yet been "converted" to a new index
						NeededVertices.Add(WedgeIndices[i]);
						IndicesMapper[WedgeIndices[i]] = CurrentMapperIndex;
						CurrentMapperIndex++;
					}

					// Replace the old index with the new one
					WedgeIndices[i] = IndicesMapper[WedgeIndices[i]];
				}

				if (!RawMesh.WedgeIndices.IsValidIndex(ValidVertexId + 2))
					break;

				// Flip wedge indices to fix the winding order.
				RawMesh.WedgeIndices[ValidVertexId + 0] = WedgeIndices[0];
				RawMesh.WedgeIndices[ValidVertexId + 1] = WedgeIndices[2];
				RawMesh.WedgeIndices[ValidVertexId + 2] = WedgeIndices[1];

				// Check if we need to patch UVs.
				for (int32 TexCoordIdx = 0; TexCoordIdx < MAX_STATIC_TEXCOORDS; ++TexCoordIdx)
				{
					if (RawMesh.WedgeTexCoords[TexCoordIdx].IsValidIndex(ValidVertexId + 2))
					{
						Swap(RawMesh.WedgeTexCoords[TexCoordIdx][ValidVertexId + 1],
							RawMesh.WedgeTexCoords[TexCoordIdx][ValidVertexId + 2]);
					}
				}

				// Check if we need to patch colors.
				if (RawMesh.WedgeColors.IsValidIndex(ValidVertexId + 2))
					Swap(RawMesh.WedgeColors[ValidVertexId + 1], RawMesh.WedgeColors[ValidVertexId + 2]);

				// Check if we need to patch Normals and tangents.
				if (RawMesh.WedgeTangentZ.IsValidIndex(ValidVertexId + 2))
					Swap(RawMesh.WedgeTangentZ[ValidVertexId + 1], RawMesh.WedgeTangentZ[ValidVertexId + 2]);

				if (RawMesh.WedgeTangentX.IsValidIndex(ValidVertexId + 2))
					Swap(RawMesh.WedgeTangentX[ValidVertexId + 1], RawMesh.WedgeTangentX[ValidVertexId + 2]);

				if (RawMesh.WedgeTangentY.IsValidIndex(ValidVertexId + 2))
					Swap(RawMesh.WedgeTangentY[ValidVertexId + 1], RawMesh.WedgeTangentY[ValidVertexId + 2]);

				ValidVertexId += 3;
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			// POSITIONS
			//--------------------------------------------------------------------------------------------------------------------- 
			UpdatePartPositionIfNeeded();

			//
			// Transfer vertex positions:
			//
			// Because of the split, we're only interested in the needed vertices.
			// Instead of declaring all the Positions, we'll only declare the vertices
			// needed by the current split.
			//
			int32 VertexPositionsCount = NeededVertices.Num();
			RawMesh.VertexPositions.SetNumZeroed(VertexPositionsCount);

			for (int32 VertexPositionIdx = 0; VertexPositionIdx < VertexPositionsCount; ++VertexPositionIdx)
			{
				int32 NeededVertexIndex = NeededVertices[VertexPositionIdx];
				if (!PartPositions.IsValidIndex(NeededVertexIndex * 3 + 2))
				{
					// Error retrieving positions.
					HOUDINI_LOG_WARNING(
						TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] invalid position/index data ")
						TEXT("- skipping."),
						HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);

					continue;
				}

				// We need to swap Z and Y coordinate here, and convert from m to cm. 
				RawMesh.VertexPositions[VertexPositionIdx].X = PartPositions[NeededVertexIndex * 3 + 0] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
				RawMesh.VertexPositions[VertexPositionIdx].Y = PartPositions[NeededVertexIndex * 3 + 2] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
				RawMesh.VertexPositions[VertexPositionIdx].Z = PartPositions[NeededVertexIndex * 3 + 1] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
			}

			/*
			// TODO:
			// Check if this mesh contains only degenerate triangles.
			if (FHoudiniEngineUtils::CountDegenerateTriangles(RawMesh) == SplitGroupFaceCount)
			{
				// This mesh contains only degenerate triangles, there's nothing we can do.
				if (bStaticMeshCreated)
					StaticMesh->MarkPendingKill();

				continue;
			}
			*/
		}

		//--------------------------------------------------------------------------------------------------------------------- 
		// MATERIAL ATTRIBUTE OVERRIDES
		//---------------------------------------------------------------------------------------------------------------------

		// TODO: These are actually per faces, not per vertices...
		// Need to update!!
		UpdatePartFaceMaterialOverridesIfNeeded();

		//--------------------------------------------------------------------------------------------------------------------- 
		// FACE MATERIALS
		//---------------------------------------------------------------------------------------------------------------------

		// TODO:
		// Handle Materials!!!!

		// Get face indices for this split.
		TArray<int32>& SplitFaceIndices = AllSplitFaceIndices[SplitGroupName];

		// We need to reset the Static Mesh's materials once per SM:
		// so, for the first lod, or the main geo...
		if (!MeshMaterialsHaveBeenReset && (SplitType == EHoudiniSplitType::LOD || SplitType == EHoudiniSplitType::Normal))
		{
			FoundStaticMesh->StaticMaterials.Empty();
			MeshMaterialsHaveBeenReset = true;
		}

		// ..  or for each visible complex collider
		if (SplitType == EHoudiniSplitType::RenderedComplexCollider)
			FoundStaticMesh->StaticMaterials.Empty();

		// Process material overrides first
		if (PartFaceMaterialOverrides.Num() > 0)
		{
			// If the part has material overrides
			RawMesh.FaceMaterialIndices.SetNumZeroed(SplitFaceIndices.Num());
			for (int32 FaceIdx = 0; FaceIdx < SplitFaceIndices.Num(); ++FaceIdx)
			{
				int32 SplitFaceIndex = SplitFaceIndices[FaceIdx];
				if (!PartFaceMaterialOverrides.IsValidIndex(SplitFaceIndex))
					continue;

				const FString & MaterialName = PartFaceMaterialOverrides[SplitFaceIndex];
				int32 const * FoundFaceMaterialIdx = MapHoudiniMatAttributesToUnrealIndex.Find(MaterialName);
				int32 CurrentFaceMaterialIdx = 0;
				if (FoundFaceMaterialIdx)
				{
					// We already know what material index to use for that override
					CurrentFaceMaterialIdx = *FoundFaceMaterialIdx;
				}
				else
				{
					// Try to locate the corresponding material interface
					UMaterialInterface * MaterialInterface = nullptr;

					// Start by looking in our assignment map
					auto FoundMaterialInterface = OutputAssignmentMaterials.Find(MaterialName);
					if (FoundMaterialInterface)
						MaterialInterface = *FoundMaterialInterface;

					if (!MaterialInterface && !MaterialName.IsEmpty())
					{
						// Only try to load a material if has a chance to be valid!
						MaterialInterface = Cast<UMaterialInterface>(
							StaticLoadObject(UMaterialInterface::StaticClass(),
								nullptr, *MaterialName, nullptr, LOAD_NoWarn, nullptr));
					}

					if (MaterialInterface)
					{
						// We managed to load the UE4 material
						// Make sure this material is in the assignments before replacing it.
						OutputAssignmentMaterials.Add(MaterialName, MaterialInterface);

						// See if we have a replacement material and use it on the mesh instead
						UMaterialInterface * const *ReplacementMaterialInterface = ReplacementMaterials.Find(MaterialName);
						if (ReplacementMaterialInterface && *ReplacementMaterialInterface)
							MaterialInterface = *ReplacementMaterialInterface;

						// Add this material to the map
						CurrentFaceMaterialIdx = FoundStaticMesh->StaticMaterials.Add(FStaticMaterial(MaterialInterface));
						MapHoudiniMatAttributesToUnrealIndex.Add(MaterialName, CurrentFaceMaterialIdx);
					}
					else
					{
						// The Attribute Material and its replacement do not exist
						// See if we can fallback to the Houdini material assigned on the face

						// Get the unreal material corresponding to this houdini one
						HAPI_NodeId MaterialId = PartFaceMaterialIds[SplitFaceIndex];

						// See if we have already treated that material
						int32 const * FoundUnrealMatIndex = MapHoudiniMatIdToUnrealIndex.Find(MaterialId);
						if (FoundUnrealMatIndex)
						{
							// This material has been mapped already, just assign the mat index
							CurrentFaceMaterialIdx = *FoundUnrealMatIndex;
						}
						else
						{
							// If everything fails, we'll use the default material
							MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

							// We need to add this material to the map
							FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
							FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, MaterialId, MaterialPathName);
							UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
							if (FoundMaterial)
								MaterialInterface = *FoundMaterial;

							// See if we have a replacement material and use it on the mesh instead
							UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(MaterialPathName);
							if (ReplacementMaterial && *ReplacementMaterial)
								MaterialInterface = *ReplacementMaterial;

							// Add the material to the Static mesh
							CurrentFaceMaterialIdx = FoundStaticMesh->StaticMaterials.Add(FStaticMaterial(MaterialInterface));

							// Map the Houdini ID to the unreal one
							MapHoudiniMatIdToUnrealIndex.Add(MaterialId, CurrentFaceMaterialIdx);
						}
					}
				}

				// Update the Face Material on the mesh
				RawMesh.FaceMaterialIndices[FaceIdx] = CurrentFaceMaterialIdx;
			}
		}
		else if (PartUniqueMaterialIds.Num() > 0)
		{
			// The part has houdini materials
			if (bOnlyOneFaceMaterial)
			{
				// We have only one material.
				RawMesh.FaceMaterialIndices.SetNumZeroed(SplitFaceIndices.Num());

				// Use default Houdini material if no valid material is assigned to any of the faces.
				UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

				// Get id of this single material.
				FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
				FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, PartFaceMaterialIds[0], MaterialPathName);
				UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
				if (FoundMaterial)
					MaterialInterface = *FoundMaterial;

				// See if we have a replacement material and use it on the mesh instead
				UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(MaterialPathName);
				if (ReplacementMaterial && *ReplacementMaterial)
					MaterialInterface = *ReplacementMaterial;

				FoundStaticMesh->StaticMaterials.Empty();
				FoundStaticMesh->StaticMaterials.Add(FStaticMaterial(MaterialInterface));
			}
			else
			{
				// We have multiple houdini materials
				// Get default Houdini material.
				UMaterial * DefaultMaterial = FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get();

				// Reset Rawmesh material face assignments.
				RawMesh.FaceMaterialIndices.SetNumZeroed(SplitFaceIndices.Num());
				for (int32 FaceIdx = 0; FaceIdx < SplitFaceIndices.Num(); ++FaceIdx)
				{
					int32 SplitFaceIndex = SplitFaceIndices[FaceIdx];
					if (!PartFaceMaterialIds.IsValidIndex(SplitFaceIndex))
						continue;

					// Get material id for this face.
					HAPI_NodeId MaterialId = PartFaceMaterialIds[SplitFaceIndex];

					// See if we have already treated that material
					int32 const * FoundUnrealMatIndex = MapHoudiniMatIdToUnrealIndex.Find(MaterialId);
					if (FoundUnrealMatIndex)
					{
						// This material has been mapped already, just assign the mat index
						RawMesh.FaceMaterialIndices[FaceIdx] = *FoundUnrealMatIndex;
						continue;
					}

					UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(DefaultMaterial);

					FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
					FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, MaterialId, MaterialPathName);
					UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
					if (FoundMaterial)
						MaterialInterface = *FoundMaterial;

					// See if we have a replacement material and use it on the mesh instead
					UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(MaterialPathName);
					if (ReplacementMaterial && *ReplacementMaterial)
						MaterialInterface = *ReplacementMaterial;

					// Add the material to the Static mesh
					int32 UnrealMatIndex = FoundStaticMesh->StaticMaterials.Add(FStaticMaterial(MaterialInterface));

					// Map the houdini ID to the unreal one
					MapHoudiniMatIdToUnrealIndex.Add(MaterialId, UnrealMatIndex);

					// Update the face index
					RawMesh.FaceMaterialIndices[FaceIdx] = UnrealMatIndex;
				}
			}
		}
		else
		{
			// No materials were found, we need to use default Houdini material.
			int32 SplitFaceCount = SplitFaceIndices.Num();
			RawMesh.FaceMaterialIndices.SetNumZeroed(SplitFaceCount);

			UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

			// See if we have a replacement material and use it on the mesh instead
			UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(HAPI_UNREAL_DEFAULT_MATERIAL_NAME);
			if (ReplacementMaterial && *ReplacementMaterial)
				MaterialInterface = *ReplacementMaterial;

			FoundStaticMesh->StaticMaterials.Empty();
			FoundStaticMesh->StaticMaterials.Add(FStaticMaterial(MaterialInterface));
		}
		
		// Update the Build Settings using the default setting values
		SetMeshBuildSettings(
			SrcModel->BuildSettings, 
			RawMesh.WedgeTangentZ.Num() > 0, 
			(RawMesh.WedgeTangentX.Num() > 0 && RawMesh.WedgeTangentY.Num() > 0),
			RawMesh.WedgeTexCoords->Num() > 0);

		// Check for a lightmap resolution override
		int32 LightMapResolutionOverride = -1;
		if (PartLightMapResolutions.Num() > 0)
			LightMapResolutionOverride = PartLightMapResolutions[0];

		if (LightMapResolutionOverride > 0)
			FoundStaticMesh->LightMapResolution = LightMapResolutionOverride;
		else
			FoundStaticMesh->LightMapResolution = 64;

		// TODO
		//StaticMeshGenerationProperties.bGeneratedUseMaximumStreamingTexelRatio;
		//StaticMeshGenerationProperties.GeneratedStreamingDistanceMultiplier;
		//StaticMeshGenerationProperties.GeneratedFoliageDefaultSettings;

		// TODO:
		// Turnoff bGenerateLightmapUVs if lightmap uv sets has bad uvs ?

		// By default the distance field resolution should be set to 2.0
		SrcModel->BuildSettings.DistanceFieldResolutionScale = StaticMeshGenerationProperties.GeneratedDistanceFieldResolutionScale;

		// This is required due to the impeding deprecation of FRawMesh
		// If we dont update this UE4 will crash upon deleting an asset.
		SrcModel->StaticMeshOwner = FoundStaticMesh;
		// Store the new raw mesh.
		SrcModel->SaveRawMesh(RawMesh);

		// LOD Screensize
		// default values has already been set, see if we have any attribute override for this
		float screensize = GetLODSCreensizeForSplit(SplitGroupName);
		if (screensize >= 0.0f)
		{
			// Only apply the LOD screensize if it's valid
			SrcModel->ScreenSize = screensize;
			//FoundStaticMesh->GetSourceModel(LODIndex).ScreenSize = screensize;
			FoundStaticMesh->bAutoComputeLODScreenSize = false;
		}

		// TODO:
		// SET STATIC MESH GENERATION PARAM
		// HANDLE COLLIDERS
		// REMOVE OLD COLLIDERS
		// CUSTOM BAKE NAME OVERRIDE

		// Update property attributes on the SM
		TArray<FHoudiniGenericAttribute> PropertyAttributes;
		if (GetGenericPropertiesAttributes(
			HGPO.GeoId, HGPO.PartId,
			AllSplitFirstValidVertexIndex[SplitGroupName],
			AllSplitFirstValidPrimIndex[SplitGroupName],
			PropertyAttributes))
		{
			UpdateGenericPropertiesAttributes(
				FoundStaticMesh, PropertyAttributes);
		}

		TArray<FString> LevelPaths;
		if (FoundOutputObject && FHoudiniEngineUtils::GetLevelPathAttribute(HGPO.GeoId, HGPO.PartId, LevelPaths))
		{
			if (LevelPaths.Num() > 0 && !LevelPaths[0].IsEmpty())
			{
				// cache the level path attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_LEVEL_PATH, LevelPaths[0]);
			}
		}

		TArray<FString> OutputNames;
		if (FoundOutputObject && FHoudiniEngineUtils::GetOutputNameAttribute(HGPO.GeoId, HGPO.PartId, OutputNames))
		{
			if (OutputNames.Num() > 0 && !OutputNames[0].IsEmpty())
			{
				// cache the output name attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_CUSTOM_OUTPUT_NAME_V2, OutputNames[0]);
			}
		}

		TArray<int32> TileValues;
		if (FoundOutputObject && FHoudiniEngineUtils::GetTileAttribute(HGPO.GeoId, HGPO.PartId, TileValues))
		{
			if (TileValues.Num() > 0 && TileValues[0] >= 0)
			{
				// cache the tile attribute as a token on the output object
				FoundOutputObject->CachedTokens.Add(TEXT("tile"), FString::FromInt(TileValues[0]));
			}
		}

		TArray<FString> BakeOutputActorNames;
		if (FoundOutputObject && FHoudiniEngineUtils::GetBakeActorAttribute(HGPO.GeoId, HGPO.PartId, BakeOutputActorNames))
		{
			if (BakeOutputActorNames.Num() > 0 && !BakeOutputActorNames[0].IsEmpty())
			{
				// cache the bake actor attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_BAKE_ACTOR, BakeOutputActorNames[0]);
			}
		}

		TArray<FString> BakeFolders;
		if (FoundOutputObject && FHoudiniEngineUtils::GetBakeFolderAttribute(HGPO.GeoId, BakeFolders, HGPO.PartId))
		{
			if (BakeFolders.Num() > 0 && !BakeFolders[0].IsEmpty())
			{
				// cache the unreal_bake_folder attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_BAKE_FOLDER, BakeFolders[0]);
			}
		}

		TArray<FString> BakeOutlinerFolders;
		if (FoundOutputObject && FHoudiniEngineUtils::GetBakeOutlinerFolderAttribute(HGPO.GeoId, HGPO.PartId, BakeOutlinerFolders))
		{
			if (BakeOutlinerFolders.Num() > 0 && !BakeOutlinerFolders[0].IsEmpty())
			{
				// cache the bake actor attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_BAKE_OUTLINER_FOLDER, BakeOutlinerFolders[0]);
			}
		}

		// Notify that we created a new Static Mesh if needed
		if (bNewStaticMeshCreated)
			FAssetRegistryModule::AssetCreated(FoundStaticMesh);

		// Add the Static mesh to the output maps and the build map if we haven't already
		if (FoundOutputObject)
		{
			FoundOutputObject->OutputObject = FoundStaticMesh;
			FoundOutputObject->bProxyIsCurrent = false;
			OutputObjects.FindOrAdd(OutputObjectIdentifier, *FoundOutputObject);
		}

		StaticMeshToBuild.FindOrAdd(OutputObjectIdentifier, FoundStaticMesh);
	}

	// Look if we only have colliders
	// If we do, we'll allow attaching sockets to the collider meshes
	bool bCollidersOnly = true;
	for (auto& Current : StaticMeshToBuild)
	{
		EHoudiniSplitType CurrentSplitType = GetSplitTypeFromSplitName(Current.Key.SplitIdentifier);
		if (CurrentSplitType == EHoudiniSplitType::LOD || CurrentSplitType == EHoudiniSplitType::Normal)
		{
			bCollidersOnly = false;
			break;
		}
	}

	FHoudiniScopedGlobalSilence ScopedGlobalSilence;
	for (auto& Current : StaticMeshToBuild)
	{
		UStaticMesh* SM = Current.Value;
		if (!SM || SM->IsPendingKill())
			continue;

		UBodySetup * BodySetup = SM->BodySetup;
		if (!BodySetup)
		{
			SM->CreateBodySetup();
			BodySetup = SM->BodySetup;
		}

		EHoudiniSplitType SplitType = GetSplitTypeFromSplitName(Current.Key.SplitIdentifier);

		// Handle the Static Mesh's colliders
		if (BodySetup && !BodySetup->IsPendingKill())
		{
			// Make sure rendering is done - so we are not changing data being used by collision drawing.
			FlushRenderingCommands();

			// Clean up old colliders from a previous cook
			BodySetup->Modify();
			BodySetup->RemoveSimpleCollision();
			// Create new GUID
			BodySetup->InvalidatePhysicsData();

			FHoudiniOutputObjectIdentifier CurrentObjId = Current.Key;
			FKAggregateGeom* CurrentAggColl = AllAggregateCollisions.Find(Current.Key);
			if (CurrentAggColl && CurrentAggColl->GetElementCount() > 0)
			{
				BodySetup->AddCollisionFrom(*CurrentAggColl);
				BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseDefault;
			}

			RefreshCollisionChange(*SM);
			SM->bCustomizedCollision = true;

			// See if we need to enable collisions on the whole mesh
			if (SplitType == EHoudiniSplitType::InvisibleComplexCollider || SplitType == EHoudiniSplitType::RenderedComplexCollider)
			{
				// Complex collider, enable collisions for this static mesh.
				BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;
			}
			else
			{
				// TODO
				// if the LODForCollision uproperty attribute is set, we need to activate complex collision
				// on the static mesh for that lod to be picked up properly as a collider
				if (FHoudiniEngineUtils::HapiCheckAttributeExists(HGPO.GeoId, HGPO.PartId,
					"unreal_uproperty_LODForCollision", HAPI_ATTROWNER_DETAIL))
				{
					BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;
				}
			}
		}

		// Add the Sockets to the StaticMesh
		// We only add them to the main geo, or to the colliders if we only generate colliders
		bool bAddSocket = SplitType == EHoudiniSplitType::Normal ? true : bCollidersOnly ? true : false;
		if (bAddSocket)
		{
			if (!FHoudiniEngineUtils::AddMeshSocketsToStaticMesh(SM, AllSockets, true))
			{
				HOUDINI_LOG_WARNING(TEXT("Failed to import sockets for StaticMesh %s."), *(SM->GetName()));
			}
		}

		// BUILD the Static Mesh
		// bSilent doesnt add the Build Errors...
		double build_start = FPlatformTime::Seconds();
		TArray<FText> SMBuildErrors;
		SM->Build(true, &SMBuildErrors);
		double build_end = FPlatformTime::Seconds();
		HOUDINI_LOG_MESSAGE(TEXT("StaticMesh->Build() executed in %f seconds."), build_end - build_start);

		SM->GetOnMeshChanged().Broadcast();

		/*
		// Try to find the outer package so we can dirty it up
		if (SM->GetOuter())
		{
			SM->GetOuter()->MarkPackageDirty();
		}
		else
		{
			SM->MarkPackageDirty();
		}
		*/
		

		UPackage* MeshPackage = SM->GetOutermost();
		if (MeshPackage && !MeshPackage->IsPendingKill())
		{
			MeshPackage->MarkPackageDirty();

			/*
			// DPT: deactivated auto saving mesh/material package
			// only dirty for now, as we'll save them when saving the world.
			TArray<UPackage*> PackageToSave;
			PackageToSave.Add(MeshPackage);

			// Save the created package
			FEditorFileUtils::PromptForCheckoutAndSave(PackageToSave, false, false);
			*/
		}
	}

	// TODO: Still necessary ? SM->Build should actually update the navmesh...
	// Now that all the meshes are built and their collisions meshes and primitives updated,
	// we need to update their pre-built navigation collision used by the navmesh
	for (auto& Iter : OutputObjects)
	{
		UStaticMesh* StaticMesh = Cast<UStaticMesh>(Iter.Value.OutputObject);
		if (!StaticMesh || StaticMesh->IsPendingKill())
			continue;

		UBodySetup * BodySetup = StaticMesh->BodySetup;
		if (BodySetup && !BodySetup->IsPendingKill() && StaticMesh->NavCollision)
		{
			// Unreal caches the Navigation Collision and never updates it for StaticMeshes,
			// so we need to manually flush and recreate the data to have proper navigation collision
			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
			StaticMesh->NavCollision->Setup(BodySetup);
		}
	}

	double time_end = FPlatformTime::Seconds();
	HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_RawMesh() executed in %f seconds."), time_end - time_start);

	return true;
}

bool
FHoudiniMeshTranslator::CreateStaticMesh_MeshDescription()
{
	double time_start = FPlatformTime::Seconds();

	// Start by updating the vertex list
	if (!UpdatePartVertexList())
		return false;

	// Sort the split groups
	// Simple colliders first, lods and finally, invisible colliders (that are separate Static Mesh)
	SortSplitGroups();

	// Handles the split groups found in the part
	// and builds the corresponding faces and indices arrays
	if (!UpdateSplitsFacesAndIndices())
		return true;

	// Resets the containers used for the raw data extraction.
	ResetPartCache();

	// Prepare the object that will store UCX and simple colliders
	AllAggregateCollisions.Empty();

	// We need to know the number of LODs that will be needed for this part
	int32 NumberOfLODs = 0;
	bool bHasMainGeo = false;
	for (auto& curSplit : AllSplitGroups)
	{
		if (GetSplitTypeFromSplitName(curSplit) == EHoudiniSplitType::LOD)
			NumberOfLODs++;
		else if (GetSplitTypeFromSplitName(curSplit) == EHoudiniSplitType::Normal)
			bHasMainGeo = true;
	}

	// Update the part's material's IDS and info now
	CreateNeededMaterials();

	// Check if the materials were updated
	bool bMaterialHasChanged = false;
	for (const auto& MatInfo : PartUniqueMaterialInfos)
	{
		if (MatInfo.hasChanged)
		{
			bMaterialHasChanged = true;
			break;
		}
	}

	// Get the current target platform for default lod policies
	ITargetPlatform * CurrentPlatform = GetTargetPlatformManagerRef().GetRunningTargetPlatform();
	check(CurrentPlatform);

	// New mesh list
	TMap<FHoudiniOutputObjectIdentifier, UStaticMesh*> StaticMeshToBuild;

	// Map of Houdini Material IDs to Unreal Material Indices
	TMap< HAPI_NodeId, int32 > MapHoudiniMatIdToUnrealIndex;
	// Map of Houdini Material Attributes to Unreal Material Indices
	TMap< FString, int32 > MapHoudiniMatAttributesToUnrealIndex;

	bool MeshMaterialsHaveBeenReset = false;

	// Mesh Socket array
	TArray<FHoudiniMeshSocket> AllSockets;
	FHoudiniEngineUtils::AddMeshSocketsToArray_DetailAttribute(
		HGPO.GeoId, HGPO.PartId, AllSockets, HGPO.PartInfo.bIsInstanced);
	FHoudiniEngineUtils::AddMeshSocketsToArray_Group(
		HGPO.GeoId, HGPO.PartId, AllSockets, HGPO.PartInfo.bIsInstanced);

	double tick = FPlatformTime::Seconds();
	HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - Pre Split-Loop in %f seconds."), tick - time_start);

	// Iterate through all detected split groups we care about and split geometry.
	// The split are ordered in the following way:
	// Invisible Simple/Convex Colliders > LODs > MainGeo > Visible Colliders > Invisible Colliders
	for (int32 SplitId = 0; SplitId < AllSplitGroups.Num(); SplitId++)
	{
		// Get split group name
		const FString& SplitGroupName = AllSplitGroups[SplitId];

		// Get the vertex indices for this group
		TArray<int32>& SplitVertexList = AllSplitVertexLists[SplitGroupName];

		// Get valid count of vertex indices for this split.
		const int32& SplitVertexCount = AllSplitVertexCounts[SplitGroupName];

		// Make sure we have a  valid vertex count for this split
		if (SplitVertexCount % 3 != 0 || SplitVertexList.Num() % 3 != 0)
		{
			// Invalid vertex count, skip this split or we'd crash trying to create a mesh for it.
			HOUDINI_LOG_WARNING(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] invalid vertex count.")
				TEXT("- skipping."),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);

			continue;
		}

		// Get the current split type
		EHoudiniSplitType SplitType = GetSplitTypeFromSplitName(SplitGroupName);
		if (SplitType == EHoudiniSplitType::Invalid)
		{
			// Invalid split, skip
			HOUDINI_LOG_WARNING(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] unknown split type.")
				TEXT("- skipping."),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
			continue;
		}

		// Get the output identifer for this split
		FHoudiniOutputObjectIdentifier OutputObjectIdentifier(
			HGPO.ObjectId, HGPO.GeoId, HGPO.PartId, GetMeshIdentifierFromSplit(SplitGroupName, SplitType));
		OutputObjectIdentifier.PartName = HGPO.PartName;
		OutputObjectIdentifier.PrimitiveIndex = AllSplitFirstValidVertexIndex[SplitGroupName],
		OutputObjectIdentifier.PointIndex = AllSplitFirstValidPrimIndex[SplitGroupName];		

		// Get/Create the Aggregate Collisions for this mesh identifier
		FKAggregateGeom& AggregateCollisions = AllAggregateCollisions.FindOrAdd(OutputObjectIdentifier);

		// Handle UCX / Convex Hull colliders
		if (SplitType == EHoudiniSplitType::InvisibleUCXCollider || SplitType == EHoudiniSplitType::RenderedUCXCollider)
		{
			// Get the part position if needed
			UpdatePartPositionIfNeeded();

			// Create the convex hull colliders and add them to the Aggregate
			if (!AddConvexCollisionToAggregate(SplitGroupName, AggregateCollisions))
			{
				// Failed to generate a convex collider
				HOUDINI_LOG_WARNING(
					TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] failed to create convex collider."),
					HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
			}

			// If the collider is not visible, stop here
			if (SplitType == EHoudiniSplitType::InvisibleUCXCollider)
				continue;
		}
		else if (SplitType == EHoudiniSplitType::InvisibleSimpleCollider || SplitType == EHoudiniSplitType::RenderedSimpleCollider)
		{
			// Get the part position if needed
			UpdatePartPositionIfNeeded();

			// Create the simple colliders and add them to the aggregate
			if (!AddSimpleCollisionToAggregate(SplitGroupName, AggregateCollisions))
			{
				// Failed to generate a convex collider
				HOUDINI_LOG_WARNING(
					TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] failed to create simple collider."),
					HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
			}

			// If the collider is not visible, stop here
			if (SplitType == EHoudiniSplitType::InvisibleSimpleCollider)
				continue;
		}

		// Try to find existing properties for this identifier
		FHoudiniOutputObject* FoundOutputObject = InputObjects.Find(OutputObjectIdentifier);
		// Try to find an existing SM from a previous cook
		UStaticMesh* FoundStaticMesh = FindExistingStaticMesh(OutputObjectIdentifier);

		// Flag whether or not we need to rebuild the mesh
		bool bRebuildStaticMesh = false;
		if (HGPO.GeoInfo.bHasGeoChanged || HGPO.PartInfo.bHasChanged || ForceRebuild || !FoundStaticMesh || !FoundOutputObject)
			bRebuildStaticMesh = true;

		// TODO: Handle materials
		if (!bRebuildStaticMesh && !bMaterialHasChanged)
		{
			// We can simply reuse the found static mesh
			OutputObjects.Add(OutputObjectIdentifier, *FoundOutputObject);
			continue;
		}

		// Prepare LOD Group data for this static mesh
		FStaticMeshLODGroup LODGroup;
		
		bool bNewStaticMeshCreated = false;
		if (!FoundStaticMesh)
		{
			// If we couldn't find a valid existing static mesh, create a new one
			FoundStaticMesh = CreateNewStaticMesh(OutputObjectIdentifier.SplitIdentifier);
			if (!FoundStaticMesh || FoundStaticMesh->IsPendingKill())
				continue;

			bNewStaticMeshCreated = true;

			// Use the platform's default LODGroup policy
			// TODO? Add setting for default LOD Group?
			LODGroup = CurrentPlatform->GetStaticMeshLODSettings().GetLODGroup(NAME_None);
		}
		else
		{
			// Try to reuse the existing SM's LOD group instead of the default one
			LODGroup = CurrentPlatform->GetStaticMeshLODSettings().GetLODGroup(FoundStaticMesh->LODGroup);
		}

		if (!FoundOutputObject)
		{
			FHoudiniOutputObject NewOutputObject;
			FoundOutputObject = &OutputObjects.Add(OutputObjectIdentifier, NewOutputObject);
		}
		else
		{
			// If this is not a new output object we have to clear the CachedAttributes and CachedTokens before
			// setting the new values (so that we do not re-use any values from the previous cook)
			FoundOutputObject->CachedAttributes.Empty();
			FoundOutputObject->CachedTokens.Empty();
		}
		FoundOutputObject->bProxyIsCurrent = false;

		// TODO: Needed?
		// Free any RHI resources for existing mesh before we re-create in place.
		FoundStaticMesh->PreEditChange(NULL);

		// Check that the Static Mesh we found has the appropriate number of Source models/LODs
		int32 NeededNumberOfLODs = FMath::Max(NumberOfLODs + (bHasMainGeo ? 1 : 0), LODGroup.GetDefaultNumLODs());

		// LODs are only for the "main" mesh, not for complex colliders!
		if (SplitType == EHoudiniSplitType::InvisibleComplexCollider || SplitType == EHoudiniSplitType::RenderedComplexCollider)
			NeededNumberOfLODs = FMath::Max(1, LODGroup.GetDefaultNumLODs());

		if (FoundStaticMesh->GetNumSourceModels() != NeededNumberOfLODs)
		{
			while (FoundStaticMesh->GetNumSourceModels() < NeededNumberOfLODs)
				FoundStaticMesh->AddSourceModel();

			// We may have to remove excessive LOD levels
			if (FoundStaticMesh->GetNumSourceModels() > NeededNumberOfLODs)
				FoundStaticMesh->SetNumSourceModels(NeededNumberOfLODs);

			// Initialize their default reduction setting
			for (int32 ModelLODIndex = 0; ModelLODIndex < NeededNumberOfLODs; ModelLODIndex++)
			{
				FoundStaticMesh->GetSourceModel(ModelLODIndex).ReductionSettings = LODGroup.GetDefaultSettings(ModelLODIndex);
			}
			FoundStaticMesh->LightMapResolution = LODGroup.GetDefaultLightMapResolution();
		}

		// By default, always work on the first source model, unless we're a LOD
		int32 SrcModelIndex = 0;
		int32 LODIndex = 0;
		if (SplitType == EHoudiniSplitType::LOD)
		{
			for (auto& curSplit : AllSplitGroups)
			{
				EHoudiniSplitType CurrentSplitType = GetSplitTypeFromSplitName(curSplit);
				if (CurrentSplitType == EHoudiniSplitType::LOD
					|| CurrentSplitType == EHoudiniSplitType::Normal)
				{
					LODIndex++;
				}

				if (curSplit == SplitGroupName)
					break;
			}

			// Fix for the case where we don't have a main geo
			if(!bHasMainGeo)
				LODIndex--;
		}

		// Grab the appropriate SourceModel
		FStaticMeshSourceModel* SrcModel = (FoundStaticMesh->IsSourceModelValid(LODIndex)) ? &(FoundStaticMesh->GetSourceModel(LODIndex)) : nullptr;
		if (!SrcModel)
		{
			HOUDINI_LOG_ERROR(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d, %s] Could not access SourceModel for the LOD %d - skipping."),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName, LODIndex);
			continue;
		}

		HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - PreMeshDescription in %f seconds."), FPlatformTime::Seconds() - tick);
		tick = FPlatformTime::Seconds();

		bool bHasNormal = false;
		bool bHasTangents = false;

		// Load the existing mesh description if we don't need to rebuild the mesh		
		FMeshDescription* MeshDescription;
		if (!bRebuildStaticMesh)
		{
			// We dont need to rebuild the mesh itself:
			// the geometry hasn't changed, but the materials have.
			// We can just reuse the old MeshDescription and reuse it.
			MeshDescription = FoundStaticMesh->GetMeshDescription(LODIndex);
		}
		else
		{
			// Extract all the data needed for this split
			// Start by initializing the MeshDescription for this LOD			
			MeshDescription = FoundStaticMesh->CreateMeshDescription(LODIndex);
			FStaticMeshAttributes(*MeshDescription).Register();

			// Mesh description uses material to create its PolygonGroups,
			// so we first need to know how many different materials we have for this split
			// and what vertices/indices belong to each material for remapping

			//--------------------------------------------------------------------------------------------------------------------- 
			//  INDICES
			//--------------------------------------------------------------------------------------------------------------------- 

			//
			// Because of the splits, we don't need to declare all the vertices in the Part, 
			// but only the one that are currently used by the split's faces.
			// The indicesMapper array is used to map those indices from Part Vertices to Split Vertices.
			// We also keep track of the needed vertices index to declare them easily afterwards.
			//

			// SplitNeededVertices
			// Array containing the (unique) part indices for the vertices that are needed for this split
			// SplitNeededVertices[splitIndex] = PartIndex
			TArray<int32> SplitNeededVertices;
			//SplitNeededVertices.SetNumZeroed(SplitVertexCount);

			// IndicesMapper:
			// Maps index values for all vertices in the Part:
			// - Vertices unused by the split will be set to -1
			// - Used vertices will have their value set to the "NewIndex" so that IndicesMapper[ partIndex ] => splitIndex
			TArray<int32> PartToSplitIndicesMapper;
			PartToSplitIndicesMapper.Init(-1, SplitVertexList.Num());
			//TMap<int32, int32> SplitToPartIndicesMapper;

			// SplitIndices
			// Array of SplitIndices used to describe this split's polygons
			TArray<uint32> SplitIndices;
			SplitIndices.SetNumZeroed(SplitVertexCount);

			int32 CurrentSplitIndex = 0;
			int32 ValidVertexId = 0;
			for (int32 VertexIdx = 0; VertexIdx < SplitVertexList.Num(); VertexIdx += 3)
			{
				int32 WedgeCheck = SplitVertexList[VertexIdx + 0];
				if (WedgeCheck == -1)
					continue;

				int32 WedgeIndices[3] =
				{
					SplitVertexList[VertexIdx + 0],
					SplitVertexList[VertexIdx + 1],
					SplitVertexList[VertexIdx + 2]
				};

				// Ensure the indices are valid
				if (!PartToSplitIndicesMapper.IsValidIndex(WedgeIndices[0])
					|| !PartToSplitIndicesMapper.IsValidIndex(WedgeIndices[1])
					|| !PartToSplitIndicesMapper.IsValidIndex(WedgeIndices[2]))
				{
					// Invalid face index.
					HOUDINI_LOG_MESSAGE(
						TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] has some invalid face indices"),
						HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
					continue;
				}

				// Converting Old (Part) Indices to New (Split) Indices:
				for (int32 i = 0; i < 3; i++)
				{
					if (PartToSplitIndicesMapper[WedgeIndices[i]] < 0)
					{
						// This part index has not yet been "converted" to a new split index
						SplitNeededVertices.Add(WedgeIndices[i]);
						PartToSplitIndicesMapper[WedgeIndices[i]] = CurrentSplitIndex;
						//SplitToPartIndicesMapper.Add(CurrentSplitIndex, WedgeIndices[i]);
						CurrentSplitIndex++;
					}

					// Replace the old part index with the new split index
					WedgeIndices[i] = PartToSplitIndicesMapper[WedgeIndices[i]];
				}

				if (!SplitIndices.IsValidIndex(ValidVertexId + 2))
					break;

				// Flip wedge indices to fix the winding order.
				SplitIndices[ValidVertexId + 0] = WedgeIndices[0];
				SplitIndices[ValidVertexId + 1] = WedgeIndices[2];
				SplitIndices[ValidVertexId + 2] = WedgeIndices[1];

				ValidVertexId += 3;
			}
			
			HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - Indices in %f seconds."), FPlatformTime::Seconds() - tick);
			tick = FPlatformTime::Seconds();

			//--------------------------------------------------------------------------------------------------------------------- 
			// POSITIONS
			//--------------------------------------------------------------------------------------------------------------------- 			
			
			// Extract position for this part
			UpdatePartPositionIfNeeded();

			// Transfer vertex positions:
			//
			// Because of the split, we're only interested in the needed vertices.
			// Instead of declaring all the Positions, we'll only declare the vertices
			// needed by the current split.
			//
			TVertexAttributesRef<FVector> VertexPositions =
				MeshDescription->VertexAttributes().GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);
				
			MeshDescription->ReserveNewVertices(SplitNeededVertices.Num());
			for ( const int32& NeededVertexIndex : SplitNeededVertices)
			{
				// Create a new Vertex
				FVertexID VertexID = MeshDescription->CreateVertex();
				if (PartPositions.IsValidIndex(NeededVertexIndex * 3 + 2))
				{
					// We need to swap Z and Y coordinate here, and convert from m to cm. 
					VertexPositions[VertexID].X = PartPositions[NeededVertexIndex * 3 + 0] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
					VertexPositions[VertexID].Y = PartPositions[NeededVertexIndex * 3 + 2] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
					VertexPositions[VertexID].Z = PartPositions[NeededVertexIndex * 3 + 1] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
				}
				else
				{
					// Error when retrieving positions.
					HOUDINI_LOG_WARNING(
						TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] invalid position/index data ")
						TEXT("- skipping."),
						HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);

					continue;
				}
			}

			HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - Positions in %f seconds."), FPlatformTime::Seconds() - tick);
			tick = FPlatformTime::Seconds();

			//--------------------------------------------------------------------------------------------------------------------- 
			// MATERIALS
			//---------------------------------------------------------------------------------------------------------------------

			// TODO: Check if still needed for MeshDescription
			// We need to reset the Static Mesh's materials once per SM:
			// so, for the first lod, or the main geo...
			if (!MeshMaterialsHaveBeenReset && (SplitType == EHoudiniSplitType::LOD || SplitType == EHoudiniSplitType::Normal))
			{
				FoundStaticMesh->StaticMaterials.Empty();
				MeshMaterialsHaveBeenReset = true;
			}

			// ..  or for each visible complex collider
			if (SplitType == EHoudiniSplitType::RenderedComplexCollider)
				FoundStaticMesh->StaticMaterials.Empty();

			// Get this split's faces
			TArray<int32>& SplitGroupFaceIndices = AllSplitFaceIndices[SplitGroupName];
			// Array holding the materials needed for this split
			//TArray<UMaterialInterface*> SplitMaterials;
			// Split Material indices per face, by default all faces are set to use the first Material
			TArray<int32> SplitFaceMaterialIndices;
			SplitFaceMaterialIndices.SetNumZeroed(SplitGroupFaceIndices.Num());

			bool HasHoudiniMaterials = PartUniqueMaterialIds.Num() > 0;
			bool HasMaterialOverrides = PartFaceMaterialOverrides.Num() > 0;
			if (!HasHoudiniMaterials && !HasMaterialOverrides)
			{
				// We don't have any material override or houdini material
				// we just need one polygon group using the default Houdini material.
				UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

				// See if we have a replacement material and use it on the mesh instead
				UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(HAPI_UNREAL_DEFAULT_MATERIAL_NAME);
				if (ReplacementMaterial && *ReplacementMaterial)
					MaterialInterface = *ReplacementMaterial;

				FoundStaticMesh->StaticMaterials.Add(MaterialInterface);

				// TODO: ? Add default mat to the assignement map?
			}
			else if (HasHoudiniMaterials && !HasMaterialOverrides)
			{
				// We have Houdini Material but no overrides
				if (bOnlyOneFaceMaterial || PartUniqueMaterialIds.Num() == 1)
				{
					// We have only one Houdini material.
					// Use default Houdini material if no valid material is assigned to any of the faces.
					UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

					// Get id of this single material.
					FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
					FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, PartFaceMaterialIds[0], MaterialPathName);
					UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
					if (FoundMaterial)
						MaterialInterface = *FoundMaterial;

					// See if we have a replacement material and use it on the mesh instead
					UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(MaterialPathName);
					if (ReplacementMaterial && *ReplacementMaterial)
						MaterialInterface = *ReplacementMaterial;

					FoundStaticMesh->StaticMaterials.Add(MaterialInterface);

					// TODO: ? Add the mat to the assignement map?
				}
				else
				{
					// We have multiple houdini materials
					// Get default Houdini material.
					UMaterial * MaterialDefault = FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get();

					// Reset Rawmesh material face assignments.
					for (int32 FaceIdx = 0; FaceIdx < SplitGroupFaceIndices.Num(); ++FaceIdx)
					{
						int32 SplitFaceIndex = SplitGroupFaceIndices[FaceIdx];
						if (!PartFaceMaterialIds.IsValidIndex(SplitFaceIndex))
							continue;

						// Get material id for this face.
						HAPI_NodeId MaterialId = PartFaceMaterialIds[SplitFaceIndex];

						// See if we have already treated that material
						int32 const * FoundUnrealMatIndex = MapHoudiniMatIdToUnrealIndex.Find(MaterialId);
						if (FoundUnrealMatIndex)
						{
							// This material has been mapped already, just use its material index
							SplitFaceMaterialIndices[FaceIdx] = *FoundUnrealMatIndex;
							continue;
						}

						UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(MaterialDefault);

						FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
						FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, MaterialId, MaterialPathName);
						UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
						if (FoundMaterial)
							MaterialInterface = *FoundMaterial;

						// See if we have a replacement material and use it on the mesh instead
						UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(MaterialPathName);
						if (ReplacementMaterial && *ReplacementMaterial)
							MaterialInterface = *ReplacementMaterial;

						// Add the material to the Static mesh
						//int32 UnrealMatIndex = SplitMaterials.Add(Material);
						int32 UnrealMatIndex = FoundStaticMesh->StaticMaterials.Add(MaterialInterface);

						// Map the houdini ID to the unreal one
						MapHoudiniMatIdToUnrealIndex.Add(MaterialId, UnrealMatIndex);

						// Update the face index
						SplitFaceMaterialIndices[FaceIdx] = UnrealMatIndex;
					}
				}
			}
			else
			{
				// If we have material overrides
				for (int32 FaceIdx = 0; FaceIdx < SplitGroupFaceIndices.Num(); ++FaceIdx)
				{
					int32 SplitFaceIndex = SplitGroupFaceIndices[FaceIdx];

					int32 CurrentFaceMaterialIdx = -1;
					if (PartFaceMaterialOverrides.IsValidIndex(SplitFaceIndex))
					{
						const FString & MaterialName = PartFaceMaterialOverrides[SplitFaceIndex];
						int32 const * FoundFaceMaterialIdx = MapHoudiniMatAttributesToUnrealIndex.Find(MaterialName);
						if (FoundFaceMaterialIdx)
						{
							CurrentFaceMaterialIdx = *FoundFaceMaterialIdx;
						}
						else
						{
							// Try to locate the corresponding material interface
							UMaterialInterface * MaterialInterface = nullptr;
							if (!MaterialName.IsEmpty())
							{
								// Only try to load a material if has a chance to be valid!
								MaterialInterface = Cast< UMaterialInterface >(
									StaticLoadObject(UMaterialInterface::StaticClass(),
										nullptr, *MaterialName, nullptr, LOAD_NoWarn, nullptr));
							}

							if (MaterialInterface)
							{
								// We managed to load the UE4 material
								// Make sure this material is in the assignments before replacing it.
								OutputAssignmentMaterials.Add(MaterialName, MaterialInterface);
								
								// See if we have a replacement material and use it on the mesh instead
								UMaterialInterface * const *ReplacementMaterialInterface = ReplacementMaterials.Find(MaterialName);
								if (ReplacementMaterialInterface && *ReplacementMaterialInterface)
									MaterialInterface = *ReplacementMaterialInterface;

								// Add this material to the map
								CurrentFaceMaterialIdx = FoundStaticMesh->StaticMaterials.Add(FStaticMaterial(MaterialInterface));
								MapHoudiniMatAttributesToUnrealIndex.Add(MaterialName, CurrentFaceMaterialIdx);
							}
						}

						if (CurrentFaceMaterialIdx < 0)
						{
							// The attribute Material or its replacement do not exist
							// See if we can fallback to the Houdini material assigned on the face

							// Get the unreal material corresponding to this houdini one
							HAPI_NodeId MaterialId = PartFaceMaterialIds[SplitFaceIndex];

							// See if we have already treated that material
							int32 const * FoundUnrealMatIndex = MapHoudiniMatIdToUnrealIndex.Find(MaterialId);
							if (FoundUnrealMatIndex)
							{
								// This material has been mapped already, just assign the mat index
								CurrentFaceMaterialIdx = *FoundUnrealMatIndex;
							}
							else
							{
								// If everything else fails, we'll use the default material
								UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

								// We need to add this material to the map
								FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
								FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, MaterialId, MaterialPathName);
								UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
								if (FoundMaterial)
									MaterialInterface = *FoundMaterial;

								// See if we have a replacement material and use it on the mesh instead
								UMaterialInterface * const *ReplacementMaterialInterface = ReplacementMaterials.Find(MaterialPathName);
								if (ReplacementMaterialInterface && *ReplacementMaterialInterface)
									MaterialInterface = *ReplacementMaterialInterface;

								// Add the material to the Static mesh
								CurrentFaceMaterialIdx = FoundStaticMesh->StaticMaterials.Add(FStaticMaterial(MaterialInterface));

								// Map the Houdini ID to the unreal one
								MapHoudiniMatIdToUnrealIndex.Add(MaterialId, CurrentFaceMaterialIdx);
							}
						}
					}

					// Update the Face Material on the mesh
					SplitFaceMaterialIndices[FaceIdx] = CurrentFaceMaterialIdx;
				}
			}

			// Create a Polygon Group for each material slot
			TPolygonGroupAttributesRef<FName> PolygonGroupImportedMaterialSlotNames =
				MeshDescription->PolygonGroupAttributes().GetAttributesRef<FName>(MeshAttribute::PolygonGroup::ImportedMaterialSlotName);

			// We must use the number of assignment materials found to reserve the number of material slots
			// Don't use the SM's StaticMaterials here as we may not reserve enough polygon groups when adding more materials
			int32 NumberOfMaterials = OutputAssignmentMaterials.Num();
			if (NumberOfMaterials <= 0)
			{
				// No materials, create a polygon group for the default one
				const FPolygonGroupID& PolygonGroupID = MeshDescription->CreatePolygonGroup();
				PolygonGroupImportedMaterialSlotNames[PolygonGroupID] = FName(HAPI_UNREAL_DEFAULT_MATERIAL_NAME);
			}
			else
			{
				MeshDescription->ReserveNewPolygonGroups(NumberOfMaterials);
				//for (int32 MatIndex = 0; MatIndex < NumberOfMaterials; ++MatIndex)
				for (auto& CurrentMatAssignement : OutputAssignmentMaterials)
				{
					const FPolygonGroupID& PolygonGroupID = MeshDescription->CreatePolygonGroup();
					PolygonGroupImportedMaterialSlotNames[PolygonGroupID] =
						FName(CurrentMatAssignement.Value ? *(CurrentMatAssignement.Value->GetName()) : *(CurrentMatAssignement.Key));
				}
			}

			HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - Materials in %f seconds."), FPlatformTime::Seconds() - tick);
			tick = FPlatformTime::Seconds();

			//
			// VERTEX INSTANCE ATTRIBUTES
			// NORMALS, TANGENTS, COLORS, UVS, Alpha
			//

			// Extract the normals
			UpdatePartNormalsIfNeeded();
			// Get the normals for this split
			TArray<float> SplitNormals;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoNormals, PartNormals, SplitNormals);

			TVertexInstanceAttributesRef<FVector> VertexInstanceNormals = MeshDescription->VertexInstanceAttributes().GetAttributesRef<FVector>(MeshAttribute::VertexInstance::Normal);

			// No need to read the tangents if we want unreal to recompute them after
			const UHoudiniRuntimeSettings* HoudiniRuntimeSettings = GetDefault<UHoudiniRuntimeSettings>();
			bool bReadTangents = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->RecomputeTangentsFlag != EHoudiniRuntimeSettingsRecomputeFlag::HRSRF_Always : true;

			// Extract the tangents
			TArray<float> SplitTangentU;
			TArray<float> SplitTangentV;
			if (bReadTangents)
			{
				// Extract this part's Tangents if needed
				UpdatePartTangentsIfNeeded();

				// Get the Tangents for this split
				FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
					SplitVertexList, AttribInfoTangentU, PartTangentU, SplitTangentU);

				// Get the binormals for this split
				FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
					SplitVertexList, AttribInfoTangentV, PartTangentV, SplitTangentV);

				// We need to manually generate tangents if:
				// - we have normals but dont have tangentu or tangentv attributes
				// - we have not specified that we wanted unreal to generate them
				int32 NormalCount = SplitNormals.Num();
				bool bGenerateTangents = (NormalCount > 0) && (SplitTangentU.Num() <= 0 || SplitTangentV.Num() <= 0);
				// Check that the number of tangents read matches the number of normals
				if (SplitTangentU.Num() != NormalCount || SplitTangentV.Num() != NormalCount)
					bGenerateTangents = true;

				if (bGenerateTangents && (HoudiniRuntimeSettings->RecomputeTangentsFlag == EHoudiniRuntimeSettingsRecomputeFlag::HRSRF_Always))
				{
					// No need to generate tangents if we want unreal to recompute them after
					bGenerateTangents = false;
				}

				// Generate the tangents if needed
				if (bGenerateTangents)
				{
					SplitTangentU.SetNumZeroed(NormalCount);
					SplitTangentV.SetNumZeroed(NormalCount);
					for (int32 Idx = 0; Idx + 2 < NormalCount; Idx += 3)
					{
						FVector TangentZ;
						TangentZ.X = SplitNormals[Idx + 0];
						TangentZ.Y = SplitNormals[Idx + 2];
						TangentZ.Z = SplitNormals[Idx + 1];

						FVector TangentX, TangentY;
						TangentZ.FindBestAxisVectors(TangentX, TangentY);

						SplitTangentU[Idx + 0] = TangentX.X;
						SplitTangentU[Idx + 2] = TangentX.Y;
						SplitTangentU[Idx + 1] = TangentX.Z;

						SplitTangentV[Idx + 0] = TangentY.X;
						SplitTangentV[Idx + 2] = TangentY.Y;
						SplitTangentV[Idx + 1] = TangentY.Z;
					}
				}
			}
			TVertexInstanceAttributesRef<FVector> VertexInstanceTangents = MeshDescription->VertexInstanceAttributes().GetAttributesRef<FVector>(MeshAttribute::VertexInstance::Tangent);
			TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = MeshDescription->VertexInstanceAttributes().GetAttributesRef<float>(MeshAttribute::VertexInstance::BinormalSign);

			// Extract the color values
			UpdatePartColorsIfNeeded();
			// Get the colors values for this split
			TArray<float> SplitColors;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoColors, PartColors, SplitColors);

			// Extract the alpha values
			UpdatePartAlphasIfNeeded();
			// Get the colors values for this split
			TArray<float> SplitAlphas;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoAlpha, PartAlphas, SplitAlphas);
			TVertexInstanceAttributesRef<FVector4> VertexInstanceColors = MeshDescription->VertexInstanceAttributes().GetAttributesRef<FVector4>(MeshAttribute::VertexInstance::Color);

			// Extract UVs
			UpdatePartUVSetsIfNeeded(true);
			// See if we need to transfer uv point attributes to vertex attributes.
			int32 UVSetCount = PartUVSets.Num();
			TArray<TArray<float>> SplitUVSets;
			SplitUVSets.SetNum(UVSetCount);
			for (int32 TexCoordIdx = 0; TexCoordIdx < UVSetCount; TexCoordIdx++)
			{
				FHoudiniMeshTranslator::TransferPartAttributesToSplit<float>(
					SplitVertexList, AttribInfoUVSets[TexCoordIdx], PartUVSets[TexCoordIdx], SplitUVSets[TexCoordIdx]);
			}
			TVertexInstanceAttributesRef<FVector2D> VertexInstanceUVs = MeshDescription->VertexInstanceAttributes().GetAttributesRef<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);					
			VertexInstanceUVs.SetNumIndices(UVSetCount);

			HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - VertexAttr extracted in %f seconds."), FPlatformTime::Seconds() - tick);
			tick = FPlatformTime::Seconds();

			// Allocate space for the vertex instances and polygons
			MeshDescription->ReserveNewVertexInstances(SplitIndices.Num());
			MeshDescription->ReserveNewPolygons(SplitIndices.Num() / 3);
			//Approximately 2.5 edges per polygons
			MeshDescription->ReserveNewEdges(SplitIndices.Num() * 2.5f / 3);

			bHasNormal = SplitNormals.Num() > 0;
			bHasTangents = SplitTangentU.Num() > 0 && SplitTangentV.Num() > 0;
			bool bHasRGB = SplitColors.Num() > 0;
			bool bHasRGBA = bHasRGB && AttribInfoColors.tupleSize == 4;
			bool bHasAlpha = SplitAlphas.Num() > 0;

			TArray<bool> HasUVSets;
			HasUVSets.SetNumZeroed(PartUVSets.Num());
			for (int32 Idx = 0; Idx < PartUVSets.Num(); Idx++)
				HasUVSets[Idx] = PartUVSets[Idx].Num() > 0;

			uint32 FaceCount = SplitIndices.Num() / 3;
			for (uint32 FaceIndex = 0; FaceIndex < FaceCount; FaceIndex++)
			{
				TArray<FVertexInstanceID> FaceVertexInstanceIDs;
				FaceVertexInstanceIDs.SetNum(3);

				// Ignore degenerate triangles
				FVertexID VertexIDs[3];
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					VertexIDs[Corner] = FVertexID(SplitIndices[(FaceIndex * 3) + Corner]);
				}
				if (VertexIDs[0] == VertexIDs[1] || VertexIDs[0] == VertexIDs[2] || VertexIDs[1] == VertexIDs[2])
					continue;

				//FVertexID FaceVertexIDs[3];
				for (int32 Corner = 0; Corner < 3; Corner++)
				{
					uint32 SplitIndex = (FaceIndex * 3) + Corner;
					uint32 SplitVertexIndex = SplitIndices[SplitIndex];
					const FVertexInstanceID& VertexInstanceID = MeshDescription->CreateVertexInstance(FVertexID(SplitVertexIndex));

					// Fix the winding order by updating the SplitIndex (invert corner 1 and 2)
					// instead of going 0 1 2 go 0 2 1
					// TODO; this slows down StaticMesh->Build() considerably!
					Corner == 1 ? SplitIndex++ : Corner == 2 ? SplitIndex-- : SplitIndex;

					const uint32 SplitVertexIndex_X = SplitIndex * 3 + 0;
					const uint32 SplitVertexIndex_Y = SplitIndex * 3 + 2;
					const uint32 SplitVertexIndex_Z = SplitIndex * 3 + 1;
					// Normals
					if (bHasNormal)
					{
						// We need to swap Z and Y coordinate here, and convert from m to cm. 
						VertexInstanceNormals[VertexInstanceID].X = SplitNormals[SplitVertexIndex_X];
						VertexInstanceNormals[VertexInstanceID].Y = SplitNormals[SplitVertexIndex_Y];
						VertexInstanceNormals[VertexInstanceID].Z = SplitNormals[SplitVertexIndex_Z];
					}

					// Tangents and binormals
					if (bHasTangents)
					{
						// We need to swap Z and Y coordinate here, and convert from m to cm.
						VertexInstanceTangents[VertexInstanceID].X = SplitTangentU[SplitVertexIndex_X];
						VertexInstanceTangents[VertexInstanceID].Y = SplitTangentU[SplitVertexIndex_Y];
						VertexInstanceTangents[VertexInstanceID].Z = SplitTangentU[SplitVertexIndex_Z];

						FVector TangentY;
						TangentY.X = SplitTangentV[SplitVertexIndex_X];
						TangentY.Y = SplitTangentV[SplitVertexIndex_Y];
						TangentY.Z = SplitTangentV[SplitVertexIndex_Z];

						VertexInstanceBinormalSigns[VertexInstanceID] = GetBasisDeterminantSign(
							VertexInstanceTangents[VertexInstanceID].GetSafeNormal(),
							TangentY.GetSafeNormal(),
							VertexInstanceNormals[VertexInstanceID].GetSafeNormal());
					}

					// Color
					FLinearColor Color = FLinearColor::White;
					if (bHasRGB)
					{
						Color.R = FMath::Clamp(
							SplitColors[SplitIndex * AttribInfoColors.tupleSize + 0], 0.0f, 1.0f);
						Color.G = FMath::Clamp(
							SplitColors[SplitIndex * AttribInfoColors.tupleSize + 1], 0.0f, 1.0f);
						Color.B = FMath::Clamp(
							SplitColors[SplitIndex * AttribInfoColors.tupleSize + 2], 0.0f, 1.0f);
					}
					// Alpha
					if (bHasAlpha)
					{
						Color.A = FMath::Clamp(SplitAlphas[SplitIndex], 0.0f, 1.0f);
					}
					else if (bHasRGBA)
					{
						Color.A = FMath::Clamp(SplitColors[SplitIndex * AttribInfoColors.tupleSize + 3], 0.0f, 1.0f);
					}
					VertexInstanceColors[VertexInstanceID] = FVector4(Color);

					// UVs
					for (int32 UVIndex = 0; UVIndex < SplitUVSets.Num(); UVIndex++)
					{
						if (HasUVSets[UVIndex])
						{
							// We need to flip V coordinate when it's coming from HAPI.
							FVector2D CurrentUV;
							CurrentUV.X = SplitUVSets[UVIndex][SplitIndex * 2 + 0];
							CurrentUV.Y = 1.0f - SplitUVSets[UVIndex][SplitIndex * 2 + 1];

							VertexInstanceUVs.Set(VertexInstanceID, UVIndex, CurrentUV);
						}
					}

					FaceVertexInstanceIDs[Corner] = VertexInstanceID;
				}

				const FPolygonGroupID PolygonGroupID(SplitFaceMaterialIndices[FaceIndex]);

				// Insert a triangle into the mesh
				MeshDescription->CreateTriangle(PolygonGroupID, FaceVertexInstanceIDs);
			}

			HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - VertexAttr filled in %f seconds."), FPlatformTime::Seconds() - tick);
			tick = FPlatformTime::Seconds();

			//--------------------------------------------------------------------------------------------------------------------- 
			//  FACE SMOOTHING
			//---------------------------------------------------------------------------------------------------------------------

			// Extract this part's FaceSmoothing values if needed
			UpdatePartFaceSmoothingIfNeeded();

			// Get the FaceSmoothing values for this split
			TArray<int32> SplitFaceSmoothingMasks;
			FHoudiniMeshTranslator::TransferPartAttributesToSplit<int32>(
				SplitVertexList, AttribInfoFaceSmoothingMasks, PartFaceSmoothingMasks, SplitFaceSmoothingMasks);

			// FaceSmoothing masks must be initialized even if we don't have a value from Houdini!
			// TODO: Expose the default FaceSmoothing value
			// 0 will make hard face
			TArray<uint32> FaceSmoothingMasks;
			FaceSmoothingMasks.Init(DefaultMeshSmoothing, SplitVertexCount / 3);

			// Check that the number of face smoothing values we retrieved is correct
			int32 WedgeFaceSmoothCount = SplitFaceSmoothingMasks.Num() / 3;
			if (SplitFaceSmoothingMasks.Num() != 0 && !SplitFaceSmoothingMasks.IsValidIndex((WedgeFaceSmoothCount - 1) * 3 + 2))
			{
				// Ignore our face smoothing values
				WedgeFaceSmoothCount = 0;
				HOUDINI_LOG_WARNING(TEXT("Invalid face smoothing mask count detected - Skipping them."));
			}

			// Transfer the face smoothing masks to the raw mesh if we have any
			for (int32 WedgeFaceSmoothIdx = 0; WedgeFaceSmoothIdx < WedgeFaceSmoothCount; WedgeFaceSmoothIdx += 3)
			{
				FaceSmoothingMasks[WedgeFaceSmoothIdx] = SplitFaceSmoothingMasks[WedgeFaceSmoothIdx * 3];
			}

			// TODO
			// Check
			FStaticMeshOperations::ConvertSmoothGroupToHardEdges(FaceSmoothingMasks, *MeshDescription);

			HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - FaceSoothing filled in %f seconds."), FPlatformTime::Seconds() - tick);
			tick = FPlatformTime::Seconds();

			//--------------------------------------------------------------------------------------------------------------------- 
			// LIGHTMAP RESOLUTION
			//--------------------------------------------------------------------------------------------------------------------- 
			// Extract this part's LightmapResolution values if needed
			UpdatePartLightmapResolutionsIfNeeded();

			// make sure the mesh has a new lighting guid
			FoundStaticMesh->LightingGuid = FGuid::NewGuid();
		}

		// Update the Build Settings using the default setting values
		SetMeshBuildSettings(
			SrcModel->BuildSettings,
			bHasNormal,
			bHasTangents,
			PartUVSets.Num() > 0);

		// Set the lightmap Coordinate Index
		// If we have more than one UV set, the 2nd valid set is used for lightmaps by convention
		FoundStaticMesh->LightMapCoordinateIndex = PartUVSets.Num() > 1 ? 1 : 0;

		// Check for a lightmapa resolution override
		int32 LightMapResolutionOverride = -1;
		if ( PartLightMapResolutions.Num() > 0)
			LightMapResolutionOverride = PartLightMapResolutions[0];

		if (LightMapResolutionOverride > 0)
			FoundStaticMesh->LightMapResolution = LightMapResolutionOverride;
		else
			FoundStaticMesh->LightMapResolution = 64;

		// TODO:
		// Turnoff bGenerateLightmapUVs if lightmap uv sets has bad uvs ?

		// By default the distance field resolution should be set to 2.0
		// TODO should come from the HAC
		//SrcModel->BuildSettings.DistanceFieldResolutionScale = 2.0;

		// RAW MESH CHECKS

		// TODO: Check not needed w/ FMeshDesc
		// This is required due to the impeding deprecation of FRawMesh
		// If we dont update this UE4 will crash upon deleting an asset.
		//SrcModel->StaticMeshOwner = FoundStaticMesh;

		// Store the new MeshDescription
		FoundStaticMesh->CommitMeshDescription(LODIndex);
		//Set the Imported version before calling the build
		FoundStaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;
		
		// LOD Screensize
		// default values has already been set, see if we have any attribute override for this
		float screensize = GetLODSCreensizeForSplit(SplitGroupName);
		if (screensize >= 0.0f)
		{
			// Only apply the LOD screensize if it's valid
			SrcModel->ScreenSize = screensize;
			//FoundStaticMesh->GetSourceModel(LodIndex).ScreenSize = screensize;
			FoundStaticMesh->bAutoComputeLODScreenSize = false;
		}

		// SET STATIC MESH GENERATION PARAM
		// HANDLE COLLIDERS
		// REMOVE OLD COLLIDERS
		// CUSTOM BAKE NAME OVERRIDE
		
		// UPDATE UPROPERTY ATTRIBS
		// Update property attributes on the SM
		TArray<FHoudiniGenericAttribute> PropertyAttributes;
		if (GetGenericPropertiesAttributes(
			HGPO.GeoId, HGPO.PartId,
			AllSplitFirstValidVertexIndex[SplitGroupName],
			AllSplitFirstValidPrimIndex[SplitGroupName],
			PropertyAttributes))
		{
			UpdateGenericPropertiesAttributes(
				FoundStaticMesh, PropertyAttributes);
		}

		TArray<FString> LevelPaths;
		if (FoundOutputObject && FHoudiniEngineUtils::GetLevelPathAttribute(HGPO.GeoId, HGPO.PartId, LevelPaths))
		{
			if (LevelPaths.Num() > 0 && !LevelPaths[0].IsEmpty())
			{
				// cache the level path attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_LEVEL_PATH, LevelPaths[0]);
			}
		}

		TArray<FString> OutputNames;
		if (FoundOutputObject && FHoudiniEngineUtils::GetOutputNameAttribute(HGPO.GeoId, HGPO.PartId, OutputNames))
		{
			if (OutputNames.Num() > 0 && !OutputNames[0].IsEmpty())
			{
				// cache the output name attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_CUSTOM_OUTPUT_NAME_V2, OutputNames[0]);
			}
		}

		TArray<int32> TileValues;
		if (FoundOutputObject && FHoudiniEngineUtils::GetTileAttribute(HGPO.GeoId, HGPO.PartId, TileValues))
		{
			if (TileValues.Num() > 0 && TileValues[0] >= 0)
			{
				// cache the tile attribute as a token on the output object
				FoundOutputObject->CachedTokens.Add(TEXT("tile"), FString::FromInt(TileValues[0]));
			}
		}

		TArray<FString> BakeOutputActorNames;
		if (FoundOutputObject && FHoudiniEngineUtils::GetBakeActorAttribute(HGPO.GeoId, HGPO.PartId, BakeOutputActorNames))
		{
			if (BakeOutputActorNames.Num() > 0 && !BakeOutputActorNames[0].IsEmpty())
			{
				// cache the bake actor attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_BAKE_ACTOR, BakeOutputActorNames[0]);
			}
		}

		TArray<FString> BakeFolders;
		if (FoundOutputObject && FHoudiniEngineUtils::GetBakeFolderAttribute(HGPO.GeoId, BakeFolders, HGPO.PartId))
		{
			if (BakeFolders.Num() > 0 && !BakeFolders[0].IsEmpty())
			{
				// cache the unreal_bake_folder attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_BAKE_FOLDER, BakeFolders[0]);
			}
		}

		TArray<FString> BakeOutlinerFolders;
		if (FoundOutputObject && FHoudiniEngineUtils::GetBakeOutlinerFolderAttribute(HGPO.GeoId, HGPO.PartId, BakeOutlinerFolders))
		{
			if (BakeOutlinerFolders.Num() > 0 && !BakeOutlinerFolders[0].IsEmpty())
			{
				// cache the bake actor attribute on the output object
				FoundOutputObject->CachedAttributes.Add(HAPI_UNREAL_ATTRIB_BAKE_OUTLINER_FOLDER, BakeOutlinerFolders[0]);
			}
		}

		// Notify that we created a new Static Mesh if needed
		if(bNewStaticMeshCreated)
			FAssetRegistryModule::AssetCreated(FoundStaticMesh);

		// Add the Static mesh to the output maps and the build map if we haven't already
		if (FoundOutputObject)
		{
			FoundOutputObject->OutputObject = FoundStaticMesh;
			FoundOutputObject->bProxyIsCurrent = false;
			OutputObjects.FindOrAdd(OutputObjectIdentifier, *FoundOutputObject);
		}

		StaticMeshToBuild.FindOrAdd(OutputObjectIdentifier, FoundStaticMesh);

		HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() - Finished MD in %f seconds."), FPlatformTime::Seconds() - tick);
		tick = FPlatformTime::Seconds();
	}

	// Look if we only have colliders
	// If we do, we'll allow attaching sockets to the collider meshes
	bool bCollidersOnly = true;
	for (auto& Current : StaticMeshToBuild)
	{
		EHoudiniSplitType CurrentSplitType = GetSplitTypeFromSplitName(Current.Key.SplitIdentifier);
		if (CurrentSplitType == EHoudiniSplitType::LOD || CurrentSplitType == EHoudiniSplitType::Normal)
		{
			bCollidersOnly = false;
			break;
		}
	}

	FHoudiniScopedGlobalSilence ScopedGlobalSilence;
	for (auto& Current : StaticMeshToBuild)
	{
		UStaticMesh* SM = Current.Value;
		if (!SM || SM->IsPendingKill())
			continue;
		
		UBodySetup * BodySetup = SM->BodySetup;
		if (!BodySetup)
		{
			SM->CreateBodySetup();
			BodySetup = SM->BodySetup;
		}

		EHoudiniSplitType SplitType = GetSplitTypeFromSplitName(Current.Key.SplitIdentifier);

		// Handle the Static Mesh's colliders
		if (BodySetup && !BodySetup->IsPendingKill())
		{
			// Make sure rendering is done - so we are not changing data being used by collision drawing.
			FlushRenderingCommands();

			// Clean up old colliders from a previous cook
			BodySetup->Modify();
			BodySetup->RemoveSimpleCollision();
			// Create new GUID
			BodySetup->InvalidatePhysicsData();

			FHoudiniOutputObjectIdentifier CurrentObjId = Current.Key;
			FKAggregateGeom* CurrentAggColl = AllAggregateCollisions.Find(Current.Key);
			if (CurrentAggColl && CurrentAggColl->GetElementCount() > 0)
			{
				BodySetup->AddCollisionFrom(*CurrentAggColl);
				BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseDefault;
			}

			// Moved RefreshCollisionChange to after the SM->Build call
			// RefreshCollisionChange(*SM);
			SM->bCustomizedCollision = true;

			// See if we need to enable collisions on the whole mesh
			if (SplitType == EHoudiniSplitType::InvisibleComplexCollider || SplitType == EHoudiniSplitType::RenderedComplexCollider)
			{
				// Complex collider, enable collisions for this static mesh.
				BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;
			}
			else
			{
				// TODO
				// if the LODForCollision uproperty attribute is set, we need to activate complex collision
				// on the static mesh for that lod to be picked up properly as a collider
				if ( FHoudiniEngineUtils::HapiCheckAttributeExists(	HGPO.GeoId, HGPO.PartId, 
					"unreal_uproperty_LODForCollision", HAPI_ATTROWNER_DETAIL))
				{
					BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;
				}
			}
		}

		// Add the Sockets to the StaticMesh
		// We only add them to the main geo, or to the colliders if we only generate colliders
		bool bAddSocket = SplitType == EHoudiniSplitType::Normal ? true : bCollidersOnly ? true : false;
		if (bAddSocket)
		{
			if (!FHoudiniEngineUtils::AddMeshSocketsToStaticMesh(SM, AllSockets, true))
			{
				HOUDINI_LOG_WARNING(TEXT("Failed to import sockets for StaticMesh %s."), *(SM->GetName()));
			}
		}

		// BUILD the Static Mesh
		// bSilent doesnt add the Build Errors...
		double build_start = FPlatformTime::Seconds();
		TArray<FText> SMBuildErrors;
		SM->Build(true, &SMBuildErrors);
		double build_end = FPlatformTime::Seconds();
		HOUDINI_LOG_MESSAGE(TEXT("StaticMesh->Build() executed in %f seconds."), build_end - build_start);

		// TODO: copied the content of RefreshCollision below and commented out CreateNavCollision
		// it is already called by UStaticMesh::PostBuildInternal as part of the ::Build call,
		// and can be expensive depending on the vert/poly count of the mesh
		// TODO: also moved this to after the call to Build, since Build updates the mesh's
		// physics state (calling this before Build when rebuilding an existing high poly mesh as 
		// low poly mesh, incurs quite a performance hit. This is likely due to processing of physics
		// meshes with high vert/poly count before the Build
		// RefreshCollisionChange(*SM);
		{
			// SM->CreateNavCollision(/*bIsUpdate=*/true);

			for (FObjectIterator Iter(UStaticMeshComponent::StaticClass()); Iter; ++Iter)
			{
				UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(*Iter);
				if (StaticMeshComponent->GetStaticMesh() == SM)
				{
					// it needs to recreate IF it already has been created
					if (StaticMeshComponent->IsPhysicsStateCreated())
					{
						StaticMeshComponent->RecreatePhysicsState();
					}
				}
			}

			FEditorSupportDelegates::RedrawAllViewports.Broadcast();
		}

		SM->GetOnMeshChanged().Broadcast();
		/*
		// Try to find the outer package so we can dirty it up
		if (SM->GetOuter())
		{
			SM->GetOuter()->MarkPackageDirty();
		}
		else
		{
			SM->MarkPackageDirty();
		}
		*/

		UPackage* MeshPackage = SM->GetOutermost();
		if (MeshPackage && !MeshPackage->IsPendingKill())
		{
			MeshPackage->MarkPackageDirty();
			/*
			// DPT: deactivated auto saving mesh/material package
			// only dirty for now, as we'll save them when saving the world.
			TArray<UPackage*> PackageToSave;
			PackageToSave.Add(MeshPackage);

			// Save the created package
			FEditorFileUtils::PromptForCheckoutAndSave(PackageToSave, false, false);
			*/
		}
	}

	// TODO: Still necessary ? SM->Build should actually update the navmesh...
	// TODO: Commented out for now, since it appears that the content of the loop is
	// already called in UStaticMesh::BuildInternal and UStaticMesh::PostBuildInternal
	//// Now that all the meshes are built and their collisions meshes and primitives updated,
	//// we need to update their pre-built navigation collision used by the navmesh
	//for (auto& Iter : OutputObjects)
	//{
	//	UStaticMesh* StaticMesh = Cast<UStaticMesh>(Iter.Value.OutputObject);
	//	if (!StaticMesh || StaticMesh->IsPendingKill())
	//		continue;

	//	UBodySetup * BodySetup = StaticMesh->BodySetup;
	//	if (BodySetup && !BodySetup->IsPendingKill() && StaticMesh->NavCollision)
	//	{
	//		// Unreal caches the Navigation Collision and never updates it for StaticMeshes,
	//		// so we need to manually flush and recreate the data to have proper navigation collision
	//		// TODO: Is this still required? These two functions are called by 
	//		// UStaticMesh::BuildInternal, which is called by UStaticMesh::Build/BatchBuild
	//		// BodySetup->InvalidatePhysicsData();
	//		// BodySetup->CreatePhysicsMeshes();

	//		// TODO: Is this still required? This function is called by UStaticMesh::CreateNavCollision 
	//		// which is called by the UStaticMesh::PostBuildInternal function, which is called at the 
	//		// end of the build.
	//		// StaticMesh->NavCollision->Setup(BodySetup);
	//	}
	//}

	double time_end = FPlatformTime::Seconds();
	HOUDINI_LOG_MESSAGE(TEXT("CreateStaticMesh_MeshDescription() executed in %f seconds."), time_end - time_start);

	return true;
}

bool
FHoudiniMeshTranslator::CreateHoudiniStaticMesh()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh"));

	const double time_start = FPlatformTime::Seconds();

	// Start by updating the vertex list
	if (!UpdatePartVertexList())
		return false;

	// Sort the split groups
	SortSplitGroups();

	// Handles the split groups found in the part
	// and builds the corresponding faces and indices arrays
	if (!UpdateSplitsFacesAndIndices())
		return true;

	// Resets the containers used for the raw data extraction.
	ResetPartCache();

	// Determine if there is "main" geo, if not we'll use the first LOD
	// as main geo
	bool bHasMainGeo = false;
	for (auto& curSplit : AllSplitGroups)
	{
		if (GetSplitTypeFromSplitName(curSplit) == EHoudiniSplitType::Normal)
		{
			bHasMainGeo = true;
			break;
		}
	}

	// Update the part's material's IDS and info now
	//UpdatePartFaceMaterialsIfNeeded();
	CreateNeededMaterials();

	// Check if the materials were updated
	bool bMaterialHasChanged = false;
	for (const auto& MatInfo : PartUniqueMaterialInfos)
	{
		if (MatInfo.hasChanged)
		{
			bMaterialHasChanged = true;
			break;
		}
	}

	// Map of Houdini Material IDs to Unreal Material Indices
	TMap< HAPI_NodeId, int32 > MapHoudiniMatIdToUnrealIndex;
	// Map of Houdini Material Attributes to Unreal Material Indices
	TMap< FString, int32 > MapHoudiniMatAttributesToUnrealIndex;

	bool MeshMaterialsHaveBeenReset = false;

	double tick = FPlatformTime::Seconds();
	HOUDINI_LOG_MESSAGE(TEXT("CreateHoudiniStaticMesh() - Pre Split-Loop in %f seconds."), tick - time_start);

	// Iterate through all detected split groups we care about and split geometry.
	bool bMainGeoOrFirstLODFound = false;
	for (int32 SplitId = 0; SplitId < AllSplitGroups.Num(); SplitId++)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Per Split"));

		// Get split group name
		const FString& SplitGroupName = AllSplitGroups[SplitId];

		// Get the current split type
		EHoudiniSplitType SplitType = GetSplitTypeFromSplitName(SplitGroupName);
		if (SplitType == EHoudiniSplitType::Invalid)
		{
			// Invalid split, skip
			HOUDINI_LOG_WARNING(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] unknown split type.")
				TEXT("- skipping."),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
			continue;
		}

		// We are only interested in the Normal/main geo and visible colliders
		if (SplitType != EHoudiniSplitType::Normal &&
			SplitType != EHoudiniSplitType::LOD &&
			SplitType != EHoudiniSplitType::RenderedComplexCollider &&
			SplitType != EHoudiniSplitType::RenderedSimpleCollider &&
			SplitType != EHoudiniSplitType::RenderedUCXCollider)
		{
			continue;
		}

		// We only use LOD if there is no Normal geo
		if (SplitType == EHoudiniSplitType::Normal)
		{
			bMainGeoOrFirstLODFound = true;
			HOUDINI_LOG_MESSAGE(TEXT("Found Normal geo for mesh."));
		}
		else if (SplitType == EHoudiniSplitType::LOD)
		{
			if (bHasMainGeo)
			{
				HOUDINI_LOG_MESSAGE(TEXT("Skipping LOD since the mesh has Normal geo."));
				continue;
			}
			else if (bMainGeoOrFirstLODFound)
			{
				HOUDINI_LOG_MESSAGE(TEXT("Skipping LOD since we have already processed the first LOD."));
				continue;
			}
			else
			{
				bMainGeoOrFirstLODFound = true;
				HOUDINI_LOG_MESSAGE(TEXT("Mesh does not have Normal geo, found first LOD."));
			}
		}

		// Get the vertex indices for this group
		TArray<int32>& SplitVertexList = AllSplitVertexLists[SplitGroupName];

		// Get valid count of vertex indices for this split.
		const int32& SplitVertexCount = AllSplitVertexCounts[SplitGroupName];

		// Make sure we have a  valid vertex count for this split
		if (SplitVertexCount % 3 != 0 || SplitVertexList.Num() % 3 != 0)
		{
			// Invalid vertex count, skip this split or we'd crash trying to create a mesh for it.
			HOUDINI_LOG_WARNING(
				TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] invalid vertex count.")
				TEXT("- skipping."),
				HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);

			continue;
		}

		// Get the output identifer for this split
		FHoudiniOutputObjectIdentifier OutputObjectIdentifier(
			HGPO.ObjectId, HGPO.GeoId, HGPO.PartId, GetMeshIdentifierFromSplit(SplitGroupName, SplitType));
		OutputObjectIdentifier.PartName = HGPO.PartName;
		OutputObjectIdentifier.PrimitiveIndex = AllSplitFirstValidVertexIndex[SplitGroupName],
			OutputObjectIdentifier.PointIndex = AllSplitFirstValidPrimIndex[SplitGroupName];

		// Try to find existing properties for this identifier
		FHoudiniOutputObject* FoundOutputObject = InputObjects.Find(OutputObjectIdentifier);
		// Try to find an existing DM from a previous cook
		UHoudiniStaticMesh* FoundStaticMesh = FindExistingHoudiniStaticMesh(OutputObjectIdentifier);

		// Flag whether or not we need to rebuild the mesh
		bool bRebuildStaticMesh = false;
		if (HGPO.GeoInfo.bHasGeoChanged || HGPO.PartInfo.bHasChanged || ForceRebuild || !FoundStaticMesh || !FoundOutputObject)
			bRebuildStaticMesh = true;

		// TODO: Handle materials
		if (!bRebuildStaticMesh && !bMaterialHasChanged)
		{
			// We can simply reuse the found static mesh
			OutputObjects.Add(OutputObjectIdentifier, *FoundOutputObject);
			continue;
		}

		bool bNewStaticMeshCreated = false;
		if (!FoundStaticMesh)
		{
			// If we couldn't find a valid existing dynamic mesh, create a new one
			FoundStaticMesh = CreateNewHoudiniStaticMesh(OutputObjectIdentifier.SplitIdentifier);
			if (!FoundStaticMesh || FoundStaticMesh->IsPendingKill())
				continue;

			bNewStaticMeshCreated = true;
		}

		if (!FoundOutputObject)
		{
			// If we couldnt find a previous output object, create a new one
			FHoudiniOutputObject NewOutputObject;
			FoundOutputObject = &OutputObjects.Add(OutputObjectIdentifier, NewOutputObject);
		}
		FoundOutputObject->bProxyIsCurrent = true;

		HOUDINI_LOG_MESSAGE(TEXT("CreateHoudiniStaticMesh() - PreBuildMesh in %f seconds."), FPlatformTime::Seconds() - tick);
		tick = FPlatformTime::Seconds();

		if (bRebuildStaticMesh)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Build/Rebuild UHoudiniStaticMesh"));

			//--------------------------------------------------------------------------------------------------------------------- 
			//  INDICES
			//--------------------------------------------------------------------------------------------------------------------- 

			//
			// Because of the splits, we don't need to declare all the vertices in the Part, 
			// but only the one that are currently used by the split's faces.
			// The indicesMapper array is used to map those indices from Part Vertices to Split Vertices.
			// We also keep track of the needed vertices index to declare them easily afterwards.
			//

			// IndicesMapper:
			// Maps index values for all vertices in the Part:
			// - Vertices unused by the split will be set to -1
			// - Used vertices will have their value set to the "NewIndex"
			// So that IndicesMapper[ oldIndex ] => newIndex
			TArray<int32> IndicesMapper;
			IndicesMapper.Init(-1, SplitVertexList.Num());
			int32 CurrentMapperIndex = 0;

			// NeededVertices:
			// Array containing the old index of the needed vertices for the current split
			// NeededVertices[ newIndex ] => oldIndex
			TArray< int32 > NeededVertices;
			NeededVertices.Reserve(SplitVertexList.Num() / 3);
			TArray< int32 > TriangleIndices;
			TriangleIndices.Reserve(SplitVertexList.Num());

			{
				TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Build IndicesMapper and NeededVertices"));

				int32 ValidVertexId = 0;
				for (int32 VertexIdx = 0; VertexIdx < SplitVertexList.Num(); VertexIdx += 3)
				{
					int32 WedgeCheck = SplitVertexList[VertexIdx + 0];
					if (WedgeCheck == -1)
						continue;

					int32 WedgeIndices[3] =
					{
						SplitVertexList[VertexIdx + 0],
						SplitVertexList[VertexIdx + 1],
						SplitVertexList[VertexIdx + 2]
					};

					// Ensure the indices are valid
					if (!IndicesMapper.IsValidIndex(WedgeIndices[0])
						|| !IndicesMapper.IsValidIndex(WedgeIndices[1])
						|| !IndicesMapper.IsValidIndex(WedgeIndices[2]))
					{
						// Invalid face index.
						HOUDINI_LOG_MESSAGE(
							TEXT("Creating Dynamic Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] has some invalid face indices"),
							HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
						continue;
					}

					// Converting Old (Part) Indices to New (Split) Indices:
					for (int32 i = 0; i < 3; i++)
					{
						if (IndicesMapper[WedgeIndices[i]] < 0)
						{
							// This old index has not yet been "converted" to a new index
							NeededVertices.Add(WedgeIndices[i]);
							IndicesMapper[WedgeIndices[i]] = CurrentMapperIndex;
							CurrentMapperIndex++;
						}

						// Replace the old index with the new one
						WedgeIndices[i] = IndicesMapper[WedgeIndices[i]];
					}

					// Flip wedge indices to fix the winding order.
					TriangleIndices.Add(WedgeIndices[0]);
					TriangleIndices.Add(WedgeIndices[2]);
					TriangleIndices.Add(WedgeIndices[1]);

					ValidVertexId += 3;
				}
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			// NORMALS 
			//--------------------------------------------------------------------------------------------------------------------- 

			// Extract this part's normal if needed
			UpdatePartNormalsIfNeeded();

			// Get the normals for this split
			TArray<float> SplitNormals;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoNormals, PartNormals, SplitNormals);

			// Check that the number of normal we retrieved is correct
			int32 NormalCount = SplitNormals.Num() / 3;
			if (NormalCount < 0 || NormalCount < NeededVertices.Num())
			{
				// Ignore normals
				NormalCount = 0;
				HOUDINI_LOG_WARNING(TEXT("Invalid normal count detected - Skipping normals."));
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			// TANGENTS
			//--------------------------------------------------------------------------------------------------------------------- 

			TArray<float> SplitTangentU;
			TArray<float> SplitTangentV;
			int32 TangentUCount = 0;
			int32 TangentVCount = 0;
			// No need to read the tangents if we want unreal to recompute them after		
			const UHoudiniRuntimeSettings* HoudiniRuntimeSettings = GetDefault<UHoudiniRuntimeSettings>();
			bool bReadTangents = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->RecomputeTangentsFlag != EHoudiniRuntimeSettingsRecomputeFlag::HRSRF_Always : true;

			bool bGenerateTangents = bReadTangents;
			if (bReadTangents)
			{
				// Extract this part's Tangents if needed
				UpdatePartTangentsIfNeeded();

				// Get the Tangents for this split
				FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
					SplitVertexList, AttribInfoTangentU, PartTangentU, SplitTangentU);

				// Get the binormals for this split
				FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
					SplitVertexList, AttribInfoTangentV, PartTangentV, SplitTangentV);

				// We need to manually generate tangents if:
				// - we have normals but dont have tangentu or tangentv attributes
				// - we have not specified that we wanted unreal to generate them
				bGenerateTangents = (SplitNormals.Num() > 0) && (SplitTangentU.Num() <= 0 || SplitTangentV.Num() <= 0);

				// Check that the number of tangents read matches the number of normals
				TangentUCount = SplitTangentU.Num() / 3;
				TangentVCount = SplitTangentV.Num() / 3;
				if (TangentUCount != NormalCount || TangentVCount != NormalCount)
				{
					HOUDINI_LOG_MESSAGE(TEXT("CreateHoudiniStaticMesh: Generate tangents due to count mismatch (# U Tangents = %d; # V Tangents = %d; # Normals = %d)"), TangentUCount, TangentVCount, NormalCount);
					bGenerateTangents = true;
				}

				if (bGenerateTangents && (HoudiniRuntimeSettings->RecomputeTangentsFlag == EHoudiniRuntimeSettingsRecomputeFlag::HRSRF_Always))
				{
					// No need to generate tangents if we want unreal to recompute them after
					bGenerateTangents = false;
				}
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			//  VERTEX COLORS AND ALPHAS
			//---------------------------------------------------------------------------------------------------------------------

			// Extract this part's colors if needed
			UpdatePartColorsIfNeeded();

			// Get the colors values for this split
			TArray<float> SplitColors;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoColors, PartColors, SplitColors);

			// Extract this part's alpha values if needed
			UpdatePartAlphasIfNeeded();

			// Get the colors values for this split
			TArray<float> SplitAlphas;
			FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
				SplitVertexList, AttribInfoAlpha, PartAlphas, SplitAlphas);

			const int32 ColorsCount = AttribInfoColors.exists ? SplitColors.Num() / AttribInfoColors.tupleSize : 0;
			const bool bSplitColorValid = AttribInfoColors.exists && (AttribInfoColors.tupleSize >= 3) && ColorsCount > 0;
			const bool bSplitAlphaValid = AttribInfoAlpha.exists && (SplitAlphas.Num() == ColorsCount);

			//--------------------------------------------------------------------------------------------------------------------- 
			//  UVS
			//--------------------------------------------------------------------------------------------------------------------- 

			// Extract this part's UV sets if needed
			UpdatePartUVSetsIfNeeded();

			// See if we need to transfer uv point attributes to vertex attributes.
			int32 NumUVLayers = 0;
			TArray<TArray<float>> SplitUVSets;
			SplitUVSets.SetNum(MAX_STATIC_TEXCOORDS);
			for (int32 TexCoordIdx = 0; TexCoordIdx < MAX_STATIC_TEXCOORDS; ++TexCoordIdx)
			{
				FHoudiniMeshTranslator::TransferPartAttributesToSplit<float>(
					SplitVertexList, AttribInfoUVSets[TexCoordIdx], PartUVSets[TexCoordIdx], SplitUVSets[TexCoordIdx]);
				if (SplitUVSets[TexCoordIdx].Num() > 0)
				{
					NumUVLayers++;
				}
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			// MATERIAL ATTRIBUTE OVERRIDES
			//---------------------------------------------------------------------------------------------------------------------

			// TODO: These are actually per faces, not per vertices...
			// Need to update!!
			UpdatePartFaceMaterialOverridesIfNeeded();

			//
			// Initialize mesh
			// 
			const int32 NumVertexPositions = NeededVertices.Num();
			const int32 NumTriangles = TriangleIndices.Num() / 3;
			const bool bHasPerFaceMaterials = PartFaceMaterialOverrides.Num() > 0 || (PartUniqueMaterialIds.Num() > 0 && !bOnlyOneFaceMaterial);

			FoundStaticMesh->Initialize(
				NumVertexPositions,
				NumTriangles,
				NumUVLayers,					   // NumUVLayers
				0,								   // InitialNumStaticMaterials
				NormalCount > 0,				   // HasNormals
				NormalCount > 0 && bReadTangents,  // HasTangents
				bSplitColorValid,				   // HasColors
				bHasPerFaceMaterials			   // HasPerFaceMaterials
			);

			//--------------------------------------------------------------------------------------------------------------------- 
			// POSITIONS
			//--------------------------------------------------------------------------------------------------------------------- 
			UpdatePartPositionIfNeeded();

			//
			// Transfer vertex positions:
			//
			// Because of the split, we're only interested in the needed vertices.
			// Instead of declaring all the Positions, we'll only declare the vertices
			// needed by the current split.
			//
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Set Vertex Positions"));

				for (int32 VertexPositionIdx = 0; VertexPositionIdx < NumVertexPositions; ++VertexPositionIdx)
				//ParallelFor(NumVertexPositions, [&](uint32 VertexPositionIdx)
				{
					int32 NeededVertexIndex = NeededVertices[VertexPositionIdx];
					if (!PartPositions.IsValidIndex(NeededVertexIndex * 3 + 2))
					{
						// Error retrieving positions.
						HOUDINI_LOG_WARNING(
							TEXT("Creating Dynamic Static Meshes: Object [%d %s], Geo [%d], Part [%d %s], Split [%d %s] invalid position/index data ")
							TEXT("- skipping."),
							HGPO.ObjectId, *HGPO.ObjectName, HGPO.GeoId, HGPO.PartId, *HGPO.PartName, SplitId, *SplitGroupName);
					}

					// We need to swap Z and Y coordinate here, and convert from m to cm. 
					FoundStaticMesh->SetVertexPosition(VertexPositionIdx, FVector(
						PartPositions[NeededVertexIndex * 3 + 0] * HAPI_UNREAL_SCALE_FACTOR_POSITION,
						PartPositions[NeededVertexIndex * 3 + 2] * HAPI_UNREAL_SCALE_FACTOR_POSITION,
						PartPositions[NeededVertexIndex * 3 + 1] * HAPI_UNREAL_SCALE_FACTOR_POSITION
					));
				}//);
			}

			//--------------------------------------------------------------------------------------------------------------------- 
			// FACES / TRIS
			// Now set Normals, UVs and Colors on mesh points and AttributeSet
			//---------------------------------------------------------------------------------------------------------------------

			{
				TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Set Triangle Indices & Per Vertex Instance Attribute Values"));

				// Now add the triangles to the mesh
				for (int32 TriangleIdx = 0; TriangleIdx < NumTriangles; ++TriangleIdx)
				// ParallelFor(NumTriangles, [&](uint32 TriangleIdx)
				{

					const int32 TriVertIdx0 = TriangleIdx * 3;
					FoundStaticMesh->SetTriangleVertexIndices(TriangleIdx, FIntVector(
						TriangleIndices[TriVertIdx0 + 0],
						TriangleIndices[TriVertIdx0 + 1],
						TriangleIndices[TriVertIdx0 + 2]
					));

					const int32 TriWindingIndex[3] = { 0, 2, 1 };
					if (NormalCount > 0 && SplitNormals.IsValidIndex(TriVertIdx0 * 3 + 3 * 3 - 1))
					{
						// Flip Z and Y coordinate for normal, but don't scale
						for (int32 ElementIdx = 0; ElementIdx < 3; ++ElementIdx)
						{
							const FVector Normal(
								SplitNormals[TriVertIdx0 * 3 + 3 * ElementIdx + 0],
								SplitNormals[TriVertIdx0 * 3 + 3 * ElementIdx + 2],
								SplitNormals[TriVertIdx0 * 3 + 3 * ElementIdx + 1]
							);

							FoundStaticMesh->SetTriangleVertexNormal(TriangleIdx, TriWindingIndex[ElementIdx], Normal);

							if (bReadTangents)
							{
								FVector TangentU, TangentV;
								if (bGenerateTangents)
								{
									// Generate the tangents if needed
									Normal.FindBestAxisVectors(TangentU, TangentV);
								}
								else
								{
									// Transfer the tangents from Houdini
									TangentU.X = SplitTangentU[TriVertIdx0 * 3 + 3 * ElementIdx + 0];
									TangentU.Y = SplitTangentU[TriVertIdx0 * 3 + 3 * ElementIdx + 2];
									TangentU.Z = SplitTangentU[TriVertIdx0 * 3 + 3 * ElementIdx + 1];

									TangentU.X = SplitTangentV[TriVertIdx0 * 3 + 3 * ElementIdx + 0];
									TangentU.Y = SplitTangentV[TriVertIdx0 * 3 + 3 * ElementIdx + 2];
									TangentU.Z = SplitTangentV[TriVertIdx0 * 3 + 3 * ElementIdx + 1];
								}

								FoundStaticMesh->SetTriangleVertexUTangent(TriangleIdx, TriWindingIndex[ElementIdx], TangentU);
								FoundStaticMesh->SetTriangleVertexVTangent(TriangleIdx, TriWindingIndex[ElementIdx], TangentV);
							}
						}
					}

					if (bSplitColorValid && SplitColors.IsValidIndex(TriVertIdx0 * AttribInfoColors.tupleSize + 3 * AttribInfoColors.tupleSize - 1))
					{
						FLinearColor VertexLinearColor;
						for (int32 ElementIdx = 0; ElementIdx < 3; ++ElementIdx)
						{
							VertexLinearColor.R = FMath::Clamp(
								SplitColors[TriVertIdx0 * AttribInfoColors.tupleSize + AttribInfoColors.tupleSize * ElementIdx + 0], 0.0f, 1.0f);
							VertexLinearColor.G = FMath::Clamp(
								SplitColors[TriVertIdx0 * AttribInfoColors.tupleSize + AttribInfoColors.tupleSize * ElementIdx + 1], 0.0f, 1.0f);
							VertexLinearColor.B = FMath::Clamp(
								SplitColors[TriVertIdx0 * AttribInfoColors.tupleSize + AttribInfoColors.tupleSize * ElementIdx + 2], 0.0f, 1.0f);

							if (bSplitAlphaValid)
							{
								VertexLinearColor.A = FMath::Clamp(SplitAlphas[TriVertIdx0 + ElementIdx], 0.0f, 1.0f);
							}
							else if (AttribInfoColors.tupleSize >= 4)
							{
								VertexLinearColor.A = FMath::Clamp(
									SplitColors[TriVertIdx0 * AttribInfoColors.tupleSize + AttribInfoColors.tupleSize * ElementIdx + 3], 0.0f, 1.0f);
							}
							else
							{
								VertexLinearColor.A = 1.0f;
							}
							const FColor VertexColor = VertexLinearColor.ToFColor(false);
							FoundStaticMesh->SetTriangleVertexColor(TriangleIdx, TriWindingIndex[ElementIdx], VertexColor);
						}
					}

					if (NumUVLayers > 0)
					{
						// Dynamic mesh supports only 1 UV layer on the mesh it self. So we set the first layer
						// on the mesh itself only, and we set all layers on the AttributeSet
						for (int32 TexCoordIdx = 0; TexCoordIdx < NumUVLayers; ++TexCoordIdx)
						{
							const TArray<float>& SplitUVs = SplitUVSets[TexCoordIdx];
							if (SplitUVs.IsValidIndex(TriVertIdx0 * 2 + 3 * 2 - 1))
							{
								for (int32 ElementIdx = 0; ElementIdx < 3; ++ElementIdx)
								{
									const int32 UVIdx = TriVertIdx0 * 2 + ElementIdx * 2;
									// We need to flip V coordinate when it's coming from HAPI.
									const FVector2D UV(SplitUVs[UVIdx + 0], 1.0f - SplitUVs[UVIdx + 1]);
									// Set the UV on the vertex instance in the UVLayer
									FoundStaticMesh->SetTriangleVertexUV(TriangleIdx, TriWindingIndex[ElementIdx], TexCoordIdx, UV);
								}
							}
						}
					}
				}
			}
		}

		//--------------------------------------------------------------------------------------------------------------------- 
		// MATERIALS / FACE MATERIALS
		//---------------------------------------------------------------------------------------------------------------------

		// Get face indices for this split.
		TArray<int32>& SplitFaceIndices = AllSplitFaceIndices[SplitGroupName];

		// Process material overrides first
		if (PartFaceMaterialOverrides.Num() > 0)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Set Per Face Material Overrides"));

			for (int32 FaceIdx = 0; FaceIdx < SplitFaceIndices.Num(); ++FaceIdx)
			{
				int32 SplitFaceIndex = SplitFaceIndices[FaceIdx];
				if (!PartFaceMaterialOverrides.IsValidIndex(SplitFaceIndex))
					continue;

				const FString & MaterialName = PartFaceMaterialOverrides[SplitFaceIndex];
				int32 const * FoundFaceMaterialIdx = MapHoudiniMatAttributesToUnrealIndex.Find(MaterialName);
				int32 CurrentFaceMaterialIdx = 0;
				if (FoundFaceMaterialIdx)
				{
					// We already know what material index to use for that override
					CurrentFaceMaterialIdx = *FoundFaceMaterialIdx;
				}
				else
				{
					// Try to locate the corresponding material interface
					UMaterialInterface * MaterialInterface = nullptr;

					// Start by looking in our assignment map
					auto FoundMaterialInterface = OutputAssignmentMaterials.Find(MaterialName);
					if (FoundMaterialInterface)
						MaterialInterface = *FoundMaterialInterface;

					if (!MaterialInterface && !MaterialName.IsEmpty())
					{
						// Only try to load a material if has a chance to be valid!
						MaterialInterface = Cast<UMaterialInterface>(
							StaticLoadObject(UMaterialInterface::StaticClass(),
								nullptr, *MaterialName, nullptr, LOAD_NoWarn, nullptr));
					}

					if (MaterialInterface)
					{
						// We managed to load the UE4 material
						// Make sure this material is in the assignments before replacing it.
						OutputAssignmentMaterials.Add(MaterialName, MaterialInterface);

						// See if we have a replacement material and use it on the mesh instead
						UMaterialInterface * const *ReplacementMaterialInterface = ReplacementMaterials.Find(MaterialName);
						if (ReplacementMaterialInterface && *ReplacementMaterialInterface)
							MaterialInterface = *ReplacementMaterialInterface;

						// Add this material to the map
						CurrentFaceMaterialIdx = FoundStaticMesh->AddStaticMaterial(FStaticMaterial(MaterialInterface));
						MapHoudiniMatAttributesToUnrealIndex.Add(MaterialName, CurrentFaceMaterialIdx);
					}
					else
					{
						// The Attribute Material and its replacement do not exist
						// See if we can fallback to the Houdini material assigned on the face

						// Get the unreal material corresponding to this houdini one
						HAPI_NodeId MaterialId = PartFaceMaterialIds[SplitFaceIndex];

						// See if we have already treated that material
						int32 const * FoundUnrealMatIndex = MapHoudiniMatIdToUnrealIndex.Find(MaterialId);
						if (FoundUnrealMatIndex)
						{
							// This material has been mapped already, just assign the mat index
							CurrentFaceMaterialIdx = *FoundUnrealMatIndex;
						}
						else
						{
							// If everything fails, we'll use the default material
							MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

							// We need to add this material to the map
							FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
							FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, MaterialId, MaterialPathName);
							UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
							if (FoundMaterial)
								MaterialInterface = *FoundMaterial;

							// See if we have a replacement material and use it on the mesh instead
							UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(MaterialPathName);
							if (ReplacementMaterial && *ReplacementMaterial)
								MaterialInterface = *ReplacementMaterial;

							// Add the material to the mesh
							CurrentFaceMaterialIdx = FoundStaticMesh->AddStaticMaterial(FStaticMaterial(MaterialInterface));

							// Map the Houdini ID to the unreal one
							MapHoudiniMatIdToUnrealIndex.Add(MaterialId, CurrentFaceMaterialIdx);
						}
					}
				}

				// Update the Face Material on the mesh
				FoundStaticMesh->SetTriangleMaterialID(FaceIdx, CurrentFaceMaterialIdx);
			}
		}
		else if (PartUniqueMaterialIds.Num() > 0)
		{
			// The part has houdini materials
			if (bOnlyOneFaceMaterial)
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Set Single Material"));

				// Use default Houdini material if no valid material is assigned to any of the faces.
				UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

				// Get id of this single material.
				FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
				FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, PartFaceMaterialIds[0], MaterialPathName);
				UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
				if (FoundMaterial)
					MaterialInterface = *FoundMaterial;

				// See if we have a replacement material and use it on the mesh instead
				UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(MaterialPathName);
				if (ReplacementMaterial && *ReplacementMaterial)
					MaterialInterface = *ReplacementMaterial;

				FoundStaticMesh->AddStaticMaterial(FStaticMaterial(MaterialInterface));
			}
			else
			{
				TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Set Per Face Materials"));

				// We have multiple houdini materials
				// Get default Houdini material.
				UMaterial * DefaultMaterial = FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get();

				for (int32 FaceIdx = 0; FaceIdx < SplitFaceIndices.Num(); ++FaceIdx)
				{
					int32 SplitFaceIndex = SplitFaceIndices[FaceIdx];
					if (!PartFaceMaterialIds.IsValidIndex(SplitFaceIndex))
						continue;

					// Get material id for this face.
					HAPI_NodeId MaterialId = PartFaceMaterialIds[SplitFaceIndex];

					// See if we have already treated that material
					int32 const * FoundUnrealMatIndex = MapHoudiniMatIdToUnrealIndex.Find(MaterialId);
					if (FoundUnrealMatIndex)
					{
						// This material has been mapped already, just assign the mat index
						FoundStaticMesh->SetTriangleMaterialID(FaceIdx, *FoundUnrealMatIndex);
						continue;
					}

					UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(DefaultMaterial);

					FString MaterialPathName = HAPI_UNREAL_DEFAULT_MATERIAL_NAME;
					FHoudiniMaterialTranslator::GetMaterialRelativePath(HGPO.AssetId, MaterialId, MaterialPathName);
					UMaterialInterface * const * FoundMaterial = OutputAssignmentMaterials.Find(MaterialPathName);
					if (FoundMaterial)
						MaterialInterface = *FoundMaterial;

					// See if we have a replacement material and use it on the mesh instead
					UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(MaterialPathName);
					if (ReplacementMaterial && *ReplacementMaterial)
						MaterialInterface = *ReplacementMaterial;

					// Add the material to the mesh
					int32 UnrealMatIndex = FoundStaticMesh->AddStaticMaterial(FStaticMaterial(MaterialInterface));

					// Map the houdini ID to the unreal one
					MapHoudiniMatIdToUnrealIndex.Add(MaterialId, UnrealMatIndex);

					// Update the face index
					FoundStaticMesh->SetTriangleMaterialID(FaceIdx, UnrealMatIndex);
				}
			}
		}
		else
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateHoudiniStaticMesh -- Set Default Material"));
		
			// No materials were found, we need to use default Houdini material.
			int32 SplitFaceCount = SplitFaceIndices.Num();

			UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(FHoudiniEngine::Get().GetHoudiniDefaultMaterial(HGPO.bIsTemplated).Get());

			// See if we have a replacement material and use it on the mesh instead
			UMaterialInterface * const * ReplacementMaterial = ReplacementMaterials.Find(HAPI_UNREAL_DEFAULT_MATERIAL_NAME);
			if (ReplacementMaterial && *ReplacementMaterial)
				MaterialInterface = *ReplacementMaterial;

			FoundStaticMesh->AddStaticMaterial(FStaticMaterial(MaterialInterface));
		}

		//// Update property attributes on the mesh
		//TArray<FHoudiniGenericAttribute> PropertyAttributes;
		//if (GetGenericPropertiesAttributes(
		//	HGPO.GeoId, HGPO.PartId,
		//	AllSplitFirstValidVertexIndex[SplitGroupName],
		//	AllSplitFirstValidPrimIndex[SplitGroupName],
		//	PropertyAttributes))
		//{
		//	UpdateGenericPropertiesAttributes(
		//		FoundStaticMesh, PropertyAttributes);
		//}

		FoundStaticMesh->Optimize();

		//// Try to find the outer package so we can dirty it up
		//if (FoundStaticMesh->GetOuter())
		//{
		//	FoundStaticMesh->GetOuter()->MarkPackageDirty();
		//}
		//else
		//{
		//	FoundStaticMesh->MarkPackageDirty();
		//}
		UPackage *MeshPackage = FoundStaticMesh->GetOutermost();
		if (MeshPackage && !MeshPackage->IsPendingKill())
		{
			MeshPackage->MarkPackageDirty();
			
			/*
			// DPT: deactivated auto saving mesh/material package
			// only dirty for now, as we'll save them when saving the world.
			// Save the created/updated package
			FEditorFileUtils::PromptForCheckoutAndSave({ MeshPackage }, false, false);
			*/
		}

		// Add the Proxy mesh to the output maps
		if (FoundOutputObject)
		{
			FoundOutputObject->ProxyObject = FoundStaticMesh;
			FoundOutputObject->bProxyIsCurrent = true;
			OutputObjects.FindOrAdd(OutputObjectIdentifier, *FoundOutputObject);
		}
	}

	const double time_end = FPlatformTime::Seconds();
	HOUDINI_LOG_MESSAGE(TEXT("CreateHoudiniStaticMesh() executed in %f seconds."), time_end - time_start);

	return true;
}

bool
FHoudiniMeshTranslator::CreateNeededMaterials()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::CreateNeededMaterials"));

	UpdatePartNeededMaterials();

	TArray<UPackage*> MaterialAndTexturePackages;
	FHoudiniMaterialTranslator::CreateHoudiniMaterials(
		HGPO.AssetId, PackageParams,
		PartUniqueMaterialIds, PartUniqueMaterialInfos,
		InputAssignmentMaterials, OutputAssignmentMaterials,
		MaterialAndTexturePackages, false, bTreatExistingMaterialsAsUpToDate);

	/*
	// Save the created packages if needed
	// DPT: deactivated, only dirty for now, as we'll save them when saving the world.
	if (MaterialAndTexturePackages.Num() > 0)
		FEditorFileUtils::PromptForCheckoutAndSave(MaterialAndTexturePackages, true, false);
	*/

	if (bMaterialOverrideNeedsCreateInstance && PartFaceMaterialOverrides.Num() > 0)
	{
		// Map containing unique face materials override attribute
		// and their first valid prim index
		// We create only one material instance per attribute
		TMap<FString, int32> UniqueFaceMaterialOverrides;
		for (int FaceIdx = 0; FaceIdx < PartFaceMaterialOverrides.Num(); FaceIdx++)
		{
			FString MatOverrideAttr = PartFaceMaterialOverrides[FaceIdx];
			if (UniqueFaceMaterialOverrides.Contains(MatOverrideAttr))
				continue;

			// Add the material override and face index to the map
			UniqueFaceMaterialOverrides.Add(MatOverrideAttr, FaceIdx);
		}

		FHoudiniMaterialTranslator::CreateMaterialInstances(
			HGPO, PackageParams,
			UniqueFaceMaterialOverrides, MaterialAndTexturePackages,
			InputAssignmentMaterials, OutputAssignmentMaterials,
			false);
	}

	return true;
}

FString
FHoudiniMeshTranslator::GetMeshIdentifierFromSplit(const FString& InSplitName, const EHoudiniSplitType& InSplitType)
{
	FString MeshIdentifier = TEXT("");
	switch (InSplitType)
	{
		case EHoudiniSplitType::Normal:
		case EHoudiniSplitType::LOD:
		case EHoudiniSplitType::InvisibleUCXCollider:
		case EHoudiniSplitType::InvisibleSimpleCollider:
			// LODs and Invisible simple colliders use the main mesh
			MeshIdentifier = HAPI_UNREAL_GROUP_GEOMETRY_NOT_COLLISION;
			break;

		case EHoudiniSplitType::InvisibleComplexCollider:
		case EHoudiniSplitType::RenderedComplexCollider:
		case EHoudiniSplitType::RenderedUCXCollider:
		case EHoudiniSplitType::RenderedSimpleCollider:
			// Rendered colliders or invisible complex colliders have their own static mesh
			MeshIdentifier = InSplitName;
			break;

		default:
			break;
	}

	return MeshIdentifier;
}

UStaticMesh*
FHoudiniMeshTranslator::FindExistingStaticMesh(const FHoudiniOutputObjectIdentifier& InIdentifier)
{
	// See if we already have an input object for that output identifier
	FHoudiniOutputObject const * FoundOutputObjectPtr = InputObjects.Find(InIdentifier);
	UStaticMesh* FoundStaticMesh = nullptr;
	if (FoundOutputObjectPtr)
	{
		// Make sure it's a valid static mesh
		FoundStaticMesh = Cast<UStaticMesh>(FoundOutputObjectPtr->OutputObject);
		if (!FoundStaticMesh || FoundStaticMesh->IsPendingKill())
			FoundStaticMesh = nullptr;
	}

	if (!FoundStaticMesh)
	{
		// No input object matching this identifier, see if we have created an output object that matches
		FoundOutputObjectPtr = OutputObjects.Find(InIdentifier);
		if (!FoundOutputObjectPtr)
			return nullptr;

		// Make sure it's a valid static mesh
		FoundStaticMesh = Cast<UStaticMesh>(FoundOutputObjectPtr->OutputObject);
		if (!FoundStaticMesh || FoundStaticMesh->IsPendingKill())
			return nullptr;
	}

	if (FoundStaticMesh)
	{
		UObject* OuterMost = FoundStaticMesh->GetOutermostObject();
		if (OuterMost->IsA<ULevel>())
		{
			// The Outermost for this static mesh is a level
			// This is likely a SM created by V1, and we should not reuse it.
			// This will force the plugin to recreate a "proper" SM in the temp folder.
			FoundStaticMesh->MarkPendingKill();
			FoundStaticMesh = nullptr;
		}
	}

	return FoundStaticMesh;
}

UHoudiniStaticMesh*
FHoudiniMeshTranslator::FindExistingHoudiniStaticMesh(const FHoudiniOutputObjectIdentifier& InIdentifier)
{
	// See if we already have an input object for that output identifier
	FHoudiniOutputObject const * FoundOutputObjectPtr = InputObjects.Find(InIdentifier);
	UHoudiniStaticMesh* FoundStaticMesh = nullptr;
	if (FoundOutputObjectPtr)
	{
		// Make sure it's a valid static mesh
		FoundStaticMesh = Cast<UHoudiniStaticMesh>(FoundOutputObjectPtr->ProxyObject);
		if (!FoundStaticMesh || FoundStaticMesh->IsPendingKill())
			FoundStaticMesh = nullptr;
	}

	if (!FoundStaticMesh)
	{
		// No input object matching this identifier, see if we have created an output object that matches
		FoundOutputObjectPtr = OutputObjects.Find(InIdentifier);
		if (!FoundOutputObjectPtr)
			return nullptr;

		// Make sure it's a valid static mesh
		FoundStaticMesh = Cast<UHoudiniStaticMesh>(FoundOutputObjectPtr->ProxyObject);
		if (!FoundStaticMesh || FoundStaticMesh->IsPendingKill())
			return nullptr;
	}

	return FoundStaticMesh;
}

EHoudiniSplitType
FHoudiniMeshTranslator::GetSplitTypeFromSplitName(const FString& InSplitName)
{
	const FString MainGroup = HAPI_UNREAL_GROUP_GEOMETRY_NOT_COLLISION;
	if (InSplitName.StartsWith(MainGroup, ESearchCase::IgnoreCase))
		return EHoudiniSplitType::Normal;

	const FString LODGroupPrefix = HAPI_UNREAL_GROUP_LOD_PREFIX;
	if (InSplitName.StartsWith(LODGroupPrefix, ESearchCase::IgnoreCase))
	{
		return EHoudiniSplitType::LOD;
	}
		
	const FString RenderedCollisionPrefix = HAPI_UNREAL_GROUP_RENDERED_COLLISION_PREFIX;
	if (InSplitName.StartsWith(RenderedCollisionPrefix, ESearchCase::IgnoreCase))
	{
		// Rendered colliders
		// See if it is a simple/ucx/complex
		const FString RenderedUCXCollisionPrefix = HAPI_UNREAL_GROUP_RENDERED_UCX_COLLISION_PREFIX;
		const FString RenderedSimpleCollisionPrefix = HAPI_UNREAL_GROUP_RENDERED_SIMPLE_COLLISION_PREFIX;
		if (InSplitName.StartsWith(RenderedUCXCollisionPrefix, ESearchCase::IgnoreCase))
		{
			return EHoudiniSplitType::RenderedUCXCollider;
		}
		else if (InSplitName.StartsWith(RenderedSimpleCollisionPrefix, ESearchCase::IgnoreCase))
		{
			return EHoudiniSplitType::RenderedSimpleCollider;
		}
		else
		{
			return EHoudiniSplitType::RenderedComplexCollider;
		}
	}

	const FString InvisibleCollisionPrefix = HAPI_UNREAL_GROUP_INVISIBLE_COLLISION_PREFIX;
	if (InSplitName.StartsWith(InvisibleCollisionPrefix, ESearchCase::IgnoreCase))
	{
		// Invisible colliders
		// See if it is a simple/ucx/complex
		const FString InvisibleUCXCollisionPrefix = HAPI_UNREAL_GROUP_INVISIBLE_UCX_COLLISION_PREFIX;
		const FString InvisibleSimpleCollisionPrefix = HAPI_UNREAL_GROUP_INVISIBLE_SIMPLE_COLLISION_PREFIX;
		if (InSplitName.StartsWith(InvisibleUCXCollisionPrefix, ESearchCase::IgnoreCase))
		{
			return EHoudiniSplitType::InvisibleUCXCollider;
		}
		else if (InSplitName.StartsWith(InvisibleSimpleCollisionPrefix, ESearchCase::IgnoreCase))
		{
			return EHoudiniSplitType::InvisibleSimpleCollider;
		}
		else
		{
			return EHoudiniSplitType::InvisibleComplexCollider;
		}
	}

	// ?
	return EHoudiniSplitType::Invalid;
	//return EHoudiniSplitType::Normal;
}

bool
FHoudiniMeshTranslator::AddConvexCollisionToAggregate(const FString& SplitGroupName, FKAggregateGeom& AggCollisions)
{
	// Get the vertex indices for the split group
	TArray<int32>& SplitGroupVertexList = AllSplitVertexLists[SplitGroupName];

	// We're only interested in unique vertices
	TArray<int32> UniqueVertexIndexes;
	for (int32 VertexIdx = 0; VertexIdx < SplitGroupVertexList.Num(); VertexIdx++)
	{
		int32 Index = SplitGroupVertexList[VertexIdx];
		if (!PartPositions.IsValidIndex(Index))
			continue;

		UniqueVertexIndexes.AddUnique(Index);
	}

	// Extract the collision geo's vertices
	TArray< FVector > VertexArray;
	VertexArray.SetNum(UniqueVertexIndexes.Num());
	for (int32 Idx = 0; Idx < UniqueVertexIndexes.Num(); Idx++)
	{
		int32 VertexIndex = UniqueVertexIndexes[Idx];
		if (!PartPositions.IsValidIndex(VertexIndex * 3 + 2))
			continue;

		VertexArray[Idx].X = PartPositions[VertexIndex * 3 + 0] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
		VertexArray[Idx].Y = PartPositions[VertexIndex * 3 + 2] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
		VertexArray[Idx].Z = PartPositions[VertexIndex * 3 + 1] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
	}

#if WITH_EDITOR
	// Do we want to create multiple convex hulls?
	bool bDoMultiHullDecomp = false;
	if (SplitGroupName.Contains(TEXT("ucx_multi"), ESearchCase::IgnoreCase))
		bDoMultiHullDecomp = true;

	uint32 HullCount = 8;
	int32 MaxHullVerts = 16;
	if (bDoMultiHullDecomp)
	{
		// TODO:
		// Look for extra attributes for the decomposition parameters? (HullCount/MaxHullVerts)
	}

	if (bDoMultiHullDecomp && (VertexArray.Num() >= 3 || UniqueVertexIndexes.Num() >= 3))
	{
		// creating multiple convex hull collision
		// ... this might take a while

		// We're only interested in the valid indices!
		TArray<uint32> Indices;
		for (int32 VertexIdx = 0; VertexIdx < SplitGroupVertexList.Num(); VertexIdx++)
		{
			int32 Index = SplitGroupVertexList[VertexIdx];
			if (!PartPositions.IsValidIndex(Index))
				continue;

			Indices.Add(Index);
		}

		// But we need all the positions as vertex
		TArray< FVector > Vertices;
		Vertices.SetNum(PartPositions.Num() / 3);

		for (int32 Idx = 0; Idx < Vertices.Num(); Idx++)
		{
			Vertices[Idx].X = PartPositions[Idx * 3 + 0] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
			Vertices[Idx].Y = PartPositions[Idx * 3 + 2] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
			Vertices[Idx].Z = PartPositions[Idx * 3 + 1] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
		}

		// We are using Unreal's DecomposeMeshToHulls() 
		// We need a BodySetup so create a fake/transient one
		UBodySetup* BodySetup = NewObject<UBodySetup>();

		// Run actual util to do the work (if we have some valid input)
		DecomposeMeshToHulls(BodySetup, Vertices, Indices, HullCount, MaxHullVerts);

		// If we succeed, return here
		// If not, keep going and we'll try to do a single hull decomposition
		if (BodySetup->AggGeom.ConvexElems.Num() > 0)
		{
			// Copy the convex elem to our aggregate
			for (int32 n = 0; n < BodySetup->AggGeom.ConvexElems.Num(); n++)
				AggCollisions.ConvexElems.Add(BodySetup->AggGeom.ConvexElems[n]);

			return true;
		}
	}
#endif

	// Creating a single Convex collision
	FKConvexElem ConvexCollision;
	ConvexCollision.VertexData = VertexArray;
	ConvexCollision.UpdateElemBox();

	AggCollisions.ConvexElems.Add(ConvexCollision);

	return true;
}

bool
FHoudiniMeshTranslator::AddSimpleCollisionToAggregate(const FString& SplitGroupName, FKAggregateGeom& AggCollisions)
{
	// Get the vertex indices for the split group
	TArray<int32>& SplitGroupVertexList = AllSplitVertexLists[SplitGroupName];

	// We're only interested in unique vertices
	TArray<int32> UniqueVertexIndexes;
	for (int32 VertexIdx = 0; VertexIdx < SplitGroupVertexList.Num(); VertexIdx++)
	{
		int32 Index = SplitGroupVertexList[VertexIdx];
		if (!PartPositions.IsValidIndex(Index))
			continue;

		UniqueVertexIndexes.AddUnique(Index);
	}

	// Extract the collision geo's vertices
	TArray< FVector > VertexArray;
	VertexArray.SetNum(UniqueVertexIndexes.Num());
	for (int32 Idx = 0; Idx < UniqueVertexIndexes.Num(); Idx++)
	{
		int32 VertexIndex = UniqueVertexIndexes[Idx];
		if (!PartPositions.IsValidIndex(VertexIndex * 3 + 2))
			continue;

		VertexArray[Idx].X = PartPositions[VertexIndex * 3 + 0] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
		VertexArray[Idx].Y = PartPositions[VertexIndex * 3 + 2] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
		VertexArray[Idx].Z = PartPositions[VertexIndex * 3 + 1] * HAPI_UNREAL_SCALE_FACTOR_POSITION;
	}

	int32 NewColliders = 0;
	if (SplitGroupName.Contains("Box"))
	{
		NewColliders = FHoudiniMeshTranslator::GenerateBoxAsSimpleCollision(VertexArray, AggCollisions);
	}
	else if (SplitGroupName.Contains("Sphere"))
	{
		NewColliders = FHoudiniMeshTranslator::GenerateSphereAsSimpleCollision(VertexArray, AggCollisions);
	}
	else if (SplitGroupName.Contains("Capsule"))
	{
		NewColliders = FHoudiniMeshTranslator::GenerateSphylAsSimpleCollision(VertexArray, AggCollisions);
	}
	else
	{
		// We need to see what type of collision the user wants
		// by default, a kdop26 will be created
		uint32 NumDirections = 26;
		const FVector* Directions = KDopDir26;
		if (SplitGroupName.Contains("kdop10X"))
		{
			NumDirections = 10;
			Directions = KDopDir10X;
		}
		else if (SplitGroupName.Contains("kdop10Y"))
		{
			NumDirections = 10;
			Directions = KDopDir10Y;
		}
		else if (SplitGroupName.Contains("kdop10Z"))
		{
			NumDirections = 10;
			Directions = KDopDir10Z;
		}
		else if (SplitGroupName.Contains("kdop18"))
		{
			NumDirections = 18;
			Directions = KDopDir18;
		}

		// Converting the directions to a TArray
		TArray<FVector> DirArray;
		DirArray.SetNum(NumDirections);
		for (uint32 DirectionIndex = 0; DirectionIndex < NumDirections; DirectionIndex++)
		{
			DirArray[DirectionIndex] = Directions[DirectionIndex];
		}

		NewColliders = FHoudiniMeshTranslator::GenerateKDopAsSimpleCollision(VertexArray, DirArray, AggCollisions);
	}

	return (NewColliders > 0);
}

int32
FHoudiniMeshTranslator::TransferRegularPointAttributesToVertices(
	const TArray<int32>& InVertexList,
	const HAPI_AttributeInfo& InAttribInfo,
	const TArray<float>& InData,
	TArray<float>& OutVertexData)
{
	return FHoudiniMeshTranslator::TransferPartAttributesToSplit<float>(
		InVertexList, InAttribInfo,	InData,	OutVertexData);
}

/*
int32
FHoudiniMeshTranslator::GetSplitNormals(
	const TArray<int32>& InSplitVertexList, TArray<FVector>& OutNormals)
{
	// Extract the normals
	UpdatePartNormalsIfNeeded();

	// Get the normals for this split
	TArray<float> VertexData;
	int32 WedgeCount = FHoudiniMeshTranslator::TransferPartAttributesToSplit<float>(
		InSplitVertexList, AttribInfoNormals, PartNormals, VertexData);

	// Convert the float data to Vectors and fix the winding order
	OutNormals.SetNum(WedgeCount);
	for (int32 WedgeIdx = 0; WedgeIdx + 2 < WedgeCount; WedgeIdx += 3)
	{
		OutNormals[WedgeIdx].X = VertexData[WedgeIdx * 3 + 0];
		OutNormals[WedgeIdx].Y = VertexData[WedgeIdx * 3 + 2];
		OutNormals[WedgeIdx].Z = VertexData[WedgeIdx * 3 + 1];

		OutNormals[WedgeIdx + 2].X = VertexData[(WedgeIdx + 1) * 3 + 0];
		OutNormals[WedgeIdx + 2].Y = VertexData[(WedgeIdx + 1) * 3 + 2];
		OutNormals[WedgeIdx + 2].Z = VertexData[(WedgeIdx + 1) * 3 + 1];

		OutNormals[WedgeIdx + 1].X = VertexData[(WedgeIdx + 2) * 3 + 0];
		OutNormals[WedgeIdx + 1].Y = VertexData[(WedgeIdx + 2) * 3 + 2];
		OutNormals[WedgeIdx + 1].Z = VertexData[(WedgeIdx + 2) * 3 + 1];
	}

	return WedgeCount;
}

int32
FHoudiniMeshTranslator::GetSplitUVs(
	const TArray<int32>& InSplitVertexList, TArray<FVector2D>& OutUVs)
{
	// Extract the normals
	UpdatePartUVSetsIfNeeded();

	// Get the normals for this split
	TArray<float> VertexData;
	int32 WedgeCount = FHoudiniMeshTranslator::TransferPartAttributesToSplit<float>(
		InSplitVertexList, AttribInfoNormals, PartNormals, VertexData);

	// Convert the float data to Vectors and fix the winding order
	OutNormals.SetNum(WedgeCount);
	for (int32 WedgeIdx = 0; WedgeIdx + 2 < WedgeCount; WedgeIdx += 3)
	{
		OutNormals[WedgeIdx].X = VertexData[WedgeIdx * 3 + 0];
		OutNormals[WedgeIdx].Y = VertexData[WedgeIdx * 3 + 2];
		OutNormals[WedgeIdx].Z = VertexData[WedgeIdx * 3 + 1];

		OutNormals[WedgeIdx + 2].X = VertexData[(WedgeIdx + 1) * 3 + 0];
		OutNormals[WedgeIdx + 2].Y = VertexData[(WedgeIdx + 1) * 3 + 2];
		OutNormals[WedgeIdx + 2].Z = VertexData[(WedgeIdx + 1) * 3 + 1];

		OutNormals[WedgeIdx + 1].X = VertexData[(WedgeIdx + 2) * 3 + 0];
		OutNormals[WedgeIdx + 1].Y = VertexData[(WedgeIdx + 2) * 3 + 2];
		OutNormals[WedgeIdx + 1].Z = VertexData[(WedgeIdx + 2) * 3 + 1];
	}

	return WedgeCount;
}


int32
FHoudiniMeshTranslator::TransferPartAttributesToSplit(
	const TArray<int32>& InVertexList,
	const HAPI_AttributeInfo& InAttribInfo,
	const TArray<float>& InData,
	TArray<FVector2D>& OutData,
	const float& ScaleFactor)
{
	TArray<float> VertexData;
	int32 WedgeCount = FHoudiniMeshTranslator::TransferPartAttributesToSplit<float>(
		InVertexList, InAttribInfo, InData, VertexData);

	// Convert the float data to Vectors and fix the winding order
	OutData.SetNum(WedgeCount);
	for (int32 WedgeIdx = 0; WedgeIdx + 2 < WedgeCount; WedgeIdx += 3)
	{
		OutData[WedgeIdx].X = VertexData[WedgeIdx * 2 + 0];
		OutData[WedgeIdx].Y = VertexData[WedgeIdx * 2 + 1];

		OutData[WedgeIdx + 2].X = VertexData[(WedgeIdx + 1) * 2 + 0];
		OutData[WedgeIdx + 2].Y = VertexData[(WedgeIdx + 1) * 2 + 1];

		OutData[WedgeIdx + 1].X = VertexData[(WedgeIdx + 2) * 2 + 0];
		OutData[WedgeIdx + 1].Y = VertexData[(WedgeIdx + 2) * 2 + 1];
	}

	return WedgeCount;
}

int32
FHoudiniMeshTranslator::TransferPartAttributesToSplit(
	const TArray<int32>& InVertexList,
	const HAPI_AttributeInfo& InAttribInfo,
	const TArray<float>& InData,
	TArray<FVector2D>& OutData,
	const float& ScaleFactor)
{
	TArray<float> VertexData;
	int32 WedgeCount = FHoudiniMeshTranslator::TransferPartAttributesToSplit<float>(
		InVertexList, InAttribInfo, InData, VertexData);

	// Convert the float data to Vectors and fix the winding order
	OutData.SetNum(WedgeCount);
	for (int32 WedgeIdx = 0; WedgeIdx + 2 < WedgeCount; WedgeIdx += 3)
	{
		OutData[WedgeIdx].X = VertexData[WedgeIdx * 2 + 0];
		OutData[WedgeIdx].Y = VertexData[WedgeIdx * 2 + 1];

		OutData[WedgeIdx + 2].X = VertexData[(WedgeIdx + 1) * 2 + 0];
		OutData[WedgeIdx + 2].Y = VertexData[(WedgeIdx + 1) * 2 + 1];

		OutData[WedgeIdx + 1].X = VertexData[(WedgeIdx + 2) * 2 + 0];
		OutData[WedgeIdx + 1].Y = VertexData[(WedgeIdx + 2) * 2 + 1];
	}

	return WedgeCount;
}
*/


template <typename TYPE>
int32 FHoudiniMeshTranslator::TransferPartAttributesToSplit(
	const TArray<int32>& InVertexList,
	const HAPI_AttributeInfo& InAttribInfo,
	const TArray<TYPE>& InData,
	TArray<TYPE>& OutVertexData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("FHoudiniMeshTranslator::TransferPartAttributesToSplit"));

	if (!InAttribInfo.exists || InAttribInfo.tupleSize <= 0)
		return 0;

	if (InData.Num() <= 0)
		return 0;

	int32 ValidWedgeCount = 0;

	// Future optimization - see if we can do direct vertex transfer.
	int32 WedgeCount = InVertexList.Num();
	int32 LastValidWedgeIdx = 0;
	if (InAttribInfo.owner == HAPI_ATTROWNER_POINT)
	{		
		// Point attribute transfer
		OutVertexData.SetNumZeroed(WedgeCount * InAttribInfo.tupleSize);
		for (int32 WedgeIdx = 0; WedgeIdx < WedgeCount; ++WedgeIdx)
		{
			int32 VertexIdx = InVertexList[WedgeIdx];
			if (VertexIdx < 0)
			{
				// This is an index/wedge we are skipping due to split.
				continue;
			}

			int32 OutIdx = LastValidWedgeIdx * InAttribInfo.tupleSize;
			for (int32 TupleIdx = 0; TupleIdx < InAttribInfo.tupleSize; TupleIdx++)
			{
				OutVertexData[OutIdx + TupleIdx] = InData[VertexIdx * InAttribInfo.tupleSize + TupleIdx];
			}

			// We are re-indexing wedges.
			LastValidWedgeIdx++;
			// Increment wedge count, since this is a valid wedge.
			ValidWedgeCount++;
		}
	}
	else if (InAttribInfo.owner == HAPI_ATTROWNER_VERTEX)
	{
		// Vertex attribute transfer
		OutVertexData.SetNumZeroed(WedgeCount * InAttribInfo.tupleSize);
		for (int32 WedgeIdx = 0; WedgeIdx < WedgeCount; ++WedgeIdx)
		{
			if (InVertexList[WedgeIdx] < 0)
			{
				// This is an index/wedge we are skipping due to split.
				continue;
			}

			int32 OutIdx = LastValidWedgeIdx * InAttribInfo.tupleSize;
			for (int32 TupleIdx = 0; TupleIdx < InAttribInfo.tupleSize; TupleIdx++)
			{				
				OutVertexData[OutIdx + TupleIdx] = InData[WedgeIdx * InAttribInfo.tupleSize + TupleIdx];
			}

			// We are re-indexing wedges.
			LastValidWedgeIdx++;
			// Increment wedge count, since this is a valid wedge.
			ValidWedgeCount++;
		}
	}
	else if (InAttribInfo.owner == HAPI_ATTROWNER_PRIM)
	{
		// Primitive attribute transfer
		OutVertexData.SetNumZeroed(WedgeCount * InAttribInfo.tupleSize);
		for (int32 WedgeIdx = 0; WedgeIdx < WedgeCount; ++WedgeIdx)
		{
			if (InVertexList[WedgeIdx] < 0)
			{
				// This is an index/wedge we are skipping due to split.
				continue;
			}

			int32 PrimIdx = WedgeIdx / 3;
			int32 OutIdx = LastValidWedgeIdx * InAttribInfo.tupleSize;
			for (int32 TupleIdx = 0; TupleIdx < InAttribInfo.tupleSize; TupleIdx++)
			{
				OutVertexData[OutIdx + TupleIdx] = InData[PrimIdx * InAttribInfo.tupleSize + TupleIdx];
			}

			// We are re-indexing wedges.
			LastValidWedgeIdx++;
			// Increment wedge count, since this is a valid wedge.
			ValidWedgeCount++;
		}
	}
	else if (InAttribInfo.owner == HAPI_ATTROWNER_DETAIL)
	{
		// Detail attribute transfer
		// We have one value to copy for all output split vertices
		// if the attribute is a single value (not a tuple)
		// then we can simply use the array init function instead of looping
		if (InAttribInfo.tupleSize == 1)
		{
			OutVertexData.Init(InData[0], WedgeCount);
		}
		else
		{
			OutVertexData.SetNumZeroed(WedgeCount * InAttribInfo.tupleSize);
			for (int32 WedgeIdx = 0; WedgeIdx < WedgeCount; ++WedgeIdx)
			{
				if (InVertexList[WedgeIdx] < 0)
				{
					// This is an index/wedge we are skipping due to split.
					continue;
				}

				int32 OutIdx = LastValidWedgeIdx * InAttribInfo.tupleSize;
				for (int32 TupleIdx = 0; TupleIdx < InAttribInfo.tupleSize; TupleIdx++)
				{
					OutVertexData[OutIdx + TupleIdx] = InData[TupleIdx];
				}

				// We are re-indexing wedges.
				LastValidWedgeIdx++;
				// Increment wedge count, since this is a valid wedge.
				ValidWedgeCount++;
			}
		}
	}
	else
	{
		// Invalid attribute owner, shouldn't happen
		check(false);
	}

	OutVertexData.SetNumZeroed(ValidWedgeCount * InAttribInfo.tupleSize);

	return ValidWedgeCount;
}

float
FHoudiniMeshTranslator::GetLODSCreensizeForSplit(const FString& SplitGroupName)
{
	// LOD Screensize
	// default values has already been set, see if we have any attribute override for this
	float screensize = -1.0f;

	// Start by looking at the lod_screensize primitive attribute
	bool bAttribValid = false;
	UpdatePartLODScreensizeIfNeeded();

	if (PartLODScreensize.Num() > 0)
	{
		// use the "lod_screensize" primitive attribute
		int32 FirstValidPrimIndex = AllSplitFirstValidPrimIndex[SplitGroupName];
		if (PartLODScreensize.IsValidIndex(FirstValidPrimIndex))
			screensize = PartLODScreensize[FirstValidPrimIndex];
	}
	
	if (screensize < 0.0f)
	{
		// We couldn't find the primitive attribute, look for a "lodX_screensize" detail attribute
		FString LODAttributeName = SplitGroupName + HAPI_UNREAL_ATTRIB_LOD_SCREENSIZE_POSTFIX;

		TArray<float> LODScreenSizes;
		HAPI_AttributeInfo AttribInfoScreenSize;
		FHoudiniApi::AttributeInfo_Init(&AttribInfoScreenSize);

		FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
			HGPO.GeoId, HGPO.PartId, TCHAR_TO_ANSI(*LODAttributeName),
			AttribInfoScreenSize, LODScreenSizes, 0, HAPI_ATTROWNER_DETAIL);

		if (AttribInfoScreenSize.exists && LODScreenSizes.Num() > 0)
		{
			screensize = LODScreenSizes[0];
		}			
	}

	if (screensize < 0.0f)
	{
		// finally, look for a potential uproperty style attribute
		// aka, "unreal_uproperty_screensize"
		TArray<float> LODScreenSizes;
		HAPI_AttributeInfo AttribInfoScreenSize;
		FHoudiniApi::AttributeInfo_Init(&AttribInfoScreenSize);

		FHoudiniEngineUtils::HapiGetAttributeDataAsFloat(
			HGPO.GeoId, HGPO.PartId, "unreal_uproperty_screensize",
			AttribInfoScreenSize, LODScreenSizes);

		if (AttribInfoScreenSize.exists)
		{
			if (AttribInfoScreenSize.owner == HAPI_ATTROWNER_DETAIL && LODScreenSizes.Num() > 0)
			{
				screensize = LODScreenSizes[0];
			}
			else if (AttribInfoScreenSize.owner == HAPI_ATTROWNER_PRIM)
			{
				int32 FirstValidPrimIndex = AllSplitFirstValidPrimIndex[SplitGroupName];
				if (LODScreenSizes.IsValidIndex(FirstValidPrimIndex))
					screensize = LODScreenSizes[FirstValidPrimIndex];
			}
		}
	}

	// Make sure the screensize is in percent, so if its above 1, divide by 100
	if (screensize > 1.0f)
		screensize /= 100.0f;

	return screensize;
}

int32 
FHoudiniMeshTranslator::GenerateBoxAsSimpleCollision(const TArray<FVector>& InPositionArray, FKAggregateGeom& OutAggregateCollisions)
{
	//
	// Code simplified and adapted to work with a simple vector array from GeomFitUtils.cpp
	//

	// Calculate bounding Box.	
	FVector Center, Extents;
	FVector unitVec = FVector::OneVector;// bs->BuildScale3D;
	CalcBoundingBox(InPositionArray, Center, Extents, unitVec);

	FKBoxElem BoxElem;
	BoxElem.Center = Center;
	BoxElem.X = Extents.X * 2.0f;
	BoxElem.Y = Extents.Y * 2.0f;
	BoxElem.Z = Extents.Z * 2.0f;
	OutAggregateCollisions.BoxElems.Add(BoxElem);

	return 1;
}

void
FHoudiniMeshTranslator::CalcBoundingBox(const TArray<FVector>& PositionArray, FVector& Center, FVector& Extents, FVector& LimitVec)
{
	//
	// Code simplified and adapted to work with a simple vector array from GeomFitUtils.cpp
	//

	FBox Box(ForceInit);
	for (const FVector& CurPos : PositionArray)
	{
		Box += CurPos;
	}
	Box.GetCenterAndExtents(Center, Extents);
}

int32
FHoudiniMeshTranslator::GenerateSphereAsSimpleCollision(const TArray<FVector>& InPositionArray, FKAggregateGeom& OutAggregateCollisions)
{
	//
	// Code simplified and adapted to work with a simple vector array from GeomFitUtils.cpp
	//

	FSphere bSphere, bSphere2, bestSphere;
	FVector unitVec = FVector::OneVector;

	// Calculate bounding sphere.
	CalcBoundingSphere(InPositionArray, bSphere, unitVec);
	CalcBoundingSphere2(InPositionArray, bSphere2, unitVec);

	if (bSphere.W < bSphere2.W)
		bestSphere = bSphere;
	else
		bestSphere = bSphere2;

	// Don't use if radius is zero.
	if (bestSphere.W <= 0.f)
	{
		HOUDINI_LOG_WARNING(TEXT("Failed to generate a simple Sphere collider."));
		return 0;
	}

	FKSphereElem SphereElem;
	SphereElem.Center = bestSphere.Center;
	SphereElem.Radius = bestSphere.W;
	OutAggregateCollisions.SphereElems.Add(SphereElem);

	return 1;
}

void
FHoudiniMeshTranslator::CalcBoundingSphere(const TArray<FVector>& PositionArray, FSphere& sphere, FVector& LimitVec)
{
	//
	// Code simplified and adapted to work with a simple vector array from GeomFitUtils.cpp
	//

	if (PositionArray.Num() == 0)
		return;

	FBox Box;
	FVector MinIx[3];
	FVector MaxIx[3];

	bool bFirstVertex = true;
	for (const FVector& CurPosition : PositionArray)
	{
		FVector p = CurPosition * LimitVec;
		if (bFirstVertex)
		{
			// First, find AABB, remembering furthest points in each dir.
			Box.Min = p;
			Box.Max = Box.Min;

			MinIx[0] = CurPosition;
			MinIx[1] = CurPosition;
			MinIx[2] = CurPosition;

			MaxIx[0] = CurPosition;
			MaxIx[1] = CurPosition;
			MaxIx[2] = CurPosition;
			bFirstVertex = false;
			continue;
		}

		// X //
		if (p.X < Box.Min.X)
		{
			Box.Min.X = p.X;
			MinIx[0] = CurPosition;
		}
		else if (p.X > Box.Max.X)
		{
			Box.Max.X = p.X;
			MaxIx[0] = CurPosition;
		}

		// Y //
		if (p.Y < Box.Min.Y)
		{
			Box.Min.Y = p.Y;
			MinIx[1] = CurPosition;
		}
		else if (p.Y > Box.Max.Y)
		{
			Box.Max.Y = p.Y;
			MaxIx[1] = CurPosition;
		}

		// Z //
		if (p.Z < Box.Min.Z)
		{
			Box.Min.Z = p.Z;
			MinIx[2] = CurPosition;
		}
		else if (p.Z > Box.Max.Z)
		{
			Box.Max.Z = p.Z;
			MaxIx[2] = CurPosition;
		}
	}

	const FVector Extremes[3] = { (MaxIx[0] - MinIx[0]) * LimitVec,
		(MaxIx[1] - MinIx[1]) * LimitVec,
		(MaxIx[2] - MinIx[2]) * LimitVec };

	// Now find extreme points furthest apart, and initial center and radius of sphere.
	float d2 = 0.f;
	for (int32 i = 0; i < 3; i++)
	{
		const float tmpd2 = Extremes[i].SizeSquared();
		if (tmpd2 > d2)
		{
			d2 = tmpd2;
			sphere.Center = (MinIx[i] + (0.5f * Extremes[i])) * LimitVec;
			sphere.W = 0.f;
		}
	}

	const FVector Extents = FVector(Extremes[0].X, Extremes[1].Y, Extremes[2].Z);

	// radius and radius squared
	float r = 0.5f * Extents.GetMax();
	float r2 = FMath::Square(r);

	// Now check each point lies within this sphere. If not - expand it a bit.
	for (const FVector& curPos : PositionArray)
	{
		const FVector cToP = (curPos * LimitVec) - sphere.Center;

		const float pr2 = cToP.SizeSquared();

		// If this point is outside our current bounding sphere's radius
		if (pr2 > r2)
		{
			// ..expand radius just enough to include this point.
			const float pr = FMath::Sqrt(pr2);
			r = 0.5f * (r + pr);
			r2 = FMath::Square(r);

			sphere.Center += ((pr - r) / pr * cToP);
		}
	}

	sphere.W = r;
}

void
FHoudiniMeshTranslator::CalcBoundingSphere2(const TArray<FVector>& PositionArray, FSphere& sphere, FVector& LimitVec)
{
	//
	// Code simplified and adapted to work with a simple vector array from GeomFitUtils.cpp
	//

	FVector Center, Extents;
	CalcBoundingBox(PositionArray, Center, Extents, LimitVec);

	sphere.Center = Center;
	sphere.W = 0.0f;

	for (const FVector& curPos : PositionArray)
	{
		float Dist = FVector::DistSquared(curPos * LimitVec, sphere.Center);
		if (Dist > sphere.W)
			sphere.W = Dist;
	}
	sphere.W = FMath::Sqrt(sphere.W);
}

int32 
FHoudiniMeshTranslator::GenerateSphylAsSimpleCollision(const TArray<FVector>& InPositionArray, FKAggregateGeom& OutAggregateCollisions)
{
	//
	// Code simplified and adapted to work with a simple vector array from GeomFitUtils.cpp
	//

	FSphere sphere;
	float length;
	FRotator rotation;
	FVector unitVec = FVector::OneVector;

	// Calculate bounding sphyl.
	CalcBoundingSphyl(InPositionArray, sphere, length, rotation, unitVec);

	// Dont use if radius is zero.
	if (sphere.W <= 0.f)
	{
		HOUDINI_LOG_WARNING(TEXT("Failed to generate a simple Capsule collider."));
		return 0;
	}

	// If height is zero, then a sphere would be better (should we just create one instead?)
	if (length <= 0.f)
	{
		length = SMALL_NUMBER;
	}

	FKSphylElem SphylElem;
	SphylElem.Center = sphere.Center;
	SphylElem.Rotation = rotation;
	SphylElem.Radius = sphere.W;
	SphylElem.Length = length;
	OutAggregateCollisions.SphylElems.Add(SphylElem);

	return 1;
}

void
FHoudiniMeshTranslator::CalcBoundingSphyl(const TArray<FVector>& PositionArray, FSphere& sphere, float& length, FRotator& rotation, FVector& LimitVec)
{
	//
	// Code simplified and adapted to work with a simple vector array from GeomFitUtils.cpp
	//

	if (PositionArray.Num() == 0)
		return;

	FVector Center, Extents;
	CalcBoundingBox(PositionArray, Center, Extents, LimitVec);

	// @todo sphere.Center could perhaps be adjusted to best fit if model is non-symmetric on it's longest axis
	sphere.Center = Center;

	// Work out best axis aligned orientation (longest side)
	float Extent = Extents.GetMax();
	if (Extent == Extents.X)
	{
		rotation = FRotator(90.f, 0.f, 0.f);
		Extents.X = 0.0f;
	}
	else if (Extent == Extents.Y)
	{
		rotation = FRotator(0.f, 0.f, 90.f);
		Extents.Y = 0.0f;
	}
	else
	{
		rotation = FRotator(0.f, 0.f, 0.f);
		Extents.Z = 0.0f;
	}

	// Cleared the largest axis above, remaining determines the radius
	float r = Extents.GetMax();
	float r2 = FMath::Square(r);

	// Now check each point lies within this the radius. If not - expand it a bit.
	for (const FVector& CurPos : PositionArray)
	{
		FVector cToP = (CurPos * LimitVec) - sphere.Center;
		cToP = rotation.UnrotateVector(cToP);

		const float pr2 = cToP.SizeSquared2D();	// Ignore Z here...

		// If this point is outside our current bounding sphere's radius
		if (pr2 > r2)
		{
			// ..expand radius just enough to include this point.
			const float pr = FMath::Sqrt(pr2);
			r = 0.5f * (r + pr);
			r2 = FMath::Square(r);
		}
	}

	// The length is the longest side minus the radius.
	float hl = FMath::Max(0.0f, Extent - r);

	// Now check each point lies within the length. If not - expand it a bit.
	for (const FVector& CurPos : PositionArray)
	{
		FVector cToP = (CurPos * LimitVec) - sphere.Center;
		cToP = rotation.UnrotateVector(cToP);

		// If this point is outside our current bounding sphyl's length
		if (FMath::Abs(cToP.Z) > hl)
		{
			const bool bFlip = (cToP.Z < 0.f ? true : false);
			const FVector cOrigin(0.f, 0.f, (bFlip ? -hl : hl));

			const float pr2 = (cOrigin - cToP).SizeSquared();

			// If this point is outside our current bounding sphyl's radius
			if (pr2 > r2)
			{
				FVector cPoint;
				FMath::SphereDistToLine(cOrigin, r, cToP, (bFlip ? FVector(0.f, 0.f, 1.f) : FVector(0.f, 0.f, -1.f)), cPoint);

				// Don't accept zero as a valid diff when we know it's outside the sphere (saves needless retest on further iterations of like points)
				hl += FMath::Max(FMath::Abs(cToP.Z - cPoint.Z), 1.e-6f);
			}
		}
	}

	sphere.W = r;
	length = hl * 2.0f;
}

int32
FHoudiniMeshTranslator::GenerateKDopAsSimpleCollision(const TArray<FVector>& InPositionArray, const TArray<FVector> &Dirs, FKAggregateGeom& OutAggregateCollisions)
{
	//
	// Code simplified and adapted to work with a simple vector array from GeomFitUtils.cpp
	//

	const float my_flt_max = 3.402823466e+38F;

	// Do k- specific stuff.
	int32 kCount = Dirs.Num();

	TArray<float> maxDist;
	maxDist.Init(-my_flt_max, kCount);	
	/*
	for (int32 i = 0; i < kCount; i++)
		maxDist.Add(my_flt_max);
	*/

	// Construct temporary UModel for kdop creation. We keep no refs to it, so it can be GC'd.
	auto TempModel = NewObject<UModel>();
	TempModel->Initialize(nullptr, 1);

	// For each vertex, project along each kdop direction, to find the max in that direction.
	for (int32 i = 0; i < InPositionArray.Num(); i++)
	{
		for (int32 j = 0; j < kCount; j++)
		{
			float dist = InPositionArray[i] | Dirs[j];
			maxDist[j] = FMath::Max(dist, maxDist[j]);
		}
	}

	// Inflate kdop to ensure it is no degenerate
	const float MinSize = 0.1f;
	for (int32 i = 0; i < kCount; i++)
	{
		maxDist[i] += MinSize;
	}

	// Now we have the planes of the kdop, we work out the face polygons.
	TArray<FPlane> planes;
	for (int32 i = 0; i < kCount; i++)
		planes.Add(FPlane(Dirs[i], maxDist[i]));

	for (int32 i = 0; i < planes.Num(); i++)
	{
		FPoly*	Polygon = new(TempModel->Polys->Element) FPoly();
		FVector Base, AxisX, AxisY;

		Polygon->Init();
		Polygon->Normal = planes[i];
		Polygon->Normal.FindBestAxisVectors(AxisX, AxisY);

		Base = planes[i] * planes[i].W;

		new(Polygon->Vertices) FVector(Base + AxisX * HALF_WORLD_MAX + AxisY * HALF_WORLD_MAX);
		new(Polygon->Vertices) FVector(Base + AxisX * HALF_WORLD_MAX - AxisY * HALF_WORLD_MAX);
		new(Polygon->Vertices) FVector(Base - AxisX * HALF_WORLD_MAX - AxisY * HALF_WORLD_MAX);
		new(Polygon->Vertices) FVector(Base - AxisX * HALF_WORLD_MAX + AxisY * HALF_WORLD_MAX);

		for (int32 j = 0; j < planes.Num(); j++)
		{
			if (i != j)
			{
				if (!Polygon->Split(-FVector(planes[j]), planes[j] * planes[j].W))
				{
					Polygon->Vertices.Empty();
					break;
				}
			}
		}

		if (Polygon->Vertices.Num() < 3)
		{
			// If poly resulted in no verts, remove from array
			TempModel->Polys->Element.RemoveAt(TempModel->Polys->Element.Num() - 1);
		}
		else
		{
			// Other stuff...
			Polygon->iLink = i;
			Polygon->CalcNormal(1);
		}
	}

	if (TempModel->Polys->Element.Num() < 4)
	{
		TempModel = NULL;
		HOUDINI_LOG_WARNING(TEXT("Failed to generate a simple KDOP collider."));
		return 0;
	}

	// Build bounding box.
	TempModel->BuildBound();

	// Build BSP for the brush.
	FBSPOps::bspBuild(TempModel, FBSPOps::BSP_Good, 15, 70, 1, 0);
	FBSPOps::bspRefresh(TempModel, 1);
	FBSPOps::bspBuildBounds(TempModel);

	// Now, create a temporary BodySetup to build the colliders
	UBodySetup* TempBS = NewObject<UBodySetup>();
	TempBS->CreateFromModel(TempModel, false);

	// Copy the convex elements back to our aggregate
	int32 NewConvexElems = 0;
	if (TempBS && TempBS->AggGeom.ConvexElems.Num() > 0)
	{
		for (const auto& CurConvexElem : TempBS->AggGeom.ConvexElems)
		{
			OutAggregateCollisions.ConvexElems.Add(CurConvexElem);
			NewConvexElems++;
		}
	}

	return NewConvexElems;
}


bool
FHoudiniMeshTranslator::GetGenericPropertiesAttributes(
	const HAPI_NodeId& InGeoNodeId, const HAPI_PartId& InPartId,
	const int32& InFirstValidVertexIndex, const int32& InFirstValidPrimIndex,
	TArray<FHoudiniGenericAttribute>& OutPropertyAttributes)
{
	// List all the generic property detail attributes ...
	int32 FoundCount = FHoudiniEngineUtils::GetGenericAttributeList(
		InGeoNodeId, InPartId, HAPI_UNREAL_ATTRIB_GENERIC_UPROP_PREFIX, OutPropertyAttributes, HAPI_ATTROWNER_DETAIL);

	// .. then the primitive property attributes for the given prim
	FoundCount += FHoudiniEngineUtils::GetGenericAttributeList(
		InGeoNodeId, InPartId, HAPI_UNREAL_ATTRIB_GENERIC_UPROP_PREFIX, OutPropertyAttributes, HAPI_ATTROWNER_PRIM, InFirstValidPrimIndex);

	// .. then finally, point uprop attributes for the given vert
	// TODO: !! get the correct Index here?
	FoundCount += FHoudiniEngineUtils::GetGenericAttributeList(
		InGeoNodeId, InPartId, HAPI_UNREAL_ATTRIB_GENERIC_UPROP_PREFIX, OutPropertyAttributes, HAPI_ATTROWNER_POINT, InFirstValidVertexIndex);

	return FoundCount > 0;
}

bool
FHoudiniMeshTranslator::UpdateGenericPropertiesAttributes(
	UObject* InObject, const TArray<FHoudiniGenericAttribute>& InAllPropertyAttributes)
{
	if (!InObject || InObject->IsPendingKill())
		return false;

	// Iterate over the found Property attributes
	int32 NumSuccess = 0;
	for (const auto& CurrentPropAttribute : InAllPropertyAttributes)
	{
		// Update the current Property Attribute
		if (!FHoudiniGenericAttribute::UpdatePropertyAttributeOnObject(InObject, CurrentPropAttribute))
			continue;

		// Success!
		NumSuccess++;
		FString ClassName = InObject->GetClass() ? InObject->GetClass()->GetName() : TEXT("Object");
		FString ObjectName = InObject->GetName();
		HOUDINI_LOG_MESSAGE(TEXT("Modified UProperty %s on %s named %s"), *CurrentPropAttribute.AttributeName, *ClassName, *ObjectName);
	}

	return (NumSuccess > 0);
}


void 
FHoudiniMeshTranslator::SetPackageParams(const FHoudiniPackageParams& InPackageParams, const bool& bUpdateHGPO)
{
	PackageParams = InPackageParams;

	if (bUpdateHGPO)
	{
		PackageParams.ObjectId = HGPO.ObjectId;
		PackageParams.GeoId = HGPO.ObjectId;
		PackageParams.PartId = HGPO.ObjectId;
	}
}

bool 
FHoudiniMeshTranslator::RemoveAndDestroyComponent(UObject* InComponent)
{
	if (!InComponent || InComponent->IsPendingKill())
		return false;

	USceneComponent* SceneComponent = Cast<USceneComponent>(InComponent);
	if (SceneComponent && !SceneComponent->IsPendingKill())
	{
		// Remove from the HoudiniAssetActor
		if (SceneComponent->GetOwner())
			SceneComponent->GetOwner()->RemoveOwnedComponent(SceneComponent);

		SceneComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		SceneComponent->UnregisterComponent();
		SceneComponent->DestroyComponent();

		return true;
	}

	return false;
}

UMeshComponent*
FHoudiniMeshTranslator::CreateMeshComponent(UObject *InOuterComponent, const TSubclassOf<UMeshComponent> &InComponentType)
{
	// Create a new SMC as we couldn't find an existing one
	USceneComponent* OuterSceneComponent = Cast<USceneComponent>(InOuterComponent);
	UObject * Outer = nullptr;
	if (OuterSceneComponent && !OuterSceneComponent->IsPendingKill())
		Outer = OuterSceneComponent->GetOwner() ? OuterSceneComponent->GetOwner() : OuterSceneComponent->GetOuter();

	UMeshComponent *MeshComponent = NewObject<UMeshComponent>(Outer, InComponentType, NAME_None, RF_Transactional);

	// Initialize it
	MeshComponent->SetVisibility(true);
	//MeshComponent->SetMobility(Mobility);

	// TODO:
	// Property propagation: set the new SMC's properties to the HAC's current settings
	//CopyComponentPropertiesTo(MeshComponent);

	// Change the creation method so the component is listed in the details panels
	MeshComponent->CreationMethod = EComponentCreationMethod::Instance;

	// Attach created static mesh component to our Houdini component.
	MeshComponent->AttachToComponent(OuterSceneComponent, FAttachmentTransformRules::KeepRelativeTransform);
	MeshComponent->OnComponentCreated();
	MeshComponent->RegisterComponent();

	return MeshComponent;
}

bool
FHoudiniMeshTranslator::PostCreateStaticMeshComponent(UStaticMeshComponent *InComponent, UObject *InMesh)
{
	UStaticMesh *Mesh = Cast<UStaticMesh>(InMesh);
	if (Mesh)
	{
		InComponent->SetStaticMesh(Mesh);

		return true;
	}

	return false;
}

bool
FHoudiniMeshTranslator::PostCreateHoudiniStaticMeshComponent(UHoudiniStaticMeshComponent *InComponent, UObject *InMesh)
{
	UHoudiniStaticMesh *Mesh = Cast<UHoudiniStaticMesh>(InMesh);
	if (Mesh)
	{
		InComponent->SetMesh(Mesh);

		return true;
	}

	return false;
}

UMeshComponent*
FHoudiniMeshTranslator::CreateOrUpdateMeshComponent(
	const UHoudiniOutput *InOutput, 
	UObject *InOuterComponent, 
	const FHoudiniOutputObjectIdentifier& InOutputIdentifier,
	const TSubclassOf<UMeshComponent>& InComponentType,
	FHoudiniOutputObject& OutputObject,
	FHoudiniGeoPartObject const *& OutFoundHGPO,
	bool& bCreated)
{
	bCreated = false;
	OutFoundHGPO = nullptr;

	// Find the HGPO that matches this mesh
	for (auto& curHGPO : InOutput->HoudiniGeoPartObjects)
	{
		if (curHGPO.ObjectId != InOutputIdentifier.ObjectId
			|| curHGPO.GeoId != InOutputIdentifier.GeoId
			|| curHGPO.PartId != InOutputIdentifier.PartId)
		{
			continue;
		}

		if (InOutputIdentifier.SplitIdentifier.Equals(HAPI_UNREAL_GROUP_GEOMETRY_NOT_COLLISION)
			|| curHGPO.SplitGroups.Contains(InOutputIdentifier.SplitIdentifier))
		{
			OutFoundHGPO = &curHGPO;
		}
	}

	// No need to create a component for instanced meshes!
	if (OutFoundHGPO && OutFoundHGPO->bIsInstanced)
		return nullptr;

	bool bIsProxyComponent = InComponentType == UHoudiniStaticMeshComponent::StaticClass();

	// See if we already have a component for that mesh
	UMeshComponent* MeshComponent = nullptr;
	if (bIsProxyComponent)
		MeshComponent = Cast<UMeshComponent>(OutputObject.ProxyComponent);
	else
		MeshComponent = Cast<UMeshComponent>(OutputObject.OutputComponent);

	// If there is an existing component, but it is pending kill, then it was likely
	// deleted by some other process, such as by the user in the editor, so don't use it
	if (!MeshComponent || MeshComponent->IsPendingKill() || !MeshComponent->IsA(InComponentType))
	{
		// If the component is not of type InComponentType, or the found component is pending kill, destroy 
		// the existing component (a new one is then created below)
		RemoveAndDestroyComponent(MeshComponent);
		MeshComponent = nullptr;
	}

	if (!MeshComponent)
	{
		// Create a new SMC/HSMC as we couldn't find an existing one
		MeshComponent = CreateMeshComponent(InOuterComponent, InComponentType);

		if (MeshComponent)
		{
			// Add to the output object
			if (bIsProxyComponent)
				OutputObject.ProxyComponent = MeshComponent;
			else
				OutputObject.OutputComponent = MeshComponent;

			bCreated = true;
		}
	}

	return MeshComponent;
}

bool 
FHoudiniMeshTranslator::AddActorsToMeshSocket(UStaticMeshSocket * Socket, UStaticMeshComponent * StaticMeshComponent, 
		TArray<AActor*> & HoudiniCreatedSocketActors, TArray<AActor*> & HoudiniAttachedSocketActors)
{
	if (!Socket || Socket->IsPendingKill() || !StaticMeshComponent || StaticMeshComponent->IsPendingKill())
		return false;

	// The actor to assign is stored is the socket's tag
	FString ActorString = Socket->Tag;
	if (ActorString.IsEmpty())
		return false;

	// The actor to assign are listed after a |
	TArray<FString> ActorStringArray;
	ActorString.ParseIntoArray(ActorStringArray, TEXT("|"), false);

	// The "real" Tag is the first
	if (ActorStringArray.Num() > 0)
		Socket->Tag = ActorStringArray[0];

	// We just add a Tag, no Actor
	if (ActorStringArray.Num() == 1)
		return false;

	// Extract the parsed actor string to split it further
	ActorString = ActorStringArray[1];

	// Converting the string to a string array using delimiters
	const TCHAR* Delims[] = { TEXT(","), TEXT(";") };
	ActorString.ParseIntoArray(ActorStringArray, Delims, 2);

	// And try to find the corresponding HoudiniAssetActor in the editor world
	// to avoid finding "deleted" assets with the same name
	//UWorld* editorWorld = GEditor->GetEditorWorldContext().World();
#if WITH_EDITOR
	UWorld* EditorWorld = StaticMeshComponent->GetOwner() ? StaticMeshComponent->GetOwner()->GetWorld() : nullptr;
	if (!EditorWorld || EditorWorld->IsPendingKill())
		return false;

	// Remove the previous created actors which were attached to this socket
	{
		for (int32 Idx = HoudiniCreatedSocketActors.Num() - 1; Idx >= 0; --Idx) 
		{
			AActor * CurActor = HoudiniCreatedSocketActors[Idx];
			if (!CurActor || CurActor->IsPendingKill()) 
			{
				HoudiniCreatedSocketActors.RemoveAt(Idx);
				continue;
			}

			if (CurActor->GetAttachParentSocketName() == Socket->SocketName) 
			{
				HoudiniCreatedSocketActors.RemoveAt(Idx);
				CurActor->Destroy();
			}
		}
	}

	// Detach the previous in level actors which was attached to this socket
	{
		for (int32 Idx = HoudiniAttachedSocketActors.Num() - 1; Idx >= 0; --Idx) 
		{
			AActor * CurActor = HoudiniAttachedSocketActors[Idx];
			if (!CurActor || CurActor->IsPendingKill()) 
			{
				HoudiniAttachedSocketActors.RemoveAt(Idx);
				continue;
			}

			if (CurActor->GetAttachParentSocketName() == Socket->SocketName) 
			{
				CurActor->DetachFromActor(FDetachmentTransformRules::KeepRelativeTransform);
				HoudiniAttachedSocketActors.RemoveAt(Idx);		
			}
		}
	}

	auto CreateDefaultActor = [EditorWorld, StaticMeshComponent, Socket, HoudiniCreatedSocketActors]() 
	{
		AActor * CreatedDefaultActor = nullptr;

		UStaticMesh * DefaultReferenceSM = FHoudiniEngine::Get().GetHoudiniDefaultReferenceMesh().Get();
		if (DefaultReferenceSM && !DefaultReferenceSM->IsPendingKill())
		{
			TArray<AActor*> NewActors = FLevelEditorViewportClient::TryPlacingActorFromObject(
				EditorWorld->GetCurrentLevel(), DefaultReferenceSM, false, RF_Transactional, nullptr);

			if (NewActors.Num() <= 0 || !NewActors[0] || NewActors[0]->IsPendingKill())
			{
				HOUDINI_LOG_WARNING(
					TEXT("Failed to load default mesh."));
			}
			else
			{

				// Set the default mesh actor components mobility to the same as output SMC's
				EComponentMobility::Type OutputSMCMobility = StaticMeshComponent->Mobility;
				for (auto & CurComp : NewActors[0]->GetComponents())
				{
					UStaticMeshComponent * CurSMC = Cast<UStaticMeshComponent>(CurComp);
					if (CurSMC && !CurSMC->IsPendingKill())
						CurSMC->SetMobility(OutputSMCMobility);
				}

				// Set the default mesh actor hidden in game.
				NewActors[0]->SetActorHiddenInGame(true);

				Socket->AttachActor(NewActors[0], StaticMeshComponent);
				CreatedDefaultActor = NewActors[0];
				//HoudiniCreatedSocketActors.Add(NewActors[0]);
			}
		}
		else
		{
			HOUDINI_LOG_WARNING(
				TEXT("Failed to load default mesh."));
		}

		return CreatedDefaultActor;
	};

	bool bUseDefaultActor = true;
	// Get from the Houdini runtime setting if use default object when the reference is invalid
	// true by default if fail to access HoudiniRuntimeSettings
	const UHoudiniRuntimeSettings * HoudiniRuntimeSettings = GetDefault< UHoudiniRuntimeSettings >();
	if (HoudiniRuntimeSettings) 
	{
		bUseDefaultActor = HoudiniRuntimeSettings->bShowDefaultMesh;
	}

	if (ActorStringArray.Num() <= 0) 
	{
		if (!bUseDefaultActor)
			return true;

		HOUDINI_LOG_WARNING(
			TEXT("Output static mesh: Socket '%s' actor is not specified. Spawn a default mesh (hidden in game)."), *(Socket->GetName()));

		AActor * DefaultActor = CreateDefaultActor();
		if (DefaultActor && !DefaultActor->IsPendingKill())
			HoudiniCreatedSocketActors.Add(DefaultActor);

		return true;
	}

	// try to find the actor in level first
	for (TActorIterator<AActor> ActorItr(EditorWorld); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		AActor *Actor = *ActorItr;
		if (!Actor || Actor->IsPendingKillOrUnreachable())
			continue;

		for (int32 StringIdx = 0; StringIdx < ActorStringArray.Num(); StringIdx++)
		{
			if (Actor->GetName() != ActorStringArray[StringIdx]
				&& Actor->GetActorLabel() != ActorStringArray[StringIdx])
				continue;

			// Set the actor components mobility to the same as output SMC's
			EComponentMobility::Type OutputSMCMobility = StaticMeshComponent->Mobility;
			for (auto & CurComp : Actor->GetComponents()) 
			{
				UStaticMeshComponent * SMC = Cast<UStaticMeshComponent>(CurComp);
				if (SMC && !SMC->IsPendingKill())
					SMC->SetMobility(OutputSMCMobility);
			}

			Socket->AttachActor(Actor, StaticMeshComponent);
			HoudiniAttachedSocketActors.Add(Actor);

			// Remove the string if the actor is found in the editor level
			ActorStringArray.RemoveAt(StringIdx);
			break;
		}
	}

	bool bSuccess = true;
	// If some of the actors are not found in the level, try to find them in the content browser. Spawn one if existed
	for (int32 Idx = ActorStringArray.Num() - 1; Idx>= 0; --Idx) 
	{
		UObject * Obj = StaticLoadObject(UObject::StaticClass(), nullptr, *ActorStringArray[Idx]);
		if (!Obj || Obj->IsPendingKill()) 
		{
			bSuccess = false;
			continue;
		}

		// Spawn a new actor with the found object
		TArray<AActor*> NewActors = FLevelEditorViewportClient::TryPlacingActorFromObject(
			EditorWorld->GetCurrentLevel(), Obj, false, RF_Transactional, nullptr);

		if (NewActors.Num() <= 0 || !NewActors[0] || NewActors[0]->IsPendingKill()) 
		{
			bSuccess = false;
			continue;
		}

		// Set the new actor components mobility to the same as output SMC's
		EComponentMobility::Type OutputSMCMobility = StaticMeshComponent->Mobility;
		for (auto & CurComp : NewActors[0]->GetComponents()) 
		{
			UStaticMeshComponent * CurSMC = Cast<UStaticMeshComponent>(CurComp);
			if (CurSMC && !CurSMC->IsPendingKill())
				CurSMC->SetMobility(OutputSMCMobility);
		}

		Socket->AttachActor(NewActors[0], StaticMeshComponent);
		HoudiniCreatedSocketActors.Add(NewActors[0]);

		ActorStringArray.RemoveAt(Idx);
	}

	// Failed to find actors in both level and content browser
	// Spawn default actors if enabled
	if (bUseDefaultActor)
	{
		for (int32 Idx = ActorStringArray.Num() - 1; Idx >= 0; --Idx)
		{
			HOUDINI_LOG_WARNING(
				TEXT("Output static mesh: Failed to attach '%s' to socket '%s', spawn a default mesh (hidden in game)."), *(ActorStringArray[Idx]), *(Socket->GetName()));

			// If failed to load this object, spawn a default mesh
			AActor * CurDefaultActor = CreateDefaultActor();
			if (CurDefaultActor && !CurDefaultActor->IsPendingKill())
				HoudiniCreatedSocketActors.Add(CurDefaultActor);
		}
	}

	if (ActorStringArray.Num() > 0)
		return false;
#endif

	return bSuccess;
}

void
FHoudiniMeshTranslator::SetMeshBuildSettings(
	FMeshBuildSettings& OutMeshBuildSettings,
	const bool& bHasNormals, 
	const bool& bHasTangents, 
	const bool& bHasLightmapUVSet)
{
	const UHoudiniRuntimeSettings* HoudiniRuntimeSettings = GetDefault<UHoudiniRuntimeSettings>();
	OutMeshBuildSettings.bRemoveDegenerates = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bRemoveDegenerates : true;
	OutMeshBuildSettings.bUseMikkTSpace = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bUseMikkTSpace : true;
	OutMeshBuildSettings.bBuildAdjacencyBuffer = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bBuildAdjacencyBuffer : false;
	OutMeshBuildSettings.MinLightmapResolution = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->MinLightmapResolution : 64;
	OutMeshBuildSettings.bUseFullPrecisionUVs = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bUseFullPrecisionUVs : false;
	OutMeshBuildSettings.SrcLightmapIndex = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->SrcLightmapIndex : 0;
	OutMeshBuildSettings.DstLightmapIndex = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->DstLightmapIndex : 1;

	OutMeshBuildSettings.bComputeWeightedNormals = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bComputeWeightedNormals : false;
	OutMeshBuildSettings.bBuildReversedIndexBuffer = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bBuildReversedIndexBuffer : true;
	OutMeshBuildSettings.bUseHighPrecisionTangentBasis = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bUseHighPrecisionTangentBasis : false;
	OutMeshBuildSettings.bGenerateDistanceFieldAsIfTwoSided = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bGenerateDistanceFieldAsIfTwoSided : false;
	OutMeshBuildSettings.bSupportFaceRemap = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->bSupportFaceRemap : false;
	OutMeshBuildSettings.DistanceFieldResolutionScale = HoudiniRuntimeSettings ? HoudiniRuntimeSettings->DistanceFieldResolutionScale : 2.0f;

	// Recomputing normals.
	EHoudiniRuntimeSettingsRecomputeFlag RecomputeNormalFlag = HoudiniRuntimeSettings ? (EHoudiniRuntimeSettingsRecomputeFlag)HoudiniRuntimeSettings->RecomputeNormalsFlag : HRSRF_OnlyIfMissing;
	switch (RecomputeNormalFlag)
	{
		case HRSRF_Always:
		{
			OutMeshBuildSettings.bRecomputeNormals = true;
			break;
		}

		case HRSRF_OnlyIfMissing:
		{
			OutMeshBuildSettings.bRecomputeNormals = !bHasNormals;
			break;
		}

		case HRSRF_Never:
		default:
		{
			OutMeshBuildSettings.bRecomputeNormals = false;
			break;
		}
	}

	// Recomputing tangents.
	EHoudiniRuntimeSettingsRecomputeFlag RecomputeTangentFlag = HoudiniRuntimeSettings ? (EHoudiniRuntimeSettingsRecomputeFlag)HoudiniRuntimeSettings->RecomputeTangentsFlag : HRSRF_OnlyIfMissing;
	switch (RecomputeTangentFlag)
	{
		case HRSRF_Always:
		{
			OutMeshBuildSettings.bRecomputeTangents = true;
			break;
		}

		case HRSRF_OnlyIfMissing:
		{
			OutMeshBuildSettings.bRecomputeTangents = !bHasTangents;
			break;
		}

		case HRSRF_Never:
		default:
		{
			OutMeshBuildSettings.bRecomputeTangents = false;
			break;
		}
	}

	// Lightmap UV generation.
	EHoudiniRuntimeSettingsRecomputeFlag GenerateLightmapUVFlag = HoudiniRuntimeSettings ? (EHoudiniRuntimeSettingsRecomputeFlag)HoudiniRuntimeSettings->RecomputeTangentsFlag : HRSRF_OnlyIfMissing;
	switch (GenerateLightmapUVFlag)
	{
		case HRSRF_Always:
		{
			OutMeshBuildSettings.bGenerateLightmapUVs = true;
			break;
		}

		case HRSRF_OnlyIfMissing:
		{
			OutMeshBuildSettings.bGenerateLightmapUVs = !bHasLightmapUVSet;
			break;
		}

		case HRSRF_Never:
		default:
		{
			OutMeshBuildSettings.bGenerateLightmapUVs = false;
			break;
		}
	}
}

#undef LOCTEXT_NAMESPACE