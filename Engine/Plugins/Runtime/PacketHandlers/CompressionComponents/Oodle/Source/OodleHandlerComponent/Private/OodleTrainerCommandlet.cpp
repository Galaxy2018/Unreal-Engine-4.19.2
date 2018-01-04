// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OodleTrainerCommandlet.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FeedbackContext.h"
#include "Misc/App.h"
#include "UObject/CoreNet.h"


#define LOCTEXT_NAMESPACE "Oodle"


// @todo #JohnB: Make sure to note that >100mb data (for production games, even more) should be captured

// @todo #JohnB: Gather stats while merging, to give the commandlet some more informative output


// @todo #JohnB: QA Request: Change all the logs for the trainer, to output to an OodleTrainer.log file
//					(only the logs in this file would output to it, not general engine logs)


/**
 * UOodleTrainerCommandlet
 */

UOodleTrainerCommandlet::UOodleTrainerCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LogToConsole = true;
}

int32 UOodleTrainerCommandlet::Main(const FString& Params)
{
#if HAS_OODLE_SDK
	TArray<FString> Tokens, Switches;

	ParseCommandLine(*Params, Tokens, Switches);

	if (Tokens.Num() > 0)
	{
		FString MainCmd = Tokens[0];

		if (MainCmd == TEXT("Enable"))
		{
			HandleEnable();
		}
#if !UE_BUILD_SHIPPING || OODLE_DEV_SHIPPING
		else if (MainCmd == TEXT("MergePackets"))
		{
			if (Tokens.Num() > 2)
			{
				FString OutputCapFile = Tokens[1];

				if (FPaths::IsRelative(OutputCapFile))
				{
					OutputCapFile = FPaths::Combine(*GOodleSaveDir, TEXT("Server"), *OutputCapFile);
				}

				FString MergeList = Tokens[2];
				bool bMergeAll = MergeList == TEXT("all");

				if (!bMergeAll)
				{
					TArray<FString> MergeFiles;
					MergeList.ParseIntoArray(MergeFiles, TEXT(","));
					HandleMergePackets(OutputCapFile, MergeFiles);
				}
				else if (Tokens.Num() > 3)
				{
					MergeList = Tokens[3];

					FString MergeDir = MergeList;

					// Adjust non-absolute directories, so they are relative to the Saved\Oodle\Server directory
					if (FPaths::IsRelative(MergeDir))
					{
						MergeDir = FPaths::Combine(*GOodleSaveDir, TEXT("Server"), *MergeDir);
					}

					TArray<FString> MergeFiles;
					GetCaptureFiles(TEXT(""), -1, MergeDir, MergeFiles);
					HandleMergePackets(OutputCapFile, MergeFiles);
				}
				else
				{
					UE_LOG(OodleHandlerComponentLog, Error,
							TEXT("Need to specify a target directory, e.g: MergePackets OutFile All Dir"));
				}
			}
			else
			{
				UE_LOG(OodleHandlerComponentLog, Error,
						TEXT("Error parsing 'MergePackets' parameters. Must specify an output file, and packet files to merge"));
			}
		}
		else if (MainCmd == TEXT("GenerateDictionary"))
		{
			if (Tokens.Num() > 3)
			{
				FString OutputDictionaryFile = Tokens[1];
				if (FPaths::IsRelative(OutputDictionaryFile))
				{
					OutputDictionaryFile = FPaths::Combine(*GOodleContentDir, *FString::Printf(TEXT("%s%s.udic"), FApp::GetProjectName(), *OutputDictionaryFile));
				}

				FString FilenameFilter = Tokens[2];
				if (FilenameFilter == TEXT("all"))
				{
					FilenameFilter = TEXT("");
				}

				FString Changelist = Tokens[3];
				FString InputCaptureFiles = Tokens[4];
				bool bMergeAll = InputCaptureFiles == TEXT("all");
				FOodleDictionaryGenerator Generator;

				int32 ChangelistNumber = -1;
				if (Changelist != TEXT("all"))
				{
					ChangelistNumber = FCString::Atoi(*Changelist);
				}

				if (!bMergeAll)
				{
					TArray<FString> InputCaptureFilesArray;
					InputCaptureFiles.ParseIntoArray(InputCaptureFilesArray, TEXT(","));
					Generator.BeginGenerateDictionary(OutputDictionaryFile, InputCaptureFilesArray);
				}
				else if (Tokens.Num() > 4)
				{
					InputCaptureFiles = Tokens[5];

					FString InputDir = InputCaptureFiles;

					if (FPaths::IsRelative(InputDir))
					{
						InputDir = FPaths::Combine(*GOodleSaveDir, TEXT("Server"), *InputDir);
					}

					TArray<FString> InputCaptureFilesArray;
					GetCaptureFiles(FilenameFilter, ChangelistNumber, InputDir, InputCaptureFilesArray);
					Generator.BeginGenerateDictionary(OutputDictionaryFile, InputCaptureFilesArray);
				}
				else
				{
					UE_LOG(OodleHandlerComponentLog, Error,
							TEXT("Need to specify a target directory, e.g: GenerateDictionary OutFile All Dir"));
				}
			}
			else
			{
				UE_LOG(OodleHandlerComponentLog, Error,
						TEXT("Error parsing 'GenerateDictionary' parameters. Must specify an output file, packet files as input, and a changelist (specify \"all\" to ignore changelist number)"));
			}
		}
		else if (MainCmd == TEXT("AutoGenerateDictionaries"))
		{
			int32 ChangelistNumber = -1;
			if (Tokens.Num() > 1)
			{
				FString Changelist = Tokens[1];
				if (Changelist != TEXT("all"))
				{
					ChangelistNumber = FCString::Atoi(*Changelist);
				}
			}

			HandleAutoGenerateDictionaries(ChangelistNumber);
		}
		else if (MainCmd == TEXT("DebugDump"))
		{
			if (Tokens.Num() > 2)
			{
				FString OutputDirectory = Tokens[1];
				FString SourceDirectory = Tokens[2];
				int32 Changelist = (Tokens.Num() > 3 ? FCString::Atoi(*Tokens[3]) : -1);
				TArray<FString> DumpFiles;

				GetCaptureFiles(TEXT(""), Changelist, SourceDirectory, DumpFiles);

				HandleDebugDumpPackets(OutputDirectory, SourceDirectory, DumpFiles);
			}
			else
			{
				UE_LOG(OodleHandlerComponentLog, Error,
						TEXT("Error parsing 'DebugDump' parameters. Must specify an output directory, and a source directory"));
			}
		}
#endif
		else
		{
			UE_LOG(OodleHandlerComponentLog, Error, TEXT("Unrecognized sub-command '%s'"), *MainCmd);
		}
	}
	else
	{
		UE_LOG(OodleHandlerComponentLog, Error, TEXT("Error parsing main commandlet sub-command"));

		// @todo #JohnB: Add a 'help' command, with documentation output
	}
#else
	UE_LOG(OodleHandlerComponentLog, Error, TEXT("Oodle unavailable."));
#endif


	return (GWarn->GetNumErrors() == 0 ? 0 : 1);
}

