#include "SQLUISampleLayoutDrivenFilterListDemoActor.h"

#include "Actions/SQLUIActionRegistry.h"
#include "Layout/ISQLUILayoutRepository.h"
#include "Layout/SQLUIJsonFileLayoutRepository.h"
#include "Layout/SQLUILayoutRepositoryFactory.h"
#include "Misc/Paths.h"
#include "SQLUISamplesModule.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Runtime/SQLUIRuntimeContext.h"
#include "WidgetCatalog/SQLUIWidgetCatalog.h"
#include "WidgetCatalog/SQLUIWidgetCatalogRegistrar.h"
#include "Widgets/SQLUIBaseWidget.h"
#include "Widgets/SQLUIFilterBox.h"
#include "Widgets/SQLUIListWidget.h"

namespace
{
const TCHAR* SQLUISampleLayoutDrivenFilterListRootWidgetId =
	TEXT("SQLUI.Sample.LayoutDrivenFilterList.Root");
const TCHAR* SQLUISampleLayoutDrivenFilterListFilterBoxWidgetId =
	TEXT("SQLUI.Sample.LayoutDrivenFilterList.FilterBox");
const TCHAR* SQLUISampleLayoutDrivenFilterListListWidgetId =
	TEXT("SQLUI.Sample.LayoutDrivenFilterList.ListWidget");
const TCHAR* SQLUISampleLayoutDrivenFilterListRowClickedActionKey =
	TEXT("SQLUI.Sample.LogRowClicked");

const TCHAR* SQLUISampleLayoutDrivenFilterListDemoStepStatusToString(
	const ESQLUIRuntimeWidgetPipelineStepStatus Status)
{
	switch (Status)
	{
	case ESQLUIRuntimeWidgetPipelineStepStatus::NotRun:
		return TEXT("NotRun");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Skipped:
		return TEXT("Skipped");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Succeeded:
		return TEXT("Succeeded");
	case ESQLUIRuntimeWidgetPipelineStepStatus::Failed:
		return TEXT("Failed");
	default:
		return TEXT("Unknown");
	}
}

const TCHAR* SQLUISampleLayoutDrivenFilterListDemoBoolToString(const bool bValue)
{
	return bValue ? TEXT("true") : TEXT("false");
}

FString MakeSQLUISampleLayoutDrivenFilterListRepositoryBaseDirectory()
{
	FString BaseDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("SQLUI"),
		TEXT("Samples"),
		TEXT("LayoutDrivenFilterList"),
		TEXT("Layouts"));
	FPaths::NormalizeDirectoryName(BaseDirectory);
	return BaseDirectory;
}

FString NormalizeSQLUISampleLayoutDrivenFilterListRepositoryBaseDirectory(
	const FString& BaseDirectory)
{
	FString NormalizedBaseDirectory = BaseDirectory;
	if (NormalizedBaseDirectory.IsEmpty())
	{
		return MakeSQLUISampleLayoutDrivenFilterListRepositoryBaseDirectory();
	}

	if (FPaths::IsRelative(NormalizedBaseDirectory))
	{
		NormalizedBaseDirectory = FPaths::ConvertRelativePathToFull(NormalizedBaseDirectory);
	}

	FPaths::NormalizeDirectoryName(NormalizedBaseDirectory);
	return NormalizedBaseDirectory;
}

FString GetSQLUISampleActionParameterString(
	const FSQLUIActionRequest& Request,
	const FString& ParameterName)
{
	const FSQLUIVariableValue* Value = Request.Parameters.Find(ParameterName);
	if (!Value)
	{
		return FString();
	}

	switch (Value->Type)
	{
	case ESQLUIVariableValueType::String:
		return Value->StringValue;
	case ESQLUIVariableValueType::Number:
		return FString::SanitizeFloat(Value->NumberValue);
	case ESQLUIVariableValueType::Boolean:
		return SQLUISampleLayoutDrivenFilterListDemoBoolToString(Value->bBooleanValue);
	default:
		return FString();
	}
}

FSQLUIActionResult MakeSQLUISampleSucceededActionResult()
{
	FSQLUIActionResult Result;
	Result.bSucceeded = true;
	return Result;
}

