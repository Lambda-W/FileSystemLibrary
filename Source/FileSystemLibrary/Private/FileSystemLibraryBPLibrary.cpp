// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright Lambda Works, Samuel Metters 2019. All rights reserved.

#include "FileSystemLibraryBPLibrary.h"
#include "FileSystemLibrary.h"
#include "TimerManager.h"

UFileSystemLibraryBPLibrary::UFileSystemLibraryBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

UCreateProcessWithCallback* UCreateProcessWithCallback::CreateProcessWithCallback(UObject* WorldContextObj, FString PathToExecutable, FString Arguments,bool LaunchDetached, bool LaunchedHidden, bool LaunchReallyHidden, int PriorityModifier, bool UseWorkingDirectory, FString WorkingDirectory)
{
	int32 tProcessID;
	UFileSystemLibraryBPLibrary::CreateProcess(PathToExecutable, Arguments,LaunchDetached,LaunchedHidden,LaunchReallyHidden, PriorityModifier,UseWorkingDirectory,WorkingDirectory, tProcessID); 
	
	auto* AsyncAction = NewObject<UCreateProcessWithCallback>();
	AsyncAction->ProcessID = tProcessID;
	AsyncAction->WorldContextObj = WorldContextObj;
	return AsyncAction;

}

void UCreateProcessWithCallback::Activate()
{
	Super::Activate();

	if(WorldContextObj && ProcessID != 0)
	{
		WorldContextObj->GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UCreateProcessWithCallback::bIsProcessRunning);
	}
}

void UCreateProcessWithCallback::bIsProcessRunning()
{
	bool const bIsRunning = FPlatformProcess::IsApplicationRunning(ProcessID);
	
	if(bIsRunning)
	{
		if(WorldContextObj)
		{
			WorldContextObj->GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UCreateProcessWithCallback::bIsProcessRunning);
		}
	}
	else
	{
		{
			Completed.Broadcast();
		}
	}
	
}
