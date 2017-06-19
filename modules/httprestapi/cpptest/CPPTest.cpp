#include "CPPTest.h"

void CPPTest::func()
{

}

void CPPTest::setCallback(add_publish_callback callback)
{
    m_callback = callback;
}

void CPPTest::notify(const char* message)
{
    m_callback((void*)this, message);
}

void* httpServer_Create(void* broker, const char* url, const char* port)
{
    CPPTest* instance = new CPPTest();
    return (void*)instance;
}
void httpServer_Start(void* handle)
{
    CPPTest* instance = (CPPTest*)handle;
    instance->func();
}

void httpServer_Stop(void* handle)
{

}

void httpServer_AddPublishCallback(void* handle, add_publish_callback callback)
{
    CPPTest* instance = (CPPTest*)handle;
    instance->setCallback(callback);
    instance->notify("hello");
}    
