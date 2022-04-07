#include "ParserBase.h"

#include "Utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static const char* error_msg = nullptr;

void ParserBase::error(int rollback) {
    mIndex -= rollback;
    error();
}

void ParserBase::error(const char* msg) {
    if (msg || error_msg)
        fprintf(stderr, ErrorFormatString "error: %s\n",
            mCurrentFilenames.top().c_str(), symbol().lineNum, 1, msg ? msg : error_msg);
    else
        fprintf(stderr, ErrorFormatString "error: Parse error at \"%s\"\n",
            mCurrentFilenames.top().c_str(), symbol().lineNum, 1, symbol().lexem().data());
    exit(EXIT_FAILURE);
}

void ParserBase::warning(const char* msg) {
    if (mDisplayWarnings && msg)
        fprintf(stderr, ErrorFormatString "warning: %s\n",
            mCurrentFilenames.top().c_str(), std::max(0, mIndex > 0 ? symbol().lineNum : 0), 1, msg);
}

void ParserBase::note(const char* msg) {
    if (mDisplayNotes && msg)
        fprintf(stderr, ErrorFormatString "note: %s\n",
            mCurrentFilenames.top().c_str(), std::max(0, mIndex > 0 ? symbol().lineNum : 0), 1, msg);
}

