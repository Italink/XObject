#ifndef XMetaObject_h__
#define XMetaObject_h__

#include <map>
#include <unordered_map>
#include <sol/sol.hpp>
#include "rttr\type.h"

class XObject;

// 实例无关的元数据
struct XMetaObject {
public:
	XMetaObject();

	XObject* newInstance(std::vector<rttr::argument> args = {});	//创建实例

	rttr::property getProperty(std::string name);					//获取名为name的property

	rttr::array_range<rttr::property> getProperties();				//获取该类型的所有property

	rttr::method getMethod(std::string name);						//获取名为name的method

	rttr::array_range<rttr::method> getMethods();					//获取该类型的所有method

	virtual void registerLua(sol::state& lua) {};					//注册到lua

	virtual rttr::type getRttrType() { return rttr::type::get<void>(); };		//
};

#endif // XMetaObject_h__
