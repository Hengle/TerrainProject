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

#pragma once

#include "HoudiniParameter.h"

#include "HoudiniParameterFolder.generated.h"

UENUM()
enum class EHoudiniFolderParameterType : uint8
{
	Invalid,

	Collapsible,
	Simple,
	Tabs,
	Radio,
	Other,
};

UCLASS()
class HOUDINIENGINERUNTIME_API UHoudiniParameterFolder : public UHoudiniParameter
{
	GENERATED_UCLASS_BODY()

public:

	// Create instance of this class.
	static UHoudiniParameterFolder * Create(
		UObject* Outer,
		const FString& ParamName);

	FORCEINLINE
	EHoudiniFolderParameterType GetFolderType() const { return FolderType; };

	FORCEINLINE
	void SetFolderType(EHoudiniFolderParameterType Type) { FolderType = Type; };

	FORCEINLINE
	void SetExpanded(const bool InExpanded) { bExpanded = InExpanded; };
	FORCEINLINE
	bool IsExpanded() const { return bExpanded; };
	FORCEINLINE
	void ExpandButtonClicked() { bExpanded = !bExpanded; };

	FORCEINLINE
	void SetChosen(const bool InChosen) { bChosen = InChosen; };
	FORCEINLINE
	bool IsChosen() const { return bChosen; };

	FORCEINLINE
	bool IsTab() const { return FolderType == EHoudiniFolderParameterType::Tabs || FolderType == EHoudiniFolderParameterType::Radio; };


	FORCEINLINE
	void ResetChildCounter() { ChildCounter = TupleSize; }

	FORCEINLINE
	int32& GetChildCounter() { return ChildCounter; };

	FORCEINLINE
	bool IsContentShown() const { return bIsContentShown; };

	FORCEINLINE
	void SetIsContentShown(const bool& bInShown) { bIsContentShown = bInShown; };



private:
	UPROPERTY()
	EHoudiniFolderParameterType FolderType;

	UPROPERTY()
	bool bExpanded;

	UPROPERTY()
	bool bChosen;


	UPROPERTY()
	int32 ChildCounter;

	UPROPERTY()
	bool bIsContentShown;

};