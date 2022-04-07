#include "FileGenerator.h"
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <queue>

bool FileGenerator::generator(FileDataDef* data) {
	fileData = data;
	return fileData && generateSource();
}

bool FileGenerator::generateSource()
{
	FILE* out;
	std::filesystem::path outputPath(fileData->outputPath);

	if (fopen_s(&out, outputPath.string().c_str(), "w") != 0) {
		return false;
	}
	std::string header_path = std::regex_replace(std::filesystem::relative(fileData->inputFilePath, outputPath.parent_path()).string(), std::regex("\\\\"), "/");
	fprintf(out, "#include \"%s\"\n", header_path.c_str());					//生成当前cpp相较于h目录的include代码
	fprintf(out, "#include <rttr/registration>\n");
	fprintf(out, "#include <XMetaObject.h>\n");
	fprintf(out, "#include <Serialization/SerializationBriefSyntax.h>\n");

	fputs("\n", out);														

	for (auto classdef : fileData->classList) {
		generateAxonClass(out, classdef);
	}
	fputs("", out);

	generateGlobalData(out);											
	fclose(out);
	return true;
}

void FileGenerator::generateAxonClass(FILE* out, const ClassDef& def)
{
	//std::string base_class_list;
	//for (auto iter : def.superclassList) {
	//	if (!base_class_list.empty())
	//		base_class_list.push_back(',');
	//	base_class_list += iter.first;
	//}
	//fprintf(out, "template<>\n", base_class_list.c_str());
	//fprintf(out, "struct rttr::rttr_super_class<%s> {\n", def.classname.c_str());
	//fprintf(out, "    using base_class_list = rttr::type_list<%s>;\n", base_class_list.c_str());
	//fprintf(out, "};\n\n");

	fprintf(out, "struct %sXMetaObject: public XMetaObject{ \n", def.classname.c_str());
	fprintf(out, "    %sXMetaObject()  {\n", def.classname.c_str());
	fprintf(out, "        rttr::registration::class_<%s>(\"%s\")\n", def.classname.c_str(), def.classname.c_str());
	for (auto& func : def.functionList) {
		if (func.isAxonFunction) {
			if (func.isConstructor) {
				fprintf(out, "            .constructor<");
				for (int i = 0; i < func.arguments.size(); i++) {
					fprintf(out, "%s%s", (i ? "," : ""), func.arguments[i].type.name.c_str());
				}
				fprintf(out, ">()(rttr::policy::ctor::as_raw_ptr)\n");
				continue;
			}
			if (func.isDestructor)
				continue;
			std::string arguments;
			for (auto& argument : func.arguments) {
				if (!arguments.empty())
					arguments.push_back(',');
				arguments += argument.type.rawName;
			}
			if (arguments.empty())
				arguments = "void";
			fprintf(out, "            .method(\"%s\",rttr::select_overload<%s(%s)>(&%s::%s))\n", func.name.c_str(), func.returnType.rawName.c_str(), arguments.c_str(), def.classname.c_str(), func.name.c_str());
			if (!func.arguments.empty()) {
				fprintf(out, "            (\n");
				fprintf(out, "                rttr::parameter_names(");
				for (int i = 0; i < func.arguments.size(); i++) {
					if (i != 0)
						fprintf(out, ",");
					fprintf(out, "\"%s\"", func.arguments[i].name.c_str());
				}
				fprintf(out, ")\n");
				fprintf(out, "            )\n");
			}
		}
	}
	for (auto& property : def.propertyList) {
		if (!property.getter.empty() && !property.setter.empty()) {
			fprintf(out, "            .property(\"%s\", &%s::%s, &%s::%s, &%s::%s)\n", property.name.c_str(), def.classname.c_str(), property.name.c_str(), def.classname.c_str(), property.getter.c_str(), def.classname.c_str(), property.setter.c_str());
		}
		else {
			fprintf(out, "            .property(\"%s\", &%s::%s)\n", property.name.c_str(), def.classname.c_str(), property.name.c_str());
		}
	}

	for (auto& enumdef : def.enumList) {
		fprintf(out, "            .enumeration<%s::%s>(\"%s\")\n", def.classname.c_str(), enumdef.name.c_str(), enumdef.name.c_str());
		fprintf(out, "            (\n");
		for (int i = 0; i < enumdef.values.size(); i++) {
			if (i != 0)
				fprintf(out, ",\n");
			fprintf(out, "                rttr::value(\"%s\",%s::%s::%s)", enumdef.values[i].c_str(), def.classname.c_str(), enumdef.name.c_str(), enumdef.values[i].c_str());
		}
		fprintf(out, "\n            )\n");
	}
	fprintf(out, "        ;\n");
	fprintf(out, "        mType = rttr::type::get_by_name(\"%s\");\n", def.classname.c_str());
	for (int i = 0; i < def.superclassList.size(); i++) {
		fprintf(out, "        %s::staticMetaObject();\n", def.superclassList[i].first.c_str());
	}

	fprintf(out, "    }\n");
	fprintf(out, "\n");

	fprintf(out, "    void registerLua(sol::state &lua) override{ \n");
	for (auto& enumdef : def.enumList) {
		fprintf(out, "        static std::map<std::string,%s::%s> Static%s = { ", def.classname.c_str(), enumdef.name.c_str(), enumdef.name.c_str());
		for (int i = 0; i < enumdef.values.size(); i++) {
			if (i != 0)
				fprintf(out, ",");
			fprintf(out, "{\"%s\", %s::%s::%s}", enumdef.values[i].c_str(), def.classname.c_str(), enumdef.name.c_str(), enumdef.values[i].c_str());
		}
		fprintf(out, "};\n");
	}
	fprintf(out, "        lua.new_usertype<%s>(\"%s\"", def.classname.c_str(), def.classname.c_str());

	std::vector<FunctionDef> constructs;
	for (auto& func : def.functionList) {
		if (func.isAxonFunction && func.isConstructor)
			constructs.push_back(func);
	}
	if (!constructs.empty()) {
		fprintf(out, ",\n             sol::constructors<");
		for (int i = 0; i < constructs.size(); ++i) {
			if (i != 0)
				fprintf(out, ",");
			std::string arguments;
			for (auto& argument : constructs[i].arguments) {
				if (!arguments.empty())
					arguments.push_back(',');
				arguments += argument.type.rawName;
			}
			fprintf(out, "%s(%s)", def.classname.c_str(), arguments.c_str());
		}
		fprintf(out, ">()");
	}

	std::queue<std::string> superQueue;
	for (auto& iter : def.superclassList) {
		superQueue.push(iter.first);
	}
	if (!superQueue.empty()) {
		fprintf(out, ",\n             sol::base_classes, sol::bases<");
		bool flag = false;
		while (!superQueue.empty()) {
			if (flag)
				fprintf(out, ",");
			std::string top = superQueue.back();
			superQueue.pop();
			flag = true;
			fprintf(out, "%s", top.c_str());
			auto it = fileData->superclass.find(top);
			if (it != fileData->superclass.end()) {
				for (auto& iter : it->second) {
					superQueue.push(iter.first);
				}
			}
		}
		fprintf(out, ">()");
	}
	std::unordered_map<std::string, std::vector<FunctionDef>> overloadFunc;
	for (auto& func : def.functionList) {
		if (func.isAxonFunction && !func.isConstructor && !func.isDestructor)
			overloadFunc[func.name].push_back(func);
	}

	for (auto& funcs : overloadFunc) {
		const FunctionDef& first = funcs.second.front();
		fprintf(out, ",\n             \"%s\",sol::overload(", first.name.c_str());
		for (int i = 0; i < funcs.second.size(); ++i) {
			if (i != 0)
				fprintf(out, ",");
			std::string arguments;
			for (auto& argument : funcs.second[i].arguments) {
				if (!arguments.empty())
					arguments.push_back(',');
				arguments += argument.type.rawName;
			}
			fprintf(out, "static_cast<%s (%s*)(%s)>(&%s::%s)", first.returnType.name.c_str(), first.isStatic ? "" : (def.classname + "::").c_str(), arguments.c_str(), def.classname.c_str(), first.name.c_str());
		}
		fprintf(out, ")");
	}

	for (auto& property : def.propertyList) {
		fprintf(out, ",\n             \"%s\", ", property.name.c_str());
		if (property.getter.empty() && property.setter.empty()) {
			fprintf(out, "sol::readonly(&%s::%s)", def.classname.c_str(), property.name.c_str());
		}
		else if (!property.getter.empty() && !property.getter.empty()) {
			fprintf(out, "sol::property(&%s::%s, &%s::%s)", def.classname.c_str(), property.getter.c_str(), def.classname.c_str(), property.setter.c_str());
		}
		else if (!property.getter.empty()) {
			fprintf(out, "sol::property(&%s::%s)", def.classname.c_str(), property.getter.c_str());
		}
		else {
			fprintf(out, "sol::property(&%s::%s)", def.classname.c_str(), property.setter.c_str());
		}
	}

	for (auto& enumdef : def.enumList) {
		fprintf(out, ",\n              \"%s\",sol::var(&Static%s)", enumdef.name.c_str(), enumdef.name.c_str());
	}
	fprintf(out, "\n        );\n");
	fprintf(out, "    }\n");
	fprintf(out, "    rttr::type getRttrType() override{ return mType; } \n");
	fprintf(out, "    rttr::type mType = rttr::type::get<void>();\n");
	fprintf(out, "};\n\n");

	fprintf(out, "XMetaObject* %s::staticMetaObject() {\n", def.classname.c_str());
	fprintf(out, "    static %sXMetaObject metaObject;\n", def.classname.c_str());
	fprintf(out, "    return &metaObject;\n");
	fprintf(out, "}\n\n");

	fprintf(out, "XMetaObject* %s::metaObject() {\n", def.classname.c_str());
	fprintf(out, "    return %s::staticMetaObject();\n", def.classname.c_str());
	fprintf(out, "}\n\n");

	fprintf(out, "void %s::__intrusive_deserialize(Deserializer & deserializer){ \n", def.classname.c_str());
	for (auto& super : def.superclassList) {
		fprintf(out, "    %s::__intrusive_deserialize(deserializer);\n", super.first.c_str());
	}

	for (auto& property : def.propertyList) {
		fprintf(out, "    deserializer(%s);\n", property.name.c_str());
	}

	fprintf(out, "}\n\n");

	fprintf(out, "void %s::__intrusive_serialize(Serializer & serializer){ \n", def.classname.c_str());
	for (auto& super : def.superclassList) {
		fprintf(out, "    %s::__intrusive_serialize(serializer);\n", super.first.c_str());
	}
	for (auto& property : def.propertyList) {
		fprintf(out, "    serializer(%s);\n", property.name.c_str());
	}
	fprintf(out, "}\n\n");

	fprintf(out, "void %s::__intrusive_to_json(nlohmann::json& json) const { \n", def.classname.c_str());
	for (auto& super : def.superclassList) {
		fprintf(out, "    %s::__intrusive_to_json(json);\n", super.first.c_str());
	}
	for (auto& property : def.propertyList) {
		fprintf(out, "    json[\"%s\"] = %s;\n", property.name.c_str(), property.name.c_str());
	}
	fprintf(out, "}\n\n");

	fprintf(out, "\n");
}

