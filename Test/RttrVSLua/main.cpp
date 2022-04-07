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

		lua.script("instances = {} ");				//ʹ��table�洢ʵ�������� GC

		//��ǰ����һ�飬����table���ݲ��������������
		lua.script(R"(for i=1,1000 do
							instances[i] = Base:new()
					 end)");

		TimeBlock(LuaConstruct,						//ʹ��Lua����Baseʵ��
				  lua.script("for i=1,1000 do\
							instances[i] = Base:new()\
						end");
		);

		rttr::type tBase = rttr::type::get_by_name("Base");
		std::array<Base*, 1000> instances;
		TimeBlock(RTTRConstruct,					//ʹ��Rttr����Baseʵ��
				  for (int i = 0; i < 1000; i++) {
					  instances[i] = tBase.create().get_value<Base*>();
				  }
		);

		TimeBlock(LuaInvokeStaticFunctionByCpp,		//��C++��ʹ��Lua���þ�̬��Ա����
				  for (int i = 0; i < 1000; i++) {
					  int ret = lua["Base"]["add"](10, 20);
				  }
		);

		TimeBlock(LuaInvokeStaticFunctionInline,	//ʹ��Lua���þ�̬��Ա����
				  lua.script("for i=1,1000 do \
				 ret = Base.add(10,20) \
			end");
		);

		rttr::method mAdd = tBase.get_method("add");
		TimeBlock(RTTRInvokeStaticFunction,			//ʹ��Rttr���þ�̬��Ա����
				  for (int i = 0; i < 1000; i++) {
					  int ret = mAdd.invoke(rttr::instance(), 10, 20).to_int();
				  }
		);

		TimeBlock(LuaInvokeGetter,					//ʹ��Lua���� Base::getVar(int)
				  lua.script("for i=1,1000 do\
							var = instances[i]:getVar(); \
						end");
		);

		rttr::method mGetter = tBase.get_method("getVar");
		TimeBlock(RTTRInvokeGetter,					//ʹ��Rttr���� Base::getVar(int)
				  for (int i = 0; i < 1000; i++) {
					  int ret = mGetter.invoke(instances[i]).to_int();
				  }
		);

		TimeBlock(LuaInvokeSetter,					//ʹ��Lua���� Base::setVar(int)
				  lua.script("for i=1,1000 do\
							instances[i]:setVar(10); \
					end");
		);

		rttr::method mSetter = tBase.get_method("setVar");
		rttr::variant var = 10;
		TimeBlock(RTTRInvokeSetter,					//ʹ��Rttr���� Base::setVar(int)
				  for (int i = 0; i < 1000; i++) {
					  mSetter.invoke(instances[i], var);
				  }
		);

		TimeBlock(RTTRInvokeContinuous,
				  Base* base = tBase.create().get_value<Base*>();		//����ʵ��
		rttr::variant sum = mAdd.invoke(rttr::instance(), 12, 41);							//����Base::add(12,41)�õ�rttr::variant
		if (sum.can_convert(mSetter.get_parameter_infos().begin()->get_type())) {			//�ж�rttr::variant(int) �ܷ�ת��ΪBase::setVar �ĵ�һ����������
			mSetter.invoke(base, sum);														//ʹ��rttr::variant��Ϊ�������� base.setVar(int)
			rttr::variant ret = mGetter.invoke(base);										//����base.getVar()->int �õ�rttr::variant;
			printf("base:var : %d \n", ret.to_int());
		}
		);
	}
	return 0;
}