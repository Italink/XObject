#ifndef Preprocessor_h__
#define Preprocessor_h__

#include "Symbols.h"
#include "ParserBase.h"
#include "DataDef.h"
#include <unordered_map>
#include <map>

struct Macro
{
    Macro() : isFunction(false), isVariadic(false) {}
    bool isFunction;
    bool isVariadic;
    Symbols arguments;
    Symbols symbols;
};

typedef std::unordered_map<std::string, Macro> Macros;

class FileParser : public ParserBase
{
public:
    FileParser();
    Symbols preprocessed(const FileDataDef& fileData);
    Macros mMacros;   
private:
    std::set<std::string> preprocessedIncludes;
    static bool preprocessOnly;
    std::unordered_map<std::string, std::string> nonlocalIncludePathResolutionCache;
    std::string resolveInclude(const std::string& filename, const std::string& relativeTo);
    void parseDefineArguments(Macro* m);
    void skipUntilEndif();
    bool skipBranch();
    void substituteUntilNewline(Symbols& substituted);
    static Symbols macroExpandIdentifier(FileParser* that, SymbolStack& symbols, int lineNum, std::string* macroName);
    static void macroExpand(Symbols* into, FileParser* that, const Symbols& toExpand, int& index, int lineNum, bool one,
        const std::set<std::string>& excludeSymbols = std::set<std::string>());
    int evaluateCondition();

    enum TokenizeMode { TokenizeCpp, TokenizePreprocessor, PreparePreprocessorStatement, TokenizePreprocessorStatement, TokenizeInclude, PrepareDefine, TokenizeDefine };
    static Symbols tokenize(const std::string& input, int lineNum = 1, TokenizeMode mode = TokenizeCpp);
private:
    void until(Token);
    void preprocess(const std::string& filename, Symbols& preprocessed);
};

#endif // Preprocessor_h__

