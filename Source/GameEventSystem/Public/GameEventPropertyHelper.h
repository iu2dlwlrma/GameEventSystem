#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/Class.h"
#include "Engine/Engine.h"
#include "Templates/UnrealTypeTraits.h"
#include "Math/Vector.h"
#include "Serialization/MemoryWriter.h"
#include <type_traits>
#include <utility>
#include <functional>
#include "Logger.h"

#pragma region "Lambda type derivation tool"

template<typename Lambda>
struct TLambdaTraits;

// Specialization for plain function pointers
template<typename R, typename... Args>
struct TLambdaTraits<R(*)(Args...)>
{
	using ReturnType = R;
	using ArgsTuple = std::tuple<Args...>;
	static constexpr size_t ArgsCount = sizeof...(Args);

	template<size_t Index>
	using ArgType = std::tuple_element_t<Index, ArgsTuple>;
};

// Specialization for member function pointers
template<typename R, typename C, typename... Args>
struct TLambdaTraits<R(C::*)(Args...)>
{
	using ReturnType = R;
	using ArgsTuple = std::tuple<Args...>;
	static constexpr size_t ArgsCount = sizeof...(Args);

	template<size_t Index>
	using ArgType = std::tuple_element_t<Index, ArgsTuple>;
};

// Specialization for const member function pointers
template<typename R, typename C, typename... Args>
struct TLambdaTraits<R(C::*)(Args...) const>
{
	using ReturnType = R;
	using ArgsTuple = std::tuple<Args...>;
	static constexpr size_t ArgsCount = sizeof...(Args);

	template<size_t Index>
	using ArgType = std::tuple_element_t<Index, ArgsTuple>;
};

// Universal specialization for Lambda expressions and function objects
template<typename Lambda>
struct TLambdaTraits : TLambdaTraits<decltype(&Lambda::operator())>
{
};

// Auxiliary template alias
template<typename Lambda>
using TLambdaArgsTuple = typename TLambdaTraits<Lambda>::ArgsTuple;

template<typename Lambda>
constexpr size_t TLambdaArgsCount = TLambdaTraits<Lambda>::ArgsCount;

template<typename Lambda, size_t Index>
using TLambdaArgType = typename TLambdaTraits<Lambda>::template ArgType<Index>;

#pragma endregion

#pragma region "TypeTraits"
template<typename T, typename = void>
struct THasStaticClass : std::false_type
{
};

template<typename T>
struct THasStaticClass<T, std::void_t<decltype(T::StaticClass())>> : std::true_type
{
};

template<typename T, typename = void>
struct THasStaticStruct : std::false_type
{
};

template<typename T>
struct THasStaticStruct<T, std::void_t<decltype(&T::StaticStruct)>> : std::true_type
{
};

template<typename T, typename = void>
struct THasBaseStructure : std::false_type
{
};

template<typename T>
struct THasBaseStructure<T, std::void_t<decltype(TBaseStructure<T>::Get())>> : std::true_type
{
};

template<typename T>
struct TIsStructSupported : std::bool_constant<
			THasStaticStruct<T>::value || THasBaseStructure<T>::value
		>
{
};

template<typename T, typename = void>
struct TTypeToPropertyType
{
};

template<typename T>
struct TTypeToPropertyType<T, std::enable_if_t<
	                           TIsStructSupported<T>::value &&
	                           std::is_class_v<T> &&
	                           !std::is_pointer_v<T>
                           >>
{
	using PropertyType = FStructProperty;
};

#define DECLARE_CPP_TO_PROPERTY_TYPE(CppType, UPropertyType) \
	template<> \
	struct TTypeToPropertyType<CppType> \
	{ \
		using PropertyType = UPropertyType; \
	};

DECLARE_CPP_TO_PROPERTY_TYPE(bool, FBoolProperty)

DECLARE_CPP_TO_PROPERTY_TYPE(int8, FInt8Property)

DECLARE_CPP_TO_PROPERTY_TYPE(uint8, FByteProperty)

DECLARE_CPP_TO_PROPERTY_TYPE(int16, FInt16Property)

DECLARE_CPP_TO_PROPERTY_TYPE(uint16, FUInt16Property)

DECLARE_CPP_TO_PROPERTY_TYPE(int32, FIntProperty)

DECLARE_CPP_TO_PROPERTY_TYPE(uint32, FUInt32Property)

