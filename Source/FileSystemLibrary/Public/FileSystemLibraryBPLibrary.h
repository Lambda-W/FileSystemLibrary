// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright Lambda Works, Samuel Metters 2019. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#if WITH_EDITOR
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#endif

#include <string>

#include "CoreMinimal.h"
#include "DialogManager.h"
#include "Win/DialogManagerWin.h"
#include "Mac/DialogManagerMac.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Widgets/SWindow.h"
#include "FileSystemLibraryBPLibrary.generated.h"

USTRUCT(BlueprintType)
struct FPathProperties
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PathProperties")
	FDateTime CreationDate;

	UPROPERTY(BlueprintReadOnly, Category = "PathProperties")
	FDateTime AccessDate;

	UPROPERTY(BlueprintReadOnly, Category = "PathProperties")
	FDateTime ModificationDate;

	UPROPERTY(BlueprintReadOnly, Category = "PathProperties")
	int FileSizeBytes;

	UPROPERTY(BlueprintReadOnly, Category = "PathProperties")
	bool isDirectory;

	UPROPERTY(BlueprintReadOnly, Category = "PathProperties")
	bool isReadOnly;

	FPathProperties()
	{
		CreationDate = FDateTime::MinValue();
		AccessDate = FDateTime::MinValue();
		ModificationDate = FDateTime::MinValue();
		FileSizeBytes = 0;
		isDirectory = false;
		isReadOnly = false;
	}

	FPathProperties(FDateTime inCreationDate, FDateTime inAccessDate, FDateTime inModificationDate, int64 inFileSizeBytes, bool inIsDirectory, bool inIsReadOnly)
	{
		CreationDate = inCreationDate;
		AccessDate = inAccessDate;
		ModificationDate = inModificationDate;
		FileSizeBytes = int(inFileSizeBytes);
		isDirectory = inIsDirectory;
		isReadOnly = inIsReadOnly;
	}
};

