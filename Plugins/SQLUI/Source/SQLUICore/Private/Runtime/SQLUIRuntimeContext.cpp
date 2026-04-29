#include "Runtime/SQLUIRuntimeContext.h"

#include "Actions/SQLUIActionRegistry.h"
#include "Layout/SQLUILayoutFactory.h"
#include "Variables/SQLUIVariableStore.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"

namespace
{
template <typename ServiceType>
ServiceType* ResolveSQLUIRuntimeService(UObject* Outer, ServiceType* ProvidedService)
{
	if (IsValid(ProvidedService))
	{
		return ProvidedService;
	}

	return NewObject<ServiceType>(Outer);
}
}

void USQLUIRuntimeContext::Initialize(const FSQLUIRuntimeContextSettings& InSettings)
{
	VariableStore = ResolveSQLUIRuntimeService(this, InSettings.VariableStore.Get());
	ActionRegistry = ResolveSQLUIRuntimeService(this, InSettings.ActionRegistry.Get());
	WidgetCatalog = ResolveSQLUIRuntimeService(this, InSettings.WidgetCatalog.Get());
	LayoutFactory = ResolveSQLUIRuntimeService(this, InSettings.LayoutFactory.Get());

	bInitialized =
		IsValid(VariableStore) &&
		IsValid(ActionRegistry) &&
		IsValid(WidgetCatalog) &&
		IsValid(LayoutFactory);
}

bool USQLUIRuntimeContext::IsInitialized() const
{
	return bInitialized;
}

USQLUIVariableStore* USQLUIRuntimeContext::GetVariableStore() const
{
	return VariableStore.Get();
}

USQLUIActionRegistry* USQLUIRuntimeContext::GetActionRegistry() const
{
	return ActionRegistry.Get();
}

USQLUIWidgetCatalog* USQLUIRuntimeContext::GetWidgetCatalog() const
{
	return WidgetCatalog.Get();
}

USQLUILayoutFactory* USQLUIRuntimeContext::GetLayoutFactory() const
{
	return LayoutFactory.Get();
}
