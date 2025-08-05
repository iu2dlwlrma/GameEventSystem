#pragma once
#include "CoreMinimal.h"
#include "Misc/StringBuilder.h"

GAMEEVENTSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogGameEventSystem, VeryVerbose, All);

/**
 * GameEventSystem Log Manager
 * Provides unified logging interface with support for different log levels and formatted output
 */
class GAMEEVENTSYSTEM_API FLogger
{
public:
	static FLogger& Get()
	{
		static FLogger Instance;
		return Instance;
	}

	template<typename FormatType, typename... Types>
	void LogDisplay(const FormatType& Format, Types... Args);

	template<typename FormatType, typename... Types>
	void LogWarning(const FormatType& Format, Types... Args);

	template<typename FormatType, typename... Types>
	void LogError(const FormatType& Format, Types... Args);

	template<typename FormatType, typename... Types>
	void LogVeryVerbose(const FormatType& Format, Types... Args);

	template<typename FormatType, typename... Types>
	void Log(const FormatType& Format, Types... Args);

	template<typename FormatType, typename... Types>
	void Log(ELogVerbosity::Type Verbosity, const FormatType& Format, Types... Args);

private:
	FLogger() = default;
	~FLogger() = default;

	FLogger(const FLogger&) = delete;
	FLogger& operator=(const FLogger&) = delete;
	FLogger(FLogger&&) = delete;
	FLogger& operator=(FLogger&&) = delete;

	template<typename FormatType, typename... Types>
	void LogInternal(ELogVerbosity::Type Verbosity, const FormatType& Format, Types... Args) const;
};

template<typename FormatType, typename... Types>
void FLogger::LogDisplay(const FormatType& Format, Types... Args)
{
	LogInternal(ELogVerbosity::Display, Format, Args...);
}

template<typename FormatType, typename... Types>
void FLogger::LogWarning(const FormatType& Format, Types... Args)
{
	LogInternal(ELogVerbosity::Warning, Format, Args...);
}

template<typename FormatType, typename... Types>
void FLogger::LogError(const FormatType& Format, Types... Args)
{
	LogInternal(ELogVerbosity::Error, Format, Args...);
}

template<typename FormatType, typename... Types>
void FLogger::LogVeryVerbose(const FormatType& Format, Types... Args)
{
	LogInternal(ELogVerbosity::VeryVerbose, Format, Args...);
}

template<typename FormatType, typename... Types>
void FLogger::Log(const FormatType& Format, Types... Args)
{
	LogInternal(ELogVerbosity::Log, Format, Args...);
}

template<typename FormatType, typename... Types>
void FLogger::Log(ELogVerbosity::Type Verbosity, const FormatType& Format, Types... Args)
{
	LogInternal(Verbosity, Format, Args...);
}

template<typename FormatType, typename... Types>
void FLogger::LogInternal(const ELogVerbosity::Type Verbosity, const FormatType& Format, Types... Args) const
{
	const auto& LogCategory = LogGameEventSystem;

	if (!LogCategory.IsSuppressed(Verbosity))
	{
		TStringBuilder<1024> FormattedMessage;
		FormattedMessage.Appendf(Format, Forward<Types>(Args)...);

		FMsg::Logf(__FILE__,
		           __LINE__,
		           LogCategory.GetCategoryName(),
		           Verbosity,
		           TEXT("[GameEventSystem] %s"),
		           FormattedMessage.ToString());
	}
}