DECLARE_CPP_TO_PROPERTY_TYPE(int64, FInt64Property)

DECLARE_CPP_TO_PROPERTY_TYPE(uint64, FUInt64Property)

DECLARE_CPP_TO_PROPERTY_TYPE(float, FFloatProperty)

DECLARE_CPP_TO_PROPERTY_TYPE(double, FDoubleProperty)

DECLARE_CPP_TO_PROPERTY_TYPE(FString, FStrProperty)

DECLARE_CPP_TO_PROPERTY_TYPE(FName, FNameProperty)

DECLARE_CPP_TO_PROPERTY_TYPE(FText, FTextProperty)

#undef DECLARE_CPP_TO_PROPERTY_TYPE

template<>
struct TTypeToPropertyType<std::nullptr_t>
{
	using PropertyType = FObjectProperty;
};

template<typename T>
struct TTypeToPropertyType<TArray<T>>
{
	using PropertyType = FArrayProperty;
};

template<typename T>
struct TTypeToPropertyType<TSet<T>>
{
	using PropertyType = FSetProperty;
};

template<typename K, typename V>
struct TTypeToPropertyType<TMap<K, V>>
{
	using PropertyType = FMapProperty;
};

template<typename T>
struct TTypeToPropertyType<T*>
{
	using PropertyType = std::conditional_t<std::is_base_of_v<UObject, T>, FObjectProperty, void>;
};

template<typename T>
struct TTypeToPropertyType<T, std::enable_if_t<std::is_enum_v<T>>>
{
	using PropertyType = std::conditional_t<sizeof(T) == 1, FByteProperty,
	                                        std::conditional_t<sizeof(T) == 2, FInt16Property,
	                                                           std::conditional_t<sizeof(T) == 8, FInt64Property, FIntProperty>>>;
};

template<typename T>
struct TIsPropertyTypeSupported : std::bool_constant<
			!std::is_void_v<typename TTypeToPropertyType<T>::PropertyType>
		>
{
};

#pragma endregion

template<typename T>
UScriptStruct* GetUniversalStruct()
{
	if constexpr (THasBaseStructure<T>::value)
	{
		return TBaseStructure<T>::Get();
	}
	else if constexpr (THasStaticStruct<T>::value)
	{
		return Cast<UScriptStruct>(T::StaticStruct());
	}
	else
	{
		return nullptr;
	}
}

template<typename T>
struct TPropertyMatcher
{
	static bool DoesMatchProperty(const FProperty* P)
	{
		if constexpr (!std::is_void_v<typename TTypeToPropertyType<T>::PropertyType>)
		{
			using PropertyType = typename TTypeToPropertyType<T>::PropertyType;

			if constexpr (std::is_same_v<PropertyType, FStructProperty>)
			{
				if (const FStructProperty* StructProp = CastField<FStructProperty>(P))
				{
					UScriptStruct* ExpectedStruct = GetUniversalStruct<T>();
					return ExpectedStruct && StructProp->Struct == ExpectedStruct;
				}
				return false;
			}
			else if constexpr (std::is_enum_v<T>)
			{
				if (CastField<PropertyType>(P))
				{
					return true;
				}
				if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(P))
				{
					if constexpr (std::is_same_v<PropertyType, FByteProperty>)
					{
						return EnumProp->GetElementSize() == sizeof(uint8);
					}
					else if constexpr (std::is_same_v<PropertyType, FInt16Property>)
					{
						return EnumProp->GetElementSize() == sizeof(int16);
					}
					else if constexpr (std::is_same_v<PropertyType, FIntProperty>)
					{
						return EnumProp->GetElementSize() == sizeof(int32);
					}
					else if constexpr (std::is_same_v<PropertyType, FInt64Property>)
					{
						return EnumProp->GetElementSize() == sizeof(int64);
					}
				}
				return false;
			}
			else
			{
				return CastField<PropertyType>(P) != nullptr;
			}
		}
		else
		{
			return false;
		}
	}
};

template<typename ContainedType>
struct TPropertyMatcher<TArray<ContainedType>>
{
	static bool DoesMatchProperty(const FProperty* P)
	{
		const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(P);
		return ArrayProperty && TPropertyMatcher<ContainedType>::DoesMatchProperty(ArrayProperty->Inner);
	}
};

