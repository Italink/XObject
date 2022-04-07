#ifndef SerializationBriefSyntax_h__
#define SerializationBriefSyntax_h__

#include "SerializationDefine.h"
#include "rttr/variant.h"
#include "bitsery/brief_syntax.h"
#include "bitsery/brief_syntax/vector.h"
#include "bitsery/brief_syntax/string.h"
#include "bitsery/brief_syntax/array.h"
#include "bitsery/brief_syntax/map.h"
#include "bitsery/brief_syntax/unordered_map.h"

class IntrusiveAccess {
public:
	template<typename BasicJsonType, typename T>
	static auto __intrusive_to_json(BasicJsonType& js, T& obj) -> decltype(obj.__intrusive_to_json(js)) {
		obj.__intrusive_to_json(js);
	}
	template<typename BasicJsonType, typename T>
	static auto __intrusive_from_json(BasicJsonType& js, T& obj) -> decltype(obj.__intrusive_from_json(js)) {
		obj.__intrusive_from_json(js);
	}
	template<typename Seri, typename T>
	static auto __intrusive_deserialize(Deserializer& deserializer, T& obj) -> decltype(obj.__intrusive_deserialize(deserializer)) {
		obj.__intrusive_deserialize(deserializer);
	}
	template<typename Seri, typename T>
	static auto __intrusive_serialize(Serializer& serializer, T& obj) -> decltype(obj.__intrusive_serialize(serializer)) {
		obj.__intrusive_serialize(serializer);
	}
};

template <typename BasicJsonType, typename T>
using try_intrusive_to_json_ = decltype(IntrusiveAccess::__intrusive_to_json(std::declval<BasicJsonType&>(), std::declval<T&>()));

template <typename BasicJsonType, typename T>
struct has_intrusive_to_json_impl {
	template <typename Q, typename R, typename = try_intrusive_to_json_<Q, R>>
	static std::true_type tester(Q&&, R&&);
	static std::false_type tester(...);
	using type = decltype(tester(std::declval<BasicJsonType>(), std::declval<T>()));
};
template <typename BasicJsonType, typename T>
struct has_intrusive_to_json : has_intrusive_to_json_impl<BasicJsonType, T>::type {};

template <typename BasicJsonType, typename T, typename std::enable_if<has_intrusive_to_json<BasicJsonType, T>::value>::type* = nullptr>
void to_json(BasicJsonType& js, T var) {
	var.__intrusive_to_json(js);
}

template <typename BasicJsonType, typename T, typename std::enable_if<std::is_pointer<T>::value&& has_intrusive_to_json<BasicJsonType, std::remove_pointer_t<T>>::value>::type* = nullptr>
void to_json(BasicJsonType& js, T ptr) {
	if (ptr != nullptr)
		ptr->__intrusive_to_json(js);
}

template <typename BasicJsonType, typename T>
using try_intrusive_from_json = decltype(IntrusiveAccess::__intrusive_from_json(std::declval<BasicJsonType&>(), std::declval<T&>()));

template <typename BasicJsonType, typename T>
struct has_intrusive_from_json_impl {
	template <typename Q, typename R, typename = try_intrusive_from_json<Q, R>>
	static std::true_type tester(Q&&, R&&);
	static std::false_type tester(...);
	using type = decltype(tester(std::declval<BasicJsonType>(), std::declval<T>()));
};
template <typename BasicJsonType, typename T>
struct has_intrusive_from_json : has_intrusive_from_json_impl<BasicJsonType, T>::type {};

template <typename BasicJsonType, typename T, typename std::enable_if<has_intrusive_from_json<BasicJsonType, T>::value>::type* = nullptr>
void from_json(BasicJsonType& js, T& var) {
	var.__intrusive_from_json(js);
}

template<typename T, typename std::enable_if< std::is_class<T>::value&& rttr::detail::has_base_class_list<T>::value>::type* = nullptr>
void serialize(Serializer& serializer, T& ptr) {
	ptr.__intrusive_serialize(serializer);
}

template<typename T, typename std::enable_if<std::is_class<T>::value&& rttr::detail::has_base_class_list<T>::value>::type* = nullptr>
void serialize(Deserializer& deserializer, T& ptr) {
	ptr.__intrusive_deserialize(deserializer);
}

template<typename T, typename std::enable_if<std::is_pointer<T>::value && !rttr::detail::has_base_class_list<rttr::detail::raw_type_t<T>>::value>::type* = nullptr>
void serialize(Serializer& writer, T& ptr) {
	std::string typeName;
	if (ptr != nullptr) {
		rttr::type type = rttr::type::get<rttr::detail::raw_type_t<T>::type>();		//根据指针类型获取原始类型
		typeName = type.get_raw_type().get_name().to_string();
	}
	writer(typeName);					//写入类型
	if (ptr != nullptr) {
		writer(*ptr);					//如果指针不为空，写入该指针所指数据
	}
}

template<typename T, typename std::enable_if<std::is_pointer<T>::value&& rttr::detail::has_base_class_list<rttr::detail::raw_type_t<T>>::value>::type* = nullptr>
void serialize(Serializer& writer, T& ptr) {
	std::string typeName;
	if (ptr != nullptr) {
		rttr::type type = ptr->metaObject()->getRttrType();							//从metaObject中获取类型
		typeName = type.get_raw_type().get_name().to_string();
	}
	writer(typeName);					//写入类型
	if (ptr != nullptr) {
		writer(*ptr);					//如果指针不为空，写入该指针所指数据
	}	
}

template<typename T, typename std::enable_if<std::is_pointer<T>::value>::type* = nullptr>
void serialize(Deserializer& reader, T& ptr) {
	std::string typeName;				
	reader(typeName);					//读取类型
	if (typeName.empty())				//如果类型为空，则直接返回
		return;
	if (ptr != nullptr) {				//如果反序列所指的对象已不为空，则直接对其进行反序列化
		reader(*ptr);
		return;
	}
	rttr::type type = rttr::type::get_by_name(typeName);		//获取Rttr类型
	if (!type.is_valid())
		return;
	rttr::variant var = type.create();							//利用Rttr创建实例
	if (!var.can_convert<T>()) {								
		return;
	}
	ptr = var.get_value<T>();									//如果实例合法，则对其进行反序列化
	reader(*ptr);
}

#endif // SerializationBriefSyntax_h__