#if HAS_OODLE_SDK
bool UOodleTrainerCommandlet::HandleEnable()
{
	bool bSuccess = true;
	TArray<FString> Components;
	bool bAlreadyEnabled = false;

	GConfig->GetArray(TEXT("PacketHandlerComponents"), TEXT("Components"), Components, GEngineIni);

	for (FString CurComponent : Components)
	{
		if (CurComponent.StartsWith(TEXT("OodleHandlerComponent")))
		{
			bAlreadyEnabled = true;
			break;
		}
	}


	if (!bAlreadyEnabled)
	{
		GConfig->SetBool(TEXT("PacketHandlerComponents"), TEXT("bEnabled"), true, GEngineIni);

		Components.Add(TEXT("OodleHandlerComponent"));
		GConfig->SetArray(TEXT("PacketHandlerComponents"), TEXT("Components"), Components, GEngineIni);

		OodleHandlerComponent::InitFirstRunConfig();

		GConfig->Flush(false);


		UE_LOG(OodleHandlerComponentLog, Log, TEXT("Initialized first-run settings for Oodle (will start in Trainer mode)."));
	}
	else
	{
		UE_LOG(OodleHandlerComponentLog, Error, TEXT("The Oodle PacketHandler is already enabled."));
	}

	return bSuccess;
}

#if !UE_BUILD_SHIPPING || OODLE_DEV_SHIPPING
bool UOodleTrainerCommandlet::HandleMergePackets(FString OutputCapFile, const TArray<FString>& MergeList)
{
	bool bSuccess = VerifyOutputFile(OutputCapFile);

	TMap<FArchive*, FString> MergeMap;
	bSuccess = bSuccess && GetMergeMapFromList(MergeList, MergeMap);

	if (bSuccess)
	{
		FArchive* OutArc = IFileManager::Get().CreateFileWriter(*OutputCapFile);

		if (OutArc != nullptr)
		{
			UE_LOG(OodleHandlerComponentLog, Log, TEXT("Merging files into output file: %s"), *OutputCapFile);

			FPacketCaptureArchive OutputFile(*OutArc);
			bool bErrorAppending = false;

			OutputFile.SerializeCaptureHeader();

			for (TMap<FArchive*, FString>::TConstIterator It(MergeMap); It; ++It)
			{
				FArchive* CurReadArc = It.Key();
				FPacketCaptureArchive ReadFile(*CurReadArc);

				UE_LOG(OodleHandlerComponentLog, Log, TEXT("    Merging: %s"), *It.Value());

				OutputFile.AppendPacketFile(ReadFile);

				if (OutputFile.IsError())
				{
					bErrorAppending = true;
					break;
				}

				delete CurReadArc;
			}

			MergeMap.Empty();


			if (!bErrorAppending)
			{
				bSuccess =  true;
			}
			else
			{
				UE_LOG(OodleHandlerComponentLog, Error, TEXT("Error appending packet file."));
			}


			if (bSuccess)
			{
				UE_LOG(OodleHandlerComponentLog, Log, TEXT("Merge packets success."));
			}


			OutArc->Close();

			delete OutArc;

			OutArc = nullptr;
		}
		else
		{
			UE_LOG(OodleHandlerComponentLog, Error, TEXT("Could not create output file."));
		}
	}

	return bSuccess;
}

