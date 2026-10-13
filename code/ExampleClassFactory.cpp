#include <Core/Class/AutoRuntimeClass.h>
#include <Core/Class/IRuntimeClassRegistrar.h>
#include "Example.h"
#include "ExampleClassFactory.h"

using namespace traktor;

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"ExampleClassFactory", 0, ExampleClassFactory, IRuntimeClassFactory)

void ExampleClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	auto classExample = new AutoRuntimeClass< Example >();
	classExample->addConstructor();
	classExample->addMethod("sayHelloWorld", &Example::sayHelloWorld);
	registrar->registerClass(classExample);
}
