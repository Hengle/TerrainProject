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

#include "HoudiniOutputTranslator.h"

#include "HoudiniOutput.h"
#include "HoudiniApi.h"
#include "HoudiniEngine.h"

#include "HoudiniEngineUtils.h"
#include "HoudiniEngineString.h"
#include "HoudiniGeoPartObject.h"
#include "HoudiniEnginePrivatePCH.h"
#include "HoudiniAsset.h"
#include "HoudiniAssetActor.h"
#include "HoudiniAssetComponent.h"
#include "HoudiniSplineComponent.h"
#include "HoudiniEngineRuntime.h"
#include "HoudiniInput.h"
#include "HoudiniStaticMesh.h"

#include "HoudiniMeshTranslator.h"
#include "HoudiniSplineTranslator.h"
#include "HoudiniLandscapeTranslator.h"
#include "HoudiniInstanceTranslator.h"

#include "Editor.h"
#include "EditorSupportDelegates.h"
#include "FileHelpers.h"
#include "LandscapeInfo.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Engine/WorldComposition.h"
#include "Modules/ModuleManager.h"
#include "WorldBrowserModule.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "InstancedFoliageActor.h"

#define LOCTEXT_NAMESPACE HOUDINI_LOCTEXT_NAMESPACE

// 
bool
FHoudiniOutputTranslator::UpdateOutputs(UHoudiniAssetComponent* HAC, const bool& bInForceUpdate, bool& bOutHasHoudiniStaticMeshOutput)
{
	if (!HAC || HAC->IsPendingKill())
		return false;

	// Get the temp folder override
	FHoudiniOutputTranslator::GetTempFolderFromAttribute(HAC);

	// Outputs that should be cleared, but only AFTER new output processing have taken place.
	// This is needed for landscape resizing where the new landscape needs to copy data from the original landscape
	// before the original landscape gets destroyed.
	TArray<UHoudiniOutput*> DeferredClearOutputs;

	// Check if the HDA has been marked as not producing outputs
	if (!HAC->bOutputless)
	{
		// Check if we want to convert legacy v1 data
		const UHoudiniRuntimeSettings * HoudiniRuntimeSettings = GetDefault<UHoudiniRuntimeSettings>();
		bool bEnableBackwardCompatibility = HoudiniRuntimeSettings->bEnableBackwardCompatibility;
		if (bEnableBackwardCompatibility && HAC->Version1CompatibilityHAC)
		{
			// Do not reuse legacy outputs!
			for (auto& OldOutput : HAC->Outputs)
			{
				ClearOutput(OldOutput);
			}
		}

		TArray<UHoudiniOutput*> NewOutputs;
		if (FHoudiniOutputTranslator::BuildAllOutputs(HAC->GetAssetId(), HAC, HAC->Outputs, NewOutputs, HAC->bOutputTemplateGeos))
		{
			// NOTE: For now we are currently forcing all outputs to be cleared here. There is still an issue where, in some
			// circumstances, landscape tiles disappear when clearing outputs after processing.
			// The reason we may need to defer landscape clearing is to allow the landscape creation code to
			// capture the extent of the landscape. The extent of the landscape can only be calculated if all landscape
			// tiles are still present in the map. If we find that we don't need this for updating of Input landscapes,
			// we can safely remove this feature.
			ClearAndRemoveOutputs(HAC, DeferredClearOutputs, true);
			// Replace with the new parameters
			HAC->Outputs = NewOutputs;
		}
	}
	else
	{
		// This HDA is marked as not supposed to produce any output
		ClearAndRemoveOutputs(HAC, DeferredClearOutputs, true);
	}

	// Look for details generic property attributes on the outputs,
	// and try to apply them to the HAC.
	// This can be used to preset some of the HDA's uproperty via attribute
	TArray<FHoudiniGenericAttribute> GenericAttributes;
	for (auto& CurrentOutput : HAC->Outputs)
	{
		const TArray<FHoudiniGeoPartObject>& CurrentOutputHGPO = CurrentOutput->GetHoudiniGeoPartObjects();
		for (auto& CurrentHGPO : CurrentOutputHGPO)
		{
			FHoudiniEngineUtils::GetGenericAttributeList(
				CurrentHGPO.GeoId,
				CurrentHGPO.PartId,
				HAPI_UNREAL_ATTRIB_GENERIC_UPROP_PREFIX, 
				GenericAttributes,
				HAPI_ATTROWNER_DETAIL);
		}
	}

	// Attempt to apply the attributes to the HAC if we have any
	for (const auto& CurrentPropAttribute : GenericAttributes)
	{
		// Get the current Property Attribute
		const FString& CurrentPropertyName = CurrentPropAttribute.AttributeName;
		if (CurrentPropertyName.IsEmpty())
			continue;

		if (!FHoudiniGenericAttribute::UpdatePropertyAttributeOnObject(HAC, CurrentPropAttribute))
			continue;

		// Success!
		HOUDINI_LOG_MESSAGE(TEXT("Modified UProperty %s on Houdini Asset Component named %s"), *CurrentPropertyName, *HAC->GetName());
	}
	
	// NOTE: PersistentWorld can be NULL when, for example, working with
	// HoudiniAssetComponents in Blueprints.
	UWorld* PersistentWorld = HAC->GetWorld();
	UWorldComposition* WorldComposition = nullptr;
	if (PersistentWorld)
	{
		WorldComposition = PersistentWorld->WorldComposition;
	}
	
	if (IsValid(WorldComposition))
	{
		// We don't want the origin to shift as we're potentially updating levels.
		WorldComposition->bTemporarilyDisableOriginTracking = true;
	}

	// "Process" the mesh.
	// TODO: Move this to the actual processing stage,
	// And see if some of this could be threaded
	UObject* OuterComponent = HAC;
	
	FString HoudiniAssetPath = FPaths::GetPath(HAC->GetPathName());
	FString ComponentGUIDString = HAC->GetComponentGUID().ToString().Left(FHoudiniEngineUtils::PackageGUIDComponentNameLength);
	FString HoudiniAssetNameString = HAC->GetDisplayName();

	FHoudiniPackageParams PackageParams;
	PackageParams.PackageMode = FHoudiniPackageParams::GetDefaultStaticMeshesCookMode();
	PackageParams.ReplaceMode = FHoudiniPackageParams::GetDefaultReplaceMode();

	PackageParams.BakeFolder = FHoudiniEngineRuntime::Get().GetDefaultBakeFolder();
	PackageParams.TempCookFolder = FHoudiniEngineRuntime::Get().GetDefaultTemporaryCookFolder();

	PackageParams.OuterPackage = HAC->GetComponentLevel();
	PackageParams.HoudiniAssetName = HAC->GetHoudiniAsset() ? HAC->GetHoudiniAsset()->GetName() : FString();
	PackageParams.HoudiniAssetActorName = HAC->GetOwner()->GetName();
	PackageParams.ComponentGUID = HAC->GetComponentGUID();
	PackageParams.ObjectName = FString();

	// ----------------------------------------------------
	// Outputs prepass
	// ----------------------------------------------------
	
	TArray<UPackage*> CreatedWorldCompositionPackages;
	bool bCreatedNewMaps = false;
	//...  for heightfield outputs  ...//

	// Collect all the landscape layers' global min/max values.
	TMap<FString, float> LandscapeLayerGlobalMinimums;
	TMap<FString, float> LandscapeLayerGlobalMaximums;
	
	// Store the instancer outputs separately so we can process them later, after all mesh output are processed.
	// Determine the total number of instances, if we have more than 1 then mesh parts with instanced geo we will not create proxy meshes
	// Also if we have object instancer (or oldschool attribute instancers), we won't be creating any proxy at all
	TArray<UHoudiniOutput*> InstancerOutputs;
	int32 NumInstances = 0;
	bool bHasObjectInstancer = false;
	
	for (auto& CurOutput : HAC->Outputs)
	{
		if (CurOutput->GetType() == EHoudiniOutputType::Instancer)
		{
			// InstancerOutputs.Add(CurOutput);
			for (const FHoudiniGeoPartObject &HGPO : CurOutput->GetHoudiniGeoPartObjects())
			{
				if (HGPO.Type == EHoudiniPartType::Instancer)
				{
					if (HGPO.InstancerType == EHoudiniInstancerType::PackedPrimitive)
					{
						NumInstances += HGPO.PartInfo.InstanceCount;
					}
					else
					{
						NumInstances += HGPO.PartInfo.PointCount;
					}

					if ((HGPO.InstancerType == EHoudiniInstancerType::ObjectInstancer)
						|| (HGPO.InstancerType == EHoudiniInstancerType::OldSchoolAttributeInstancer))
					{
						bHasObjectInstancer = true;
					}
				}
			}
		}
		else if (CurOutput->GetType() == EHoudiniOutputType::Landscape)
		{
			FHoudiniLandscapeTranslator::CalcHeightfieldsArrayGlobalZMinZMax(CurOutput->GetHoudiniGeoPartObjects(), LandscapeLayerGlobalMinimums, LandscapeLayerGlobalMaximums, false);
		}
	}

	bOutHasHoudiniStaticMeshOutput = false;
	int32 NumVisibleOutputs = 0;
	int32 NumOutputs = HAC->Outputs.Num();
	bool bHasLandscape = false;

	// Before processing all the outputs, 
	// See if we have any landscape input that have "Update Input Landscape" enabled
	// And make an array of all our input landscapes
	TArray<ALandscapeProxy *> AllInputLandscapes;
	TArray<ALandscapeProxy *> InputLandscapesToUpdate;
	
	for (auto CurrentInput : HAC->Inputs)
	{
		if (CurrentInput->GetInputType() != EHoudiniInputType::Landscape)
			continue;

		// Get the landscape input's landscape
		ALandscapeProxy* InputLandscape = Cast<ALandscapeProxy>(CurrentInput->GetInputObjectAt(0));
		if (!InputLandscape)
			continue;

		AllInputLandscapes.Add(InputLandscape);

		if (CurrentInput->GetUpdateInputLandscape())
			InputLandscapesToUpdate.Add(InputLandscape);
	}

	// ----------------------------------------------------
	// Process outputs
	// ----------------------------------------------------
	// Landscape creation will cache the first tile as a reference location
	// in this struct to be used by during construction of subsequent tiles.
	FHoudiniLandscapeReferenceLocation LandscapeReferenceLocation;
	// Landscape Size info will be cached by the first tile, similar to LandscapeReferenceLocation
	FHoudiniLandscapeTileSizeInfo LandscapeSizeInfo;
	FHoudiniLandscapeExtent LandscapeExtent;
	
	TArray<UPackage*> CreatedPackages;
	for (int32 OutputIdx = 0; OutputIdx < NumOutputs; OutputIdx++)
	{
		UHoudiniOutput* CurOutput = HAC->GetOutputAt(OutputIdx);
		if (!CurOutput || CurOutput->IsPendingKill())
			continue;

		FString Notification = FString::Format(TEXT("Processing output {0} / {1}..."), {FString::FromInt(OutputIdx + 1), FString::FromInt(NumOutputs)});
		FHoudiniEngine::Get().UpdateTaskSlateNotification(FText::FromString(Notification));

		if (!HAC->IsOutputTypeSupported(CurOutput->GetType()))
			continue;

		switch (CurOutput->GetType())
		{
			case EHoudiniOutputType::Mesh:
			{
				bool bIsProxyStaticMeshEnabled = (
					HAC->IsProxyStaticMeshEnabled() &&
					!HAC->HasNoProxyMeshNextCookBeenRequested() &&
					!HAC->IsBakeAfterNextCookEnabled());
				if (bIsProxyStaticMeshEnabled && NumInstances > 1)
				{
					if (bHasObjectInstancer)
					{
						// Completely disable proxies if we have object instancers/old school attribute instancers
						// as they rely on having a static mesh created (and the instanced mesh HGPO is not marked as instanced...)
						bIsProxyStaticMeshEnabled = false;
					}
					else
					{
						// If we dont have proxy instancer, enable proxy only for non-instanced mesh
						for (const FHoudiniGeoPartObject &HGPO : CurOutput->GetHoudiniGeoPartObjects())
						{
							if (HGPO.bIsInstanced && HGPO.Type == EHoudiniPartType::Mesh)
							{
								bIsProxyStaticMeshEnabled = false;
								break;
							}
						}
					}
				}

				FHoudiniMeshTranslator::CreateAllMeshesAndComponentsFromHoudiniOutput(
					CurOutput, 
					PackageParams, 
					bIsProxyStaticMeshEnabled ? EHoudiniStaticMeshMethod::UHoudiniStaticMesh : HAC->StaticMeshMethod,
					HAC->StaticMeshGenerationProperties,
					OuterComponent);

				NumVisibleOutputs++;

				// Look for UHoudiniStaticMesh in the output, and set bOutHasHoudiniStaticMeshOutput accordingly
				if (bIsProxyStaticMeshEnabled && !bOutHasHoudiniStaticMeshOutput)
				{
					bOutHasHoudiniStaticMeshOutput &= CurOutput->HasAnyCurrentProxy();
				}

				break;
			}

			case EHoudiniOutputType::Curve:
			{
				const TArray<FHoudiniGeoPartObject> &GeoPartObjects = CurOutput->GetHoudiniGeoPartObjects();

				if (GeoPartObjects.Num() <= 0)
					continue;

				const FHoudiniGeoPartObject & CurHGPO = GeoPartObjects[0];

				if (CurOutput->IsEditableNode())
				{
					if (!CurOutput->HasEditableNodeBuilt())
					{
						// Editable curve, only need to be built once. 
						UHoudiniSplineComponent* HoudiniSplineComponent = FHoudiniSplineTranslator::CreateHoudiniSplineComponentFromHoudiniEditableNode(
							CurHGPO.GeoId, 
							CurHGPO.PartName,
							HAC);

						HoudiniSplineComponent->SetIsEditableOutputCurve(true);

						FHoudiniOutputObjectIdentifier EditableSplineComponentIdentifier;
						EditableSplineComponentIdentifier.ObjectId = CurHGPO.ObjectId;
						EditableSplineComponentIdentifier.GeoId = CurHGPO.GeoId;
						EditableSplineComponentIdentifier.PartId = CurHGPO.PartId;
						EditableSplineComponentIdentifier.PartName = CurHGPO.PartName;
						
						TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject>& OutputObjects = CurOutput->GetOutputObjects();
						FHoudiniOutputObject& FoundOutputObject = OutputObjects.FindOrAdd(EditableSplineComponentIdentifier);
						FoundOutputObject.OutputComponent = HoudiniSplineComponent;

						CurOutput->SetHasEditableNodeBuilt(true);
					}
				}
				else
				{	
					// Output curve
					FHoudiniSplineTranslator::CreateAllSplinesFromHoudiniOutput(CurOutput, OuterComponent);
					NumVisibleOutputs += CurOutput->GetOutputObjects().Num();
					break;
				}
			}
			break;

		case EHoudiniOutputType::Instancer:
			InstancerOutputs.Add(CurOutput);
			break;

		case EHoudiniOutputType::Landscape:
		{
			NumVisibleOutputs++;

			// This gets called for each heightfield primitive from Houdini, i.e., each "tile".
			bool bNewMapCreated = false;
			// Registering of untracked actors is not currently used in the HDA
			// workflow. HDA cleanup will manually search for shared landscapes
			// and remove them. That aforementioned behaviour should really be updated to 
			// make use of untracked actors on the HAC (similar to PDG Asset Link).
			TArray<TWeakObjectPtr<AActor>> UntrackedActors;

			FHoudiniLandscapeTranslator::CreateLandscape(
				CurOutput,
				UntrackedActors,
				InputLandscapesToUpdate,
				AllInputLandscapes,
				HAC,
				TEXT("{hda_actor_name}_"),
				PersistentWorld,
				LandscapeLayerGlobalMinimums,
				LandscapeLayerGlobalMaximums,
				LandscapeExtent,
				LandscapeSizeInfo,
				LandscapeReferenceLocation,
				PackageParams,
				CreatedPackages);

			bHasLandscape = true;

			// Attach the created landscape to the parent HAC.
			ALandscapeProxy* OutputLandscape = nullptr;
			for (auto& Pair : CurOutput->GetOutputObjects()) 
			{
				UHoudiniLandscapePtr* LandscapePtr = Cast<UHoudiniLandscapePtr>(Pair.Value.OutputObject);
				OutputLandscape = LandscapePtr->GetRawPtr();
				break;
			}

			if (OutputLandscape) 
			{
				// Attach the created landscapes to HAC
				// Output Transforms are always relative to the HDA
				HAC->SetMobility(EComponentMobility::Static);
				OutputLandscape->AttachToComponent(HAC, FAttachmentTransformRules::KeepRelativeTransform);
				// Note that the above attach will cause the collision components to crap out. This manifests
				// itself via the Landscape editor tools not being able to trace Landscape collision components.
				// By recreating collision components here, it appears to put things back into working order.
				OutputLandscape->GetLandscapeInfo()->FixupProxiesTransform();
				OutputLandscape->GetLandscapeInfo()->RecreateLandscapeInfo(PersistentWorld, true);
				OutputLandscape->RecreateCollisionComponents();
			}

			bCreatedNewMaps |= bNewMapCreated;
			break;
		}
		default:
			// Do Nothing for now
			break;
		}
	}

	// Now that all meshes have been created, process the instancers
	for (auto& CurOutput : InstancerOutputs)
	{
		FHoudiniInstanceTranslator::CreateAllInstancersFromHoudiniOutput(CurOutput, HAC->Outputs, OuterComponent);
		NumVisibleOutputs++;
	}

	if (NumVisibleOutputs > 0)
	{
		// If we have valid outputs, we don't need to display the houdini logo anymore...
		FHoudiniEngineUtils::RemoveHoudiniLogoFromComponent(HAC);
	}
	else
	{
		// ... if we don't have any valid outputs however, we should
		FHoudiniEngineUtils::AddHoudiniLogoToComponent(HAC);
	}

	// Clear any old outputs that was marked as "Should Defer Clear".
	// This should happen before SharedLandscapeActor cleanup
	// since this needs to remove old landscape proxies so that empty SharedLandscapeActors
	// can be removed afterward.
	HOUDINI_LANDSCAPE_MESSAGE(TEXT("[HoudiniOutputTranslator::UpdateOutputs] Clearing old outputs: %d"), DeferredClearOutputs.Num());
	for(UHoudiniOutput* OldOutput : DeferredClearOutputs)
	{
		ClearOutput(OldOutput);
	}

	// if (IsValid(LandscapeExtents.IntermediateResizeLandscape))
	// {
	// 	LandscapeExtents.IntermediateResizeLandscape->Destroy();
	// 	LandscapeExtents.IntermediateResizeLandscape = nullptr;
	// }

	if (bHasLandscape)
	{
		// ----------------------------------------------------
		// Cleanup untracked shared landscape actors
		// ----------------------------------------------------
		// This is a nasty hack to clean up SharedLandscape actors generated by the
		// Landscape translator but aren't tracked by an HoudiniOutputObject, since the
		// translators can't dynamically create outputs.

		{
			// First collect all the landscapes that is being tracked by the HAC.
			TSet<ALandscapeProxy*> TrackedLandscapes;
			for(UHoudiniOutput* Output : HAC->Outputs)
			{
				if (Output->GetType() == EHoudiniOutputType::Landscape)
				{
					for(auto& Elem : Output->GetOutputObjects())
					{
						UHoudiniLandscapePtr* LandscapePtr = Cast<UHoudiniLandscapePtr>(Elem.Value.OutputObject);
						if (!IsValid(LandscapePtr))
							continue;
						ALandscapeProxy* LandscapeProxy = LandscapePtr->GetRawPtr();
						if (IsValid(LandscapeProxy))
						{
							TrackedLandscapes.Add(LandscapeProxy);

							// We need to recreate component states for landscapes if a tile was created, moved, or resized
							// otherwise the landscape will exhibit render artifacts (such as only rendering every other
							// component.)
							LandscapeProxy->RecreateComponentsState();
						}
					}
				}
			}

			// Iterate over Houdini asset child assets in order to find dangling Landscape actors
			TArray<USceneComponent*> AttachedComponents = HAC->GetAttachChildren();
			for(USceneComponent* Component : AttachedComponents)
			{
				if (!IsValid(Component))
					continue;
				AActor* Actor = Component->GetOwner();
				ALandscape* Landscape = Cast<ALandscape>(Actor);
				if (!Landscape)
					continue;
				if (TrackedLandscapes.Contains(Landscape))
					continue;

				if (Landscape->GetLandscapeInfo()->Proxies.Num() == 0)
					Landscape->Destroy();
			}
		}

		// Recreate Landscape Info calls WorldChange, so no need to do it manually.
		ULandscapeInfo::RecreateLandscapeInfo(PersistentWorld, true);
	}

	// Destroy the intermediate resize landscape, if there is one.
	// if (IsValid(LandscapeExtents.IntermediateResizeLandscape))
	// {
	// 	FHoudiniLandscapeTranslator::DestroyLandscape(LandscapeExtents.IntermediateResizeLandscape);
	// }

	if (IsValid(WorldComposition))
	{
		// Disable the flag that we set before starting the import process.
		WorldComposition->bTemporarilyDisableOriginTracking = false;
	}

	// If the owner component was marked as loaded, unmark all outputs
	if (HAC->HasBeenLoaded())
	{
		for (auto& CurrentOutput : HAC->Outputs)
		{
			CurrentOutput->MarkAsLoaded(false);
		}
	}

	if (bCreatedNewMaps)
	{
		// Force the asset registry to update its cache of packages paths
		// recursively for this world, otherwise world composition won't
		// pick them up during the WorldComposition::Rescan().
		FHoudiniEngineUtils::RescanWorldPath(PersistentWorld);

		ULandscapeInfo::RecreateLandscapeInfo(PersistentWorld, true);

		FHoudiniEngineUtils::LogWorldInfo(PersistentWorld);
		if (WorldComposition)
		{
			UWorldComposition::WorldCompositionChangedEvent.Broadcast(PersistentWorld);
		}

		FEditorDelegates::RefreshLevelBrowser.Broadcast();
		FEditorDelegates::RefreshAllBrowsers.Broadcast();
	}

	if (CreatedPackages.Num() > 0)
	{
		// Save created packages. For example, we don't want landscape layers deleted 
		// along with the HDA.
		FEditorFileUtils::PromptForCheckoutAndSave(CreatedPackages, true, false);
	}

	return true;
}

