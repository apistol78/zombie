#include <Core/Log/Log.h>
#include "Example.h"

using namespace traktor;

T_IMPLEMENT_RTTI_CLASS(L"Example", Example, traktor::Object)

void Example::sayHelloWorld()
{
	log::info << L"Hello World!" << Endl;
}