UCLASS()
class UFileSystemLibraryBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

		/* Creates a new process and its primary thread. The new process runs the specified executable file in the security context of the calling process. 
		@param	PathToExecutable		The path to the executable to run.
		@param	Arguments				Any command line argument to run when executing.
		@param	LaunchDetached			If true, the process will have its own window.
		@param	LaunchedHidden			If true, the new process will be minimized in the task bar
		@param	LaunchReallyHidden		If true, the new process will not have a window or be in the task bar
		@param	PriorityModifier		2 idle, -1 low, 0 normal, 1 high, 2 higher
		@param	UseWorkingDirectory		If true, will use WorkingDirectory to start the executable in instead of its current directory.
		@param	WorkingDirectory		Directory to start the executable in (required UseWorkingDirectory = true).
		*/
		UFUNCTION(BlueprintCallable, meta = (DisplayName = "CreateProcess", Keywords = "process create execute"), Category = "FileSystemLibrary")
		static FORCEINLINE bool CreateProcess(FString PathToExecutable, FString Arguments, bool LaunchDetached, bool LaunchedHidden, bool LaunchReallyHidden, int PriorityModifier, bool UseWorkingDirectory, FString WorkingDirectory)
	{
		const TCHAR* tFilename = *PathToExecutable;
		const TCHAR* tArguments = *Arguments;
		const TCHAR* tWorkingDirectory = (UseWorkingDirectory) ? *WorkingDirectory : nullptr;
		int32 PrioMod = (int32)PriorityModifier;
		FPlatformProcess::CreateProc(tFilename, tArguments, LaunchDetached, LaunchedHidden, LaunchReallyHidden, nullptr, PrioMod, tWorkingDirectory, nullptr);
		return true;
	}

	/** Opens Windows' explorer or Mac OS' finder at the specified path.
	* @param Path		Path to the directory.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Open Directory", Keywords = "explorer finder folder directory"), Category = "FileSystemLibrary")
		static void OpenDirectory(FString Path)
	{
		if (VerifyDirectory(Path))
		{
			FString ValidPath = Path;

			FString Fcommand;

#if PLATFORM_WINDOWS
			ValidPath.ReplaceCharInline('/', '\\', ESearchCase::IgnoreCase);

			Fcommand = TEXT("explorer ");
#endif

#if PLATFORM_MAC
ValidPath.InsertAt(0,'"');
ValidPath.InsertAt(ValidPath.Len(), '"');
			Fcommand = TEXT("open ");
#endif

			Fcommand.Append(ValidPath);
			std::string command = TCHAR_TO_UTF8(*Fcommand);

			system(command.c_str());
		}
	}

	/***** File Operations *****/

	/* This function will check to see if the specified file (folder) exists. You need to include the file extension. 
	@param PathToFile The path to the file to verify (including extension)
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "VerifyFile", Keywords = "FileSystemLibrary"), Category = "System File Operations")
	static FORCEINLINE bool VerifyFile(FString PathToFile = "")
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Does the file exist?
		if (PlatformFile.FileExists(*PathToFile))
		{
			// Success
			return true;
		}

		// Directory doesn't exist and wasn't created
		return false;
	}

	/* This function will copy a file from a path to another. You need to include the full path with extension for both input parameters. 
	@param	PathToFile				Path to the file to copy (including extension).
	@param	DestinationFilePath		Path to copy the file to (including filename and extension).
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "CopyFile", Keywords = "FileSystemLibrary"), Category = "System File Operations")
	static FORCEINLINE bool CopyFile(FString PathToFile, FString DestinationFilePath = "")
	{

		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (VerifyFile(*PathToFile))
		{
			if (PlatformFile.CopyFile(*DestinationFilePath, *PathToFile, EPlatformFileRead::AllowWrite, EPlatformFileWrite::AllowRead))
			{
				return true;
			}
		}

		// Failure
		return false;
	}

	/* This function will copy a file from a path to another. You need to include the full path with extension for both input parameters. 
	@param	PathToFile				Path to the file to move (including extension).
	@param	DestinationFilePath		Path to move the file to (including filename and extension).
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "MoveFile", Keywords = "FileSystemLibrary"), Category = "System File Operations")
	static FORCEINLINE bool MoveFile(FString PathToFile, FString DestinationFilePath = "")
	{

		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (VerifyFile(*PathToFile))
		{
			if (PlatformFile.MoveFile(*DestinationFilePath, *PathToFile))
			{
				return true;
			}
		}

		// Failure
		return false;
	}

	/* This function will rename the specified file. You need to include filename with extension for both input parameters. 
	@param	PathToFile		Path to the file to rename (including extension).
	@param	NewFileName		New name for the file (including extension).
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RenameFile", Keywords = "FileSystemLibrary"), Category = "System File Operations")
	static FORCEINLINE bool RenameFile(FString PathToFile, FString NewFileName = "")
	{
		FString Path = FPaths::GetPath(PathToFile);
		FString NewPathToFile = Path + "\\" + NewFileName;

		if (MoveFile(PathToFile, NewPathToFile))
		{
			return true;
		}

		return false;
	}

	/* This function will rename the specified file. You need to include filename with extension for both input parameters. 
	@param PathToFile	Path to the file to delete (including extension).
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "DeleteFile", Keywords = "FileSystemLibrary"), Category = "System File Operations")
	static FORCEINLINE bool DeleteFile(FString PathToFile)
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (VerifyFile(*PathToFile))
		{
			if (PlatformFile.DeleteFile(*PathToFile))
			{
				return true;
			}
		}

		// Failure
		return false;
	}


	/***** Directory Operations *****/

	/* This function will check to see if the specified directory (folder) exist. If it doesn't and CreateDirectory=true, it will create the directory for you. 
	@param PathToDirectory	Path to the directory to verify.
	@param CreateDirectory	If true, the directory will be created if it doesn't exist.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "VerifyAndCreateDirectory", Keywords = "FileSystemLibrary"), Category = "System Directory Operations")
	static FORCEINLINE bool VerifyAndCreateDirectory(const FString &PathToDirectory = "", bool CreateDirectory = true)
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Does the directory exist?
		if (!PlatformFile.DirectoryExists(*PathToDirectory))
		{
			if (CreateDirectory)
			{
				// If not create directory
				PlatformFile.CreateDirectoryTree(*PathToDirectory);

				// Check that the directory has been created
				if (PlatformFile.DirectoryExists(*PathToDirectory))
				{
					return true;
				}
			}

			// Directory doesn't exist and wasn't created
			return false;
		}

		// Directory exist
		return true;
	}

	/* This function will check to see if the specified directory (folder) exist.
@param PathToDirectory	Path to the directory to verify.
*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "VerifyDirectory", Keywords = "FileSystemLibrary"), Category = "System Directory Operations")
	static FORCEINLINE bool VerifyDirectory(const FString &PathToDirectory = "")
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Does the directory exist?
		if (!PlatformFile.DirectoryExists(*PathToDirectory))
		{
			// Directory doesn't exist
			return false;
		}

		// Directory exist
		return true;
	}

	/* This function will the specified directory and all file/folders inside it. 
	@param PathToDirectory The path to the directory to delete.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "DeleteDirectory", Keywords = "FileSystemLibrary"), Category = "System Directory Operations")
	static FORCEINLINE bool DeleteDirectory(FString PathToDirectory = "")
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Does the directory exist?
		if (PlatformFile.DirectoryExists(*PathToDirectory))
		{
			// If it does exist, delete it
			if (PlatformFile.DeleteDirectoryRecursively(*PathToDirectory))
			{
				// Success
				return true;
			}
		}

		// Failure
		return false;
	}

	/* This function will copy all files and folders from PathToDirectory to NewPathToDirectory. 
	@param	PathToDirectory		Path to the directory to copy.
	@param	NewPathToDirectory	Path to the directory to copy the files to.
	@param	AllowOvewrite		If true, files that already exist in the destination path will be overwritten.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "CopyDirectory", Keywords = "FileSystemLibrary"), Category = "System Directory Operations")
	static FORCEINLINE bool CopyDirectory(FString PathToDirectory = "", FString NewPathToDirectory = "", bool AllowOvewrite = true)
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Does the directory exist?
		if (PlatformFile.DirectoryExists(*PathToDirectory))
		{
			if (VerifyAndCreateDirectory(NewPathToDirectory, true))
			{
				// If it does exist, copy to directory tree
				if (PlatformFile.CopyDirectoryTree(*NewPathToDirectory, *PathToDirectory, AllowOvewrite))
				{
					// Success
					return true;
				}
			}
		}

		// Failure
		return false;
	}

	/* This function will move all files and folders from PathToDirectory to NewPathToDirectory. 
	@param	PathToDirectory		Path to the directory to move.
	@param	NewPathToDirectory	Path to the directory to move the files to.
	@param	AllowOvewrite		If true, files that already exist in the destination path will be overwritten.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "MoveDirectory", Keywords = "FileSystemLibrary"), Category = "System Directory Operations")
	static FORCEINLINE bool MoveDirectory(FString PathToDirectory = "", FString NewPathToDirectory = "", bool AllowOvewrite = true)
	{
		if (CopyDirectory(PathToDirectory, NewPathToDirectory, AllowOvewrite))
		{
			if (DeleteDirectory(PathToDirectory))
			{
				return true;
			}
		}
		return false;
	}

	/***** File & Directory Operations *****/

	/* This function will return the file's or folder's properties. 
	@param	Path		Path to the file (including extension).
	@return	Properties	The file's property.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFileOrDirectoryProperties", Keywords = "FileSystemLibrary"), Category = "File System Library")
	static FORCEINLINE bool GetFileOrDirectoryProperties(FPathProperties &Properties, FString Path = "")
	{
		FFileStatData StatData;

		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Get directory properties
		if (VerifyDirectory(Path))
		{
			StatData = PlatformFile.GetStatData(*Path);
			Properties = FPathProperties(StatData.CreationTime, StatData.AccessTime, StatData.ModificationTime, StatData.FileSize, StatData.bIsDirectory, StatData.bIsReadOnly);
			return true;
		}

		// If not a directory, check file properties
		else if (VerifyFile(Path))
		{
			StatData = PlatformFile.GetStatData(*Path);
			Properties = FPathProperties(StatData.CreationTime, StatData.AccessTime, StatData.ModificationTime, StatData.FileSize, StatData.bIsDirectory, StatData.bIsReadOnly);
			return true;
		}

		// Failure
		return false;
	}

	/* This function will return the file's or folder's properties. 
	@param	Path			Path to the file (including extension).
	@return	FileSizeBytes	The file's size in bytes (multiply by 1 000 000 to get the result in Mb).
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFileOrDirectorySize", Keywords = "FileSystemLibrary"), Category = "File System Library")
	static FORCEINLINE bool GetFileOrDirectorySize(int &FileSizeBytes, FString Path = "")
	{
		FPathProperties Properties;

		if (GetFileOrDirectoryProperties(Properties, Path))
		{
			FileSizeBytes = (int)Properties.FileSizeBytes;
			return true;
		}

		return false;
	}

	/* This function will return the name of all files present in the specified directory. 
	@param	PathToDirectory			Path to the directory.
	@param	ExtensionFilter			If set, will only return files of the input extension. (".XXX" or "XXX").
	@param	OnlyReturnFilenames		If true, will only return the filenames (without the extension).
	@return	Files					The files found in the specific directory.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFilesInDirectory", Keywords = "FileSystemLibrary"), Category = "File System Library")
	static FORCEINLINE bool GetFilesInDirectory(TArray<FString> &Files, FString PathToDirectory, FString ExtensionFilter, bool OnlyReturnFilenames)
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Does the directory exist?
		if (PlatformFile.DirectoryExists(*PathToDirectory))
		{
			TArray<FString> ReturnFiles;
			FString tempExtensionFilter = ExtensionFilter;

			// Check that the directory has been created
			PlatformFile.FindFiles(ReturnFiles, *PathToDirectory, *tempExtensionFilter);

			// Check if found any files
			if (ReturnFiles.Num() > 0)
			{
				// Check if we want to exclude the extension from the return array.
				if (OnlyReturnFilenames)
				{
					TArray<FString> ReturnFilesNoExtension;

					for (int i = 0; i < ReturnFiles.Num(); i++)
					{
						ReturnFilesNoExtension.Add(FPaths::GetBaseFilename(ReturnFiles[i]));
					}

					Files = ReturnFilesNoExtension;
					return true;
				}

				//If not return the array
				else 
				{
					Files = ReturnFiles;
					return true;
				}
			}
		}

		// No files were found
		return false;
	}

	/* This function will return the name of all files present in the specified directory and all sub-directories.
	@param	PathToDirectory			Path to the directory.
	@param	ExtensionFilter			If set, will only return files of the input extension. (".XXX" or "XXX").
	@param	OnlyReturnFilenames		If true, will only return the filenames (without the extension).
	@return	Files					The files found in the specific directory.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFilesRecursivelyInDirectory", Keywords = "FileSystemLibrary"), Category = "File System Library")
		static FORCEINLINE bool GetFilesRecursivelyInDirectory(TArray<FString> &Files, FString PathToDirectory, FString ExtensionFilter, bool OnlyReturnFilenames)
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Does the directory exist?
		if (PlatformFile.DirectoryExists(*PathToDirectory))
		{
			TArray<FString> ReturnFiles;
			FString tempExtensionFilter = ExtensionFilter;

			// Check that the directory has been created
			PlatformFile.FindFilesRecursively(ReturnFiles, *PathToDirectory, *tempExtensionFilter);

			// Check if found any files
			if (ReturnFiles.Num() > 0)
			{
				// Check if we want to exclude the extension from the return array.
				if (OnlyReturnFilenames)
				{
					TArray<FString> ReturnFilesNoExtension;

					for (int i = 0; i < ReturnFiles.Num(); i++)
					{
						ReturnFilesNoExtension.Add(FPaths::GetBaseFilename(*ReturnFiles[i]));
					}

					Files = ReturnFilesNoExtension;
					return true;
				}

				//If not return the array
				else
				{
					Files = ReturnFiles;
					return true;
				}
			}
		}

		// No files were found
		return false;
	}

	/* This function will return the directories present at the specified path.
	@param	Path		Path to the directory to search in.
	@return	Folders		If true, will only return the filenames (without the extension).
	*/
	//UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFoldersInDirectory", Keywords = "FileSystemLibrary"), Category = "File System Library folder")
		static FORCEINLINE bool GetFoldersInDirectory(TArray<FString> &Folders, FString Path)
	{
			TArray<FString> ReturnFolders;

			// Check that the directory has been created
			IFileManager::Get().FindFiles(ReturnFolders, *Path, false, true); //This is platform specific and seems to throw an error on Windows.

			// Check if found any files
			if (ReturnFolders.Num() > 0)
			{
				Folders = ReturnFolders;
				return true;
			}

		// No files were found
		return false;
	}


	/***** File IO *****/

	/* This function will load the content of the specified file to a string array. For text file, each array element represents a line from the document.
	@param	PathToFile	Path to the file to load the string array from.
	@return	FileContent	The file's content.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "LoadTextFileToStringArray", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE bool LoadTextFileToStringArray(TArray<FString> &FileContent, FString PathToFile)
	{
		// Does the file exist?
		if (VerifyFile(*PathToFile))
		{
			TArray<FString> ReturnFileContent;

			FFileHelper::LoadFileToStringArray(ReturnFileContent, *PathToFile, FFileHelper::EHashOptions::None);

			if (ReturnFileContent.Num() > 0)
			{
				// Success
				FileContent = ReturnFileContent;
				return true;
			}
		}

		// No files were found
		return false;
	}

	/* This function will load the content of the specified file to a string. For text file, each array element represents a line from the document.
	@param	PathToFile		Path to file to edit.
	@param	FileContent		Content to insert (where one element of the array represent a line of the document).
	@param	InsertAtIndex	Line number to insert to content at.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "InsertStringArrayToFile", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE bool InsertStringArrayToFile(FString PathToFile, TArray<FString> FileContent, int InsertAtIndex)
	{
		TArray<FString> ReturnFileContent;

		if (LoadTextFileToStringArray(ReturnFileContent, PathToFile))
		{
			ReturnFileContent.Insert(FileContent, InsertAtIndex);

			if (SaveStringArrayToFile(PathToFile, ReturnFileContent))
			{
				return true;
			}
		}

		return false;
	}

	/* This function will load the content of the specified file to a string array. For text file, each array element represents a line from the document.
	@param	PathToFile		Path to the file to load the string from.
	@return	FileContent		The file's content.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "LoadTextFileToString", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE bool LoadTextFileToString(FString &FileContent, FString PathToFile)
	{
		// Does the file exist?
		if (VerifyFile(*PathToFile))
		{
			TArray<FString> ReturnFileContent;

			FFileHelper::LoadFileToStringArray(ReturnFileContent, *PathToFile, FFileHelper::EHashOptions::None);

			if (ReturnFileContent.Num() > 0)
			{
				FString ReturnString;

				for (int i = 0; i < ReturnFileContent.Num(); i++)
				{
					ReturnString = ReturnFileContent[i] + '\n';
				}

				// Success
				FileContent = ReturnString;
				return true;
			}
		}

		// No files were found
		return false;
	}

	/* This function save the input content to a file.
	@param PathToFile	PathToFile	Path to the file to create (including the extension).
	@param FileContent	FileContent	The file's content.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "SaveStringArrayToFile", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE bool SaveStringArrayToFile(FString PathToFile, TArray<FString> FileContent)
	{
		IFileManager &FileManager = IFileManager::Get();

		TArray<FString> ReturnFileContent;
		uint32 Flags = 0;

		FFileHelper::SaveStringArrayToFile(FileContent, *PathToFile, FFileHelper::EEncodingOptions::AutoDetect, &FileManager, Flags);

		if (ReturnFileContent.Num() > 0)
		{
			// Success
			FileContent = ReturnFileContent;
			return true;
		}

		// No files were found
		return false;
	}

	/* This function will append the input string array to the file's content. The AppendFileToStringArray param will insert the input content before the file's. 
	@param PathToFile				Path to the file to edit.
	@param FileContent				Content to append to the file.
	@param AppendFileToStringArray	If true, will insert the input FileContent before the file's content.
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "AppendStringArrayToFile", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE bool AppendStringArrayToFile(FString PathToFile, TArray<FString> FileContent, bool AppendFileToStringArray)
	{
		TArray<FString> ReturnFileContent;

		if (LoadTextFileToStringArray(ReturnFileContent, PathToFile))
		{
			if (AppendFileToStringArray)
			{
				TArray<FString> InFileContent = FileContent;
				InFileContent.Append(ReturnFileContent);
				ReturnFileContent = InFileContent;
			}
			else
			{
				ReturnFileContent.Append(FileContent);
			}

			if (SaveStringArrayToFile(PathToFile, ReturnFileContent))
			{
				return true;
			}
		}

		return false;
	}

	/***** Path Utilities *****/

	/* This function will return a file extension from the input path. 
	@param PathToFile Path to get the extension from.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetExtension", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE FString GetFileExtension(FString Path)
	{
		return FPaths::GetExtension(Path, false);
	}

	/* This function will return a valid directory path from the input path (without a filename nor extension). 
	@param PathToFile The path to extract the valid directory from.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetDirectoryPath", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE FString GetFilePath(FString Path)
	{
		return FPaths::GetPath(Path);
	}

	/* This function will return a filename from the input path. 
	@param Path				Path to extract the filename from.
	@param IncludeExtension If true, the filename will be returned with its extension.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFilename", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE FString GetFileName(FString Path, bool IncludeExtension)
	{
		if (!IncludeExtension)
		{
			return FPaths::GetBaseFilename(Path, true);
		}

		else
		{
			return FPaths::GetCleanFilename(Path);
		}
	}

	/***** File Dialogs *****/

	/*This will open a Folder Select dialog. The FolderPath return value contain the path for the folder selected, its name and its extension.
@param DialogTitle		Title of the dialog window.
@param DefaultPath		Path to open by default (default is blank).
*/

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "OpenFolderSelectDialog", Keywords = "FileSystemLibrary"), Category = "System File Dialogs")
	static FORCEINLINE bool OpenFolderSelectDialog(FString &FolderPath, FString DialogTitle = "Select a folder", FString DefaultPath = "")
	{
		if (GEngine)
		{
			const void *ParentWindowHandle = nullptr;

			//If in game game
			if (GEngine->GameViewport)
			{
				ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
			}

			// If in editor
#if WITH_EDITOR

			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();

			if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
			{
				ParentWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}
#endif

			if (ParentWindowHandle)
			{
				FString ReturnPath;

// Use IDesktopPlatform (editor and dev only)
#if PLATFORM_LINUX
				IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
				if (!DesktopPlatform || !ParentWindowHandle)
				{
					// Couldn't initialise the platform's references
					return false;
				}

				else
				{
					// Opens the dialog window
					if (DesktopPlatform->OpenDirectoryDialog(ParentWindowHandle, DialogTitle, DefaultPath, ReturnPath))
					{
						// Checks that the returned path isn't empty
						if (ReturnPath != FString(""))
						{
							// Checks if the return path ends with "/"
							if (!ReturnPath.EndsWith(TEXT("/")))
							{
								ReturnPath.Append(TEXT("/"));
							}

							// Success
							FolderPath = ReturnPath;
							return true;
						}
					}
					return false;
				}
#endif

				// Use DialogManager.h for both windows and mac
				DialogManager *DialogMan;
#if PLATFORM_WINDOWS

				DialogMan = new DialogManagerWin();

#endif

#if PLATFORM_MAC

				DialogMan = new DialogManagerMac();
#endif

				if (DialogMan)
				{
					if (DialogMan->OpenDirectoryDialog(ParentWindowHandle, DialogTitle, DefaultPath, ReturnPath))
					{
						if (ReturnPath != TEXT(""))
						{
							if (!ReturnPath.EndsWith("/", ESearchCase::IgnoreCase))
							{
								ReturnPath.Append("/");
							}
							FolderPath = ReturnPath;
							delete DialogMan;
							return true;
						}
					}

					delete DialogMan;
				}
			}
		}

		return false;
	}

	/*This will open a Folder Select dialog that allows multiple files to be selected. The FilePath return value contain the path for the file selected, its name and its extension.
@param DialogTitle		Title of the dialog window.
@param DefaultPath		Path to open by default (default is blank).
@param FileTypes		The file type filter (you can add as many as you need). The format is: [Type Name] (*.[Type Extension]*)|*.[Type Extension]*|
*/

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "OpenFileMultiSelectDialog", Keywords = "FileSystemLibrary"), Category = "System File Dialogs")
	static FORCEINLINE bool OpenFileMultiSelectDialog(TArray<FString> &FilePaths, FString DialogTitle = "Select a file", FString DefaultPath = "", bool AllowMultiSelect = false, FString FileTypes = "All Files (*.*)|*.*|")
	{
		if (GEngine)
		{
			const void *ParentWindowHandle = nullptr;
			
			//If in game game
			if (GEngine->GameViewport)
			{
				ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
			}

			// If in editor
			#if WITH_EDITOR

			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();

			if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
			{
				ParentWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}
			#endif

			if (ParentWindowHandle)
			{
				TArray<FString> pathsToFiles;

// Use IDesktopPlatform (editor and dev only)
#if PLATFORM_LINUX
				IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
				if (!DesktopPlatform || !ParentWindowHandle)
				{
					// Couldn't initialise the platform's references
					return false;
				}

				else
				{
					EFileDialogFlags::Type Flags;

					// Set the flag based on param
					if (AllowMultiSelect)
					{
						Flags = EFileDialogFlags::Type::Multiple;
					}
					else
					{
						Flags = EFileDialogFlags::Type::None;
					}

					// Opens the dialog window
					if (DesktopPlatform->OpenFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, FString(""), FileTypes, Flags, pathsToFiles))
					{
						// Checks that there is at least 1 path to return
						if (pathsToFiles.Num() > 0)
						{
							// Checks that the first path isn't empty (this could be improved)
							if (pathsToFiles[0] != "")
							{
								// Success
								FilePaths = pathsToFiles;
								return true;
							}
						}
					}
				}

				// Failure
				return false;
#endif

				// Use DialogManager.h for both windows and mac
				DialogManager *DialogMan;
#if PLATFORM_WINDOWS

				DialogMan = new DialogManagerWin();

#endif

#if PLATFORM_MAC

				DialogMan = new DialogManagerMac();
#endif

				if (DialogMan)
				{
					if (DialogMan->OpenFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, TEXT(""), FileTypes, AllowMultiSelect, pathsToFiles))
					{
						if (pathsToFiles[0] != TEXT(""))
						{
							FilePaths = pathsToFiles;

							delete DialogMan;
							return true;
						}
					}

					delete DialogMan;
				}
			}
		}
		return false;
	}

	/*This will open a Folder Select dialog. The FilePath return value contain the path for the file selected, its name and its extension.
@param DialogTitle		Title of the dialog window.
@param DefaultPath		Path to open by default (default is blank).
@param FileTypes		The file type filter (you can add as many as you need). The format is: [Type Name] (*.[Type Extension]*)|*.[Type Extension]*|
*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "OpenFileSelectDialog", Keywords = "FileSystemLibrary"), Category = "System File Dialogs")
	static FORCEINLINE bool OpenFileSelectDialog(FString &FilePath, FString DialogTitle = "Select a file", FString DefaultPath = "", FString FileTypes = "All Files (*.*)|*.*|")
	{
		TArray<FString> FilePaths;

		if (OpenFileMultiSelectDialog(FilePaths, DialogTitle, DefaultPath, false, FileTypes))
		{

			FilePath = FilePaths[0];
			return true;
		}
		return false;
	}

	/*This will open a File Save dialog. The return value contains the path to the file selected, its name and extension.
	@param DialogTitle		Title of the dialog window.
	@param DefaultPath		Path to open by default (default is blank).
	@param DefaultFileName	Name to give the file by default.
	@param FileTypes		The file type filter (you can add as many as you need). The format is: [Type Name] (*.[Type Extension]*)|*.[Type Extension]*|
	*/
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "OpenSaveFileDialog", Keywords = "FileSystemLibrary"), Category = "System File Dialogs")
	static FORCEINLINE bool OpenSaveFileDialog(FString &SaveToPath, FString DialogTitle = "Select a file", FString DefaultPath = "", FString DefaultFileName = "", FString FileTypes = "All Files (*.*)|*.*|")
	{
		if (GEngine)
		{
			const void *ParentWindowHandle = nullptr;

			//If in game game
			if (GEngine->GameViewport)
			{
				ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
			}

			// If in editor
#if WITH_EDITOR

			IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
			const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();

			if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
			{
				ParentWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}
#endif

			if (ParentWindowHandle)
			{

				TArray<FString> pathsToFiles;
				FString ReturnedPath;

// Use IDesktopPlatform (editor and dev only)
#if PLATFORM_LINUX
				IDesktopPlatform *DesktopPlatform = FDesktopPlatformModule::Get();
				if (!DesktopPlatform || !ParentWindowHandle)
				{
					// Couldn't initialise the platform's references
					return false;
				}
				else
				{

					// Opens the dialog window
					if (DesktopPlatform->SaveFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, DefaultFileName, FileTypes, EFileDialogFlags::Type::None, pathsToFiles))
					{
						ReturnedPath = pathsToFiles[0];

						// Check that the path isn't empty
						if (ReturnedPath != FString(""))
						{
							// Success
							SaveToPath = ReturnedPath;
							return true;
						}
					}
				}

				//Failure
				return false;
#endif

				// Use DialogManager.h for both windows and mac
				DialogManager *DialogMan;
#if PLATFORM_WINDOWS

				DialogMan = new DialogManagerWin();

#endif

#if PLATFORM_MAC

				DialogMan = new DialogManagerMac();
#endif

				if (DialogMan)
				{
					if (DialogMan->SaveFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, DefaultFileName, FileTypes, false, pathsToFiles))
					{
						if (pathsToFiles[0] != TEXT(""))
						{
							ReturnedPath = pathsToFiles[0];
							delete DialogMan;

							SaveToPath = ReturnedPath;
							return true;
						}
					}

					delete DialogMan;
				}
			}
		}
		return false;
	}
};
