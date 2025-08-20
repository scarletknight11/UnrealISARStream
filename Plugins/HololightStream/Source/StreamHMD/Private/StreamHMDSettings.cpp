/*
 * Copyright 2025 Holo-Light GmbH. All Rights Reserved.
 */

#include "StreamHMDSettings.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Misc/MessageDialog.h"
#include "UnrealEdMisc.h"
#endif

#include "Algo/Find.h"

//JSON
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UStreamHMDSettings::UStreamHMDSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!LoadSettingsFromConfig())
	{
		Port = 9999;
		MinPort = 50100;
		MaxPort = 50100;
		EncoderBandwidth = -1;
		bEnableStatsLogging = false;
	}

	SaveConfig(CPF_Config, *GetDefaultConfigFilename());
	UpdateDefaultConfigFile();
}

#if WITH_EDITOR
void UStreamHMDSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Get the name of the property that was changed
	FName propertyName = PropertyChangedEvent.GetPropertyName();
	if (propertyName == GET_MEMBER_NAME_CHECKED(UStreamHMDSettings, Port) ||
		propertyName == GET_MEMBER_NAME_CHECKED(UStreamHMDSettings, MinPort) ||
		propertyName == GET_MEMBER_NAME_CHECKED(UStreamHMDSettings, MaxPort) ||
		propertyName == GET_MEMBER_NAME_CHECKED(UStreamHMDSettings, EncoderBandwidth) ||
		propertyName == GET_MEMBER_NAME_CHECKED(UStreamHMDSettings, bEnableStatsLogging))
	{
		// Save changes to the JSON config file
		SaveSettingsToConfig();
	}
}


void UStreamHMDSettings::SaveSettingsToConfig()
{
	FString configFilePath = FPaths::ProjectPluginsDir() / TEXT("HololightStream/Resources/remoting-config.cfg");
	TSharedPtr<FJsonObject> jsonObject = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> iceServersArray;
	TSharedPtr<FJsonObject> iceServerObject = MakeShareable(new FJsonObject);
	iceServerObject->SetStringField(TEXT("url"), TEXT("stun:stun.l.google.com:19302"));
	iceServerObject->SetStringField(TEXT("username"), TEXT(""));
	iceServerObject->SetStringField(TEXT("credential"), TEXT(""));
	iceServersArray.Add(MakeShareable(new FJsonValueObject(iceServerObject)));
	jsonObject->SetArrayField(TEXT("ice-servers"), iceServersArray);

	// Add "diagnostic-options" array
	TArray<TSharedPtr<FJsonValue>> diagnosticsArray;
	TSharedPtr<FJsonValue> diagnosticsVal = MakeShareable(new FJsonValueString("stats-collector"));
	if (bEnableStatsLogging)
	{
		diagnosticsArray.Add(diagnosticsVal);
	}
	jsonObject->SetArrayField(TEXT("diagnostic-options"), diagnosticsArray);

	// Add "signaling" object
	TSharedPtr<FJsonObject> signalingObject = MakeShareable(new FJsonObject);
	signalingObject->SetStringField(TEXT("ip"), TEXT("0.0.0.0"));
	signalingObject->SetNumberField(TEXT("port"), Port);
	jsonObject->SetObjectField(TEXT("signaling"), signalingObject);

	if (EncoderBandwidth == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Encoder Bandwidth cannot be 0, defaulting to -1.")));
		EncoderBandwidth = -1;
	}

	// Add "encoder-bandwidth-kbps" field
	jsonObject->SetNumberField(TEXT("encoder-bandwidth-kbps"), EncoderBandwidth);

	if (MinPort > MaxPort)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Minimum Port cannot be greater than Maximum Port, defaulting both to Maximum Port value.")));
		MinPort = MaxPort;
	}

	// Add "port-range" object
	TSharedPtr<FJsonObject> portRangeObject = MakeShareable(new FJsonObject);
	portRangeObject->SetNumberField(TEXT("min-port"), MinPort);
	portRangeObject->SetNumberField(TEXT("max-port"), MaxPort);
	jsonObject->SetObjectField(TEXT("port-range"), portRangeObject);

	// Serialize JSON object to a string
	FString outputString;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&outputString);
	if (FJsonSerializer::Serialize(jsonObject.ToSharedRef(), writer))
	{
		// Save the string to a file
		if (FFileHelper::SaveStringToFile(outputString, *configFilePath))
		{
			UE_LOG(LogTemp, Log, TEXT("Settings saved successfully to %s"), *configFilePath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to save settings to %s"), *configFilePath);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to serialize settings to JSON"));
	}
}
#endif

bool UStreamHMDSettings::LoadSettingsFromConfig()
{
	FString jsonFilePath = FPaths::ProjectPluginsDir() / TEXT("HololightStream/Resources/remoting-config.cfg");
	FString jsonString;
	bool ret = true;
	if (FFileHelper::LoadFileToString(jsonString, *jsonFilePath))
	{
		TSharedPtr<FJsonObject> jsonObject;
		TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonString);

		if (FJsonSerializer::Deserialize(reader, jsonObject) && jsonObject.IsValid())
		{
			if (jsonObject->HasTypedField<EJson::Object>(TEXT("signaling")))
			{
				TSharedPtr<FJsonObject> signalingObject = jsonObject->GetObjectField(TEXT("signaling"));
				Port = signalingObject->GetIntegerField(TEXT("port"));
			}

			if (jsonObject->HasTypedField<EJson::Object>(TEXT("port-range")))
			{
				TSharedPtr<FJsonObject> portRangeObject = jsonObject->GetObjectField(TEXT("port-range"));
				MinPort = portRangeObject->GetIntegerField(TEXT("min-port"));
				MaxPort = portRangeObject->GetIntegerField(TEXT("max-port"));
			}
			const TArray<TSharedPtr<FJsonValue>>* diagnosticsArray;
			if (jsonObject->TryGetArrayField(TEXT("diagnostic-options"), diagnosticsArray))
			{
				bEnableStatsLogging = false;
				if (diagnosticsArray->Num() > 0)
				{
					auto* statsObj = Algo::FindByPredicate(*diagnosticsArray,
														   [](const TSharedPtr<FJsonValue>& diagnosticsObj)
														   {
															   return diagnosticsObj->AsString() == TEXT(
																   "stats-collector");
														   });
					bEnableStatsLogging = (statsObj != nullptr);
				}
			}
			EncoderBandwidth = jsonObject->GetIntegerField(TEXT("encoder-bandwidth-kbps"));
		}
	}
	else
	{
		ret = false;
	}
	return ret;
}