template<typename ContainedType>
struct TPropertyMatcher<TSet<ContainedType>>
{
	static bool DoesMatchProperty(const FProperty* P)
	{
		const FSetProperty* SetProperty = CastField<FSetProperty>(P);
		return SetProperty && TPropertyMatcher<ContainedType>::DoesMatchProperty(SetProperty->ElementProp);
	}
};

template<typename KeyType, typename ValueType>
struct TPropertyMatcher<TMap<KeyType, ValueType>>
{
	static bool DoesMatchProperty(const FProperty* P)
	{
		const FMapProperty* MapProperty = CastField<FMapProperty>(P);
		return MapProperty &&
		       TPropertyMatcher<KeyType>::DoesMatchProperty(MapProperty->KeyProp) &&
		       TPropertyMatcher<ValueType>::DoesMatchProperty(MapProperty->ValueProp);
	}
};

class GAMEEVENTSYSTEM_API FGameEventPropertyHelper
{
public:
	template<typename T>
	static constexpr bool IsTypeSupported();

	template<typename T>
	static FProperty* GetPropertyForType(const UObject* Context, const T& PropertyPtr);

	template<typename... Args, typename Lambda>
	static auto CreatePropertyWrapper(Lambda&& InLambda);

	template<typename Lambda>
	static auto CreatePropertyWrapperFromLambda(Lambda&& InLambda);

private:
	template<typename... Args, typename Lambda, size_t... Indices>
	static void ExtractAndInvokeVariadicLambda(Lambda&& InLambda, const TArray<FPropertyContext>& PropertyContexts, std::index_sequence<Indices...>)
	{
		InLambda(ExtractParameterValue<Args>(PropertyContexts[Indices])...);
	}

	template<typename Lambda, size_t... Indices>
	static auto CreatePropertyWrapperFromLambdaHelper(Lambda&& InLambda, std::index_sequence<Indices...>)
	{
		return CreatePropertyWrapper<std::decay_t<TLambdaArgType<std::decay_t<Lambda>, Indices>>...>(std::forward<Lambda>(InLambda));
	}

	template<typename T>
	static T ExtractParameterValue(const FPropertyContext& Context)
	{
		if (!Context.Property || !Context.PropertyPtr)
		{
			FLogger::Get().LogWarning(TEXT("ExtractParameterValue - Invalid property context"));
			return T {};
		}

		if (!TPropertyMatcher<T>::DoesMatchProperty(Context.Property))
		{
			FLogger::Get().LogWarning(TEXT("ExtractParameterValue - Type mismatch: expected %s"), GetPropertyNameForType<T>());
			return T {};
		}

		if constexpr (std::is_pointer_v<T> && std::is_base_of_v<UObject, std::remove_pointer_t<T>>)
		{
			UObject* ObjectPtr = *static_cast<UObject* const*>(Context.PropertyPtr);
			return Cast<std::remove_pointer_t<T>>(ObjectPtr);
		}
		else if constexpr (std::is_enum_v<T>)
		{
			using UnderlyingType = std::underlying_type_t<T>;
			return static_cast<T>(*static_cast<const UnderlyingType*>(Context.PropertyPtr));
		}
		else
		{
			return *static_cast<const T*>(Context.PropertyPtr);
		}
	}

