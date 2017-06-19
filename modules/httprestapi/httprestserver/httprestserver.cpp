#include "httprestserver.h"

using namespace Concurrency::streams;

void* httpServer_Create(void* moduleHandle, const char* url, const char* port)
{
	std::string p(port);
	std::string addr(url);
	utility::string_t portNo = utility::conversions::to_string_t(p);
	utility::string_t address = utility::conversions::to_string_t(addr);
	address.append(U(":"));
	address.append(portNo);
    HttpRestServer* instance = new HttpRestServer(address, moduleHandle);
    instance->open().wait();

    return instance;
}

void httpServer_Start(void* handle)
{
	HttpRestServer* instance = (HttpRestServer*)handle;

    instance->open().wait();
}

void httpServer_Stop(void* handle)
{
    HttpRestServer* instance = (HttpRestServer*)handle;

    instance->close().wait();
}

void httpServer_AddPublishCallback(void* handle, add_publish_callback callback)
{
    HttpRestServer* instance = (HttpRestServer*)handle;

	instance->setPublishCallback(callback);
}


HttpRestServer::HttpRestServer(utility::string_t address, void* handle) : m_listener(address)
{
    m_moduleHandle = handle;
    m_listener.support(methods::GET, std::bind(&HttpRestServer::handle_get, this, std::placeholders::_1));
    m_listener.support(methods::PUT, std::bind(&HttpRestServer::handle_put, this, std::placeholders::_1));
	m_listener.support(methods::POST, std::bind(&HttpRestServer::handle_post, this, std::placeholders::_1));
	m_listener.support(methods::DEL, std::bind(&HttpRestServer::handle_delete, this, std::placeholders::_1));
}

void HttpRestServer::handle_get(http_request message)
{
	// messageのto_string()をコールすると、クライアントから送信されたBodyのデータが取得可能
	utility::string_t body = message.to_string();
	utility::string_t response = U("hello cpp rest sdk.");

	// クライアントへの応答。ステータスとContentの返送サンプル
    message.reply(status_codes::OK, response);

    return;
}

void HttpRestServer::handle_put(http_request message)
{
	// messageのto_string()をコールすると、クライアントから送信されたBodyのデータが取得可能
	utility::string_t body = message.to_string();
	utility::string_t response = U("hello cpp rest sdk.");

	utility::string_t content = U("{");

	// REST の階層（PATH）を取り出すサンプルコード
    auto paths = http::uri::split_path(http::uri::decode(message.relative_uri().path()));
	content.append(U("\"paths\":\""));
    for(auto i=paths.begin();i!=paths.end();++i)
    {
		utility::string_t path = *i;
		content.append(U("/"));
		content.append(path);
    }
	utility::string_t macAddress = *(paths.begin());
	content.append(U("\""));

	int p = 0;
	content.append(U(",\"parameters\":["));
	// REST で渡されたパラメータを取り出すサンプルコード - /rest?x=1&y=abc
    auto queries = message.relative_uri().split_query(message.request_uri().query());
	for (auto q = queries.begin(); q != queries.end(); ++q)
	{
		if (p++ > 0)
		{
			content.append(U(","));
		}
		utility::string_t key = q->first;
		utility::string_t value = q->second;
		ucout << "key=" << key.c_str() << ",value=" << value.c_str() << std::endl;
		content.append(U("{\"name\":\""));
		content.append(key);
		content.append(U("\",\"value\":\""));
		content.append(value);
		content.append(U("\"}"));
	}
	content.append(U("]"));
	content.append(U(",\"body\":\""));
	content.append(body);
	content.append(U("\"}"));
	
	
 	std::string macs = utility::conversions::to_utf8string(macAddress.c_str());
	std::string cs = utility::conversions::to_utf8string(content.c_str());
	
	m_callback(this->m_moduleHandle, macs.c_str(), cs.c_str(), strlen(cs.c_str()+1));
	
	// クライアントへの応答。ステータスとContentの返送サンプル
	message.reply(status_codes::OK, response);
    return;
}

void HttpRestServer::handle_post(http_request message)
{

}

void HttpRestServer::handle_delete(http_request message)
{

}
