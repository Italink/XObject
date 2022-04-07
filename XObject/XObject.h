#ifndef XObject_h__
#define XObject_h__

#include "XMetaObject.h"
#include "Serialization\SerializationDefine.h"

#define XENTRY(...) \
public: \
    static XMetaObject* staticMetaObject(); \
    virtual XMetaObject* metaObject() override; \
    virtual rttr::instance instance() override { return *this; } \
    using base_class_list = rttr::type_list<__VA_ARGS__>; \
	virtual void __intrusive_deserialize(Deserializer& deserializer) override; \
    virtual void __intrusive_serialize(Serializer& serializer) override; \
    virtual void __intrusive_to_json(nlohmann::json& json) const override; \
private:

#define XFUNCTION(...)
#define XPROPERTY(...)
#define XENUM(...)

#define DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

class XObject {
	DISABLE_COPY(XObject)
public:
	XObject() {}

	bool setProperty(std::string name, rttr::argument var);
	rttr::variant getProperty(std::string name);
	rttr::variant invoke(rttr::string_view name, std::vector<rttr::argument> args = {});

	static XMetaObject* staticMetaObject();		// 获取本类型的静态元对象，元对象提供一系列与实例无关的反射操作

	virtual XMetaObject* metaObject();			// 获取实例 实际类型的元对象

	std::string dump(const int indent = -1, const char indent_char = ' ', const bool ensure_ascii = false);

	SerializeBuffer serialize();
	std::pair<bitsery::ReaderError, bool> deserialize(SerializeBuffer& buffer);

	static XObject* createByData(SerializeBuffer& buffer);
public:
	/** 以下代码是为了兼容Rttr、Bitsery、的最小化接口*/
	virtual rttr::instance instance();
	virtual void __intrusive_deserialize(Deserializer& deserializer) {}	//用于序列化的自动绑定
	virtual void __intrusive_serialize(Serializer& serializer) {}
	virtual void __intrusive_to_json(nlohmann::json& json) const {}
	using base_class_list = rttr::type_list<>;		//用于Rttr的类型继承
};

#endif // XObject_h__