bool
FHoudiniOutputTranslator::BuildStaticMeshesOnHoudiniProxyMeshOutputs(UHoudiniAssetComponent* HAC, bool bInDestroyProxies)
{
	if (!HAC || HAC->IsPendingKill())
		return false;

	UObject* OuterComponent = HAC;

	FHoudiniPackageParams PackageParams;
	PackageParams.PackageMode = FHoudiniPackageParams::GetDefaultStaticMeshesCookMode();
	PackageParams.ReplaceMode = FHoudiniPackageParams::GetDefaultReplaceMode();

	PackageParams.BakeFolder = FHoudiniEngineRuntime::Get().GetDefaultBakeFolder();
	PackageParams.TempCookFolder = FHoudiniEngineRuntime::Get().GetDefaultTemporaryCookFolder();

	PackageParams.OuterPackage = HAC->GetComponentLevel();
	PackageParams.HoudiniAssetName = HAC->GetHoudiniAsset() ? HAC->GetHoudiniAsset()->GetName() : FString();
	PackageParams.HoudiniAssetActorName = HAC->GetOwner()->GetName();
	PackageParams.ComponentGUID = HAC->GetComponentGUID();
	PackageParams.ObjectName = FString();

	bool bFoundProxies = false;
	TArray<UHoudiniOutput*> InstancerOutputs;
	for (auto& CurOutput : HAC->Outputs)
	{
		const EHoudiniOutputType OutputType = CurOutput->GetType();
		if (OutputType == EHoudiniOutputType::Mesh)
		{
			if (CurOutput->HasAnyCurrentProxy())
			{
				bFoundProxies = true;
				FHoudiniMeshTranslator::CreateAllMeshesAndComponentsFromHoudiniOutput(
					CurOutput,
					PackageParams,
					HAC->StaticMeshMethod != EHoudiniStaticMeshMethod::UHoudiniStaticMesh ? HAC->StaticMeshMethod : EHoudiniStaticMeshMethod::RawMesh,
					HAC->StaticMeshGenerationProperties,
					OuterComponent,
					true,  // bInTreatExistingMaterialsAsUpToDate
					bInDestroyProxies
				);  
			}
		}
		else if (OutputType == EHoudiniOutputType::Instancer)
		{
			InstancerOutputs.Add(CurOutput);
		}
	}

	// Rebuild instancers if we built any static meshes from proxies
	if (bFoundProxies)
	{
		for (auto& CurOutput : InstancerOutputs)
		{
			FHoudiniInstanceTranslator::CreateAllInstancersFromHoudiniOutput(CurOutput, HAC->Outputs, OuterComponent);
		}
	}

	return true;
}

