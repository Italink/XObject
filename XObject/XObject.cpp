#include "XObject.h"
#include "rttr\registration.h"
#include "Serialization\SerializationBriefSyntax.h"

struct XObjectXMetaObject : public XMetaObject {
	XObjectXMetaObject() {
		rttr::registration::class_<XObject>("XObject");
		mType = rttr::type::get<XObject>();
	}

	void registerLua(sol::state& lua) override {
		lua.new_usertype<XObject>("XObject");
	}

	rttr::type mType = rttr::type::get<void>();

	rttr::type getRttrType() override {
		return mType;
	}
};

XMetaObject* XObject::staticMetaObject()
{
	static  XObjectXMetaObject metaObject;
	return &metaObject;
}

XMetaObject* XObject::metaObject() {
	return staticMetaObject();
}

rttr::instance XObject::instance() {
	return this;
}

std::string XObject::dump(const int indent /*= -1*/, const char indent_char /*= ' '*/, const bool ensure_ascii /*= false*/)
{
	nlohmann::json json;
	json[metaObject()->getRttrType().get_name().to_string()] = this;
	return json.dump(indent, indent_char, ensure_ascii);
}

SerializeBuffer XObject::serialize()
{
	SerializeBuffer buffer;
	Serializer ser{ std::move(buffer) };
	ser.object(this);
	ser.adapter().flush();
	buffer.resize(ser.adapter().writtenBytesCount());
	return buffer;
}

std::pair<bitsery::ReaderError, bool> XObject::deserialize(SerializeBuffer& buffer)
{
	InputAdapter input(buffer.begin(), buffer.size());
	Deserializer des{ std::move(input) };
	XObject* wapper = this;
	des.object(wapper);
	return { des.adapter().error(), des.adapter().isCompletedSuccessfully() };
}

XObject* XObject::createByData(SerializeBuffer& buffer)
{
	XObject* ptr = nullptr;
	InputAdapter input(buffer.begin(), buffer.size());
	Deserializer des{ std::move(input) };
	des.object(ptr);
	return ptr;
}

bool XObject::setProperty(std::string name, rttr::argument var)
{
	return metaObject()->getRttrType().set_property_value(name, instance(), var);
}

rttr::variant XObject::getProperty(std::string name) {
	return metaObject()->getRttrType().get_property_value(name, instance());
}

rttr::variant XObject::invoke(rttr::string_view name, std::vector<rttr::argument> args) {
	return metaObject()->getRttrType().invoke(name, instance(), args);
}