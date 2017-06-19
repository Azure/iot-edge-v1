#ifndef _HTTP_REST_SERVER_H_
#define _HTTP_REST_SERVER_H_

#ifdef __cplusplus


typedef void(*add_publish_callback)(void* handle, const char* mbuf);

extern "C"
{
    void* httpServer_Create(void* broker, const char* url, const char* port);
    void httpServer_Start(void* handle);
    void httpServer_Stop(void* handle);
    void httpServer_AddPublishCallback(void* handle, add_publish_callback callback);    
}

class CPPTest
{
public:
    CPPTest() {;}
    ~CPPTest() {;}
    void func();
    void setCallback(add_publish_callback callback);
    void notify(const char* message);
private:
    add_publish_callback m_callback;
};

#endif

#endif /* _HTTP_REST_SERVER_H_ */