//
bool
FHoudiniOutputTranslator::UpdateLoadedOutputs(UHoudiniAssetComponent* HAC)
{
	HAPI_NodeId & AssetId = HAC->AssetId;
	// Get the AssetInfo
	HAPI_AssetInfo AssetInfo;
	FHoudiniApi::AssetInfo_Init(&AssetInfo);
	HOUDINI_CHECK_ERROR_RETURN(FHoudiniApi::GetAssetInfo(
		FHoudiniEngine::Get().GetSession(), AssetId, &AssetInfo), false);

	// Retrieve information about each object contained within our asset.
	TArray< HAPI_ObjectInfo > ObjectInfos;
	if (!FHoudiniEngineUtils::HapiGetObjectInfos(AssetId, ObjectInfos))
		return false;

	TArray<HAPI_NodeId> EditableCurveObjIds;
	TArray<HAPI_NodeId> EditableCurveGeoIds;
	TArray<HAPI_NodeId> EditableCurvePartIds;
	TArray<FString> EditableCurvePartNames;

	// Iterate through all objects to get all editable curve's object geo and part Ids.

	for (int32 ObjectId = 0; ObjectId < ObjectInfos.Num(); ++ObjectId)
	{
		// Retrieve the object info
		const HAPI_ObjectInfo& CurrentHapiObjectInfo = ObjectInfos[ObjectId];

		// Cache/convert them
		FHoudiniObjectInfo CurrentObjectInfo;
		CacheObjectInfo(CurrentHapiObjectInfo, CurrentObjectInfo);

		// Start by getting the number of editable nodes
		int32 EditableNodeCount = 0;
		HOUDINI_CHECK_ERROR(FHoudiniApi::ComposeChildNodeList(
			FHoudiniEngine::Get().GetSession(),
			CurrentHapiObjectInfo.nodeId, HAPI_NODETYPE_SOP, HAPI_NODEFLAGS_EDITABLE,
			true, &EditableNodeCount));

		if (EditableNodeCount > 0)
		{
			TArray< HAPI_NodeId > EditableNodeIds;
			EditableNodeIds.SetNumUninitialized(EditableNodeCount);
			HOUDINI_CHECK_ERROR(FHoudiniApi::GetComposedChildNodeList(
				FHoudiniEngine::Get().GetSession(),
				AssetId, EditableNodeIds.GetData(), EditableNodeCount));

			for (int32 nEditable = 0; nEditable < EditableNodeCount; nEditable++)
			{
				HAPI_GeoInfo CurrentEditableGeoInfo;
				FHoudiniApi::GeoInfo_Init(&CurrentEditableGeoInfo);
				HOUDINI_CHECK_ERROR(FHoudiniApi::GetGeoInfo(
					FHoudiniEngine::Get().GetSession(),
					EditableNodeIds[nEditable], &CurrentEditableGeoInfo));

				// Do not process the main display geo twice!
				if (CurrentEditableGeoInfo.isDisplayGeo)
					continue;

				// We only handle editable curves for now
				if (CurrentEditableGeoInfo.type != HAPI_GEOTYPE_CURVE)
					continue;

				// Only catch editable curves
				if (CurrentEditableGeoInfo.type != HAPI_GeoType::HAPI_GEOTYPE_CURVE)
					continue;

				// Cook the editable node to get its parts
				if (CurrentEditableGeoInfo.partCount <= 0)
				{
					//HAPI_CookOptions CookOptions = FHoudiniEngine::GetDefaultCookOptions();
					//FHoudiniApi::CookNode(FHoudiniEngine::Get().GetSession(), CurrentEditableGeoInfo.nodeId, &CookOptions);
					FHoudiniEngineUtils::HapiCookNode(CurrentEditableGeoInfo.nodeId, nullptr, true);

					HOUDINI_CHECK_ERROR(FHoudiniApi::GetGeoInfo(
						FHoudiniEngine::Get().GetSession(),
						CurrentEditableGeoInfo.nodeId,
						&CurrentEditableGeoInfo));
				}

				// Iterate on this geo's parts
				for (int32 PartId = 0; PartId < CurrentEditableGeoInfo.partCount; ++PartId)
				{
					// Get part information.
					HAPI_PartInfo CurrentHapiPartInfo;
					FHoudiniApi::PartInfo_Init(&CurrentHapiPartInfo);

					if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetPartInfo(
						FHoudiniEngine::Get().GetSession(), CurrentEditableGeoInfo.nodeId, PartId, &CurrentHapiPartInfo))
						continue;

					if (CurrentHapiPartInfo.type != HAPI_PartType::HAPI_PARTTYPE_CURVE)
						continue;

					// Get the editable curve's part name
					FHoudiniEngineString hapiSTR(CurrentHapiPartInfo.nameSH);
					FString PartName;
					hapiSTR.ToFString(PartName);
					
					EditableCurveObjIds.Add(CurrentHapiObjectInfo.nodeId);
					EditableCurveGeoIds.Add(CurrentEditableGeoInfo.nodeId);
					EditableCurvePartIds.Add(CurrentHapiPartInfo.id);
					EditableCurvePartNames.Add(PartName);				
				}
			}
		}

	}

	int32 Idx = 0;
	for (auto& CurrentOutput : HAC->Outputs)
	{
		if (CurrentOutput->IsEditableNode()) 
		{
			// The HAC is Loaded, re-assign node id to its editable curves
			if (CurrentOutput->HasEditableNodeBuilt())
			{
				TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject>& OutputObjects = CurrentOutput->GetOutputObjects();
				for (auto& Pair : OutputObjects)
				{
					if (Idx >= EditableCurvePartIds.Num())
						break;

					UHoudiniSplineComponent * HoudiniSplineComponent = Cast<UHoudiniSplineComponent>(Pair.Value.OutputComponent);
					if (HoudiniSplineComponent && !HoudiniSplineComponent->IsPendingKill())
					{
						HoudiniSplineComponent->SetNodeId(EditableCurveGeoIds[Idx]);

						Pair.Key.ObjectId = EditableCurveObjIds[Idx];
						Pair.Key.GeoId = EditableCurveGeoIds[Idx];
						Pair.Key.PartId = EditableCurvePartIds[Idx];
						Pair.Key.PartName = EditableCurvePartNames[Idx];

						Idx += 1;
					}
				}
			}
			// The HAC is a Duplication, re-construct output objects with attached duplicated editable curves, matching by part name
			else 
			{
				const TArray<USceneComponent*> &Children = HAC->GetAttachChildren();
				for (auto & CurAttachedComp : Children) 
				{
					if (!CurAttachedComp || CurAttachedComp->IsPendingKill())
						continue;

					if (!CurAttachedComp->IsA<UHoudiniSplineComponent>())
						continue;

					UHoudiniSplineComponent * CurAttachedSplineComp = Cast<UHoudiniSplineComponent>(CurAttachedComp);
					if (!CurAttachedSplineComp)
						continue;

					if (!CurAttachedSplineComp->IsEditableOutputCurve())
						continue;

					if (Idx >= EditableCurvePartIds.Num())
						break;

					// Found a match
					if (CurAttachedSplineComp->GetGeoPartName().Equals(EditableCurvePartNames[Idx])) 
					{
						FHoudiniOutputObjectIdentifier EditableSplineComponentIdentifier;
						EditableSplineComponentIdentifier.ObjectId = EditableCurveObjIds[Idx];
						EditableSplineComponentIdentifier.GeoId = EditableCurveGeoIds[Idx];
						EditableSplineComponentIdentifier.PartId = EditableCurvePartIds[Idx];
						EditableSplineComponentIdentifier.PartName = EditableCurvePartNames[Idx];

						CurAttachedSplineComp->SetNodeId(EditableSplineComponentIdentifier.GeoId);

						TMap<FHoudiniOutputObjectIdentifier, FHoudiniOutputObject>& OutputObjects = CurrentOutput->GetOutputObjects();
						FHoudiniOutputObject& NewOutputObject = OutputObjects.FindOrAdd(EditableSplineComponentIdentifier);
						NewOutputObject.OutputComponent = CurAttachedSplineComp;

						CurrentOutput->SetHasEditableNodeBuilt(true);
						FHoudiniSplineTranslator::HapiUpdateNodeForHoudiniSplineComponent(CurAttachedSplineComp);
						
						Idx += 1;
						break;
					}
				}
			}
		}
		else 
		{
			// Output curve
			FHoudiniSplineTranslator::CreateAllSplinesFromHoudiniOutput(CurrentOutput, HAC); 
		}
		
		// Mark our outputs as loaded so they can be matched for potential reuse
		// This indicates that the HGPO's ids are invalid and that HGPO should be matched using partnames instead
		CurrentOutput->MarkAsLoaded(true);
	}

	return true;
}