void UOodleTrainerCommandlet::GetCaptureFiles(FString FilenameFilter, int32 ChangelistNumber, FString StartDirectory, TArray<FString>& OutFiles)
{
	class FLocalDirIterator : public IPlatformFile::FDirectoryVisitor
	{
		FString FilenameFilter;
		int32 ChangelistNumber;
		TArray<FString>& OutFiles;

	public:
		FLocalDirIterator(FString InFilenameFilter, int32 InChangelistNumber, TArray<FString>& InOutFiles)
			: FilenameFilter(InFilenameFilter)
			, ChangelistNumber(InChangelistNumber)
			, OutFiles(InOutFiles)

		{
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
		{
			FString FileOrDirectoryString(FilenameOrDirectory);
			if (!bIsDirectory && (ChangelistNumber == -1 || FileOrDirectoryString.Contains(FString::Printf(TEXT("%d"), ChangelistNumber))))
			{
				if (FileOrDirectoryString.EndsWith(FString(TEXT("ucap"))))
				{
					if (FilenameFilter == TEXT("") || FileOrDirectoryString.Contains(FilenameFilter))
					{
						OutFiles.Add(FString(FilenameOrDirectory));
					}
				}
			}

			return true;
		}


	};

	FLocalDirIterator DirIterator(FilenameFilter, ChangelistNumber, OutFiles);
	IFileManager::Get().IterateDirectoryRecursively(*StartDirectory, DirIterator);
}

bool UOodleTrainerCommandlet::HandleAutoGenerateDictionaries(int32 ChangelistNumber)
{
	return GenerateDictionary(true, ChangelistNumber) &&
		GenerateDictionary(false, ChangelistNumber);
}

bool UOodleTrainerCommandlet::HandleDebugDumpPackets(FString OutputDirectory, FString SourceDirectory, const TArray<FString>& DumpList)
{
	bool bSuccess = false;
	TMap<FArchive*, FString> SourceMap;

	bSuccess = GetMergeMapFromList(DumpList, SourceMap);

	if (bSuccess)
	{
		for (TMap<FArchive*, FString>::TConstIterator It(SourceMap); It; ++It)
		{
			FArchive* CurReadArc = It.Key();
			FPacketCaptureArchive ReadFile(*CurReadArc);

			UE_LOG(OodleHandlerComponentLog, Log, TEXT("    Dumping: %s"), *It.Value());

			FString PartialFileDir = FPaths::ChangeExtension(*(It.Value().Mid(SourceDirectory.Len())), TEXT(".bin"));
			FString OutputFile = FPaths::Combine(*OutputDirectory, *PartialFileDir);
			FArchive* OutputArc = IFileManager::Get().CreateFileWriter(*OutputFile);

			// Oodle example packet format:
			//		[4:ChannelNum][?:PacketData]
			//	PacketData:
			//		[4:PacketChannel][4:PacketSize][PacketSize:PacketData]
			if (OutputArc != nullptr)
			{
				uint32 DudChannelNum = 0;

				OutputArc->ByteOrderSerialize(&DudChannelNum, sizeof(uint32));

				ReadFile.SerializeCaptureHeader();

				uint32 InPacketCount = ReadFile.GetPacketCount();

				const uint32 BufferSize = 1048576;
				uint8* ReadBuffer = new uint8[BufferSize];

				for (int32 PacketIdx=0; PacketIdx<(int32)InPacketCount; PacketIdx++)
				{
					uint32 InPacketSize = BufferSize;

					ReadFile.SerializePacket(ReadBuffer, InPacketSize);

					uint32 DudPacketChannel = 0;

					OutputArc->ByteOrderSerialize(&DudPacketChannel, sizeof(uint32));
					OutputArc->ByteOrderSerialize(&InPacketSize, sizeof(uint32));
					OutputArc->Serialize(ReadBuffer, InPacketSize);
				}

				delete[] ReadBuffer;
				ReadBuffer = nullptr;

				OutputArc->Close();

				delete OutputArc;

				UE_LOG(OodleHandlerComponentLog, Log, TEXT("Successfully dumped packet data to file: %s"), *OutputFile);

				bSuccess = true;
			}

			delete CurReadArc;
		}
	}
	else
	{
		UE_LOG(OodleHandlerComponentLog, Log, TEXT("Failed to get a list of capture files for directory: %s"), *SourceDirectory);
	}

	return bSuccess;
}

bool UOodleTrainerCommandlet::GenerateDictionary(bool bIsInput, int32 ChangelistNumber)
{
	// Get a list of all directories containing saved packet captures
	FString OodleCaptureDir = FPaths::Combine(*GOodleSaveDir, TEXT("Server"));
	TArray<FString> CaptureFiles;

	FString InputOrOutput;
	if (bIsInput)
	{
		InputOrOutput = TEXT("Input");
	}
	else
	{
		InputOrOutput = TEXT("Output");
	}

	GetCaptureFiles(InputOrOutput, ChangelistNumber, OodleCaptureDir, CaptureFiles);

	bool bSuccess = false;

	if (CaptureFiles.Num() > 0)
	{
		UE_LOG(OodleHandlerComponentLog, Log, TEXT("Generating dictionaries for %s folder"), *InputOrOutput);

		FString OutDic = FPaths::Combine(*GOodleContentDir, *FString::Printf(TEXT("%s%s.udic"), FApp::GetProjectName(), *InputOrOutput));

		UE_LOG(OodleHandlerComponentLog, Log, TEXT("Generating dictionary for folder '%s', outputting to: %s"), *InputOrOutput, *OutDic);

		FOodleDictionaryGenerator Generator;

		bSuccess = Generator.BeginGenerateDictionary(OutDic, CaptureFiles);

		if (bSuccess)
		{
			UE_LOG(OodleHandlerComponentLog, Log, TEXT("Successfully generated dictionary."));
		}
		else
		{
			bool bTriggerFail = false;

			if (FApp::IsUnattended())
			{
				bTriggerFail = true;
			}
			else
			{
				FText Msg = FText::Format(
					LOCTEXT("OodleGenerateContinue", "Failed to generate dictionary for folder '{0}', continue anyway?"),
					FText::FromString(InputOrOutput));

				EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Msg);

				bTriggerFail = (Result != EAppReturnType::Yes);
			}

			if (bTriggerFail)
			{
				UE_LOG(OodleHandlerComponentLog, Error, TEXT("Failed to generate dictionary for folder '%s', aborting."), *InputOrOutput);

				return bSuccess;
			}
			else
			{
				UE_LOG(OodleHandlerComponentLog, Warning,
					TEXT("Failed to generate dictionary for folder '%s', continuing to next folder"), *InputOrOutput);
			}
		}
	}
	else
	{
		UE_LOG(OodleHandlerComponentLog, Log, TEXT("Could not find any packet captures to for %s."), *InputOrOutput);
	}
	return bSuccess;
}

