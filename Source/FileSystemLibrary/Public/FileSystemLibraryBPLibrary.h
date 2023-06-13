// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// Copyright Lambda Works, Samuel Metters 2019. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

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

	/***** File Operations *****/

	/* This function will check to see if the specified file (folder) exist. You need to include the file extension. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "VerifyFile", Keywords = "FileSystemLibrary"), Category = "SystemFileOperations")
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

	/* This function will copy a file from a path to another. You need to include the full path with extension for both input parameters. */
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

	/* This function will copy a file from a path to another. You need to include the full path with extension for both input parameters. */
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

	/* This function will rename the specified file. You need to include filename with extension for both input parameters. */
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

	/* This function will rename the specified file. You need to include filename with extension for both input parameters. */
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

	/* This function will check to see if the specified directory (folder) exist. If it doesn't and CreateDirectory=true, it will create the directory for you. */
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

	/* This function will the specified directory and all file/folders inside it. */
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

	/* This function will copy all files and folders from PathToDirectory to NewPathToDirectory. */
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

	/* This function will move all files and folders from PathToDirectory to NewPathToDirectory. */
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

	/* This function will return the file's or folder's properties. */
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

	/* This function will return the file's or folder's properties. */
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

	/* This function will return the name of all files present in the specified directory. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFilesInDirectory", Keywords = "FileSystemLibrary"), Category = "File System Library")
	static FORCEINLINE bool GetFilesInDirectory(TArray<FString> &Files, FString PathToDirectory)
	{
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Does the directory exist?
		if (PlatformFile.DirectoryExists(*PathToDirectory))
		{
			TArray<FString> ReturnFiles;
			FString ExtensionFilter;

			// Check that the directory has been created
			PlatformFile.FindFiles(ReturnFiles, *PathToDirectory, *ExtensionFilter);

			if (ReturnFiles.Num() > 0)
			{
				// Success
				Files = ReturnFiles;
				return true;
			}
		}

		// No files were found
		return false;
	}

	/***** File IO *****/

	/* This function will load the content of the specified file to a string array. For text file, each array element represents a line from the document.*/
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

	/* This function will load the content of the specified file to a string. For text file, each array element represents a line from the document.*/
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

	/* This function will load the content of the specified file to a string array. For text file, each array element represents a line from the document.*/
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

	/* This function will load the content of the specified file to a string array. */
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

	/* This function will append the input string array to the file's content. The AppendFileToStringArray param will insert the input content before the file's.  */
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

	/* This function will return the extension for the specified file. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFileExtension", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE FString GetFileExtension(FString PathToFile)
	{
		return FPaths::GetExtension(PathToFile, false);
	}

	/* This function will return the specified file's path. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFilePath", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE FString GetFilePath(FString PathToFile)
	{
		return FPaths::GetPath(PathToFile);
	}

	/* This function will return the specified file's name. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetFileName", Keywords = "FileSystemLibrary"), Category = "SystemFile I/O")
	static FORCEINLINE FString GetFileName(FString PathToFile, bool IncludeExtension)
	{
		if (!IncludeExtension)
		{
			return FPaths::GetBaseFilename(PathToFile, true);
		}

		else
		{
			return FPaths::GetCleanFilename(PathToFile);
		}
	}

	/***** File Dialogs *****/

	/* This will open a Folder Select dialog on any desktop platform supported by Unreal Engine. It also ensures that the returned path always ends with "/". */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "OpenFolderSelectDialog", Keywords = "FileSystemLibrary"), Category = "System File Dialogs")
	static FORCEINLINE bool OpenFolderSelectDialog(FString &FolderPath, FString DialogTitle = "Select a folder", FString DefaultPath = "")
	{
		if (GEngine && GEngine->GameViewport)
		{
			const void *ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();

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

	/*This will open a File Select dialog on any platform supported by Unreal Engine. The FilePaths return value contains the paths for files selected, their names and its extensions. */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "OpenFileMultiSelectDialog", Keywords = "FileSystemLibrary"), Category = "System File Dialogs")
	static FORCEINLINE bool OpenFileMultiSelectDialog(TArray<FString> &FilePaths, FString DialogTitle = "Select a file", FString DefaultPath = "", bool AllowMultiSelect = false, FString FileTypes = "All Files (*.*)|*.*|")
	{
		if (GEngine && GEngine->GameViewport)
		{
			const void *ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();

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

	/*This will open a File Select dialog on any platform supported by Unreal Engine. The FilePath return value contain the path for file selected, its name and its extension. */
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

	/*This will open a File Save dialog on any platform supported by Unreal Engine. The return value contains the path to the file selected, its name and extension.  */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "OpenSaveFileDialog", Keywords = "FileSystemLibrary"), Category = "System File Dialogs")
	static FORCEINLINE bool OpenSaveFileDialog(FString &SaveToPath, FString DialogTitle = "Select a file", FString DefaultPath = "", FString DefaultFileName = "", FString FileTypes = "All Files (*.*)|*.*|")
	{
		if (GEngine && GEngine->GameViewport)
		{
			const void *ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();

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