	template<typename T>
	static FProperty* CreateBasicProperty(const UObject* Context, const FString& PropertyName)
	{
		if constexpr (!std::is_void_v<typename TTypeToPropertyType<T>::PropertyType>)
		{
			using PropertyType = typename TTypeToPropertyType<T>::PropertyType;
			PropertyType* Property = new PropertyType(Context->GetClass(), *PropertyName, RF_Public);

			EPropertyFlags ComputedFlags = CPF_None;

			if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
			{
				ComputedFlags |= CPF_IsPlainOldData | CPF_NoDestructor;
			}

			if constexpr (TIsZeroConstructType<T>::Value)
			{
				ComputedFlags |= CPF_ZeroConstructor;
			}

			if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T> ||
			              std::is_same_v<T, FString> || std::is_same_v<T, FName> || std::is_same_v<T, FText>)
			{
				ComputedFlags |= CPF_HasGetValueTypeHash;
			}

			Property->SetPropertyFlags(Property->GetPropertyFlags() | ComputedFlags);
			return Property;
		}
		else
		{
			return nullptr;
		}
	}

	template<typename T>
	static FProperty* CreateEnumProperty(const UObject* Context, const FString& PropertyName)
	{
		static_assert(std::is_enum_v<T>, "T must be an enum type");

		UEnum* EnumClass = StaticEnum<T>();
		if (EnumClass && IsValid(EnumClass))
		{
			FEnumProperty* EnumProperty = new FEnumProperty(Context->GetClass(), *PropertyName, RF_Public);
			EnumProperty->SetEnum(EnumClass);

			if constexpr (sizeof(T) == sizeof(uint8))
			{
				FByteProperty* UnderlyingProp = new FByteProperty(EnumProperty, TEXT("UnderlyingType"), RF_Public);
				EnumProperty->AddCppProperty(UnderlyingProp);
			}
			else if constexpr (sizeof(T) == sizeof(uint16))
			{
				FUInt16Property* UnderlyingProp = new FUInt16Property(EnumProperty, TEXT("UnderlyingType"), RF_Public);
				EnumProperty->AddCppProperty(UnderlyingProp);
			}
			else if constexpr (sizeof(T) == sizeof(uint64))
			{
				FUInt64Property* UnderlyingProp = new FUInt64Property(EnumProperty, TEXT("UnderlyingType"), RF_Public);
				EnumProperty->AddCppProperty(UnderlyingProp);
			}
			else
			{
				FUInt32Property* UnderlyingProp = new FUInt32Property(EnumProperty, TEXT("UnderlyingType"), RF_Public);
				EnumProperty->AddCppProperty(UnderlyingProp);
			}

			FArchive DummyArchive;
			EnumProperty->Link(DummyArchive);
			return EnumProperty;
		}

		return CreateBasicProperty<T>(Context, PropertyName);
	}

	template<typename T>
	static FProperty* CreateStructProperty(const UObject* Context, const FString& PropertyName)
	{
		if constexpr (!std::is_void_v<typename TTypeToPropertyType<T>::PropertyType>)
		{
			UScriptStruct* StructType = GetUniversalStruct<T>();
			if (!StructType)
			{
				return nullptr;
			}

			FStructProperty* Property = new FStructProperty(Context->GetClass(), *PropertyName, RF_Public);
			Property->Struct = StructType;
			if constexpr (THasBaseStructure<T>::value)
			{
				Property->SetElementSize(sizeof(T));
			}
			else if constexpr (THasStaticStruct<T>::value)
			{
				Property->SetElementSize(StructType->GetStructureSize());
			}

			return Property;
		}
		else
		{
			return nullptr;
		}
	}

	template<typename T>
	static FProperty* CreateContainerProperty(const UObject* Context, const FString& PropertyName, const T& ParamValue)
	{
		if constexpr (TIsTArray<T>::Value)
		{
			using ElementType = typename T::ElementType;
			FArrayProperty* Property = new FArrayProperty(FFieldVariant(Context->GetClass()),
			                                              *PropertyName,
			                                              RF_Public);
			FProperty* ElementProperty = nullptr;

			if (ParamValue.Num() > 0)
			{
				ElementProperty = GetPropertyForType<ElementType>(Context, ParamValue[0]);
			}
			else
			{
				ElementType DefaultElement {};
				ElementProperty = GetPropertyForType<ElementType>(Context, DefaultElement);
			}

			if (!ElementProperty)
			{
				delete Property;
				return nullptr;
			}

			Property->AddCppProperty(ElementProperty);
			FArchive DummyArchive;
			Property->Link(DummyArchive);

			return Property;
		}
		else if constexpr (TIsTSet<T>::Value)
		{
			using ElementType = typename T::ElementType;
			FSetProperty* Property = new FSetProperty(FFieldVariant(Context->GetClass()),
			                                          *PropertyName,
			                                          RF_Public);
			FProperty* ElementProperty = nullptr;
			if (ParamValue.Num() > 0)
			{
				auto Iterator = ParamValue.CreateConstIterator();
				ElementProperty = GetPropertyForType<ElementType>(Context, *Iterator);
			}
			else
			{
				ElementType DefaultElement {};
				ElementProperty = GetPropertyForType<ElementType>(Context, DefaultElement);
			}

			if (!ElementProperty)
			{
				delete Property;
				return nullptr;
			}

			ElementProperty->SetPropertyFlags(ElementProperty->GetPropertyFlags() | CPF_HasGetValueTypeHash);
			Property->AddCppProperty(ElementProperty);

			FArchive DummyArchive;
			Property->Link(DummyArchive);
			return Property;
		}
		else if constexpr (TIsTMap<T>::Value)
		{
			using KeyType = typename T::KeyType;
			using ValueType = typename T::ValueType;

			FMapProperty* Property = new FMapProperty(FFieldVariant(Context->GetClass()), *PropertyName, RF_Public);

			FProperty* KeyProperty = nullptr;
			FProperty* ValueProperty = nullptr;

			if (ParamValue.Num() > 0)
			{
				auto Iterator = ParamValue.CreateConstIterator();
				KeyProperty = GetPropertyForType<KeyType>(Context, Iterator.Key());
				ValueProperty = GetPropertyForType<ValueType>(Context, Iterator.Value());
			}
			else
			{
				// Create default properties for empty containers
				KeyType DefaultKey {};
				ValueType DefaultValue {};
				KeyProperty = GetPropertyForType<KeyType>(Context, DefaultKey);
				ValueProperty = GetPropertyForType<ValueType>(Context, DefaultValue);
			}

			if (!KeyProperty || !ValueProperty)
			{
				delete Property;
				return nullptr;
			}

			// Make sure the Key attribute has a hash flag
			KeyProperty->SetPropertyFlags(KeyProperty->GetPropertyFlags() | CPF_HasGetValueTypeHash);

			Property->AddCppProperty(KeyProperty);
			Property->AddCppProperty(ValueProperty);

			FArchive DummyArchive;
			Property->Link(DummyArchive);

			return Property;
		}
		else
		{
			// It shouldn't arrive here, because template specialization should handle all situations
			return nullptr;
		}
	}

	template<typename T, typename Lambda>
	static void ExtractAndInvokeValue(const void* PropertyPtr, Lambda&& InLambda);

	template<typename T>
	static constexpr const TCHAR* GetBasicTypePropertyName()
	{
		if constexpr (std::is_same_v<T, bool>)
			return TEXT("BoolValue");
		else if constexpr (std::is_same_v<T, int8>)
			return TEXT("Int8Value");
		else if constexpr (std::is_same_v<T, uint8>)
			return TEXT("UInt8Value");
		else if constexpr (std::is_same_v<T, int16>)
			return TEXT("Int16Value");
		else if constexpr (std::is_same_v<T, uint16>)
			return TEXT("UInt16Value");
		else if constexpr (std::is_same_v<T, int32>)
			return TEXT("IntValue");
		else if constexpr (std::is_same_v<T, uint32>)
			return TEXT("UInt32Value");
		else if constexpr (std::is_same_v<T, int64>)
			return TEXT("Int64Value");
		else if constexpr (std::is_same_v<T, uint64>)
			return TEXT("UInt64Value");
		else if constexpr (std::is_same_v<T, float>)
			return TEXT("FloatValue");
		else if constexpr (std::is_same_v<T, double>)
			return TEXT("DoubleValue");
		else if constexpr (std::is_same_v<T, FString>)
			return TEXT("StringValue");
		else if constexpr (std::is_same_v<T, FName>)
			return TEXT("NameValue");
		else if constexpr (std::is_same_v<T, FText>)
			return TEXT("TextValue");
		else
			checkf(false, TEXT("UnknownValue"));
		return TEXT("UnknownValue");
	}

