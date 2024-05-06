// Copyright (c) Yevhenii Selivanov

#include "K2Node_TakeFromPool.h"
//---
#include "PoolManagerSubsystem.h"
//---
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_TakeFromPool)

FName UK2Node_TakeFromPool::GetNativeFunctionName() const
{
	return GET_FUNCTION_NAME_CHECKED(UPoolManagerSubsystem, BPTakeFromPool);
}

bool UK2Node_TakeFromPool::PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UK2Node_CallFunction& CallTakeFromPoolNode)
{
	bool bIsErrorFree = Super::PostExpandNode(CompilerContext, SourceGraph, CallTakeFromPoolNode);

	// connect to transform input
	UEdGraphPin* CallTransformPin = CallTakeFromPoolNode.FindPin(TransformInputName);
	UEdGraphPin* TransformPin = FindPin(TransformInputName);
	if (TransformPin && CallTransformPin)
	{
		if (TransformPin->LinkedTo.Num() > 0)
		{
			bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*TransformPin, *CallTransformPin).CanSafeConnect();
		}
		else
		{
			// Copy literal value
			CallTransformPin->DefaultValue = TransformPin->DefaultValue;
			CallTransformPin->DefaultObject = TransformPin->DefaultObject;
		}
	}
	else
	{
		bIsErrorFree = false;
	}

	return bIsErrorFree;
}

// Is overridden to allocate additional default pins for a given node
void UK2Node_TakeFromPool::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FTransform>::Get(), TransformInputName);
}
