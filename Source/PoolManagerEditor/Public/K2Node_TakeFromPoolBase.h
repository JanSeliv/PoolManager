// Copyright (c) Yevhenii Selivanov

#pragma once

#include "K2Node.h"
//---
#include "K2Node_TakeFromPoolBase.generated.h"

/**
 * Base class for TakeFromPool blueprint nodes.
 */
UCLASS(Abstract)
class POOLMANAGEREDITOR_API UK2Node_TakeFromPoolBase : public UK2Node
{
	GENERATED_BODY()

public:
	/** Base methods with Inputs and outputs names */
	virtual FORCEINLINE FName GetReturnValuePinName() { return TEXT("Output"); }
	virtual FORCEINLINE FName GetClassInputPinName() { return TEXT("ObjectClass"); }
	virtual FORCEINLINE FName GetCompletedPinName() { return TEXT("Completed"); }

	/** Base method to get output pin params. */
	virtual FCreatePinParams GetReturnValuePinParams() const { return FCreatePinParams(); }

	/** Base method required to be overridden to get the name of the function defined in the native class. */
	virtual FName GetNativeFunctionName() const PURE_VIRTUAL(UK2Node_TakeFromPoolBase::NativeFunctionName, return NAME_None;);

	/** Base method to connect additional pins created in child classes. */
	virtual bool PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, class UK2Node_CallFunction& CallTakeFromPoolNode) { return true; }

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual bool IsNodeSafeToIgnore() const override { return true; }
	virtual bool IsNodePure() const override { return false; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual FName GetCornerIcon() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	// End of UK2Node interface
};
