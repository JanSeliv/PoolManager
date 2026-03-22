// Copyright (c) Yevhenii Selivanov

#include "K2Node_TakeFromPoolArray.h"

// Pool Manager
#include "PoolManagerSubsystem.h"

// UE
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "Kismet/KismetArrayLibrary.h"
#include "KismetCompiler.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_TakeFromPoolArray)

#define LOCTEXT_NAMESPACE "K2Node_TakeFromPoolArray"

UEdGraphNode::FCreatePinParams UK2Node_TakeFromPoolArray::GetReturnValuePinParams() const
{
	FCreatePinParams PinParams;
	PinParams.ContainerType = EPinContainerType::Array;
	return PinParams;
}

FName UK2Node_TakeFromPoolArray::GetNativeFunctionName() const
{
	return GET_FUNCTION_NAME_CHECKED(UPoolManagerSubsystem, BPTakeFromPoolArray);
}

bool UK2Node_TakeFromPoolArray::PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UK2Node_CallFunction& CallTakeFromPoolNode)
{
	bool bIsErrorFree = Super::PostExpandNode(CompilerContext, SourceGraph, CallTakeFromPoolNode);

	// connect to Amount input
	UEdGraphPin* CallAmountPin = CallTakeFromPoolNode.FindPin(AmountInputName);
	UEdGraphPin* AmountPin = FindPin(AmountInputName);
	if (AmountPin && CallAmountPin)
	{
		if (AmountPin->LinkedTo.Num() > 0)
		{
			bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*AmountPin, *CallAmountPin).CanSafeConnect();
		}
		else
		{
			// Copy literal value
			CallAmountPin->DefaultValue = AmountPin->DefaultValue;
			CallAmountPin->DefaultObject = AmountPin->DefaultObject;
		}
	}
	else
	{
		bIsErrorFree = false;
	}

	return bIsErrorFree;
}

// Validates the array result by checking if index 0 is valid, which means the array is not empty
UEdGraphPin* UK2Node_TakeFromPoolArray::SpawnIsResultValidPin(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UEdGraphPin* ResultVariablePin)
{
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

	UK2Node_CallFunction* IsValidIndexNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, &SourceGraph);
	IsValidIndexNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_IsValidIndex), UKismetArrayLibrary::StaticClass());
	IsValidIndexNode->AllocateDefaultPins();

	// Adjust TargetArray pin type from default TArray<int32> to TArray<UObject*> to match the result variable
	UEdGraphPin* ArrayInputPin = IsValidIndexNode->FindPinChecked(TEXT("TargetArray"));
	ArrayInputPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
	ArrayInputPin->PinType.PinSubCategoryObject = UObject::StaticClass();
	ArrayInputPin->PinType.ContainerType = EPinContainerType::Array;
	Schema->TryCreateConnection(ResultVariablePin, ArrayInputPin);

	// IndexToTest defaults to 0: valid only when the array has at least one element
	return IsValidIndexNode->GetReturnValuePin();
}

// Is overridden to allocate additional default pins for a given node
void UK2Node_TakeFromPoolArray::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	UEdGraphPin* CompletedPin = FindPin(UEdGraphSchema_K2::PN_Completed);
	if (CompletedPin)
	{
		CompletedPin->PinToolTip = LOCTEXT("CompletedArrayTooltip", "Called when all requested objects are taken from pool and the array is not empty").ToString();
	}

	UEdGraphPin* FailedPin = FindPin(GetFailedPinName());
	if (FailedPin)
	{
		FailedPin->PinToolTip = LOCTEXT("FailedArrayTooltip", "Called when the pool returned an empty array, e.g. the object class is invalid or Amount is zero").ToString();
	}

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Int, AmountInputName);
}

#undef LOCTEXT_NAMESPACE