//
bool 
FHoudiniOutputTranslator::UploadChangedEditableOutput(
	UHoudiniAssetComponent* HAC,
	const bool& bInForceUpdate) 
{
	if (!HAC || HAC->IsPendingKill())
		return false;

	TArray<UHoudiniOutput*> &Outputs = HAC->Outputs;

	// Iterate through the outputs array of HAC.
	for (auto& CurrentOutput : HAC->Outputs)
	{
		if (!CurrentOutput)
			continue;

		// Only update the editable nodes that have been built before.
		if (!CurrentOutput->IsEditableNode() || !CurrentOutput->HasEditableNodeBuilt())
			continue;

		for (auto& CurrentOutputObj : CurrentOutput->GetOutputObjects())
		{
			UHoudiniSplineComponent* HoudiniSplineComponent = Cast<UHoudiniSplineComponent>(CurrentOutputObj.Value.OutputComponent);
			if (!HoudiniSplineComponent || HoudiniSplineComponent->IsPendingKill())
				continue;

			if (!HoudiniSplineComponent->HasChanged())
				continue;

			if (FHoudiniSplineTranslator::HapiUpdateNodeForHoudiniSplineComponent(HoudiniSplineComponent))
				HoudiniSplineComponent->MarkChanged(false);
			else
				HoudiniSplineComponent->SetNeedsToTriggerUpdate(false);
		}
	}

	return true;
}