void FileGenerator::generateGlobalData(FILE* out)
{
	if (fileData->globaleEnumList.empty() && fileData->globalMethodList.empty())
		return;

	std::string fileName = std::filesystem::path(fileData->inputFilePath).stem().string();
	fprintf(out, "\n\n");
	fprintf(out, "struct %sFileMetaObject: public XMetaObject{ \n", fileName.c_str());
	fprintf(out, "    %sFileMetaObject() { \n", fileName.c_str());
	bool first = true;
	fprintf(out, "        rttr::registration");
	for (auto& method : fileData->globalMethodList) {
		if (method.isAxonFunction) {
			std::string arguments;
			for (auto& argument : method.arguments) {
				if (!arguments.empty())
					arguments.push_back(',');
				arguments += argument.type.rawName;
			}
			if (arguments.empty())
				arguments = "void";
			fprintf(out, "%smethod(\"%s\",rttr::select_overload<%s(%s)>(&%s))\n", first ? "::" : "            .", method.name.c_str(), method.returnType.rawName.c_str(), arguments.c_str(), method.name.c_str());
			first = false;
			if (!method.arguments.empty()) {
				fprintf(out, "            (\n");
				fprintf(out, "                rttr::parameter_names(");

				for (int i = 0; i < method.arguments.size(); i++) {
					if (i != 0)
						fprintf(out, ",");
					fprintf(out, "\"%s\"", method.arguments[i].name.c_str());
				}
				fprintf(out, ")\n");
				fprintf(out, "            )\n");
			}
		}
	}

	for (auto& enumdef : fileData->globaleEnumList) {
		fprintf(out, "%senumeration<%s>(\"%s\")\n", first ? "::" : "            .", enumdef.name.c_str(), enumdef.name.c_str());
		first = false;
		fprintf(out, "            (\n");
		for (int i = 0; i < enumdef.values.size(); i++) {
			if (i != 0)
				fprintf(out, ",\n");
			fprintf(out, "                rttr::value(\"%s\",%s::%s)", enumdef.values[i].c_str(), enumdef.name.c_str(), enumdef.values[i].c_str());
		}
		fprintf(out, "\n            )\n");
	}
	fprintf(out, "        ;\n");
	fprintf(out, "    }\n");
	fprintf(out, "\n");
	fprintf(out, "};\n");
}