// Copyright (c) Yevhenii Selivanov

#pragma once

#include "K2Node_TakeFromPoolBase.h"
//---
#include "K2Node_TakeFromPool.generated.h"

/**
 * Represents TakeFromPool blueprint node.
 */
UCLASS()
class POOLMANAGEREDITOR_API UK2Node_TakeFromPool : public UK2Node_TakeFromPoolBase
{
	GENERATED_BODY()

public:
	static inline const FName TransformInputName = TEXT("Transform");

	// UK2Node_TakeFromPoolBase
	virtual FORCEINLINE FName GetReturnValuePinName() override { return TEXT("Object"); }
	virtual FName GetNativeFunctionName() const override;
	virtual bool PostExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph& SourceGraph, UK2Node_CallFunction& CallTakeFromPoolNode) override;
	// end of UK2Node_TakeFromPoolBase

	/** Is overridden to allocate additional default pins for a given node. */
	virtual void AllocateDefaultPins() override;
};