bool
FHoudiniOutputTranslator::BuildAllOutputs(
	const HAPI_NodeId& AssetId,
	UObject* InOuterObject,	
	TArray<UHoudiniOutput*>& InOldOutputs,
	TArray<UHoudiniOutput*>& OutNewOutputs,
	const bool& InOutputTemplatedGeos)
{
	// Ensure the asset has a valid node ID
	if (AssetId < 0)
	{
		return false;
	}

	// Get the AssetInfo
	HAPI_AssetInfo AssetInfo;
	FHoudiniApi::AssetInfo_Init(&AssetInfo);
	HOUDINI_CHECK_ERROR_RETURN(FHoudiniApi::GetAssetInfo(
		FHoudiniEngine::Get().GetSession(), AssetId, &AssetInfo), false);

	FString CurrentAssetName;
	{
		FHoudiniEngineString hapiSTR(AssetInfo.nameSH);
		hapiSTR.ToFString(CurrentAssetName);
	}

	// Retrieve the asset's transform.
	// TODO: Unused?!
	//FTransform AssetUnrealTransform;
	//if (!FHoudiniEngineUtils::HapiGetAssetTransform(AssetId, AssetUnrealTransform))
	//	return false;

	// Retrieve information about each object contained within our asset.
	TArray<HAPI_ObjectInfo> ObjectInfos;
	if (!FHoudiniEngineUtils::HapiGetObjectInfos(AssetId, ObjectInfos))
		return false;

	//const int32 ObjectCount = ObjectInfos.Num();

	// Retrieve transforms for each object in this asset.
	TArray<HAPI_Transform> ObjectTransforms;
	if (!FHoudiniEngineUtils::HapiGetObjectTransforms(AssetId, ObjectTransforms))
		return false;

	// Mark all the previous HGPOs on the outputs as stale
	// This indicates that they were from a previous cook and should then be deleted
	for (auto& CurOutput : InOldOutputs)
	{
		if (CurOutput)
			CurOutput->MarkAllHGPOsAsStale(true);
	}

	// For HF / Volumes, we only create new Outputs for height volume
	// Store all the other volumes (masks etc)  on the side and we will
	// match them with theit corresponding height volume after
	TArray<FHoudiniGeoPartObject> UnassignedVolumeParts;

	TArray<FHoudiniMeshSocket> AllSockets;

	// Iterate through all objects.
	int32 OutputIdx = 1;
	for (int32 ObjectId = 0; ObjectId < ObjectInfos.Num(); ++ObjectId)
	{
		// Retrieve the object info
		const HAPI_ObjectInfo& CurrentHapiObjectInfo = ObjectInfos[ObjectId];

		// Cache/convert them
		FHoudiniObjectInfo CurrentObjectInfo;
		CacheObjectInfo(CurrentHapiObjectInfo, CurrentObjectInfo);

		// Retrieve object name.
		FString CurrentObjectName = CurrentObjectInfo.Name;

		// Get transformation for this object.
		const HAPI_Transform & ObjectTransform = ObjectTransforms[ObjectId];
		FTransform TransformMatrix;
		FHoudiniEngineUtils::TranslateHapiTransform(ObjectTransform, TransformMatrix);

		// TODO: Check transforms??

		// Build an array of the geos we'll need to process
		// In most case, it will only be the display geo, 
		// but we may also want to process editable geos as well
		TArray<HAPI_GeoInfo> GeoInfos;

		// Get the Display Geo's info
		HAPI_GeoInfo DisplayHapiGeoInfo;
		FHoudiniApi::GeoInfo_Init(&DisplayHapiGeoInfo);
		if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetDisplayGeoInfo(
			FHoudiniEngine::Get().GetSession(), CurrentHapiObjectInfo.nodeId, &DisplayHapiGeoInfo))
		{
			HOUDINI_LOG_MESSAGE(
				TEXT("Creating Static Meshes: Object [%d %s] unable to retrieve GeoInfo, - skipping."),
				CurrentHapiObjectInfo.nodeId, *CurrentObjectName);		
		}
		else
		{
			// Add the display geo info to the array
			GeoInfos.Add(DisplayHapiGeoInfo);
		}

		// Handle the editable nodes for this geo
		// Start by getting the number of editable nodes
		int32 EditableNodeCount = 0;
		HOUDINI_CHECK_ERROR(FHoudiniApi::ComposeChildNodeList(
			FHoudiniEngine::Get().GetSession(),
			CurrentHapiObjectInfo.nodeId, HAPI_NODETYPE_SOP, HAPI_NODEFLAGS_EDITABLE,
			true, &EditableNodeCount));

		if (EditableNodeCount > 0)
		{
			TArray< HAPI_NodeId > EditableNodeIds;
			EditableNodeIds.SetNumUninitialized(EditableNodeCount);
			HOUDINI_CHECK_ERROR(FHoudiniApi::GetComposedChildNodeList(
				FHoudiniEngine::Get().GetSession(), 
				AssetId, EditableNodeIds.GetData(), EditableNodeCount));

			for (int32 nEditable = 0; nEditable < EditableNodeCount; nEditable++)
			{
				HAPI_GeoInfo CurrentEditableGeoInfo;
				FHoudiniApi::GeoInfo_Init(&CurrentEditableGeoInfo);
				HOUDINI_CHECK_ERROR(FHoudiniApi::GetGeoInfo(
					FHoudiniEngine::Get().GetSession(), 
					EditableNodeIds[nEditable], &CurrentEditableGeoInfo));

				// Do not process the main display geo twice!
				if (CurrentEditableGeoInfo.isDisplayGeo)
					continue;

				// We only handle editable curves for now
				if (CurrentEditableGeoInfo.type != HAPI_GEOTYPE_CURVE)
					continue;

				// Add this geo to the geo info array
				GeoInfos.Add(CurrentEditableGeoInfo);
			}
		}

		// Handle the templated nodes if desired
		if (InOutputTemplatedGeos)
		{
			// Start by getting the number of templated nodes
			int32 TemplatedNodeCount = 0;
			HOUDINI_CHECK_ERROR(FHoudiniApi::ComposeChildNodeList(
				FHoudiniEngine::Get().GetSession(),
				CurrentHapiObjectInfo.nodeId,
				HAPI_NODETYPE_SOP, HAPI_NODEFLAGS_TEMPLATED,
				true, &TemplatedNodeCount));

			if (TemplatedNodeCount > 0)
			{
				TArray<HAPI_NodeId> TemplatedNodeIds;
				TemplatedNodeIds.SetNumUninitialized(TemplatedNodeCount);
				HOUDINI_CHECK_ERROR(FHoudiniApi::GetComposedChildNodeList(
					FHoudiniEngine::Get().GetSession(),
					CurrentHapiObjectInfo.nodeId, TemplatedNodeIds.GetData(), TemplatedNodeCount));

				for (int32 nTemplated = 0; nTemplated < TemplatedNodeCount; nTemplated++)
				{
					HAPI_GeoInfo CurrentTemplatedGeoInfo;
					FHoudiniApi::GeoInfo_Init(&CurrentTemplatedGeoInfo);
					HOUDINI_CHECK_ERROR(FHoudiniApi::GetGeoInfo(
						FHoudiniEngine::Get().GetSession(),
						TemplatedNodeIds[nTemplated], &CurrentTemplatedGeoInfo));

					// Do not process the main display geo twice!
					if (CurrentTemplatedGeoInfo.isDisplayGeo)
						continue;

					// We don't want all the nested template node IDs,
					// as our HDA could potentially be using other HDAs with nested template flags
					// Make sure the parent of the templated node is either the HDA, the current OBJ or the Display SOP
					HAPI_NodeId ParentId = FHoudiniEngineUtils::HapiGetParentNodeId(CurrentTemplatedGeoInfo.nodeId);
					if (ParentId != CurrentHapiObjectInfo.nodeId
						&& ParentId != DisplayHapiGeoInfo.nodeId
						&& ParentId != AssetId)
					{
						continue;
					}

					// Add this geo to the geo info array
					GeoInfos.Add(CurrentTemplatedGeoInfo);
				}
			}
		}

		// Iterates through the geos we want to process
		for (int32 GeoIdx = 0; GeoIdx < GeoInfos.Num(); GeoIdx++)
		{
			// Cook editable/templated nodes to get their parts.
			const HAPI_GeoInfo& CurrentHapiGeoInfo = GeoInfos[GeoIdx];
			if ((CurrentHapiGeoInfo.isEditable && CurrentHapiGeoInfo.partCount <= 0)
				|| (CurrentHapiGeoInfo.isTemplated && CurrentHapiGeoInfo.partCount <= 0))
			{
				//HAPI_CookOptions CookOptions = FHoudiniEngine::GetDefaultCookOptions();
				//FHoudiniApi::CookNode(FHoudiniEngine::Get().GetSession(), CurrentHapiGeoInfo.nodeId, &CookOptions);
				FHoudiniEngineUtils::HapiCookNode(CurrentHapiGeoInfo.nodeId, nullptr, true);

				HOUDINI_CHECK_ERROR(FHoudiniApi::GetGeoInfo(
					FHoudiniEngine::Get().GetSession(),
					CurrentHapiGeoInfo.nodeId,
					&GeoInfos[GeoIdx]));
			}

			// Cache/convert the display geo's info
			FHoudiniGeoInfo CurrentGeoInfo;
			CacheGeoInfo(CurrentHapiGeoInfo, CurrentGeoInfo);

			// Simply create an empty array for this geo's group names
			// We might need it later for splitting
			TArray<FString> GeoGroupNames;
			bool HasSocketGroups = false;

			// Iterate on this geo's parts
			for (int32 PartId = 0; PartId < CurrentGeoInfo.PartCount; ++PartId)
			{
				// Get part information.
				HAPI_PartInfo CurrentHapiPartInfo;
				FHoudiniApi::PartInfo_Init(&CurrentHapiPartInfo);

				// If the geo is templated, cook it manually
				if (CurrentHapiGeoInfo.isTemplated && InOutputTemplatedGeos)
				{
					//HAPI_CookOptions CookOptions = FHoudiniEngine::GetDefaultCookOptions();
					//FHoudiniApi::CookNode(FHoudiniEngine::Get().GetSession(), CurrentHapiGeoInfo.nodeId, &CookOptions);
					FHoudiniEngineUtils::HapiCookNode(CurrentHapiGeoInfo.nodeId, nullptr, true);
				}

				bool bPartInfoFailed = false;
				if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetPartInfo(
					FHoudiniEngine::Get().GetSession(), CurrentHapiGeoInfo.nodeId, PartId, &CurrentHapiPartInfo))
				{
					bPartInfoFailed = true;

					// If the geo is templated, attempt to cook it manually
					if(CurrentHapiGeoInfo.isTemplated && InOutputTemplatedGeos)
					{
						//HAPI_CookOptions CookOptions = FHoudiniEngine::GetDefaultCookOptions();
						//FHoudiniApi::CookNode(FHoudiniEngine::Get().GetSession(), CurrentHapiGeoInfo.nodeId, nullptr);
						FHoudiniEngineUtils::HapiCookNode(CurrentHapiGeoInfo.nodeId, nullptr, true);

						HOUDINI_CHECK_ERROR(FHoudiniApi::GetGeoInfo(
							FHoudiniEngine::Get().GetSession(),
							CurrentHapiGeoInfo.nodeId,
							&GeoInfos[GeoIdx]));

						if (HAPI_RESULT_SUCCESS == FHoudiniApi::GetPartInfo(
							FHoudiniEngine::Get().GetSession(), CurrentHapiGeoInfo.nodeId, PartId, &CurrentHapiPartInfo))
						{
							// We managed to get the templated part infos after cooking
							bPartInfoFailed = false;
						}
					}
				}

				if (bPartInfoFailed)
				{
					// Error retrieving part info.
					HOUDINI_LOG_MESSAGE(
						TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d] unable to retrieve PartInfo - skipping."),
						CurrentHapiObjectInfo.nodeId, *CurrentObjectName, CurrentHapiGeoInfo.nodeId, PartId);
					continue;
				}

				// Convert/cache the part info
				FHoudiniPartInfo CurrentPartInfo;
				CachePartInfo(CurrentHapiPartInfo, CurrentPartInfo);

				// Retrieve part name.
				FString CurrentPartName = CurrentPartInfo.Name;

				// Unsupported/Invalid part
				if (CurrentPartInfo.Type == EHoudiniPartType::Invalid)
					continue;

				// Update part/instancer type from the part infos
				EHoudiniPartType CurrentPartType = EHoudiniPartType::Invalid;
				EHoudiniInstancerType CurrentInstancerType = EHoudiniInstancerType::Invalid;
				switch (CurrentHapiPartInfo.type)
				{
					case HAPI_PARTTYPE_BOX:
					case HAPI_PARTTYPE_SPHERE:
					case HAPI_PARTTYPE_MESH:
					{
						if (CurrentHapiGeoInfo.type == HAPI_GEOTYPE_CURVE)
						{
							// Closed curve will be seen as mesh
							CurrentPartType = EHoudiniPartType::Curve;
						}
						else
						{
							CurrentPartType = EHoudiniPartType::Mesh;
							
							if (CurrentHapiObjectInfo.isInstancer)
							{
								if (FHoudiniEngineUtils::IsAttributeInstancer(CurrentHapiGeoInfo.nodeId, CurrentHapiPartInfo.id, CurrentInstancerType))
								{
									// That part is actually an attribute instancer
									CurrentPartType = EHoudiniPartType::Instancer;
									// Instancer type is set by IsAttributeInstancer
								}
								else
								{
									// That part is actually an instancer
									CurrentPartType = EHoudiniPartType::Instancer;
									CurrentInstancerType = EHoudiniInstancerType::ObjectInstancer;
								}
								
							}
							else if (CurrentHapiPartInfo.vertexCount <= 0 && CurrentHapiPartInfo.pointCount <= 0)
							{
								// No points, no vertices, we're likely invalid
								CurrentPartType = EHoudiniPartType::Invalid;
								HOUDINI_LOG_MESSAGE(
									TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s] is a mesh with no points or vertices - skipping."),
									CurrentHapiObjectInfo.nodeId, *CurrentObjectName, CurrentHapiGeoInfo.nodeId, PartId, *CurrentPartName);
							}
							else if (CurrentHapiPartInfo.vertexCount <= 0)
							{
								// This is not an instancer, we do not have vertices, but we have points
								// Maybe this is a point cloud with attribute override instancing
								if(FHoudiniEngineUtils::IsAttributeInstancer(CurrentHapiGeoInfo.nodeId, CurrentHapiPartInfo.id, CurrentInstancerType))
								{
									// Mark it as an instancer
									CurrentPartType = EHoudiniPartType::Instancer;
									// Instancer type is set by IsAttributeInstancer
									//CurrentInstancerType = EHoudiniInstancerType::OldSchoolAttributeInstancer;
								}
								else
								{
									// No vertices, not an instancer, just a point cloud, consider ourself as invalid
									CurrentPartType = EHoudiniPartType::Invalid;
									HOUDINI_LOG_MESSAGE(
										TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s] is a point cloud mesh - skipping."),
										CurrentHapiObjectInfo.nodeId, *CurrentObjectName, CurrentHapiGeoInfo.nodeId, PartId, *CurrentPartName);
								}
							}
						}
					}
					break;

					case HAPI_PARTTYPE_CURVE:
					{
						// Make sure that this curve is not an an attribute instancer!
						if (FHoudiniEngineUtils::IsAttributeInstancer(CurrentHapiGeoInfo.nodeId, CurrentHapiPartInfo.id, CurrentInstancerType))
						{
							// Mark the part as an instancer it as an instancer
							CurrentPartType = EHoudiniPartType::Instancer;
							// Instancer type is set by IsAttributeInstancer
							//CurrentInstancerType = EHoudiniInstancerType::OldSchoolAttributeInstancer;
						}
						else
						{
							// The curve is a curve!
							CurrentPartType = EHoudiniPartType::Curve;
						}
					}
						break;

					case HAPI_PARTTYPE_INSTANCER:
						// This is a packed primitive instancer
						CurrentPartType = EHoudiniPartType::Instancer;
						CurrentInstancerType = EHoudiniInstancerType::PackedPrimitive;
						break;

					case HAPI_PARTTYPE_VOLUME:
						// Volume data, likely a Heightfield height / mask	
						CurrentPartType = EHoudiniPartType::Volume;
						break;

					default:
						// Unsupported Part Type
						break;
				}

				// There are no vertices AND no points and this part is not a packed prim instancer
				if ((CurrentPartInfo.VertexCount <= 0 && CurrentPartInfo.PointCount <= 0)
					&& (CurrentPartType != EHoudiniPartType::Instancer || CurrentInstancerType != EHoudiniInstancerType::PackedPrimitive))
				{
					HOUDINI_LOG_MESSAGE(
						TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s] no points or vertices found - skipping."),
						CurrentHapiObjectInfo.nodeId, *CurrentObjectName, CurrentHapiGeoInfo.nodeId, PartId, *CurrentPartName);
					continue;
				}

				// This is an instancer with no points.
				if (CurrentHapiObjectInfo.isInstancer && CurrentHapiPartInfo.pointCount <= 0)
				{
					HOUDINI_LOG_MESSAGE(
						TEXT("Creating Static Meshes: Object [%d %s], Geo [%d], Part [%d %s] is instancer but has 0 points - skipping."),
						CurrentHapiObjectInfo.nodeId, *CurrentObjectName, CurrentHapiGeoInfo.nodeId, PartId, *CurrentPartName);
					continue;
				}

				// Ignore invalid parts
				if (CurrentPartType == EHoudiniPartType::Invalid)
					continue;

				// Build the HGPO corresponding to this part
				FHoudiniGeoPartObject currentHGPO;
				currentHGPO.AssetId = AssetId;
				currentHGPO.AssetName = CurrentAssetName;

				currentHGPO.ObjectId = CurrentHapiObjectInfo.nodeId;
				currentHGPO.ObjectName = CurrentObjectName;

				currentHGPO.GeoId = CurrentHapiGeoInfo.nodeId;

				currentHGPO.PartId = CurrentHapiPartInfo.id;

				currentHGPO.Type = CurrentPartType;
				currentHGPO.InstancerType = CurrentInstancerType;

				currentHGPO.TransformMatrix = TransformMatrix;

				currentHGPO.NodePath = TEXT("");

				currentHGPO.bIsVisible = CurrentHapiObjectInfo.isVisible && !CurrentHapiPartInfo.isInstanced;
				currentHGPO.bIsEditable = CurrentHapiGeoInfo.isEditable;
				currentHGPO.bIsInstanced = CurrentHapiPartInfo.isInstanced;
				// Never consider a display geo as templated!
				currentHGPO.bIsTemplated = CurrentHapiGeoInfo.isDisplayGeo ? false : CurrentHapiGeoInfo.isTemplated;

				currentHGPO.bHasGeoChanged = CurrentHapiGeoInfo.hasGeoChanged;
				currentHGPO.bHasPartChanged = CurrentHapiPartInfo.hasChanged;
				currentHGPO.bHasMaterialsChanged = CurrentHapiGeoInfo.hasMaterialChanged;
				currentHGPO.bHasTransformChanged = CurrentHapiObjectInfo.hasTransformChanged;
				
				// Copy the HAPI info caches 
				currentHGPO.ObjectInfo = CurrentObjectInfo;
				currentHGPO.GeoInfo = CurrentGeoInfo;
				currentHGPO.PartInfo = CurrentPartInfo;

				// We only support meshes for templated geos
				if (currentHGPO.bIsTemplated && (CurrentPartType != EHoudiniPartType::Mesh))
					continue;

				// Update the HGPO's node path
				FHoudiniEngineUtils::HapiGetNodePath(currentHGPO, currentHGPO.NodePath);

				// Try to get the custom part name from attribute
				FString CustomPartName;
				if (FHoudiniOutputTranslator::GetCustomPartNameFromAttribute(CurrentHapiGeoInfo.nodeId, CurrentHapiPartInfo.id, CustomPartName))
					currentHGPO.SetCustomPartName(CustomPartName);
				else
					currentHGPO.PartName = CurrentPartName;

				//
				// Mesh Only - Extract split groups
				// 
				// Extract the group names used by this part to see if it will require splitting
				// Only meshes can be split, via their primitive groups
				TArray< FString > SplitGroupNames;
				if (CurrentPartType == EHoudiniPartType::Mesh)
				{
					if (!CurrentHapiPartInfo.isInstanced && GeoGroupNames.Num() > 0)
					{
						// We are not instanced and already have extracted the geo's group names
						// We can simply reuse the Geo group names / socket groups
						currentHGPO.SplitGroups = GeoGroupNames;
					}
					else
					{
						// We need to get the primitive group names from HAPI
						int32 GroupCount = 0;
						TArray<FString> GroupNames;
						if (!FHoudiniEngineUtils::HapiGetGroupNames(
							CurrentHapiGeoInfo.nodeId, CurrentHapiPartInfo.id,
							HAPI_GROUPTYPE_PRIM, CurrentHapiPartInfo.isInstanced,
							GroupNames))
						{
							GroupCount = 0;
							GroupNames.Empty();
						}

						// Convert the string handles to FStrings
						for (const FString& GroupName : GroupNames)
						{
							FString LodGroup = HAPI_UNREAL_GROUP_LOD_PREFIX;
							FString CollisionGroup = HAPI_UNREAL_GROUP_INVISIBLE_COLLISION_PREFIX;
							FString RenderedCollisionGroup = HAPI_UNREAL_GROUP_RENDERED_COLLISION_PREFIX;
							if (GroupName.StartsWith(LodGroup, ESearchCase::IgnoreCase)
								|| GroupName.StartsWith(CollisionGroup, ESearchCase::IgnoreCase)
								|| GroupName.StartsWith(RenderedCollisionGroup, ESearchCase::IgnoreCase))
								//|| GroupName.StartsWith(HAPI_UNREAL_GROUP_USER_SPLIT_PREFIX, ESearchCase::IgnoreCase))
							{
								// Split by collisions / lods
								currentHGPO.SplitGroups.Add(GroupName);
							}
						}

						// Sort the Group name array by name, 
						// this will order the LODs and other incremental group names
						currentHGPO.SplitGroups.Sort();

						// If this part is not instanced, we can copy the geo
						// group names so we can reuse them for another part
						if (!CurrentHapiPartInfo.isInstanced)
						{
							GeoGroupNames = currentHGPO.SplitGroups;
						}
					}
				}

				//
				// Volume Only - Extract volume name/tile index
				// 
				// Extract the volume's name, and see if a tile attribute is present
				FHoudiniVolumeInfo CurrentVolumeInfo;
				if (CurrentPartType == EHoudiniPartType::Volume)
				{
					// Get this volume's info
					HAPI_VolumeInfo CurrentHapiVolumeInfo;
					FHoudiniApi::VolumeInfo_Init(&CurrentHapiVolumeInfo);

					bool bVolumeValid = true;
					if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetVolumeInfo(
						FHoudiniEngine::Get().GetSession(),
						CurrentHapiGeoInfo.nodeId, CurrentHapiPartInfo.id,
						&CurrentHapiVolumeInfo))
					{
						bVolumeValid = false;
					}
					else if (CurrentHapiVolumeInfo.tupleSize != 1)
					{
						bVolumeValid = false;
					}
					else if (CurrentHapiVolumeInfo.zLength != 1)
					{
						bVolumeValid = false;
					}
					else if (CurrentHapiVolumeInfo.storage != HAPI_STORAGETYPE_FLOAT)
					{
						bVolumeValid = false;
					}

					// Only cache valid volumes
					if (bVolumeValid)
					{
						// Convert/Cache the volume info
						CacheVolumeInfo(CurrentHapiVolumeInfo, CurrentVolumeInfo);

						// Get the volume's name
						currentHGPO.VolumeName = CurrentVolumeInfo.Name;

						// Now see if this volume has a tile attribute
						TArray<int32> TileValues;
						if (FHoudiniEngineUtils::GetTileAttribute(CurrentHapiGeoInfo.nodeId, CurrentHapiPartInfo.id, TileValues, HAPI_ATTROWNER_PRIM))
						{
							if (TileValues.Num() > 0 && TileValues[0] >= 0)
								currentHGPO.VolumeTileIndex = TileValues[0];
							else
								currentHGPO.VolumeTileIndex = -1;
						}
					}
				}
				currentHGPO.VolumeInfo = CurrentVolumeInfo;

				// Cache the curve info as well
				// !!! Only call GetCurveInfo if the PartType is Curve
				// !!! Closed curves are actually Meshes, and calling GetCurveInfo on a Mesh will crash HAPI!
				FHoudiniCurveInfo CurrentCurveInfo;
				if (CurrentPartType == EHoudiniPartType::Curve && CurrentPartInfo.Type == EHoudiniPartType::Curve)
				{
					HAPI_CurveInfo CurrentHapiCurveInfo;
					FHoudiniApi::CurveInfo_Init(&CurrentHapiCurveInfo);
					if (HAPI_RESULT_SUCCESS == FHoudiniApi::GetCurveInfo(
						FHoudiniEngine::Get().GetSession(),
						CurrentHapiGeoInfo.nodeId, CurrentHapiPartInfo.id,
						&CurrentHapiCurveInfo))
					{
						// Cache/Convert this part's curve info
						CacheCurveInfo(CurrentHapiCurveInfo, CurrentCurveInfo);
					}
				}
				currentHGPO.CurveInfo = CurrentCurveInfo;


				// TODO:
				// DONE? bake folders are handled out of this loop?
				// See if a custom bake folder override for the mesh was assigned via the "unreal_bake_folder" attribute
				//TArray<FString> BakeFolderOverrides;

				// Extract socket points
				FHoudiniEngineUtils::AddMeshSocketsToArray_DetailAttribute(
					currentHGPO.GeoId, currentHGPO.PartId, AllSockets, CurrentHapiPartInfo.isInstanced);
				FHoudiniEngineUtils::AddMeshSocketsToArray_Group(
					currentHGPO.GeoId, currentHGPO.PartId, AllSockets, CurrentHapiPartInfo.isInstanced);

				// See if we have an existing output that matches this HGPO or if we need to create a new one
				bool IsFoundOutputValid = false;
				UHoudiniOutput ** FoundHoudiniOutput = nullptr;	
				// We handle volumes differently than other outputs types, as a single HF output has multiple HGPOs
				if (currentHGPO.Type != EHoudiniPartType::Volume)
				{
					// Look in the previous output if we have a match
					FoundHoudiniOutput = InOldOutputs.FindByPredicate(
						[currentHGPO](UHoudiniOutput* Output) { return Output ? Output->HasHoudiniGeoPartObject(currentHGPO) : false; });

					if (FoundHoudiniOutput && *FoundHoudiniOutput && !(*FoundHoudiniOutput)->IsPendingKill())
						IsFoundOutputValid = true;

				}
				else
				{
					// Look in the previous outputs if we have a match
					FoundHoudiniOutput = InOldOutputs.FindByPredicate(
						[currentHGPO](UHoudiniOutput* Output) { return Output ? Output->HeightfieldMatch(currentHGPO, true) : false; });
					
					if (FoundHoudiniOutput && *FoundHoudiniOutput && !(*FoundHoudiniOutput)->IsPendingKill())
						IsFoundOutputValid = true;

					// If we dont have a match in the old maps, also look in the newly created outputs
					if (!IsFoundOutputValid)
					{
						FoundHoudiniOutput = OutNewOutputs.FindByPredicate(
							[currentHGPO](UHoudiniOutput* Output) { return Output ? Output->HeightfieldMatch(currentHGPO, false) : false; });

						if (FoundHoudiniOutput && *FoundHoudiniOutput && !(*FoundHoudiniOutput)->IsPendingKill())
							IsFoundOutputValid = true;
					}
				}

				UHoudiniOutput * HoudiniOutput = nullptr;
				if (IsFoundOutputValid)
				{
					// We can reuse the existing output
					HoudiniOutput = *FoundHoudiniOutput;
					HoudiniOutput->SetIsUpdating(true);
					// Transfer this output from the old array to the new one
					InOldOutputs.Remove(HoudiniOutput);
				}
				else
				{
					// We couldn't find a valid output object, so create a new one

					// If the current part is a volume, only create a new output object
					// if the volume's name is "height", if not store the HGPO aside
					if (currentHGPO.Type == EHoudiniPartType::Volume
						&& !currentHGPO.VolumeName.Equals(HAPI_UNREAL_LANDSCAPE_HEIGHT_VOLUME_NAME, ESearchCase::IgnoreCase))
					{
						UnassignedVolumeParts.Add(currentHGPO);
						continue;
					}

					// Create a new output object
					//FString OutputName = TEXT("Output") + FString::FromInt(OutputIdx++);
					HoudiniOutput = NewObject<UHoudiniOutput>(
						InOuterObject,
						UHoudiniOutput::StaticClass(),
						NAME_None,//FName(*OutputName),
						RF_NoFlags);

					// Make sure the created object is valid 
					if (!HoudiniOutput || HoudiniOutput->IsPendingKill())
					{
						//HOUDINI_LOG_WARNING("Failed to create asset output");
						continue;
					}

					// Mark if the HoudiniOutput is editable
					HoudiniOutput->SetIsEditableNode(currentHGPO.bIsEditable);
				}

				// Add the HGPO to the output
				HoudiniOutput->AddNewHGPO(currentHGPO);
				// Add this output object to the new ouput array
				OutNewOutputs.AddUnique(HoudiniOutput);
			}
		}
	}

	// Update the output/HGPO associations from the map
	// Clear the old HGPO since we don't need them anymore
	for (auto& CurrentOuput : OutNewOutputs)
	{
		if (!CurrentOuput || CurrentOuput->IsPendingKill())
			continue;

		CurrentOuput->DeleteAllStaleHGPOs();
	}

	// If we have unassigned volumes,
	// try to find their corresponding output
	if (UnassignedVolumeParts.Num() > 0)
	{
		for (auto& currentVolumeHGPO : UnassignedVolumeParts)
		{
			UHoudiniOutput ** FoundHoudiniOutput = OutNewOutputs.FindByPredicate(
				[currentVolumeHGPO](UHoudiniOutput* Output) 
				{
					return Output ? Output->HeightfieldMatch(currentVolumeHGPO, false) : false;
				});

			if (!FoundHoudiniOutput || !(*FoundHoudiniOutput) || (*FoundHoudiniOutput)->IsPendingKill())
			{
				// Skip - consider this volume as invalid
				continue;
			}

			// Add this HGPO to the output
			(*FoundHoudiniOutput)->AddNewHGPO(currentVolumeHGPO);
		} 
	}

	// All our output objects now have their HGPO assigned
	// We can now parse them to update the output type
	for (auto& Output : OutNewOutputs)
	{
		Output->UpdateOutputType();
	}

	return true;
}