public:
	template<typename T>
	static constexpr const TCHAR* GetPropertyNameForType()
	{
		if constexpr (std::is_enum_v<T>)
			return TEXT("EnumValue");
		else if constexpr (TIsStructSupported<T>::value && std::is_class_v<T> && !std::is_pointer_v<T>)
			return TEXT("StructValue");
		else if constexpr (std::is_pointer_v<T> && std::is_base_of_v<UObject, std::remove_pointer_t<T>>)
			return TEXT("ObjectValue");
		else if constexpr (TIsTArray<T>::Value)
			return TEXT("ArrayValue");
		else if constexpr (TIsTSet<T>::Value)
			return TEXT("SetValue");
		else if constexpr (TIsTMap<T>::Value)
			return TEXT("MapValue");
		else
			return GetBasicTypePropertyName<T>();
	}
};

template<typename T>
constexpr bool FGameEventPropertyHelper::IsTypeSupported()
{
	return TIsPropertyTypeSupported<T>::value;
}

template<typename T>
FProperty* FGameEventPropertyHelper::GetPropertyForType(const UObject* Context, const T& PropertyPtr)
{
	if (!Context)
	{
		return nullptr;
	}

	if constexpr (!std::is_void_v<typename TTypeToPropertyType<T>::PropertyType>)
	{
		using PropertyType = typename TTypeToPropertyType<T>::PropertyType;
		const FString PropertyName = GetPropertyNameForType<T>();

		if constexpr (std::is_same_v<PropertyType, FStructProperty>)
		{
			return CreateStructProperty<T>(Context, PropertyName);
		}
		else if constexpr (std::is_same_v<PropertyType, FObjectProperty>)
		{
			FObjectProperty* Property = new FObjectProperty(FFieldVariant(Context->GetClass()), *PropertyName, RF_Public);
			Property->SetPropertyClass(std::remove_pointer_t<T>::StaticClass());
			return Property;
		}
		else if constexpr (std::is_same_v<PropertyType, FArrayProperty> ||
		                   std::is_same_v<PropertyType, FSetProperty> ||
		                   std::is_same_v<PropertyType, FMapProperty>)
		{
			return CreateContainerProperty<T>(Context, PropertyName, PropertyPtr);
		}
		else if constexpr (std::is_enum_v<T>)
		{
			return CreateEnumProperty<T>(Context, PropertyName);
		}
		else
		{
			return CreateBasicProperty<T>(Context, PropertyName);
		}
	}
	else
	{
		return nullptr;
	}
}

