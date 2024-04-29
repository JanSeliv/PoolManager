// Copyright (c) Yevhenii Selivanov

#include "K2Node_TakeFromPool.h"
//---
#include "PoolManagerSubsystem.h"
//---
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_TemporaryVariable.h"
#include "KismetCompiler.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_TakeFromPool)

#define LOCTEXT_NAMESPACE "K2Node_TakeFromPool"

void UK2Node_TakeFromPool::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Completed);

	UEdGraphPin* TargetPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UPoolManagerSubsystem::StaticClass(), UEdGraphSchema_K2::PN_Self);
	checkf(TargetPin, TEXT("ERROR: [%i] %s:\n'TargetPin' is null!"), __LINE__, *FString(__FUNCTION__));
	TargetPin->PinFriendlyName = LOCTEXT("Target", "Target");
	TargetPin->bDefaultValueIsIgnored = true;

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Class, UObject::StaticClass(), ClassInputName);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FTransform>::Get(), TransformInputName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, UObject::StaticClass(), ObjectOutputName);
}

void UK2Node_TakeFromPool::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	UEdGraphPin* OldThenPin = nullptr;
	const UEdGraphPin* OldCompletedPin = nullptr;

	for (UEdGraphPin* CurrentPin : OldPins)
	{
		if (CurrentPin->PinName == UEdGraphSchema_K2::PN_Then)
		{
			OldThenPin = CurrentPin;
		}
		else if (CurrentPin->PinName == UEdGraphSchema_K2::PN_Completed)
		{
			OldCompletedPin = CurrentPin;
		}
	}

	if (OldThenPin && !OldCompletedPin)
	{
		// This is an old node from when Completed was called then, rename the node to Completed and allow normal rewire to take place
		OldThenPin->PinName = UEdGraphSchema_K2::PN_Completed;
	}
}