bool
FHoudiniOutputTranslator::UpdateChangedOutputs(UHoudiniAssetComponent* HAC)
{
	if (!HAC || HAC->IsPendingKill())
		return false;

	TArray<UHoudiniOutput*>& Outputs = HAC->Outputs;

	// Iterate through the outputs array of HAC.
	for (int32 Index = 0; Index < HAC->GetNumOutputs(); ++Index)
	{
		UHoudiniOutput* CurrentOutput = HAC->GetOutputAt(Index);
		if (!CurrentOutput)
			continue;

		if (!HAC->IsOutputTypeSupported(CurrentOutput->GetType()))
			continue;

		switch (CurrentOutput->GetType())
		{
			case EHoudiniOutputType::Instancer:
			{
				bool bNeedToRecreateInstancers = false;
				for (auto& Iter : CurrentOutput->GetInstancedOutputs())
				{
					FHoudiniInstancedOutput& InstOutput = Iter.Value;
					if (!InstOutput.bChanged)
						continue;

					/*
					FHoudiniInstanceTranslator::UpdateChangedInstancedOutput(
						InstOutput, Iter.Key, CurrentOutput, HAC);
					*/

					// TODO:
					// UpdateChangedInstancedOutput needs some improvements
					// as it currently destroy too many components.
					// For now, we'll update all the instancers
					bNeedToRecreateInstancers = true;

					InstOutput.MarkChanged(false);
				}

				if (bNeedToRecreateInstancers)
				{
					if (HAC->GetAssetState() == EHoudiniAssetState::NeedInstantiation || HAC->HasBeenLoaded())
					{
						// Instantiate the HDA if it's not been
						// This is because CreateAllInstancersFromHoudiniOutput() actually reads the transform from HAPI
						// Calling it on a HDA not yet instantiated causes a crash...
						HAC->AssetState = EHoudiniAssetState::PreInstantiation;
					}
					else
					{
						FHoudiniInstanceTranslator::CreateAllInstancersFromHoudiniOutput(CurrentOutput, Outputs, HAC);
					}
				}
			}
			break;

			case EHoudiniOutputType::Curve:
			{
				//FHoudiniSplineTranslator::CreateAllSplinesFromHoudiniOutput(CurrentOutput, HAC);
			}
			break;

			default:
				break;
		}
	}

	return true;
}

