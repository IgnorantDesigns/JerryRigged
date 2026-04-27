#pragma once

#include "CoreMinimal.h"
#include "Layout/SQLUILayoutTypes.h"

DECLARE_DELEGATE_OneParam(FSQLUILayoutLoadCompleteDelegate, const FSQLUILayoutLoadResult&);
DECLARE_DELEGATE_OneParam(FSQLUILayoutSaveCompleteDelegate, const FSQLUILayoutSaveResult&);

class SQLUICORE_API ISQLUILayoutRepository
{
public:
	virtual ~ISQLUILayoutRepository() = default;

	virtual void LoadLayout(const FString& LayoutId, FSQLUILayoutLoadCompleteDelegate Callback) = 0;
	virtual void SaveLayout(const FSQLUILayoutDocument& Document, FSQLUILayoutSaveCompleteDelegate Callback) = 0;
};