bool UOodleTrainerCommandlet::GetMergeMapFromList(const TArray<FString>& FileList, TMap<FArchive*, FString>& OutMergeMap)
{
	bool bSuccess = true;

	for (FString CurMergeFile : FileList)
	{
		FArchive* ReadArc = IFileManager::Get().CreateFileReader(*CurMergeFile);

		if (ReadArc != NULL)
		{
			OutMergeMap.Add(ReadArc, CurMergeFile);
		}
		else
		{
			UE_LOG(OodleHandlerComponentLog, Error, TEXT("Could not read packet capture file: %s"), *CurMergeFile);

			bSuccess = false;
			break;
		}
	}

	return bSuccess;
}

bool UOodleTrainerCommandlet::VerifyOutputFile(FString OutputFile)
{
	bool bSuccess = true;

	if (FPaths::FileExists(OutputFile))
	{
		if (FApp::IsUnattended())
		{
			UE_LOG(OodleHandlerComponentLog, Log, TEXT("Output file '%s' already exists. Aborting (can't overwrite in unattended)."),
					*OutputFile);

			bSuccess = false;
		}
		else
		{
			FText Msg = FText::Format(LOCTEXT("OutputFileOverwrite", "Overwrite existing output file '{0}'?"),
										FText::FromString(OutputFile));

			EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, Msg);

			bSuccess = (Result == EAppReturnType::Yes);
		}
	}

	return bSuccess;
}


/**
 * FOodleDictionaryGenerator
 */

bool FOodleDictionaryGenerator::BeginGenerateDictionary(FString InOutputDictionaryFile, const TArray<FString>& InputCaptureFiles)
{
	bool bSuccess = false;

	OutputDictionaryFile = InOutputDictionaryFile;

	bSuccess = InitGenerator();
	bSuccess = bSuccess && UOodleTrainerCommandlet::GetMergeMapFromList(InputCaptureFiles, MergeMap);
	
	if (bSuccess)
	{
		ReadPackets(InputCaptureFiles);
	}

	if (bDebugDump)
	{
		bSuccess = bSuccess && DebugDumpPackets();
	}
	else
	{
		bSuccess = bSuccess && GenerateAndWriteDictionary();
	}

	Cleanup();

	return bSuccess;
}

