#ifndef XMetaObject_h__
#define XMetaObject_h__

#include <map>
#include <unordered_map>
#include <sol/sol.hpp>
#include "rttr\type.h"

class XObject;

// ʵ���޹ص�Ԫ����
struct XMetaObject {
public:
	XMetaObject();

	XObject* newInstance(std::vector<rttr::argument> args = {});	//����ʵ��

	rttr::property getProperty(std::string name);					//��ȡ��Ϊname��property

	rttr::array_range<rttr::property> getProperties();				//��ȡ�����͵�����property

	rttr::method getMethod(std::string name);						//��ȡ��Ϊname��method

	rttr::array_range<rttr::method> getMethods();					//��ȡ�����͵�����method

	virtual void registerLua(sol::state& lua) {};					//ע�ᵽlua

	virtual rttr::type getRttrType() { return rttr::type::get<void>(); };		//
};

#endif // XMetaObject_h__