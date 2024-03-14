/**
* @license MIT License
*/ 
#pragma once

#include "Containers/UnrealString.h"
#include "ConvaiChatbotComponent.h"
#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "Http.h"
#include "AILabChatGenerateProxy.generated.h"

//http log
DECLARE_LOG_CATEGORY_EXTERN(AILabChatGenHttpLog, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChatGenHttpResponseCallbackSignature, FString, Response);

/**
 * 
 */
UCLASS()
class UAILabChatGenerateProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_BODY()


	// Called when there is a successful http response
	UPROPERTY(BlueprintAssignable)
	FChatGenHttpResponseCallbackSignature OnSuccess;

	// Called when there is an unsuccessful http response
	UPROPERTY(BlueprintAssignable)
	FChatGenHttpResponseCallbackSignature OnFailure;

	virtual void BeginDestroy() override;
	virtual void FinishDestroy() override;

	void onHttpRequestComplete(FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool WasSuccessful);

	void Failed();
	void Success();
	void Finish();

	FString URL;
    FString Transcript;
	FString GeneratedResponse = "";
	bool History;

	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> HttpRequestPtr;
	TWeakObjectPtr<UConvaiChatbotComponent> ChatbotPtr;

	public:

	/**
	 *    This should not be used in the blueprint, since currently UConvaiChatbotComponent will call it
	 *    @param URL				The url of the API endpoint.
	 *    @param API_key			The API key issued from the website
	 *    @param SoundWave			SoundWave that is to be transfered to text
	 */
	UFUNCTION(BlueprintCallable, meta = (), Category = "AILab|Http")
	static UAILabChatGenerateProxy* GenerateChatResponseFromSingleQueryProxy(UConvaiChatbotComponent* ChatbotObject, FString Transcript);

	virtual void Activate() override;
	void UnbindChatbotComponent();
};