bool SaveSQLUISampleLayoutDrivenFilterListDocument(
	ISQLUILayoutRepository* Repository,
	const FSQLUILayoutDocument& Document,
	FSQLUILayoutSaveResult& OutSaveResult)
{
	OutSaveResult = FSQLUILayoutSaveResult();

	if (!Repository)
	{
		OutSaveResult.ErrorMessage = TEXT("SQLUI sample layout-driven filter/list demo failed: repository is invalid during save.");
		return false;
	}

	bool bSaveCompleted = false;
	Repository->SaveLayout(
		Document,
		FSQLUILayoutSaveCompleteDelegate::CreateLambda(
			[&OutSaveResult, &bSaveCompleted](const FSQLUILayoutSaveResult& InSaveResult)
			{
				OutSaveResult = InSaveResult;
				bSaveCompleted = true;
			}));

	if (!bSaveCompleted)
	{
		OutSaveResult.ErrorMessage = TEXT("SQLUI sample layout-driven filter/list demo failed: repository save did not complete.");
		return false;
	}

	return OutSaveResult.bSucceeded;
}

bool LoadSQLUISampleLayoutDrivenFilterListDocument(
	ISQLUILayoutRepository* Repository,
	const FString& LayoutId,
	FSQLUILayoutLoadResult& OutLoadResult)
{
	OutLoadResult = FSQLUILayoutLoadResult();

	if (!Repository)
	{
		OutLoadResult.ErrorMessage = TEXT("SQLUI sample layout-driven filter/list demo failed: repository is invalid during load.");
		return false;
	}

	bool bLoadCompleted = false;
	Repository->LoadLayout(
		LayoutId,
		FSQLUILayoutLoadCompleteDelegate::CreateLambda(
			[&OutLoadResult, &bLoadCompleted](const FSQLUILayoutLoadResult& InLoadResult)
			{
				OutLoadResult = InLoadResult;
				bLoadCompleted = true;
			}));

	if (!bLoadCompleted)
	{
		OutLoadResult.ErrorMessage = TEXT("SQLUI sample layout-driven filter/list demo failed: repository load did not complete.");
		return false;
	}

	return OutLoadResult.bSucceeded && OutLoadResult.Validation.bIsValid;
}

void LogSQLUISampleLayoutDrivenFilterListValidationMessages(
	const FSQLUILayoutValidationResult& Validation)
{
	for (const FString& Error : Validation.Errors)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo layout validation error: %s"),
			*Error);
	}

	for (const FString& Warning : Validation.Warnings)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample layout-driven filter/list demo layout validation warning: %s"),
			*Warning);
	}
}

void LogSQLUISampleLayoutDrivenFilterListDemoErrors(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo error: %s"),
			*Message);
	}
}

void LogSQLUISampleLayoutDrivenFilterListDemoWarnings(const TArray<FString>& Messages)
{
	for (const FString& Message : Messages)
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample layout-driven filter/list demo warning: %s"),
			*Message);
	}
}

void LogSQLUISampleLayoutDrivenFilterListDemoStepResults(
	const TArray<FSQLUIRuntimeWidgetPipelineStepResult>& StepResults)
{
	for (const FSQLUIRuntimeWidgetPipelineStepResult& StepResult : StepResults)
	{
		UE_LOG(
			LogSQLUISamples,
			Log,
			TEXT("SQLUI sample layout-driven filter/list demo step '%s': %s"),
			*StepResult.StepName,
			SQLUISampleLayoutDrivenFilterListDemoStepStatusToString(StepResult.Status));
	}
}