bool FOodleDictionaryGenerator::InitGenerator()
{
	bool bSuccess = false;

	bDebugDump = FParse::Param(FCommandLine::Get(), TEXT("OodleDebugDump"));

	// Parse and verify parameters
	// @todo #JohnB: Set these defaults from the .ini file as well (commandline overriding) - mandatory for tailoring to specific games,
	//					using the games .ini file

	FParse::Value(FCommandLine::Get(), TEXT("-HashTableSize="), HashTableSize);
	FParse::Value(FCommandLine::Get(), TEXT("-DictionarySize="), DictionarySize);
	FParse::Value(FCommandLine::Get(), TEXT("-DictionaryTrials="), DictionaryTrials);
	FParse::Value(FCommandLine::Get(), TEXT("-TrialRandomness="), TrialRandomness);
	FParse::Value(FCommandLine::Get(), TEXT("-TrialGenerations="), TrialGenerations);
	bNoTrials = FParse::Param(FCommandLine::Get(), TEXT("NoTrials"));
	bCompressionTest = FParse::Param(FCommandLine::Get(), TEXT("CompressionTest"));

	if (HashTableSize < 17 || HashTableSize > 21)
	{
		// @todo #JohnB: Don't know if ranges outside of this are supported or are useful
		HashTableSize = FMath::Clamp(HashTableSize, 17, 21);

		UE_LOG(OodleHandlerComponentLog, Warning, TEXT("Hash table size is usually between 17-21. Clamping to: %i"), HashTableSize);
	}

	if (DictionarySize < (1024 * 1024) || DictionarySize > OODLENETWORK1_MAX_DICTIONARY_SIZE)
	{
		 DictionarySize = FMath::Clamp(DictionarySize, 1024 * 1024, OODLENETWORK1_MAX_DICTIONARY_SIZE);

		 UE_LOG(OodleHandlerComponentLog, Warning, TEXT("Dictionary size is usually between 1MB and %iMB. Clamping to: %iMB"),
				(OODLENETWORK1_MAX_DICTIONARY_SIZE / (1024 * 1024)), (DictionarySize / (1024 * 1024)));
	}

	if (bNoTrials)
	{
		DictionaryTrials = 0;
		TrialRandomness = 0;
		TrialGenerations = 0;
	}
	else
	{
		if (DictionaryTrials < 2)
		{
			DictionaryTrials = 2;

			UE_LOG(OodleHandlerComponentLog, Warning,
					TEXT("DictionaryTrials must be at least two. Clamping to: 2. Specify -NoTrials to disable."));
		}

		if (TrialRandomness < 50 || TrialRandomness > 200)
		{
			TrialRandomness = FMath::Clamp(TrialRandomness, 50, 200);

			UE_LOG(OodleHandlerComponentLog, Warning, TEXT("Dictionary trial randomness is usually between 50-200%. Clamping to: %i"),
					TrialRandomness);
		}

		if (TrialGenerations < 1)
		{
			TrialGenerations = 1;

			UE_LOG(OodleHandlerComponentLog, Warning, TEXT("Dictionary trial generations must be at least one. Clamping to: 1."));
		}
		else if (TrialGenerations > 1)
		{
			UE_LOG(OodleHandlerComponentLog, Log, TEXT("Dictionary trial generations higher than 1, can take a long time."));
		}
	}


	UE_LOG(OodleHandlerComponentLog, Log, TEXT("Oodle trainer parameters:"));
	UE_LOG(OodleHandlerComponentLog, Log, TEXT("- HashTableSize: %i"), HashTableSize);
	UE_LOG(OodleHandlerComponentLog, Log, TEXT("- DictionarySize: %i (~%iMB)"), DictionarySize, (DictionarySize / (1024 * 1024)));
	UE_LOG(OodleHandlerComponentLog, Log, TEXT("- Performing Trials: %s"), (bNoTrials ? TEXT("No") : TEXT("Yes")));

	if (!bNoTrials)
	{
		UE_LOG(OodleHandlerComponentLog, Log, TEXT("- DictionaryTrials: %i"), DictionaryTrials);
		UE_LOG(OodleHandlerComponentLog, Log, TEXT("- TrialRandomness: %i"), TrialRandomness);
		UE_LOG(OodleHandlerComponentLog, Log, TEXT("- TrialGenerations: %i"), TrialGenerations);
	}



	// Adjust non-absolute directories, so they are relative to the game directory
	if (FPaths::IsRelative(OutputDictionaryFile))
	{
		OutputDictionaryFile = FPaths::Combine(*FPaths::ProjectDir(), TEXT("Server"), *OutputDictionaryFile);
	}

	bSuccess = UOodleTrainerCommandlet::VerifyOutputFile(OutputDictionaryFile);

	return bSuccess;
}

