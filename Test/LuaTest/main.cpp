#include "Test.h"
#include "rttr/type.h"
#include <sol/sol.hpp>
#include "rttr/detail/type/type_register_p.h"

int main() {
	sol::state lua;
	lua.open_libraries();
	Base::staticMetaObject()->registerLua(lua);	
	Derive::staticMetaObject()->registerLua(lua);	
	Derive1::staticMetaObject()->registerLua(lua);

	lua.script("print('Step0 : ���캯������Ա���������� ')");
	lua.script(" base0 = Base:new() \
				 base1 = Base:new(15) \
				 base1:print() \
				 base1:print(123) \
			   ");

	lua.script("print('\\nStep1 : ��ȡLua�б��������޸�')");
	Base* base1 = lua.get<Base*>("base1");
	base1->var = 789;

	Derive1 d;
	rttr::type t = rttr::type::get(base1);
	auto ps = t.get_properties();
	int size = ps.size();

	lua.script(" base1:print()");

	lua.script("print('\\nStep2 : ��Ա������������ std::vector')");
	lua.script("print('type',base1.std_vector)");
	lua.script("\
				   for i=1,#base1.std_vector do\
						print(base1.std_vector[i])\
				   end"
	);

	lua.script("print('\\nStep3 : ��Ա������������ std::unordered_map')");
	lua.script("print('type',base1.std_map)");
	lua.script("\
				   for key,value in base1.std_map:pairs() do\
						print(key,value)\
				   end"
	);

	lua.script("print('\\nStep4 : size��ʵ����size()������C++���Զ���Lua��#')");
	lua.script("print('Base::size:',#base1)");
	lua.script("print('Base::std_vector:size:',#base1.std_vector)");
	lua.script("print('Base::std_map:size:',#base1.std_map)");

	lua.script("print('\\nStep5 : ���������')");
	lua.script("print('base0:',base0.var)");
	lua.script("print('base1:',base1.var)");
	lua.script("print('base0 < base1',base0 < base1)");
	lua.script("print('base0 > base1',base0 > base1)");

	lua.script("print('\\nStep6 :ö��')");
	lua.script("print('Enum: Base.Alignment.Left   =',Base.Alignment.Left)");

	lua.script("print('\\nStep7 : �̳����')");
	lua.script("derive = Derive:new() \
					derive:print() \
					derive.local_base:print() \
				   ");

	lua.script("print('\\nStep8 : �����̳�')");
	lua.script("derive1 = Derive1:new() \
					derive1:print() \
					derive1.local_base:print() \
				   ");

	lua.script("print('\\nStep9 : Ƕ��')");
	Base base;
	lua["cppBase"] = &base;
	base.var = 15;
	lua.script("cppBase:print()");

	lua.script("print('\\nStep9 : ����Lua״̬�����������ڣ�����ȫ�ֱ���')");


	return 0;
}