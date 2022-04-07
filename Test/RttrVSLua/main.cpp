#include "Test.h"
#include "rttr/type.h"
#include "XMetaObject.h"
#include <sol/sol.hpp>
#include "rttr/detail/type/type_register_p.h"
#include "Utils.h"

struct Param {
	virtual rttr::variant get() = 0;
};

struct Node {
	std::vector<Param*> input;
	std::vector<rttr::variant> ouput;
};

int main() {
	{
		sol::state lua;
		lua.open_libraries();

		Base::staticMetaObject()->registerLua(lua);

		lua.script("instances = {} ");				//使用table存储实例，避免 GC

		//提前创建一遍，避免table扩容产生过多性能损耗
		lua.script(R"(for i=1,1000 do
							instances[i] = Base:new()
					 end)");

		TimeBlock(LuaConstruct,						//使用Lua创建Base实例
				  lua.script("for i=1,1000 do\
							instances[i] = Base:new()\
						end");
		);

		rttr::type tBase = rttr::type::get_by_name("Base");
		std::array<Base*, 1000> instances;
		TimeBlock(RTTRConstruct,					//使用Rttr创建Base实例
				  for (int i = 0; i < 1000; i++) {
					  instances[i] = tBase.create().get_value<Base*>();
				  }
		);

		TimeBlock(LuaInvokeStaticFunctionByCpp,		//在C++端使用Lua调用静态成员函数
				  for (int i = 0; i < 1000; i++) {
					  int ret = lua["Base"]["add"](10, 20);
				  }
		);

		TimeBlock(LuaInvokeStaticFunctionInline,	//使用Lua调用静态成员函数
				  lua.script("for i=1,1000 do \
				 ret = Base.add(10,20) \
			end");
		);

		rttr::method mAdd = tBase.get_method("add");
		TimeBlock(RTTRInvokeStaticFunction,			//使用Rttr调用静态成员函数
				  for (int i = 0; i < 1000; i++) {
					  int ret = mAdd.invoke(rttr::instance(), 10, 20).to_int();
				  }
		);

		TimeBlock(LuaInvokeGetter,					//使用Lua调用 Base::getVar(int)
				  lua.script("for i=1,1000 do\
							var = instances[i]:getVar(); \
						end");
		);

		rttr::method mGetter = tBase.get_method("getVar");
		TimeBlock(RTTRInvokeGetter,					//使用Rttr调用 Base::getVar(int)
				  for (int i = 0; i < 1000; i++) {
					  int ret = mGetter.invoke(instances[i]).to_int();
				  }
		);

		TimeBlock(LuaInvokeSetter,					//使用Lua调用 Base::setVar(int)
				  lua.script("for i=1,1000 do\
							instances[i]:setVar(10); \
					end");
		);

		rttr::method mSetter = tBase.get_method("setVar");
		rttr::variant var = 10;
		TimeBlock(RTTRInvokeSetter,					//使用Rttr调用 Base::setVar(int)
				  for (int i = 0; i < 1000; i++) {
					  mSetter.invoke(instances[i], var);
				  }
		);

		TimeBlock(RTTRInvokeContinuous,
				  Base* base = tBase.create().get_value<Base*>();		//创建实例
		rttr::variant sum = mAdd.invoke(rttr::instance(), 12, 41);							//调用Base::add(12,41)得到rttr::variant
		if (sum.can_convert(mSetter.get_parameter_infos().begin()->get_type())) {			//判断rttr::variant(int) 能否转化为Base::setVar 的第一个参数类型
			mSetter.invoke(base, sum);														//使用rttr::variant作为参数调用 base.setVar(int)
			rttr::variant ret = mGetter.invoke(base);										//调用base.getVar()->int 得到rttr::variant;
			printf("base:var : %d \n", ret.to_int());
		}
		);
	}
	return 0;
}