void UK2Node_TakeFromPool::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(Schema);
	bool bIsErrorFree = true;

	// Sequence node, defaults to two output pins
	UK2Node_ExecutionSequence* SequenceNode = CompilerContext.SpawnIntermediateNode<UK2Node_ExecutionSequence>(this, SourceGraph);
	SequenceNode->AllocateDefaultPins();

	// connect to input exe
	{
		UEdGraphPin* InputExePin = GetExecPin();
		UEdGraphPin* SequenceInputExePin = SequenceNode->GetExecPin();
		bIsErrorFree &= InputExePin && SequenceInputExePin && CompilerContext.MovePinLinksToIntermediate(*InputExePin, *SequenceInputExePin).CanSafeConnect();
	}

	// Create TakeFromPool function call
	UK2Node_CallFunction* CallTakeFromPoolNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallTakeFromPoolNode->FunctionReference.SetExternalMember(NativeFunctionName(), UPoolManagerSubsystem::StaticClass());
	CallTakeFromPoolNode->AllocateDefaultPins();

	// connect load to first sequence pin
	{
		UEdGraphPin* CallFunctionInputExePin = CallTakeFromPoolNode->GetExecPin();
		UEdGraphPin* SequenceFirstExePin = SequenceNode->GetThenPinGivenIndex(0);
		bIsErrorFree &= SequenceFirstExePin && CallFunctionInputExePin && Schema->TryCreateConnection(CallFunctionInputExePin, SequenceFirstExePin);
	}

	// Create Local Variable
	UK2Node_TemporaryVariable* TempVarOutput = CompilerContext.SpawnInternalVariable(this, UEdGraphSchema_K2::PC_Object, NAME_None, UObject::StaticClass());

	// Create assign node
	UK2Node_AssignmentStatement* AssignNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	AssignNode->AllocateDefaultPins();

	UEdGraphPin* LoadedObjectVariablePin = TempVarOutput->GetVariablePin();

	// connect local variable to assign node
	{
		UEdGraphPin* AssignLHSPPin = AssignNode->GetVariablePin();
		bIsErrorFree &= AssignLHSPPin && LoadedObjectVariablePin && Schema->TryCreateConnection(AssignLHSPPin, LoadedObjectVariablePin);
	}

	// connect local variable to output
	{
		UEdGraphPin* OutputObjectPinPin = FindPin(ObjectOutputName);
		bIsErrorFree &= LoadedObjectVariablePin && OutputObjectPinPin && CompilerContext.MovePinLinksToIntermediate(*OutputObjectPinPin, *LoadedObjectVariablePin).CanSafeConnect();
	}

	// connect to Pool Manager input from self
	UEdGraphPin* CallPoolManagerPin = CallTakeFromPoolNode->FindPin(UEdGraphSchema_K2::PN_Self);
	{
		UEdGraphPin* PoolManagerPin = FindPin(UEdGraphSchema_K2::PN_Self);
		ensure(CallPoolManagerPin);

		if (PoolManagerPin && CallPoolManagerPin)
		{
			if (PoolManagerPin->LinkedTo.Num() > 0)
			{
				bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*PoolManagerPin, *CallPoolManagerPin).CanSafeConnect();
			}
			else
			{
				// Copy literal value
				CallPoolManagerPin->DefaultValue = PoolManagerPin->DefaultValue;
			}
		}
		else
		{
			bIsErrorFree = false;
		}
	}

	// connect to class input
	UEdGraphPin* CallClassPin = CallTakeFromPoolNode->FindPin(ClassInputName);
	{
		UEdGraphPin* ClassPin = FindPin(ClassInputName);
		ensure(CallClassPin);

		if (ClassPin && CallClassPin)
		{
			if (ClassPin->LinkedTo.Num() > 0)
			{
				bIsErrorFree &= CompilerContext.MovePinLinksToIntermediate(*ClassPin, *CallClassPin).CanSafeConnect();
			}
			else
			{
				// Ensure the default value is copied even if not linked
				CallClassPin->DefaultValue = ClassPin->DefaultValue;
				CallClassPin->DefaultObject = ClassPin->DefaultObject;
			}
		}
		else
		{
			bIsErrorFree = false;
		}
	}

	// connect to transform input
	UEdGraphPin* CallTransformPin = CallTakeFromPoolNode->FindPin(TransformInputName);
	{
		UEdGraphPin* TransformPin = FindPin(TransformInputName);
		ensure(CallTransformPin);

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
			}
		}
		else
		{
			bIsErrorFree = false;
		}
	}

	// Create Completed delegate parameter
	const FName DelegateCompletedParamName(TEXT("Completed"));
	UK2Node_CustomEvent* CompletedEventNode = CompilerContext.SpawnIntermediateNode<UK2Node_CustomEvent>(this, SourceGraph);
	CompletedEventNode->CustomFunctionName = *FString::Printf(TEXT("Completed_%s"), *CompilerContext.GetGuid(this));
	CompletedEventNode->AllocateDefaultPins();
	{
		UFunction* TakeFromPoolFunction = CallTakeFromPoolNode->GetTargetFunction();
		FDelegateProperty* CompletedDelegateProperty = TakeFromPoolFunction ? FindFProperty<FDelegateProperty>(TakeFromPoolFunction, DelegateCompletedParamName) : nullptr;
		UFunction* CompletedSignature = CompletedDelegateProperty ? CompletedDelegateProperty->SignatureFunction : nullptr;
		ensure(CompletedSignature);
		for (TFieldIterator<FProperty> PropIt(CompletedSignature); PropIt && PropIt->PropertyFlags & CPF_Parm; ++PropIt)
		{
			const FProperty* Param = *PropIt;
			if (!Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				FEdGraphPinType PinType;
				bIsErrorFree &= Schema->ConvertPropertyToPinType(Param, /*out*/ PinType);
				bIsErrorFree &= nullptr != CompletedEventNode->CreateUserDefinedPin(Param->GetFName(), PinType, EGPD_Output);
			}
		}
	}

	// connect delegate
	{
		UEdGraphPin* CallFunctionDelegatePin = CallTakeFromPoolNode->FindPin(DelegateCompletedParamName);
		ensure(CallFunctionDelegatePin);
		UEdGraphPin* EventDelegatePin = CompletedEventNode->FindPin(UK2Node_CustomEvent::DelegateOutputName);
		bIsErrorFree &= CallFunctionDelegatePin && EventDelegatePin && Schema->TryCreateConnection(CallFunctionDelegatePin, EventDelegatePin);
	}

	// connect loaded object from event to assign
	{
		UEdGraphPin* LoadedAssetEventPin = CompletedEventNode->FindPin(ObjectOutputName);
		ensure(LoadedAssetEventPin);
		UEdGraphPin* AssignRHSPPin = AssignNode->GetValuePin();
		bIsErrorFree &= AssignRHSPPin && LoadedAssetEventPin && Schema->TryCreateConnection(LoadedAssetEventPin, AssignRHSPPin);
	}

	// connect assign exec input to event output
	{
		UEdGraphPin* CompletedEventOutputPin = CompletedEventNode->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* AssignInputExePin = AssignNode->GetExecPin();
		bIsErrorFree &= AssignInputExePin && CompletedEventOutputPin && Schema->TryCreateConnection(AssignInputExePin, CompletedEventOutputPin);
	}

	// connect assign exec output to output
	{
		UEdGraphPin* OutputCompletedPin = FindPin(UEdGraphSchema_K2::PN_Completed);
		UEdGraphPin* AssignOutputExePin = AssignNode->GetThenPin();
		bIsErrorFree &= OutputCompletedPin && AssignOutputExePin && CompilerContext.MovePinLinksToIntermediate(*OutputCompletedPin, *AssignOutputExePin).CanSafeConnect();
	}

	if (!bIsErrorFree)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InternalConnectionError", "K2Node_TakeFromPool: Internal connection error. @@").ToString(), this);
	}

	BreakAllNodeLinks();
}

