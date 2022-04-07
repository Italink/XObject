#include "Test.h"
#include <iostream>
#include <vector>

int main() {
	Base base;

	/*调用无参成员函数*/
	base.invoke("print");

	/*调用有参成员函数*/
	rttr::variant var = base.invoke("add", { 1,7 });
	std::cout << var.to_int() << std::endl;

	/*通过实例获取属性*/
	std::vector<std::string> dataByInstance = base.getProperty("std_vector").get_value<std::vector<std::string>>();

	/*通过元对象获取属性*/
	rttr::property prop = Base::staticMetaObject()->getProperty("std_vector");
	std::vector<std::string> dataByMetaObject = prop.get_value(base).get_value<std::vector<std::string>>();
	assert(!dataByInstance.empty() && std::equal(dataByInstance.begin(), dataByInstance.end(), dataByMetaObject.begin()));

	Derive derive;

	derive.invoke("print");

	Base* local_base = derive.getProperty("local_base").get_value<Base*>();
	local_base->print();

	return 0;
}