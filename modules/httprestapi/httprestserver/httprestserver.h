#ifndef _HTTP_REST_SERVER_H_
#define _HTTP_REST_SERVER_H_

#ifdef __cplusplus

#include <cpprest\http_listener.h>

using namespace web;
using namespace http;
using namespace utility;
using namespace http::experimental::listener;

typedef void(*add_publish_callback)(void* handle,const char* mac_address, const char* mbuf, size_t mbuf_size);

extern "C"
{
    void* httpServer_Create(void* moduleHandle, const char* url, const char* port);
    void httpServer_Start(void* handle);
    void httpServer_Stop(void* handle);
    void httpServer_AddPublishCallback(void* handle, add_publish_callback callback);    
}

class HttpRestServer
{
public:
    HttpRestServer() {}
    HttpRestServer(utility::string_t address, void* moduleHandle);

    pplx::task<void> open() { return m_listener.open(); }
    pplx::task<void> close() { return m_listener.close(); }

	void setPublishCallback(add_publish_callback callback) { m_callback = callback; }


private:

    void handle_get(http_request message);
    void handle_put(http_request message);
    void handle_post(http_request message);
    void handle_delete(http_request message);

    http_listener m_listener;
    void* m_moduleHandle;
    add_publish_callback m_callback;
};

#endif

#endif /* _HTTP_REST_SERVER_H_ */
