// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/KismetCompiler/Public/BlueprintCompilerCppBackendInterface.h"

/**
 * The public interface to this module
 */
class BLUEPRINTCOMPILERCPPBACKEND_API IBlueprintCompilerCppBackendModule : public IBlueprintCompilerCppBackendModuleInterface
{
public:

	static FString GetBaseFilename(const UObject* AssetObj);
};

