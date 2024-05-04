// Copyright (c) Yevhenii Selivanov

#include "K2Node_TakeFromPoolArray.h"
//---
#include "PoolManagerSubsystem.h"
//---
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_TakeFromPoolArray)

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

bool UK2Node_TakeFromPoolArray::PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, ::UK2Node_CallFunction& CallTakeFromPoolNode)
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

// Is overridden to allocate additional default pins for a given node
void UK2Node_TakeFromPoolArray::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Int, AmountInputName);
}
