/**
* @license MIT License
*/ 

#include "AILabChatGenerateProxy.h"
#include "Containers/UnrealString.h"
#include "ConvaiChatbotComponent.h"
#include "ConvaiUtils.h"
#include "Engine.h"
#include "Interfaces/IHttpRequest.h"
#include "JsonObjectConverter.h"

DEFINE_LOG_CATEGORY(AILabChatGenHttpLog);


UAILabChatGenerateProxy* UAILabChatGenerateProxy::GenerateChatResponseFromSingleQueryProxy(UConvaiChatbotComponent* ChatbotObject, FString Transcript)
{
	UAILabChatGenerateProxy* Proxy = NewObject<UAILabChatGenerateProxy>();
	Proxy->ChatbotPtr = ChatbotObject;
	Proxy->URL = "http://localhost:7861/chat/knowledge_base_chat";
	Proxy->Transcript = Transcript;
	Proxy->History = false;
	return Proxy;
}

void UAILabChatGenerateProxy::Activate()
{
	UConvaiChatbotComponent* Chatbot = ChatbotPtr.Get();

	if (!Chatbot)
	{
		UE_LOG(AILabChatGenHttpLog, Warning, TEXT("Could not get a pointer to Chatbot!"));
		Failed();
		return;
	}

	FHttpModule* mHttp = &FHttpModule::Get();
	if (!mHttp)
	{
		UE_LOG(AILabChatGenHttpLog, Warning, TEXT("Could not get a pointer to http module!"));
		Failed();
		return;
	}

	// Create the request
	FHttpRequestRef rRequest = mHttp->CreateRequest();
	rRequest->OnProcessRequestComplete().BindUObject(this, &UAILabChatGenerateProxy::onHttpRequestComplete);

	// Set request fields
	rRequest->SetURL(URL);
	rRequest->SetVerb("POST");
	rRequest->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	rRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	// prepare json data
	FString sJsonString;
	TSharedRef<TJsonWriter<TCHAR>> rJsonWriter = TJsonWriterFactory<TCHAR>::Create(&sJsonString);
	rJsonWriter->WriteObjectStart();
	rJsonWriter->WriteValue("query", (Transcript));
	rJsonWriter->WriteValue("knowledge_base_name", TEXT("eie_corpus_1"));
	rJsonWriter->WriteValue("top_k", 5);
	rJsonWriter->WriteValue("score_threshold", 1);
	rJsonWriter->WriteArrayStart("history");
	rJsonWriter->WriteArrayEnd();
	rJsonWriter->WriteValue("stream", false);
	rJsonWriter->WriteValue("model_name", TEXT("chatglm3-6b"));
	rJsonWriter->WriteValue("temperature", 0.75);
	rJsonWriter->WriteValue("max_tokens", 0);    
	rJsonWriter->WriteValue("prompt_name", TEXT("WangXiaoniao"));
	rJsonWriter->WriteObjectEnd();
	rJsonWriter->Close();
	// Insert the content into the request
	rRequest->SetContentAsString(sJsonString);

	UE_LOG(AILabChatGenHttpLog, Log, TEXT("ChatGenerate: %s"), *FString(sJsonString));

	// Initiate the request
	//if (!rRequest->ProcessRequest()) Failed();

	HttpRequestPtr = rRequest;
}

void UAILabChatGenerateProxy::onHttpRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool WasSuccessful)
{
	if (!WasSuccessful || ResponsePtr->GetResponseCode() < 200 || ResponsePtr->GetResponseCode() > 299)
	{
		UE_LOG(AILabChatGenHttpLog, Warning, TEXT("HTTP request Failed with code %d, and with response:%s"),ResponsePtr->GetResponseCode(), *ResponsePtr->GetContentAsString());
		Failed();
		return;
	}

    auto fGatherError = [](const FString & ResponseStr)-> FString {
        TSharedRef<TJsonReader<>> rReader = TJsonReaderFactory<>::Create(ResponseStr);
	    TSharedPtr<FJsonValue> pJsonParsed;
        if (FJsonSerializer::Deserialize(rReader, pJsonParsed) && pJsonParsed.IsValid() && pJsonParsed->Type == EJson::Object)
	    {

			UE_LOG(AILabChatGenHttpLog, Log, TEXT("Response: %s"), *FString(ResponseStr));
		    TSharedPtr<FJsonObject> pFrameObject = pJsonParsed->AsObject();
			return pFrameObject->GetStringField("answer");
        }
        else {
			UE_LOG(AILabChatGenHttpLog, Warning, TEXT("Response Failed to decode: %s"), *FString(ResponseStr));
			return "";
            //return ResponseStr.Mid(1, ResponseStr.Len() -2);
        }
    };
    this->GeneratedResponse = fGatherError(*ResponsePtr->GetContentAsString());

	if (this->GeneratedResponse == "")
	{
		UE_LOG(AILabChatGenHttpLog, Warning, TEXT("Failed to decode response content to a text response"));
		Failed();
		return;
	}
	Success();
}

void UAILabChatGenerateProxy::Failed()
{
	OnFailure.Broadcast(GeneratedResponse);
	Finish();
}

void UAILabChatGenerateProxy::Success()
{
	OnSuccess.Broadcast(GeneratedResponse);

	UConvaiChatbotComponent* pChatbot = ChatbotPtr.Get();
	if(!pChatbot)
	{
		UE_LOG(AILabChatGenHttpLog, Warning, TEXT("Could not get a pointer to Chatbot!"));
		Failed();
		return;
	}
	pChatbot->onResponseDataReceived(this->GeneratedResponse, TArray<uint8>(), 0, true); // TODO
	Finish();
}

void UAILabChatGenerateProxy::Finish()
{
	ChatbotPtr = nullptr;
	if(HttpRequestPtr != nullptr)
	{
		HttpRequestPtr->OnProcessRequestComplete().Unbind();
	}
	HttpRequestPtr = nullptr;
}

void UAILabChatGenerateProxy::BeginDestroy()
{
	UE_LOG(AILabChatGenHttpLog, Log, TEXT("Begin Destroy UAILabChatGenerateProxy"));
	Finish();
	Super::BeginDestroy();
}

void UAILabChatGenerateProxy::FinishDestroy()
{
	UE_LOG(AILabChatGenHttpLog, Log, TEXT("End Destroy UAILabChatGenerateProxy"));
	Super::FinishDestroy();
}

void UAILabChatGenerateProxy::UnbindChatbotComponent()
{
	Finish();
}