FSQLUILayoutDocument MakeSQLUISampleLayoutDrivenFilterListDocument()
{
	const FString RootWidgetId = SQLUISampleLayoutDrivenFilterListRootWidgetId;
	const FString FilterBoxWidgetId = SQLUISampleLayoutDrivenFilterListFilterBoxWidgetId;
	const FString ListWidgetId = SQLUISampleLayoutDrivenFilterListListWidgetId;

	FSQLUILayoutDocument Document;
	Document.Version.SchemaVersion = 1;
	Document.Version.Revision = 1;
	Document.Version.Label = TEXT("SQLUI Sample Layout-Driven Filter/List Demo");
	Document.Metadata.LayoutId = TEXT("sqlui.sample.layout-driven-filter-list-demo");
	Document.Metadata.DisplayName = TEXT("SQLUI Sample Layout-Driven Filter/List Demo");
	Document.Metadata.Description = TEXT("Minimal SQLUISamples layout-driven FilterBox and ListWidget visual demo.");
	Document.Metadata.CreatedBy = TEXT("SQLUISamples");
	Document.RootWidgetId = RootWidgetId;

	FSQLUILayoutNode RootNode;
	RootNode.WidgetId = RootWidgetId;
	RootNode.WidgetTypeKey = FSQLUIWidgetTypeKeys::VerticalBox().Value;
	RootNode.ChildWidgetIds.Add(FilterBoxWidgetId);
	RootNode.ChildWidgetIds.Add(ListWidgetId);
	RootNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));

	FSQLUILayoutNode FilterBoxNode;
	FilterBoxNode.WidgetId = FilterBoxWidgetId;
	FilterBoxNode.ParentWidgetId = RootWidgetId;
	FilterBoxNode.WidgetTypeKey = FSQLUIWidgetTypeKeys::FilterBox().Value;
	FilterBoxNode.Properties.Add(TEXT("PlaceholderText"), TEXT("Filter layout-driven sample rows"));
	FilterBoxNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));

	FSQLUILayoutAction RowClickedAction;
	RowClickedAction.ActionId = TEXT("SQLUI.Sample.LayoutDrivenFilterList.RowClicked");
	RowClickedAction.Trigger = TEXT("RowClicked");
	RowClickedAction.ActionTypeKey = SQLUISampleLayoutDrivenFilterListRowClickedActionKey;
	RowClickedAction.Parameters.Add(TEXT("Source"), TEXT("LayoutDrivenFilterListDemo"));

	FSQLUILayoutNode ListWidgetNode;
	ListWidgetNode.WidgetId = ListWidgetId;
	ListWidgetNode.ParentWidgetId = RootWidgetId;
	ListWidgetNode.WidgetTypeKey = FSQLUIWidgetTypeKeys::ListWidget().Value;
	ListWidgetNode.Properties.Add(TEXT("EmptyText"), TEXT("No layout-driven sample rows yet"));
	ListWidgetNode.Properties.Add(TEXT("Items"), TEXT("row-1|First sample row;row-2|Second sample row;row-3|Third sample row"));
	ListWidgetNode.Properties.Add(TEXT("IsEnabled"), TEXT("true"));
	ListWidgetNode.Actions.Add(RowClickedAction);

	Document.Nodes.Add(RootNode);
	Document.Nodes.Add(FilterBoxNode);
	Document.Nodes.Add(ListWidgetNode);

	return Document;
}

void LogSQLUISampleLayoutDrivenFilterListDemoResult(
	const FSQLUIRuntimeWidgetPipelineResult& PipelineResult)
{
	if (PipelineResult.bSucceeded)
	{
		UE_LOG(LogSQLUISamples, Log, TEXT("SQLUI sample layout-driven filter/list demo succeeded."));
	}
	else
	{
		UE_LOG(LogSQLUISamples, Error, TEXT("SQLUI sample layout-driven filter/list demo failed."));
	}

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample layout-driven filter/list demo root widget valid: %s"),
		SQLUISampleLayoutDrivenFilterListDemoBoolToString(IsValid(PipelineResult.RootWidget.Get())));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample layout-driven filter/list demo created widget count: %d"),
		PipelineResult.CreatedWidgets.Num());

	LogSQLUISampleLayoutDrivenFilterListDemoStepResults(PipelineResult.StepResults);
	LogSQLUISampleLayoutDrivenFilterListDemoErrors(PipelineResult.Errors);
	LogSQLUISampleLayoutDrivenFilterListDemoWarnings(PipelineResult.Warnings);
}
}

