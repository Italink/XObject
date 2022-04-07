#include "XMetaObject.h"
#include "XObject.h"

XMetaObject::XMetaObject() {
}

XObject* XMetaObject::newInstance(std::vector<rttr::argument> args /*= {}*/)
{
	return getRttrType().create(args).get_value<XObject*>();
}

rttr::property XMetaObject::getProperty(std::string name)
{
	return getRttrType().get_property(name);
}
rttr::array_range<rttr::property> XMetaObject::getProperties()
{
	return getRttrType().get_properties();
}

rttr::method XMetaObject::getMethod(std::string name)
{
	return getRttrType().get_method(name);
}

rttr::array_range<rttr::method> XMetaObject::getMethods()
{
	return getRttrType().get_methods();
}