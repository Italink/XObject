#include <fstream>
#include "CmdLine.h"
#include "FileParser.h"
#include "SymbolParser.h"
#include "FileGenerator.h"

int main(int argc, char** argv) {
	cli::CmdParser parser(argc, argv);
	parser.set_default<std::string>(true, "File Path");																											 //�����ļ���·��
	parser.set_optional<std::string >("o", "Output File Path", std::string(), "the output directory of generated files");										 //����ļ���·��
	parser.set_optional<std::vector<std::string>>("i", "include_dir", std::vector<std::string>(), "the include directories of input file");						 //�����ļ��İ���Ŀ¼�������ļ���#include<>���ڸ�Ŀ¼�����������ļ�������
	parser.run_and_exit_if_error();

	FileParser fileParser;
	SymbolParser symbolParser;
	FileGenerator fileGenerator;
	FileDataDef fileData;

	fileData.inputFilePath = parser.get<std::string>("");
	fileData.inputIncludeDir = parser.get<std::vector<std::string >>("i");
	fileData.outputPath = parser.get<std::string>("o");

	Symbols symbols = std::move(fileParser.preprocessed(fileData));		//�����ļ����������ļ�ת��Ϊһϵ�е�Token������include��������չ��

	if (!symbolParser.parse(fileData, symbols)) {						//�������ţ��������ż����򵥻�ԭ��Ĳ�νṹ�����Ѽ���Ϣ
		fprintf(stderr, "HeaderTool: warning: failed to parse the symbols of %s  \n", fileData.inputFilePath.c_str());
	}
	if (!fileGenerator.generator(&fileData)) {							//�����Ѽ�������Ϣ�����ɴ���
		fprintf(stderr, "HeaderTool: warning: failed to generate %s\n", fileData.inputFilePath.c_str());
	}
	return 0;
}