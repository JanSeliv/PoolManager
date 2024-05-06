// Copyright (c) Yevhenii Selivanov

#include "Factories/PoolFactory_UserWidget.h"
//---
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(PoolFactory_UserWidget)

// Is overridden to handle UserWidget-inherited classes
const UClass* UPoolFactory_UserWidget::GetObjectClass_Implementation() const
{
	return UUserWidget::StaticClass();
}

// Is overridden to create the User Widget using its engine's 'Create Widget' method
UObject* UPoolFactory_UserWidget::SpawnNow_Implementation(const FSpawnRequest& Request)
{
	// Super is not called to create it properly with own logic

	const UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	checkf(PlayerController, TEXT("ERROR: [%i] %hs:\n'PlayerController' is null!"), __LINE__, __FUNCTION__);

	return CreateWidget(PlayerController, Request.GetClassChecked<UUserWidget>());
}

// Is overridden to destroy handled User Widget using its engine's RemoveFromParent method
void UPoolFactory_UserWidget::Destroy_Implementation(UObject* Object)
{
	// Super is not called to destroy it with own logic

	UUserWidget* ParentWidget = Cast<UUserWidget>(Object);
	if (!IsValid(ParentWidget))
	{
		return;
	}

	// Get an array of all child widgets
	TArray<UWidget*> ChildWidgets;
	const UWidgetTree* WidgetTree = ParentWidget->WidgetTree;
	WidgetTree->GetAllWidgets(ChildWidgets);

	// Iterate through the child widgets
	for (UWidget* ChildWidgetIt : ChildWidgets)
	{
		UUserWidget* ChildUserWidget = Cast<UUserWidget>(ChildWidgetIt);
		const UWidgetTree* WidgetTreeIt = ChildUserWidget ? ChildUserWidget->WidgetTree : nullptr;
		const bool bHasChildWidgets = WidgetTreeIt && WidgetTreeIt->RootWidget;

		if (bHasChildWidgets)
		{
			// If the child widget has its own child widgets, recursively remove and destroy them
			Destroy(ChildUserWidget);
		}
	}

	// Hide widget to let last chance react on visibility change
	ParentWidget->SetVisibility(ESlateVisibility::Collapsed);

	// Remove the child widget from the viewport
	ParentWidget->RemoveFromParent();

	// RemoveFromParent() does not completely destroy widget, so schedule the child widget for destruction
	ParentWidget->ConditionalBeginDestroy();
}

// Is overridden to change visibility according new state
void UPoolFactory_UserWidget::OnChangedStateInPool_Implementation(EPoolObjectState NewState, UObject* InObject)
{
	Super::OnChangedStateInPool_Implementation(NewState, InObject);

	UUserWidget* UserWidget = CastChecked<UUserWidget>(InObject);
	const bool bActivate = NewState == EPoolObjectState::Active;

	UserWidget->SetVisibility(bActivate ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