FText UK2Node_TakeFromPool::GetTooltipText() const
{
	return FText(LOCTEXT("UK2Node_TakeFromPoolGetTooltipText", 
		"Get the object from a pool by specified class, where output is async that returns the object when is ready. "
		"It creates new object if there no free objects contained in pool or does not exist any. "
		"If any is found and free, activates and returns object from the pool, otherwise async spawns new one next frames and register in the pool. "
		"'SpawnObjectsPerFrame' affects how fast new objects are created, it can be changed in 'Project Settings' -> 'Plugins' -> 'Pool Manager'. "
		"Is custom blueprint node implemented in K2Node_TakeFromPool.h, so can't be overridden and accessible on graph (not inside functions). "
		"Node's output will not work in for/while loop in blueprints because of Completed delegate: to achieve loop connect output exec to input exec."
	));
}

FText UK2Node_TakeFromPool::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText(LOCTEXT("UK2Node_TakeFromPoolGetNodeTitle", "Take From Pool"));
}

bool UK2Node_TakeFromPool::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	bool bIsCompatible = false;
	// Can only place events in ubergraphs and macros (other code will help prevent macros with latents from ending up in functions), and basicasync task creates an event node:
	const EGraphType GraphType = TargetGraph->GetSchema()->GetGraphType(TargetGraph);
	if (GraphType == GT_Ubergraph || GraphType == GT_Macro)
	{
		bIsCompatible = true;
	}
	return bIsCompatible && Super::IsCompatibleWithGraph(TargetGraph);
}

FName UK2Node_TakeFromPool::GetCornerIcon() const
{
	return TEXT("Graph.Latent.LatentIcon");
}

void UK2Node_TakeFromPool::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// actions get registered under specific object-keys; the idea is that 
	// actions might have to be updated (or deleted) if their object-key is  
	// mutated (or removed)... here we use the node's class (so if the node 
	// type disappears, then the action should go with it)
	const UClass* ActionKey = GetClass();
	// to keep from needlessly instantiating a UBlueprintNodeSpawner, first   
	// check to make sure that the registrar is looking for actions of this type
	// (could be regenerating actions for a specific asset, and therefore the 
	// registrar would only accept actions corresponding to that asset)
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_TakeFromPool::GetMenuCategory() const
{
	return FText(LOCTEXT("UK2Node_TakeFromPoolGetMenuCategory", "Pool Manager"));
}

FName UK2Node_TakeFromPool::NativeFunctionName() const
{
	return GET_FUNCTION_NAME_CHECKED(UPoolManagerSubsystem, BPTakeFromPool);
}

#undef LOCTEXT_NAMESPACE
