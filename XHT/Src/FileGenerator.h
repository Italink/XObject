#ifndef FileGenerator_h__
#define FileGenerator_h__

#include "DataDef.h"

class FileGenerator {
public:
	FileDataDef* fileData;
	bool generator(FileDataDef* data);
private:
	bool generateSource();
	void generateAxonClass(FILE* out, const ClassDef& classDef);
	void generateGlobalData(FILE* out);
};

#endif // 2FileGenerator_h__