void
FHoudiniOutputTranslator::CacheObjectInfo(const HAPI_ObjectInfo& InObjInfo, FHoudiniObjectInfo& OutObjInfoCache)
{
	FHoudiniEngineString hapiSTR(InObjInfo.nameSH);
	hapiSTR.ToFString(OutObjInfoCache.Name);
	//OutObjInfoCache.Name = InObjInfo.nameSH;

	OutObjInfoCache.NodeId = InObjInfo.nodeId;
	OutObjInfoCache.ObjectToInstanceID = InObjInfo.objectToInstanceId;

	OutObjInfoCache.bHasTransformChanged = InObjInfo.hasTransformChanged;
	OutObjInfoCache.bHaveGeosChanged = InObjInfo.haveGeosChanged;

	OutObjInfoCache.bIsVisible = InObjInfo.isVisible;
	OutObjInfoCache.bIsInstancer = InObjInfo.isInstancer;
	OutObjInfoCache.bIsInstanced = InObjInfo.isInstanced;

	OutObjInfoCache.GeoCount = InObjInfo.geoCount;
};

EHoudiniGeoType
FHoudiniOutputTranslator::ConvertHapiGeoType(const HAPI_GeoType& InType)
{
	EHoudiniGeoType OutType = EHoudiniGeoType::Invalid;
	switch (InType)
	{
	case HAPI_GEOTYPE_DEFAULT:
		OutType = EHoudiniGeoType::Default;
		break;

	case HAPI_GEOTYPE_INTERMEDIATE:
		OutType = EHoudiniGeoType::Intermediate;
		break;

	case HAPI_GEOTYPE_INPUT:
		OutType = EHoudiniGeoType::Input;
		break;

	case HAPI_GEOTYPE_CURVE:
		OutType = EHoudiniGeoType::Curve;
		break;

	default:
		OutType = EHoudiniGeoType::Invalid;
		break;
	}

	return OutType;
}

void
FHoudiniOutputTranslator::CacheGeoInfo(const HAPI_GeoInfo& InGeoInfo, FHoudiniGeoInfo& OutGeoInfoCache)
{
	OutGeoInfoCache.Type = ConvertHapiGeoType(InGeoInfo.type);

	FHoudiniEngineString hapiSTR(InGeoInfo.nameSH);
	hapiSTR.ToFString(OutGeoInfoCache.Name);

	OutGeoInfoCache.NodeId = InGeoInfo.nodeId;

	OutGeoInfoCache.bIsEditable = InGeoInfo.isEditable;
	OutGeoInfoCache.bIsTemplated = InGeoInfo.isTemplated;
	OutGeoInfoCache.bIsDisplayGeo = InGeoInfo.isDisplayGeo;
	OutGeoInfoCache.bHasGeoChanged = InGeoInfo.hasGeoChanged;
	OutGeoInfoCache.bHasMaterialChanged = InGeoInfo.hasMaterialChanged;

	OutGeoInfoCache.PartCount = InGeoInfo.partCount;
	OutGeoInfoCache.PointGroupCount = InGeoInfo.pointGroupCount;
	OutGeoInfoCache.PrimitiveGroupCount = InGeoInfo.primitiveGroupCount;
};


EHoudiniPartType
FHoudiniOutputTranslator::ConvertHapiPartType(const HAPI_PartType& InType)
{
	EHoudiniPartType OutType = EHoudiniPartType::Invalid;
	switch (InType)
	{
		case HAPI_PARTTYPE_BOX:
		case HAPI_PARTTYPE_SPHERE:
		case HAPI_PARTTYPE_MESH:
			OutType = EHoudiniPartType::Mesh;
			break;

		case HAPI_PARTTYPE_CURVE:
			OutType = EHoudiniPartType::Curve;
			break;

		case HAPI_PARTTYPE_INSTANCER:
			OutType = EHoudiniPartType::Instancer;
			break;

		case HAPI_PARTTYPE_VOLUME:
			OutType = EHoudiniPartType::Volume;
			break;

		default:
			OutType = EHoudiniPartType::Invalid;
			break;
	}

	return OutType;
}

void
FHoudiniOutputTranslator::CachePartInfo(const HAPI_PartInfo& InPartInfo, FHoudiniPartInfo& OutPartInfoCache)
{
	OutPartInfoCache.PartId = InPartInfo.id;
	
	FHoudiniEngineString hapiSTR(InPartInfo.nameSH);
	hapiSTR.ToFString(OutPartInfoCache.Name);

	OutPartInfoCache.Type = ConvertHapiPartType(InPartInfo.type);

	OutPartInfoCache.FaceCount = InPartInfo.faceCount;
	OutPartInfoCache.VertexCount = InPartInfo.vertexCount;
	OutPartInfoCache.PointCount = InPartInfo.pointCount;

	OutPartInfoCache.PointAttributeCounts = InPartInfo.attributeCounts[HAPI_ATTROWNER_POINT];
	OutPartInfoCache.VertexAttributeCounts = InPartInfo.attributeCounts[HAPI_ATTROWNER_VERTEX];
	OutPartInfoCache.PrimitiveAttributeCounts = InPartInfo.attributeCounts[HAPI_ATTROWNER_PRIM];
	OutPartInfoCache.DetailAttributeCounts = InPartInfo.attributeCounts[HAPI_ATTROWNER_DETAIL];

	OutPartInfoCache.bIsInstanced = InPartInfo.isInstanced;

	OutPartInfoCache.InstancedPartCount = InPartInfo.instancedPartCount;
	OutPartInfoCache.InstanceCount = InPartInfo.instanceCount;

	OutPartInfoCache.bHasChanged = InPartInfo.hasChanged;
};

void
FHoudiniOutputTranslator::CacheVolumeInfo(const HAPI_VolumeInfo& InVolumeInfo, FHoudiniVolumeInfo& OutVolumeInfoCache)
{
	FHoudiniEngineString hapiSTR(InVolumeInfo.nameSH);
	hapiSTR.ToFString(OutVolumeInfoCache.Name);
	
	OutVolumeInfoCache.bIsVDB = (InVolumeInfo.type == HAPI_VOLUMETYPE_VDB); // replaces VolumeType Type;

	OutVolumeInfoCache.TupleSize = InVolumeInfo.tupleSize;
	OutVolumeInfoCache.bIsFloat = (InVolumeInfo.storage == HAPI_STORAGETYPE_FLOAT); // replaces StorageType StorageType;
	OutVolumeInfoCache.TileSize = InVolumeInfo.tileSize;

	FHoudiniEngineUtils::TranslateHapiTransform(InVolumeInfo.transform, OutVolumeInfoCache.Transform);
	OutVolumeInfoCache.bHasTaper = InVolumeInfo.hasTaper;

	OutVolumeInfoCache.XLength = InVolumeInfo.xLength;
	OutVolumeInfoCache.YLength = InVolumeInfo.yLength;
	OutVolumeInfoCache.ZLength = InVolumeInfo.zLength;

	OutVolumeInfoCache.MinX = InVolumeInfo.minX;
	OutVolumeInfoCache.MinY = InVolumeInfo.minY;
	OutVolumeInfoCache.MinZ = InVolumeInfo.minZ;

	OutVolumeInfoCache.XTaper = InVolumeInfo.xTaper;
	OutVolumeInfoCache.YTaper = InVolumeInfo.yTaper;
};