ASQLUISampleLayoutDrivenFilterListDemoActor::ASQLUISampleLayoutDrivenFilterListDemoActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASQLUISampleLayoutDrivenFilterListDemoActor::BeginPlay()
{
	Super::BeginPlay();

	if (bRunOnBeginPlay)
	{
		RunLayoutDrivenFilterListDemo();
	}
}

void ASQLUISampleLayoutDrivenFilterListDemoActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAddedRootWidget();

	Super::EndPlay(EndPlayReason);
}

void ASQLUISampleLayoutDrivenFilterListDemoActor::RunLayoutDrivenFilterListDemo()
{
	RemoveAddedRootWidget();
	LastPipelineResult = FSQLUIRuntimeWidgetPipelineResult();
	bLastLayoutLoadedThroughRepository = false;
	LastLayoutRepositoryBaseDirectory.Reset();

	FSQLUILayoutDocument LayoutDocument;
	if (!TryResolveLayoutDrivenFilterListDocument(LayoutDocument))
	{
		LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);
		return;
	}

	USQLUIWidgetCatalog* WidgetCatalog = NewObject<USQLUIWidgetCatalog>(this);
	if (!IsValid(WidgetCatalog))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: could not create widget catalog."));
		LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);
		return;
	}

	if (!USQLUIWidgetCatalogRegistrar::RegisterDefaultSQLUIWidgets(WidgetCatalog))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: could not register default widget catalog entries."));
		LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);
		return;
	}

	USQLUIActionRegistry* ActionRegistry = NewObject<USQLUIActionRegistry>(this);
	if (!IsValid(ActionRegistry))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: could not create action registry."));
		LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);
		return;
	}

	FSQLUIActionKey RowClickedActionKey;
	RowClickedActionKey.Name = SQLUISampleLayoutDrivenFilterListRowClickedActionKey;
	if (!ActionRegistry->RegisterAction(
		RowClickedActionKey,
		FSQLUIActionHandler::CreateUObject(
			this,
			&ASQLUISampleLayoutDrivenFilterListDemoActor::HandleLayoutDrivenRowClickedAction)))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: could not register row-click action handler."));
		LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);
		return;
	}

	USQLUIRuntimeContext* RuntimeContext = NewObject<USQLUIRuntimeContext>(this);
	if (!IsValid(RuntimeContext))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: could not create runtime context."));
		LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);
		return;
	}

	FSQLUIRuntimeContextSettings RuntimeContextSettings;
	RuntimeContextSettings.ActionRegistry = ActionRegistry;
	RuntimeContextSettings.WidgetCatalog = WidgetCatalog;
	RuntimeContext->Initialize(RuntimeContextSettings);

	if (!RuntimeContext->IsInitialized())
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: runtime context did not initialize."));
		LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);
		return;
	}

	USQLUIRuntimeWidgetPipeline* Pipeline = NewObject<USQLUIRuntimeWidgetPipeline>(this);
	if (!IsValid(Pipeline))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: could not create runtime widget pipeline."));
		LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* OwningPlayer = World ? World->GetFirstPlayerController() : nullptr;

	FSQLUIRuntimeWidgetPipelineRequest PipelineRequest;
	PipelineRequest.Document = LayoutDocument;
	PipelineRequest.RuntimeContext = RuntimeContext;
	PipelineRequest.WidgetCatalogOverride = WidgetCatalog;
	PipelineRequest.WorldContextObject = this;
	PipelineRequest.OwningPlayer = OwningPlayer;

	LastPipelineResult = Pipeline->RunPipeline(PipelineRequest);

	if (LastPipelineResult.bSucceeded)
	{
		ConnectLayoutDrivenFilterListWidgets();
	}
	else
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample layout-driven filter/list demo skipped filter/list connection because the runtime pipeline failed."));
	}

	bool bAddedToViewport = false;
	if (bAddToViewport && IsValid(LastPipelineResult.RootWidget.Get()))
	{
		LastPipelineResult.RootWidget->AddToViewport(ViewportZOrder);
		LastPipelineResult.RootWidget->SetPositionInViewport(ViewportPosition, true);
		if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
		{
			LastPipelineResult.RootWidget->SetDesiredSizeInViewport(ViewportSize);
		}

		AddedRootWidget = LastPipelineResult.RootWidget;
		bAddedToViewport = true;
	}

	LogSQLUISampleLayoutDrivenFilterListDemoResult(LastPipelineResult);

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample layout-driven filter/list demo viewport add status: %s"),
		bAddToViewport
			? (bAddedToViewport ? TEXT("added") : TEXT("failed"))
			: TEXT("skipped"));
}

