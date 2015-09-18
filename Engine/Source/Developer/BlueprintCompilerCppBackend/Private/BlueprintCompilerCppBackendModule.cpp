// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilerCppBackendModulePrivatePCH.h"
#include "IBlueprintCompilerCppBackendModule.h"
#include "ModuleManager.h"
#include "BlueprintCompilerCppBackend.h"
#include "BlueprintCompilerCppBackendUtils.h" // for FEmitHelper::GetBaseFilename()

class FBlueprintCompilerCppBackendModule : public IBlueprintCompilerCppBackendModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual IBlueprintCompilerCppBackend* Create()  override;
};

IMPLEMENT_MODULE(FBlueprintCompilerCppBackendModule, BlueprintCompilerCppBackend)



void FBlueprintCompilerCppBackendModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FBlueprintCompilerCppBackendModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

IBlueprintCompilerCppBackend* FBlueprintCompilerCppBackendModule::Create()
{
	return new FBlueprintCompilerCppBackend();
}

FString IBlueprintCompilerCppBackendModule::GetBaseFilename(const UObject* AssetObj)
{
	// use the same function that the backend uses for #includes
	return FEmitHelper::GetBaseFilename(AssetObj);
}