EHoudiniCurveType
FHoudiniOutputTranslator::ConvertHapiCurveType(const HAPI_CurveType& InType)
{
	EHoudiniCurveType OutType = EHoudiniCurveType::Invalid;
	switch (InType)
	{
	case HAPI_CURVETYPE_LINEAR:
		OutType = EHoudiniCurveType::Polygon;
		break;

	case HAPI_CURVETYPE_NURBS:
		OutType = EHoudiniCurveType::Nurbs;
		break;

	case HAPI_CURVETYPE_BEZIER:
		OutType = EHoudiniCurveType::Bezier;
		break;

	case HAPI_CURVETYPE_MAX:
		OutType = EHoudiniCurveType::Points;
		break;

	default:
		OutType = EHoudiniCurveType::Invalid;
		break;
	}

	return OutType;
}

void
FHoudiniOutputTranslator::CacheCurveInfo(const HAPI_CurveInfo& InCurveInfo, FHoudiniCurveInfo& OutCurveInfoCache)
{
	OutCurveInfoCache.Type = ConvertHapiCurveType(InCurveInfo.curveType);

	OutCurveInfoCache.CurveCount = InCurveInfo.curveCount;
	OutCurveInfoCache.VertexCount = InCurveInfo.vertexCount;
	OutCurveInfoCache.KnotCount = InCurveInfo.knotCount;

	OutCurveInfoCache.bIsPeriodic = InCurveInfo.isPeriodic;
	OutCurveInfoCache.bIsRational = InCurveInfo.isRational;

	OutCurveInfoCache.Order = InCurveInfo.order;

	OutCurveInfoCache.bHasKnots = InCurveInfo.hasKnots;
};


void
FHoudiniOutputTranslator::ClearAndRemoveOutputs(UHoudiniAssetComponent *InHAC, TArray<UHoudiniOutput*>& OutputsPendingClear, bool bForceClearAll)
{
	if (!IsValid(InHAC))
		return;
	
	// DO NOT MANUALLY DESTROY THE OLD/DANGLING OUTPUTS!
	// This messes up unreal's Garbage collection and would cause crashes on duplication
	// Simply clearing the array is enough
	for (auto& OldOutput : InHAC->Outputs)
	{
		if (OldOutput->ShouldDeferClear() && !bForceClearAll)
		{
			OutputsPendingClear.Add(OldOutput);
		}
		else
		{
			ClearOutput(OldOutput);
		}
	}

	InHAC->Outputs.Empty();	
}

void 
FHoudiniOutputTranslator::ClearOutput(UHoudiniOutput* Output) 
{
	switch (Output->GetType()) 
	{
		case EHoudiniOutputType::Landscape:
		{
			for (auto& OutputObject : Output->GetOutputObjects())
			{
				// Currently, any Landscape managed by an HDA is always present in the current level.
				// Only when it gets baked will Landscapes be serialized to other levels so for now
				// assume that a landscape should be available, unless explicitly deleted the user.
				UHoudiniLandscapePtr* LandscapePtr = Cast<UHoudiniLandscapePtr>(OutputObject.Value.OutputObject);
				if (!LandscapePtr)
					continue;

				ALandscapeProxy* Landscape = LandscapePtr->GetRawPtr();

				if (!IsValid(Landscape) || !Landscape->IsValidLowLevel())
					continue;

				Landscape->UnregisterAllComponents();
				Landscape->Destroy();

				// if (Output->IsLandscapeWorldComposition()) 
				// {
				// 	TSoftObjectPtr<ALandscapeProxy> LandscapeSoftPtr = LandscapePtr->GetSoftPtr();
				//
				// 	FString SoftPtrPath = LandscapeSoftPtr.ToSoftObjectPath().ToString();
				//
				// 	FString FileName = FPaths::GetBaseFilename(SoftPtrPath);
				// 	FString FileDirectory = FPaths::GetPath(SoftPtrPath);
				//
				// 	FString ContentPath = FPaths::ProjectContentDir();
				// 	FString ContentFullPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*ContentPath);
				//
				// 	FString AbsoluteFilePath = ContentFullPath + FileDirectory.Mid(5, FileDirectory.Len() - 5) + "/" + FPaths::GetBaseFilename(FileName) + ".umap";
				//
				// 	FPlatformFileManager::Get().GetPlatformFile().FileExists(*(AbsoluteFilePath));
				//
				// 	FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*(AbsoluteFilePath));
				// }
				// else 
				// {

				// }
			}
		}
		break;

		case EHoudiniOutputType::Instancer:
		{
			for (auto& OutputObject : Output->GetOutputObjects())
			{
				// Is this a foliage instancer? Check if the component is owned by an AInstancedFoliageActor
				UActorComponent* const Component = Cast<UActorComponent>(OutputObject.Value.OutputComponent);
				if (!IsValid(Component))
					continue;
				AActor* const OwnerActor = Component->GetOwner();
				if (!IsValid(OwnerActor) || !OwnerActor->IsA<AInstancedFoliageActor>())
					continue;
				
				UHierarchicalInstancedStaticMeshComponent* const FoliageHISMC = Cast<UHierarchicalInstancedStaticMeshComponent>(Component);
				if (IsValid(FoliageHISMC))
				{
					// Find the parent component: the output is typically owned by an HAC.
					USceneComponent* ParentComponent = nullptr;
					UObject* const OutputOuter = Output->GetOuter();
					if (IsValid(OutputOuter))
					{
						if (OutputOuter->IsA<UHoudiniAssetComponent>())
						{
							ParentComponent = Cast<USceneComponent>(OutputOuter);
						}
						// other possibilities?
					}
					
					// fallback to trying the owner of the HISMC
					if (!ParentComponent)
					{
						ParentComponent = Cast<USceneComponent>(FoliageHISMC);
					}
					
					if (IsValid(ParentComponent))
					{
						FHoudiniInstanceTranslator::CleanupFoliageInstances(FoliageHISMC, ParentComponent);
						FHoudiniEngineUtils::RepopulateFoliageTypeListInUI();
					}
				}
			}
		}
		break;
		// ... Other output types ...//

		default:
		break;
	
	}

	Output->Clear();
}


bool
FHoudiniOutputTranslator::GetCustomPartNameFromAttribute(const HAPI_NodeId & NodeId, const HAPI_PartId & PartId, FString & OutCustomPartName) 
{
	HAPI_AttributeInfo CustomPartNameInfo;
	FHoudiniApi::AttributeInfo_Init(&CustomPartNameInfo);

	bool bHasCustomName = false;
	TArray<FString> CustomNames;
	if (FHoudiniEngineUtils::HapiGetAttributeDataAsString(NodeId, PartId, HAPI_UNREAL_ATTRIB_CUSTOM_OUTPUT_NAME_V2, CustomPartNameInfo, CustomNames))
	{
		// Look for the v2 attribute (unreal_output_name)
		bHasCustomName = true;
	}
	else if (FHoudiniEngineUtils::HapiGetAttributeDataAsString(NodeId, PartId, HAPI_UNREAL_ATTRIB_CUSTOM_OUTPUT_NAME_V1, CustomPartNameInfo, CustomNames))
	{
		// If we couldnt find the new attribute, use the legacy v1 attribute (unreal_generated_mesh_name)
		bHasCustomName = true;
	}

	if (!bHasCustomName)
		return false;

	if (CustomNames.Num() <= 0)
		return false;

	OutCustomPartName = CustomNames[0];

	if (OutCustomPartName.IsEmpty())
		return false;

	return true;
}

void
FHoudiniOutputTranslator::GetTempFolderFromAttribute(UHoudiniAssetComponent * HAC)
{
	if (!HAC || HAC->IsPendingKill())
		return;
	
	HAPI_GeoInfo DisplayGeoInfo;
	FHoudiniApi::GeoInfo_Init(&DisplayGeoInfo);
	if (HAPI_RESULT_SUCCESS != FHoudiniApi::GetDisplayGeoInfo(FHoudiniEngine::Get().GetSession(), HAC->AssetId, &DisplayGeoInfo))
		return;

	FString TempFolderOverride = FString();

	HAPI_AttributeInfo TempFolderAttriInfo;
	FHoudiniApi::AttributeInfo_Init(&TempFolderAttriInfo);
	TArray<FString> StringData;
	if (FHoudiniEngineUtils::HapiGetAttributeDataAsString(
		DisplayGeoInfo.nodeId, 0, HAPI_UNREAL_ATTRIB_TEMP_FOLDER, TempFolderAttriInfo, StringData, 1, HAPI_ATTROWNER_DETAIL))
	{
		TempFolderOverride = StringData.IsValidIndex(0) ? StringData[0] : FString();
	}
	else
	{
		if (FHoudiniEngineUtils::HapiGetAttributeDataAsString(
			DisplayGeoInfo.nodeId, 0, HAPI_UNREAL_ATTRIB_TEMP_FOLDER, TempFolderAttriInfo, StringData, 1, HAPI_ATTROWNER_PRIM))
		{
			TempFolderOverride = StringData.IsValidIndex(0) ? StringData[0] : FString();
		}
	}

	if (TempFolderOverride.StartsWith("Game/"))
	{
		TempFolderOverride = "/" + TempFolderOverride;
	}

	FString AbsoluteOverridePath;
	if (TempFolderOverride.StartsWith("/Game/"))
	{
		FString RelativePath = FPaths::ProjectContentDir() + TempFolderOverride.Mid(6, TempFolderOverride.Len() - 6);
		AbsoluteOverridePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*RelativePath);
	}
	else
	{
		if (!TempFolderOverride.IsEmpty())
			AbsoluteOverridePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*TempFolderOverride);
	}

	// Check Validity of the path
	if (AbsoluteOverridePath.IsEmpty() || !FPaths::DirectoryExists(AbsoluteOverridePath))
	{
		// Only display a warning if the path is invalid, empty is fine
		if(!AbsoluteOverridePath.IsEmpty())
			HOUDINI_LOG_WARNING(TEXT("Invalid override temporary cook path: %s"), *TempFolderOverride);

		const UHoudiniRuntimeSettings * HoudiniRuntimeSettings = GetDefault< UHoudiniRuntimeSettings >();
		TempFolderOverride = HoudiniRuntimeSettings->DefaultTemporaryCookFolder;
	}

	// If the TempCookFolder of the HAC is non-empty and is different from the override path.
	// do not override it if the current temp cook path is valid. (it was user specified)
	if (!HAC->TemporaryCookFolder.Path.IsEmpty() && !HAC->TemporaryCookFolder.Path.Equals(TempFolderOverride))
		return;

	HAC->TemporaryCookFolder.Path = TempFolderOverride;
}

#undef LOCTEXT_NAMESPACE