#ifndef Utils_h__
#define Utils_h__

#include <chrono>
#include <stdio.h>

namespace Time {
inline static long long now;
inline long long tick() {
	long long current = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	long long ret = current - now;
	now = current;
	return ret;
}
inline void printTick() {
	printf("%d\n", tick());
}
struct Helper {
	Helper() {
		Time::tick();
	}
	~Helper() {
		Time::printTick();
	}
};
};

void* operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line) {
	return operator new(size);
}

void* operator new[](size_t size, size_t aligment, size_t alignmentOffset, const char* name, int flags, unsigned debugFlags, const char* file, int line) {
	return operator new(size);
}

#define TimeBlock(Type, Code) \
	{\
		Time::Helper* helper = new Time::Helper; \
		printf( #Type" Begin \n"); \
		Code \
		printf( "Cost : "); \
		delete helper; \
		printf( #Type" End \n\n"); \
     }

#endif // Utils_h__