void FOodleDictionaryGenerator::ReadPackets(const TArray<FString>& InputCaptureFiles)
{
	// Now begin training
	TArray<FArchive*> UnboundArchives;
	TArray<FPacketCaptureArchive> BoundArchives;

	MergeMap.GenerateKeyArray(UnboundArchives);

	for (FArchive* CurArc : UnboundArchives)
	{
		new(BoundArchives) FPacketCaptureArchive(*CurArc);
	}


	// Serialize capture archive headers, and retrieve packet count
	int32 PacketCount = 0;

	for (FPacketCaptureArchive& CurArc : BoundArchives)
	{
		CurArc.SerializeCaptureHeader();

		if (!CurArc.IsError())
		{
			PacketCount += CurArc.GetPacketCount();
		}
	}


	// Generate a randomized index from 0-PacketCount (no repeated values), for randomly splitting packets to 3-4 different lists
	// NOTE: There is a good reason the randomness is done in this particular way - it ensures good quality randomness.
	TArray<uint32> RandIndex;

	RandIndex.SetNum(PacketCount);

	for (int32 i=0; i<PacketCount; i++)
	{
		RandIndex[i] = i;
	}

	// Knuth shuffle
	int32 LastIdx = RandIndex.Num()-1;

	for (int32 i=0; i<LastIdx; i++)
	{
		int32 SwapIdx = i + FMath::RandRange(0, LastIdx - i);

		if (SwapIdx != i)
		{
			RandIndex.Swap(i, SwapIdx);
		}
	}

	// Now translate the randomized index, back into a sequential index, which maps random packets to each list-type (from type 0-4)
	int32 ListSplitCount = 3 + (bCompressionTest ? 1 : 0);
	TArray<uint8> PacketToListMap;

	PacketToListMap.SetNum(PacketCount);

	// Disable randomization for debug dumps, and use only the DictionaryPackets list
	if (bDebugDump)
	{
		for (int32 i=0; i<PacketCount; i++)
		{
			PacketToListMap[i] = 0;
		}

		DictionaryPackets.Empty(PacketCount);
		DictionaryPacketSizes.Empty(PacketCount);
	}
	else
	{
		for (int32 i=0; i<RandIndex.Num(); i++)
		{
			PacketToListMap[RandIndex[i]] = (i % ListSplitCount);
		}

		// Now step through the packets, and randomly assign them to the appropriate list
		int32 AvgSplitSize = (PacketCount / ListSplitCount) + 1;

		DictionaryPackets.Empty(AvgSplitSize);
		DictionaryPacketSizes.Empty(AvgSplitSize);
		DictionaryTestPackets.Empty(AvgSplitSize);
		DictionaryTestPacketSizes.Empty(AvgSplitSize);

		// Dictionary test packets may overflow into the trainer packets list, so allocate two-lists worth
		TrainerPackets.Empty(AvgSplitSize * 2);
		TrainerPacketSizes.Empty(AvgSplitSize * 2);

		if (bCompressionTest)
		{
			CompressionTestPackets.Empty(AvgSplitSize);
			CompressionTestPacketSizes.Empty(AvgSplitSize);
		}
	}


	const uint32 DictionaryTestLimit = 1024 * 1024 * 64;
	const uint32 BufferSize = 1048576;
	uint8* ReadBuffer = new uint8[BufferSize];
	uint32 PacketIdx = 0;

	for (FPacketCaptureArchive& CurArc : BoundArchives)
	{
		while (CurArc.Tell() < CurArc.TotalSize() && PacketIdx < (uint32)PacketCount)
		{
			uint32 PacketSize = BufferSize;

			CurArc.SerializePacket(ReadBuffer, PacketSize);

			if (CurArc.IsError())
			{
				UE_LOG(OodleHandlerComponentLog, Warning, TEXT("Error serializing packet from packet capture archive. Skipping."));

				break;
			}

			check(PacketSize > 0);


			int32 ListIdx = PacketToListMap[PacketIdx];

			check(ListIdx < ListSplitCount);

			TArray<uint8*>& TargetPacketList =
				(
					ListIdx == 0 ?			DictionaryPackets :
					ListIdx == 1 ?			(bDictionaryTestOverflow ? TrainerPackets : DictionaryTestPackets) :
					ListIdx == 2 ?			TrainerPackets :
					/* ListIdx == 3 ? */	CompressionTestPackets
				);

			TArray<S32>& TargetSizesList =
				(
					ListIdx == 0 ?			DictionaryPacketSizes :
					ListIdx == 1 ?			(bDictionaryTestOverflow ? TrainerPacketSizes : DictionaryTestPacketSizes) :
					ListIdx == 2 ?			TrainerPacketSizes :
					/* ListIdx == 3 ? */	CompressionTestPacketSizes
				);

			uint32& TargetBytes =
				(
					ListIdx == 0 ?			DictionaryPacketBytes :
					ListIdx == 1 ?			(bDictionaryTestOverflow ? TrainerPacketBytes : DictionaryTestPacketBytes) :
					ListIdx == 2 ?			TrainerPacketBytes :
					/* ListIdx == 3 ? */	CompressionTestPacketBytes
				);


			uint8* PacketDest = new uint8[PacketSize];

			TargetPacketList.Add(PacketDest);


			FMemory::Memcpy(PacketDest, ReadBuffer, PacketSize);

			TargetSizesList.Add(PacketSize);
			TargetBytes += PacketSize;


			// Check if DictionaryTestPackets has overflowed (allow it to overflow, but just once)
			if (!bDictionaryTestOverflow && ListIdx == 1)
			{
				bDictionaryTestOverflow = DictionaryTestPacketBytes > DictionaryTestLimit;

				if (bDictionaryTestOverflow)
				{
					UE_LOG(OodleHandlerComponentLog, Log, TEXT("DictionaryTestPackets overflowed into TrainerPackets (normal)."));
				}
			}


			PacketIdx++;
		}
	}

	delete[] ReadBuffer;
	ReadBuffer = nullptr;
}

