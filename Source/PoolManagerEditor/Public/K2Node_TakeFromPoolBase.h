// Copyright (c) Yevhenii Selivanov

#pragma once

// UE
#include "K2Node.h"

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
	virtual FORCEINLINE FName GetReturnValuePinName() const { return TEXT("Output"); }
	virtual FORCEINLINE FName GetClassInputPinName() const { return TEXT("ObjectClass"); }
	virtual FORCEINLINE FName GetCompletedPinName() const { return TEXT("Completed"); }
	virtual FORCEINLINE FName GetFailedPinName() const { return TEXT("Failed"); }
	virtual FORCEINLINE FName GetPriorityPinName() const { return TEXT("Priority"); }

	/** Base method to get output pin params. */
	virtual FCreatePinParams GetReturnValuePinParams() const { return FCreatePinParams(); }

	/** Base method required to be overridden to get the name of the function defined in the native class. */
	virtual FName GetNativeFunctionName() const PURE_VIRTUAL(UK2Node_TakeFromPoolBase::NativeFunctionName, return NAME_None;);

	/** Base method to connect additional pins created in child classes. */
	virtual bool PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, class UK2Node_CallFunction& CallTakeFromPoolNode);

	/** Returns the selected object class from the class input pin, or nullptr if not set */
	UClass* GetSelectedClass(const TArray<UEdGraphPin*>* InPinsToSearch = nullptr) const;

	/** Updates the output pin type to match the selected class */
	void OnClassPinChanged() const;

	/** Spawns validation nodes and returns the bool output pin to branch on, base checks IsValid for single objects */
	virtual UEdGraphPin* SpawnIsResultValidPin(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UEdGraphPin* ResultVariablePin);

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual FString GetPinMetaData(FName InPinName, FName InKey) override;
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