bool ASQLUISampleLayoutDrivenFilterListDemoActor::TryResolveLayoutDrivenFilterListDocument(
	FSQLUILayoutDocument& OutDocument)
{
	OutDocument = MakeSQLUISampleLayoutDrivenFilterListDocument();

	if (!bLoadLayoutThroughJsonFileRepository)
	{
		return true;
	}

	FSQLUILayoutRepositoryFactorySettings RepositorySettings;
	RepositorySettings.Backend = ESQLUILayoutRepositoryBackend::JsonFile;
	RepositorySettings.JsonFileBaseDirectory = NormalizeSQLUISampleLayoutDrivenFilterListRepositoryBaseDirectory(
		LayoutRepositoryBaseDirectory);

	USQLUIJsonFileLayoutRepository* LayoutRepository = Cast<USQLUIJsonFileLayoutRepository>(
		USQLUILayoutRepositoryFactory::CreateLayoutRepository(this, RepositorySettings));
	if (!IsValid(LayoutRepository))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: layout repository factory did not create a JSON file layout repository."));
		return false;
	}

	LastLayoutRepositoryBaseDirectory = LayoutRepository->GetResolvedBaseDirectory();

	FSQLUILayoutSaveResult SaveResult;
	if (!SaveSQLUISampleLayoutDrivenFilterListDocument(LayoutRepository, OutDocument, SaveResult))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: could not save layout '%s' to repository '%s'. %s"),
			*OutDocument.Metadata.LayoutId,
			*LastLayoutRepositoryBaseDirectory,
			*SaveResult.ErrorMessage);
		LogSQLUISampleLayoutDrivenFilterListValidationMessages(SaveResult.Validation);
		return false;
	}

	FSQLUILayoutLoadResult LoadResult;
	if (!LoadSQLUISampleLayoutDrivenFilterListDocument(LayoutRepository, SaveResult.SavedLayoutId, LoadResult))
	{
		UE_LOG(
			LogSQLUISamples,
			Error,
			TEXT("SQLUI sample layout-driven filter/list demo failed: could not load layout '%s' from repository '%s'. %s"),
			*SaveResult.SavedLayoutId,
			*LastLayoutRepositoryBaseDirectory,
			*LoadResult.ErrorMessage);
		LogSQLUISampleLayoutDrivenFilterListValidationMessages(LoadResult.Validation);
		return false;
	}

	OutDocument = LoadResult.Document;
	bLastLayoutLoadedThroughRepository = true;

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample layout-driven filter/list demo loaded layout '%s' through JSON file repository '%s'."),
		*OutDocument.Metadata.LayoutId,
		*LastLayoutRepositoryBaseDirectory);

	if (bRemoveRepositoryLayoutAfterLoad)
	{
		const FSQLUILayoutRepositoryRemoveResult RemoveResult =
			LayoutRepository->RemoveLayout(OutDocument.Metadata.LayoutId);
		if (!RemoveResult.bSucceeded || !RemoveResult.bRemoved)
		{
			UE_LOG(
				LogSQLUISamples,
				Warning,
				TEXT("SQLUI sample layout-driven filter/list demo repository cleanup did not remove layout '%s' from '%s'. %s"),
				*OutDocument.Metadata.LayoutId,
				*LastLayoutRepositoryBaseDirectory,
				*RemoveResult.ErrorMessage);
		}
	}

	return true;
}