bool FOodleDictionaryGenerator::GenerateAndWriteDictionary()
{
#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX
	bool bSuccess = false;
	uint8* NewDictionaryData = new uint8[DictionarySize];


	UE_LOG(OodleHandlerComponentLog, Log, TEXT("Beginning dictionary generation..."));

	UE_LOG(OodleHandlerComponentLog, Log, TEXT("- DictionaryPackets: %i, Size: %i (~%iMB)"),
		DictionaryPackets.Num(), DictionaryPacketBytes, (DictionaryPacketBytes / (1024 * 1024)));

	UE_LOG(OodleHandlerComponentLog, Log, TEXT("- DictionaryTestPackets: %i, Size: %i (~%iMB), Overflowed: %s"),
		DictionaryTestPackets.Num(), DictionaryTestPacketBytes, (DictionaryTestPacketBytes / (1024 * 1024)),
		(bDictionaryTestOverflow ? TEXT("Yes") : TEXT("No")));

	UE_LOG(OodleHandlerComponentLog, Log, TEXT("- TrainerPackets: %i, Size: %i (~%iMB)"),
		TrainerPackets.Num(), TrainerPacketBytes, (TrainerPacketBytes / (1024 * 1024)));

	UE_LOG(OodleHandlerComponentLog, Log, TEXT("Dictionary generation may take a very long time (up to 7 mins for 1GB of packets)."));


	if (bNoTrials)
	{
		OodleNetwork1_SelectDictionaryFromPackets((void*)NewDictionaryData, DictionarySize, HashTableSize,
			(const void**)DictionaryPackets.GetData(), DictionaryPacketSizes.GetData(), DictionaryPackets.Num(),
			(const void**)DictionaryTestPackets.GetData(), DictionaryTestPacketSizes.GetData(), DictionaryTestPackets.Num());
	}
	else
	{
		OodleNetwork1_SelectDictionaryFromPackets_Trials((void*)NewDictionaryData, DictionarySize, HashTableSize,
			(const void**)DictionaryPackets.GetData(), DictionaryPacketSizes.GetData(), DictionaryPackets.Num(),
			(const void**)DictionaryTestPackets.GetData(), DictionaryTestPacketSizes.GetData(), DictionaryTestPackets.Num(),
			DictionaryTrials, TrialRandomness, TrialGenerations);
	}

	UE_LOG(OodleHandlerComponentLog, Log, TEXT("Dictionary generation complete. Beginning Oodle state training..."));


	// Begin training the initial Oodle state
	const SINTa SharedStruct_Size = OodleNetwork1_Shared_Size(HashTableSize);
	const SINTa StateStruct_Size = OodleNetwork1UDP_State_Size();
	OodleNetwork1_Shared* SharedDictionary = (OodleNetwork1_Shared*)FMemory::Malloc(SharedStruct_Size);
	OodleNetwork1UDP_State* CompressorState = (OodleNetwork1UDP_State*)FMemory::Malloc(StateStruct_Size);

	OodleNetwork1_Shared_SetWindow(SharedDictionary, HashTableSize, (void*)NewDictionaryData, (S32)DictionarySize);

	OodleNetwork1UDP_Train(CompressorState, SharedDictionary,
		(const void**)TrainerPackets.GetData(), TrainerPacketSizes.GetData(), TrainerPackets.Num());



	// Now compact the dictionary and initial state data, in preparation for writing to file
	const SINTa CompactState_Size = OodleNetwork1UDP_StateCompacted_MaxSize();
	OodleNetwork1UDP_StateCompacted* CompactCompressorState = (OodleNetwork1UDP_StateCompacted*)FMemory::Malloc(CompactState_Size);
	SINTa CompactCompressorStateBytes = OodleNetwork1UDP_State_Compact(CompactCompressorState, CompressorState);


	// Now write to the final file
	FArchive* OutArc = IFileManager::Get().CreateFileWriter(*OutputDictionaryFile);

	if (OutArc != nullptr)
	{
		FOodleDictionaryArchive OutputFile(*OutArc);

		OutputFile.SetDictionaryHeaderValues(HashTableSize);
		OutputFile.SerializeHeader();

		bSuccess = !OutputFile.IsError();

		bSuccess = bSuccess && OutputFile.SerializeOodleCompressData(OutputFile.Header.DictionaryData, NewDictionaryData, DictionarySize);

		bSuccess = bSuccess && OutputFile.SerializeOodleCompressData(OutputFile.Header.CompressorData, (uint8*)CompactCompressorState,
			CompactCompressorStateBytes);


		if (bSuccess)
		{
			UE_LOG(OodleHandlerComponentLog, Log, TEXT("Successfully processed packet captures, and wrote dictionary file."));
		}
		else
		{
			UE_LOG(OodleHandlerComponentLog, Error, TEXT("Error writing dictionary file."));
		}


		OutArc->Close();

		delete OutArc;
	}
	else
	{
		UE_LOG(OodleHandlerComponentLog, Error, TEXT("Failed to create output dictionary file '%s'"), *OutputDictionaryFile);
	}


	if (bSuccess && bCompressionTest)
	{
		UE_LOG(OodleHandlerComponentLog, Log, TEXT("Testing packet compression results..."));

		uint8* CompressedData = new uint8[MAX_OODLE_BUFFER];
		uint64 TotalUncompressed = 0;
		uint64 TotalCompressed = 0;

		for (int32 CurPacketIdx = 0; CurPacketIdx < CompressionTestPackets.Num(); CurPacketIdx++)
		{
			uint8* CurPacket = CompressionTestPackets[CurPacketIdx];
			int32 CurPacketSize = CompressionTestPacketSizes[CurPacketIdx];

			check(OodleLZ_GetCompressedBufferSizeNeeded(CurPacketSize) <= MAX_OODLE_BUFFER);

			SINTa CompressedLengthSINT = 0;

			CompressedLengthSINT = OodleNetwork1UDP_Encode(CompressorState, SharedDictionary, CurPacket, CurPacketSize,
				CompressedData);

			check(CompressedLengthSINT <= CurPacketSize);

			TotalUncompressed += CurPacketSize;
			TotalCompressed += (uint64)CompressedLengthSINT;
		}

		UE_LOG(OodleHandlerComponentLog, Log, TEXT("Compression test results:"));

		UE_LOG(OodleHandlerComponentLog, Log, TEXT("- CompressionTestPackets: %i, Size: %i (~%iMB)"),
			CompressionTestPackets.Num(), CompressionTestPacketBytes, (CompressionTestPacketBytes / (1024 * 1024)));

		UE_LOG(OodleHandlerComponentLog, Log, TEXT("- Uncompressed: %i (~%iMB), Compressed: %i (~%iMB)"), TotalUncompressed,
			(TotalUncompressed / (1024 * 1024)), TotalCompressed, (TotalCompressed / (1024 * 1024)));

		UE_LOG(OodleHandlerComponentLog, Log, TEXT("- Total Savings: %f%"),
			((float)(TotalUncompressed - TotalCompressed) * 100.f) / (float)TotalUncompressed);


		delete[] CompressedData;
	}


	// Cleanup allocated data
	delete[] NewDictionaryData;
	NewDictionaryData = nullptr;

	if (CompactCompressorState != nullptr)
	{
		FMemory::Free(CompactCompressorState);
	}

	if (CompressorState != nullptr)
	{
		FMemory::Free(CompressorState);
	}

	if (SharedDictionary != nullptr)
	{
		FMemory::Free(SharedDictionary);
	}

	CompressorState = nullptr;
	SharedDictionary = nullptr;



	// @todo #JohnB: IMPORTANT: Add a disabled-by-default fourth 'compression test' list,
	//							which also is randomly selected like above, for testing the dictionary after generation
	//							(keep it disabled normally though, to maximize packet capture usage)


	return bSuccess;
#else
	return false;
#endif
}