template<typename T, typename Lambda>
void FGameEventPropertyHelper::ExtractAndInvokeValue(const void* PropertyPtr, Lambda&& InLambda)
{
	if constexpr (!std::is_void_v<typename TTypeToPropertyType<T>::PropertyType>)
	{
		using PropertyType = typename TTypeToPropertyType<T>::PropertyType;

		if constexpr (std::is_same_v<PropertyType, FObjectProperty>)
		{
			UObject* ObjectPtr = *static_cast<UObject* const*>(PropertyPtr);
			if constexpr (std::is_same_v<T, std::nullptr_t>)
			{
				InLambda(nullptr);
			}
			else
			{
				T CastedPtr = Cast<std::remove_pointer_t<T>>(ObjectPtr);
				InLambda(CastedPtr);
			}
		}
		else if constexpr (std::is_enum_v<T>)
		{
			using UnderlyingType = std::underlying_type_t<T>;
			T Value = static_cast<T>(*static_cast<const UnderlyingType*>(PropertyPtr));
			InLambda(Value);
		}
		else
		{
			T Value = *static_cast<const T*>(PropertyPtr);
			InLambda(Value);
		}
	}
	else
	{
		const T& Value = *static_cast<const T*>(PropertyPtr);
		InLambda(Value);
	}
}

template<typename... Args, typename Lambda>
auto FGameEventPropertyHelper::CreatePropertyWrapper(Lambda&& InLambda)
{
	return [InLambda](const FEventProperty& Property) mutable -> void
	{
		constexpr size_t ParamCount = sizeof...(Args);

		if constexpr (ParamCount == 0)
		{
			InLambda();
			return;
		}

		if (!Property.PropertyPtr)
		{
			FLogger::Get().LogWarning(TEXT("CreatePropertyWrapper - PropertyPtr is null"));
			return;
		}

		const TArray<FPropertyContext>* PropertyContexts = static_cast<const TArray<FPropertyContext>*>(Property.PropertyPtr);

		if (!PropertyContexts || PropertyContexts->Num() != ParamCount)
		{
			FLogger::Get().LogWarning(TEXT("CreatePropertyWrapper - Parameter count mismatch: expected %zu, got %d"),
			                          ParamCount,
			                          PropertyContexts ? PropertyContexts->Num() : 0);
			return;
		}

		ExtractAndInvokeVariadicLambda<Args...>(InLambda, *PropertyContexts, std::make_index_sequence<ParamCount> {});
	};
}

template<typename Lambda>
auto FGameEventPropertyHelper::CreatePropertyWrapperFromLambda(Lambda&& InLambda)
{
	return CreatePropertyWrapperFromLambdaHelper(
	                                             std::forward<Lambda>(InLambda),
	                                             std::make_index_sequence<TLambdaArgsCount<std::decay_t<Lambda>>> {}
	                                            );
}
