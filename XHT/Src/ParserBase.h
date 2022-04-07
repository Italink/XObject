#ifndef ParserBase_h__
#define ParserBase_h__

#include "Token.h"
#include "Symbols.h"

class ParserBase
{
public:
    ParserBase():mIndex(0), mDisplayWarnings(true), mDisplayNotes(true) {}
    Symbols mSymbols;
    int mIndex;
    bool mDisplayWarnings;
    bool mDisplayNotes;
    std::vector<std::string> mIncludeDir;
    std::stack<std::string, std::vector<std::string>> mCurrentFilenames;

    inline bool hasNext() const { return (mIndex < mSymbols.size()); }
    inline Token next() { if (mIndex >= mSymbols.size()) return NOTOKEN; return mSymbols.at(mIndex++).token; }
    inline Token peek() { if (mIndex >= mSymbols.size()) return NOTOKEN; return mSymbols.at(mIndex).token; }
    bool test(Token);
    void next(Token);
    void next(Token, const char* msg);
    inline void prev() { --mIndex; }
    inline Token lookup(int k = 1);
    inline const Symbol& symbol_lookup(int k = 1) { return mSymbols.at(mIndex - 1 + k); }
    inline Token token() { return mSymbols.at(mIndex - 1).token; }
    inline std::string lexem() { return mSymbols.at(mIndex - 1).lexem(); }
    inline std::string unquotedLexem() { return mSymbols.at(mIndex - 1).unquotedLexem(); }
    inline const Symbol& symbol() { return mSymbols.at(mIndex - 1); }

    void error(int rollback);
    void error(const char* msg = nullptr);
    void warning(const char* = nullptr);
    void note(const char* = nullptr);

};

inline bool ParserBase::test(Token token)
{
    if (mIndex < mSymbols.size() && mSymbols.at(mIndex).token == token) {
        ++mIndex;
        return true;
    }
    return false;
}

inline Token ParserBase::lookup(int k)
{
    const int l = mIndex - 1 + k;
    return l < mSymbols.size() ? mSymbols.at(l).token : NOTOKEN;
}

inline void ParserBase::next(Token token)
{
    if (!test(token))
        error();
}

inline void ParserBase::next(Token token, const char* msg)
{
    if (!test(token))
        error(msg);
}

#endif // ParserBase_h__


