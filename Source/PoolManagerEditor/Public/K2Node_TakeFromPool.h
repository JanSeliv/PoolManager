// Copyright (c) Yevhenii Selivanov

#pragma once

#include "K2Node.h"
//---
#include "K2Node_TakeFromPool.generated.h"

UCLASS(MinimalAPI)
class UK2Node_TakeFromPool : public UK2Node
{
	GENERATED_BODY()

public:
	static inline const FName ClassInputName = TEXT("ObjectClass");
	static inline const FName TransformInputName = TEXT("Transform");
	static inline const FName ObjectOutputName = TEXT("Object");

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

protected:
	virtual FName NativeFunctionName() const;
};