bool FOodleDictionaryGenerator::DebugDumpPackets()
{
	bool bSuccess = false;

	FString PacketsFile = OutputDictionaryFile + TEXT(".bin");
	FArchive* OutPackets = IFileManager::Get().CreateFileWriter(*PacketsFile);

	// Oodle example packet format:
	//		[4:ChannelNum][?:PacketData]
	//	PacketData:
	//		[4:PacketChannel][4:PacketSize][PacketSize:PacketData]
	if (OutPackets != nullptr)
	{
		uint32 DudChannelNum = 0;

		OutPackets->ByteOrderSerialize(&DudChannelNum, sizeof(uint32));

		for (int32 PacketIdx=0; PacketIdx<DictionaryPackets.Num(); PacketIdx++)
		{
			uint32 DudPacketChannel = 0;
			uint32 PacketSize = DictionaryPacketSizes[PacketIdx];

			OutPackets->ByteOrderSerialize(&DudPacketChannel, sizeof(uint32));
			OutPackets->ByteOrderSerialize(&PacketSize, sizeof(uint32));
			OutPackets->Serialize(DictionaryPackets[PacketIdx], PacketSize);
		}

		OutPackets->Close();

		delete OutPackets;

		UE_LOG(OodleHandlerComponentLog, Log, TEXT("Successfully dumped packet data to file: %s"), *PacketsFile);

		bSuccess = true;
	}

	return bSuccess;
}

void FOodleDictionaryGenerator::Cleanup()
{
	for (uint8* CurPacket : DictionaryPackets)
	{
		if (CurPacket != nullptr)
		{
			delete[] CurPacket;
		}
	}

	for (uint8* CurPacket : DictionaryTestPackets)
	{
		if (CurPacket != nullptr)
		{
			delete[] CurPacket;
		}
	}

	for (uint8* CurPacket : TrainerPackets)
	{
		if (CurPacket != nullptr)
		{
			delete[] CurPacket;
		}
	}

	for (uint8* CurPacket : CompressionTestPackets)
	{
		if (CurPacket != nullptr)
		{
			delete[] CurPacket;
		}
	}

	DictionaryPackets.Empty();
	DictionaryTestPackets.Empty();
	TrainerPackets.Empty();
	CompressionTestPackets.Empty();


	if (MergeMap.Num() > 0)
	{
		for (TMap<FArchive*, FString>::TConstIterator It(MergeMap); It; ++It)
		{
			FArchive* CurKey = It.Key();

			if (CurKey != nullptr)
			{
				delete CurKey;
			}
		}

		MergeMap.Empty();
	}
}
#endif // HAS_OODLE_SDK

#endif // !UE_BUILD_SHIPPING || OODLE_DEV_SHIPPING

#undef LOCTEXT_NAMESPACE