void ASQLUISampleLayoutDrivenFilterListDemoActor::ConnectLayoutDrivenFilterListWidgets()
{
	DisconnectLayoutDrivenFilterListWidgets();

	USQLUIBaseWidget* const* FilterBoxBaseWidget =
		LastPipelineResult.CreatedWidgetMap.Find(FString(SQLUISampleLayoutDrivenFilterListFilterBoxWidgetId));
	USQLUIBaseWidget* const* ListBaseWidget =
		LastPipelineResult.CreatedWidgetMap.Find(FString(SQLUISampleLayoutDrivenFilterListListWidgetId));

	USQLUIFilterBox* FilterBox = FilterBoxBaseWidget
		? Cast<USQLUIFilterBox>(*FilterBoxBaseWidget)
		: nullptr;
	USQLUIListWidget* ListWidget = ListBaseWidget
		? Cast<USQLUIListWidget>(*ListBaseWidget)
		: nullptr;

	if (!IsValid(FilterBox) || !IsValid(ListWidget))
	{
		UE_LOG(
			LogSQLUISamples,
			Warning,
			TEXT("SQLUI sample layout-driven filter/list demo could not connect filter/list events. FilterBox valid: %s. ListWidget valid: %s."),
			SQLUISampleLayoutDrivenFilterListDemoBoolToString(IsValid(FilterBox)),
			SQLUISampleLayoutDrivenFilterListDemoBoolToString(IsValid(ListWidget)));
		return;
	}

	ConnectedFilterBoxWidget = FilterBox;
	ConnectedListWidget = ListWidget;
	FilterTextChangedDelegateHandle = ConnectedFilterBoxWidget->OnFilterTextChanged.AddUObject(
		this,
		&ASQLUISampleLayoutDrivenFilterListDemoActor::HandleLayoutDrivenFilterTextChanged);
	ConnectedListWidget->SetFilterText(ConnectedFilterBoxWidget->GetFilterText());

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample layout-driven filter/list demo connected FilterBox '%s' to ListWidget '%s'."),
		SQLUISampleLayoutDrivenFilterListFilterBoxWidgetId,
		SQLUISampleLayoutDrivenFilterListListWidgetId);
}

void ASQLUISampleLayoutDrivenFilterListDemoActor::DisconnectLayoutDrivenFilterListWidgets()
{
	if (IsValid(ConnectedFilterBoxWidget.Get()) && FilterTextChangedDelegateHandle.IsValid())
	{
		ConnectedFilterBoxWidget->OnFilterTextChanged.Remove(FilterTextChangedDelegateHandle);
	}

	FilterTextChangedDelegateHandle.Reset();
	ConnectedFilterBoxWidget = nullptr;
	ConnectedListWidget = nullptr;
}

void ASQLUISampleLayoutDrivenFilterListDemoActor::HandleLayoutDrivenFilterTextChanged(
	const FText& InFilterText)
{
	if (IsValid(ConnectedListWidget.Get()))
	{
		ConnectedListWidget->SetFilterText(InFilterText);
	}
}

FSQLUIActionResult ASQLUISampleLayoutDrivenFilterListDemoActor::HandleLayoutDrivenRowClickedAction(
	const FSQLUIActionRequest& Request)
{
	const FString ActionId = GetSQLUISampleActionParameterString(Request, TEXT("ActionId"));
	const FString WidgetId = GetSQLUISampleActionParameterString(Request, TEXT("WidgetId"));
	const FString ItemId = GetSQLUISampleActionParameterString(Request, TEXT("ItemId"));
	const FString DisplayText = GetSQLUISampleActionParameterString(Request, TEXT("DisplayText"));

	UE_LOG(
		LogSQLUISamples,
		Log,
		TEXT("SQLUI sample layout-driven filter/list demo row-click action dispatched. ActionId='%s', WidgetId='%s', ItemId='%s', DisplayText='%s'."),
		*ActionId,
		*WidgetId,
		*ItemId,
		*DisplayText);

	return MakeSQLUISampleSucceededActionResult();
}

void ASQLUISampleLayoutDrivenFilterListDemoActor::RemoveAddedRootWidget()
{
	DisconnectLayoutDrivenFilterListWidgets();

	if (IsValid(AddedRootWidget.Get()))
	{
		AddedRootWidget->RemoveFromParent();
	}

	AddedRootWidget = nullptr;
}
