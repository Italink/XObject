#include <fstream>
#include "CmdLine.h"
#include "FileParser.h"
#include "SymbolParser.h"
#include "FileGenerator.h"

int main(int argc, char** argv) {
	cli::CmdParser parser(argc, argv);
	parser.set_default<std::string>(true, "File Path");																											 //输入文件的路径
	parser.set_optional<std::string >("o", "Output File Path", std::string(), "the output directory of generated files");										 //输出文件的路径
	parser.set_optional<std::vector<std::string>>("i", "include_dir", std::vector<std::string>(), "the include directories of input file");						 //输入文件的包含目录：输入文件的#include<>会在该目录下搜索代码文件并导入
	parser.run_and_exit_if_error();

	FileParser fileParser;
	SymbolParser symbolParser;
	FileGenerator fileGenerator;
	FileDataDef fileData;

	fileData.inputFilePath = parser.get<std::string>("");
	fileData.inputIncludeDir = parser.get<std::vector<std::string >>("i");
	fileData.outputPath = parser.get<std::string>("o");

	Symbols symbols = std::move(fileParser.preprocessed(fileData));		//解析文件，将代码文件转换为一系列的Token，包含include操作，宏展开

	if (!symbolParser.parse(fileData, symbols)) {						//解析符号，遍历符号集，简单还原类的层次结构，并搜集信息
		fprintf(stderr, "HeaderTool: warning: failed to parse the symbols of %s  \n", fileData.inputFilePath.c_str());
	}
	if (!fileGenerator.generator(&fileData)) {							//根据搜集到的信息，生成代码
		fprintf(stderr, "HeaderTool: warning: failed to generate %s\n", fileData.inputFilePath.c_str());
	}
	return 0;
}