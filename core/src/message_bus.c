// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <stddef.h>
#include <signal.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/condition.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/refcount.h"
#include "azure_c_shared_utility/list.h"
#include "azure_c_shared_utility/uniqueid.h"

#include "nn.h"
#include "pubsub.h"

#include "message.h"
#include "module.h"
#include "message_bus.h"

#define MESSAGE_BUS_GUID_SIZE            37
#define INPROC_URL_HEAD "inproc://"
#define INPROC_URL_HEAD_SIZE  9
#define URL_SIZE (INPROC_URL_HEAD_SIZE + MESSAGE_BUS_GUID_SIZE +1)

/*The message bus implementation shall use the following definition as the backing structure for the message bus handle*/
typedef struct MESSAGE_BUS_HANDLE_DATA_TAG
{
    LIST_HANDLE                modules;
    LOCK_HANDLE             modules_lock;
	int                     publish_socket;
	STRING_HANDLE           url;
}MESSAGE_BUS_HANDLE_DATA;

DEFINE_REFCOUNT_TYPE(MESSAGE_BUS_HANDLE_DATA);

typedef struct MESSAGE_BUS_MODULEINFO_TAG
{
    /**
    * Handle to the module that's connected to the bus.
    */
    MODULE*             module;

    /**
    * Handle to the thread on which this module's message processing loop is
    * running.
    */
    THREAD_HANDLE           thread;

	/**
	* Socket this module will receive messages on.
	*/
	int                     receive_socket;
	/**
	* lock to prevent nanomsg race condition
	*/
	LOCK_HANDLE				socket_lock;
	/**
	* guid sent to moduel worker thread to close task.
	*/
	STRING_HANDLE			quit_message_guid;

}MESSAGE_BUS_MODULEINFO;

// This variable is unused in this library, but is defined in the header file.
size_t BUS_offsetof_quit_worker = 0;

static STRING_HANDLE construct_url()
{
	STRING_HANDLE result;

	/*Codes_SRS_MESSAGE_BUS_17_002: [ MessageBus_Create shall create a unique id. ]*/
	char uuid[MESSAGE_BUS_GUID_SIZE];
	memset(uuid, 0, MESSAGE_BUS_GUID_SIZE);
	if (UniqueId_Generate(uuid, MESSAGE_BUS_GUID_SIZE) != UNIQUEID_OK)
	{
		LogError("Unable to generate unique Id.");
		result = NULL;
	}
	else
	{
		/*Codes_SRS_MESSAGE_BUS_17_003: [ MessageBus_Create shall initialize a url consisting of "inproc://" + unique id. ]*/
		result = STRING_construct(INPROC_URL_HEAD);
		if (result == NULL)
		{
			LogError("Unable to construct url.");
		}
		else
		{
			if (STRING_concat(result, uuid) != 0)
			{
				/*Codes_SRS_MESSAGE_BUS_13_003: [ This function shall return NULL if an underlying API call to the platform causes an error. ]*/
				STRING_delete(result);
				LogError("Unable to append uuid to url.");
				result = NULL;
			}
		}
	}
	return result;
}

MESSAGE_BUS_HANDLE MessageBus_Create(void)
{
    MESSAGE_BUS_HANDLE_DATA* result;

    /*Codes_SRS_MESSAGE_BUS_13_067: [MessageBus_Create shall malloc a new instance of MESSAGE_BUS_HANDLE_DATA and return NULL if it fails.]*/
    result = REFCOUNT_TYPE_CREATE(MESSAGE_BUS_HANDLE_DATA);
    if (result == NULL)
    {
        LogError("malloc returned NULL");
        /*return as is*/
    }
    else
    {
        /*Codes_SRS_MESSAGE_BUS_13_007: [MessageBus_Create shall initialize MESSAGE_BUS_HANDLE_DATA::modules with a valid VECTOR_HANDLE.]*/
        result->modules = list_create();
        if (result->modules == NULL)
        {
            /*Codes_SRS_MESSAGE_BUS_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]*/
            LogError("VECTOR_create failed");
            free(result);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_MESSAGE_BUS_13_023: [MessageBus_Create shall initialize MESSAGE_BUS_HANDLE_DATA::modules_lock with a valid LOCK_HANDLE.]*/
            result->modules_lock = Lock_Init();
            if (result->modules_lock == NULL)
            {
                /*Codes_SRS_MESSAGE_BUS_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]*/
                LogError("Lock_Init failed");
                list_destroy(result->modules);
                free(result);
                result = NULL;
            }
			else
			{
				/*Codes_SRS_MESSAGE_BUS_17_001: [ MessageBus_Create shall initialize a socket for publishing messages. ]*/
				result->publish_socket = nn_socket(AF_SP, NN_PUB);
				if (result->publish_socket < 0)
				{
					/*Codes_SRS_MESSAGE_BUS_13_003: [ This function shall return NULL if an underlying API call to the platform causes an error. ]*/
					LogError("nanomsg puclish socket create failedL %d", result->publish_socket);
					list_destroy(result->modules);
					Lock_Deinit(result->modules_lock);
					free(result);
					result = NULL;
				}
				else
				{
					result->url = construct_url();
					if (result->url == NULL)
					{
						/*Codes_SRS_MESSAGE_BUS_13_003: [ This function shall return NULL if an underlying API call to the platform causes an error. ]*/
						list_destroy(result->modules);
						Lock_Deinit(result->modules_lock);
						nn_close(result->publish_socket);
						free(result);
						LogError("Unable to generate unique url.");
						result = NULL;
					}
					else
					{
						/*Codes_SRS_MESSAGE_BUS_17_004: [ MessageBus_Create shall bind the socket to the MESSAGE_BUS_HANDLE_DATA::url. ]*/
						if (nn_bind(result->publish_socket, STRING_c_str(result->url)) < 0)
						{
							/*Codes_SRS_MESSAGE_BUS_13_003: [ This function shall return NULL if an underlying API call to the platform causes an error. ]*/
							LogError("nanomsg bind failed");
							list_destroy(result->modules);
							Lock_Deinit(result->modules_lock);
							nn_close(result->publish_socket);
							STRING_delete(result->url);				
							free(result);
							result = NULL;
						}
					}
				}
			}
        }
    }

    /*Codes_SRS_MESSAGE_BUS_13_001: [This API shall yield a MESSAGE_BUS_HANDLE representing the newly created message bus.This handle value shall not be equal to NULL when the API call is successful.]*/
    return result;
}

void MessageBus_IncRef(MESSAGE_BUS_HANDLE bus)
{
    /*Codes_SRS_MESSAGE_BUS_13_108: [If `bus` is NULL then MessageBus_IncRef shall do nothing.]*/
    if (bus == NULL)
    {
        LogError("invalid arg: bus is NULL");
    }
    else
    {
        /*Codes_SRS_MESSAGE_BUS_13_109: [Otherwise, MessageBus_IncRef shall increment the internal ref count.]*/
        INC_REF(MESSAGE_BUS_HANDLE_DATA, bus);
    }
}

/**
* This is the worker function that runs for each module. The module_publish_worker
* function is passed in a pointer to the relevant MODULE_INFO object as it's
* thread context parameter. The function's job is to basically wait on the
* mq_cond condition variable and process messages in module.mq when the
* condition is signaled.
*/
static int module_publish_worker(void * user_data)
{
    /*Codes_SRS_MESSAGE_BUS_13_026: [This function shall assign `user_data` to a local variable called `module_info` of type `MESSAGE_BUS_MODULEINFO*`.]*/
    MESSAGE_BUS_MODULEINFO* module_info = (MESSAGE_BUS_MODULEINFO*)user_data;

	int should_continue = 1;
	while (should_continue)
	{
		/*Codes_SRS_MESSAGE_BUS_13_089: [ This function shall acquire the lock on module_info->socket_lock. ]*/
		if (Lock(module_info->socket_lock))
		{
			/*Codes_SRS_MESSAGE_BUS_02_004: [ If acquiring the lock fails, then module_publish_worker shall return. ]*/
			LogError("unable to Lock");
			should_continue = 0;
			break;
		}
		int nn_fd = module_info->receive_socket;
		int nbytes;
		unsigned char *buf = NULL;

		/*Codes_SRS_MESSAGE_BUS_17_005: [ For every iteration of the loop, the function shall wait on the receive_socket for messages. ]*/
		nbytes = nn_recv(nn_fd, (void *)&buf, NN_MSG, 0);
		/*Codes_SRS_MESSAGE_BUS_13_091: [ The function shall unlock module_info->socket_lock. ]*/
		if (Unlock(module_info->socket_lock) != LOCK_OK)
		{
			/*Codes_SRS_MESSAGE_BUS_17_016: [ If releasing the lock fails, then module_publish_worker shall return. ]*/
			should_continue = 0;
			if (nbytes > 0)
			{
				/*Codes_SRS_MESSAGE_BUS_17_019: [ The function shall free the buffer received on the receive_socket. ]*/
				nn_freemsg(buf);
			}
			break;
		}

		if (nbytes < 0)
		{
			/*Codes_SRS_MESSAGE_BUS_17_006: [ An error on receiving a message shall terminate the loop. ]*/
			should_continue = 0;
		}
		else
		{
			if (nbytes == MESSAGE_BUS_GUID_SIZE &&
				(strncmp(STRING_c_str(module_info->quit_message_guid), (const char *)buf, MESSAGE_BUS_GUID_SIZE-1)==0))
			{
				/*Codes_SRS_MESSAGE_BUS_13_068: [ This function shall run a loop that keeps running until module_info->quit_message_guid is sent to the thread. ]*/
				/* received special quit message for this module */
				should_continue = 0;
			}
			else
			{
				/*Codes_SRS_MESSAGE_BUS_17_017: [ The function shall deserialize the message received. ]*/
				MESSAGE_HANDLE msg = Message_CreateFromByteArray((const unsigned char*)buf, nbytes);
				/*Codes_SRS_MESSAGE_BUS_17_018: [ If the deserialization is not successful, the message loop shall continue. ]*/
				if (msg != NULL)
				{
					/*Codes_SRS_MESSAGE_BUS_13_092: [ The function shall deliver the message to the module's callback function via module_info->module_apis. ]*/
#ifdef UWP_BINDING
					/*Codes_SRS_MESSAGE_BUS_99_012: [The function shall deliver the message to the module's Receive function via the IInternalGatewayModule interface. ]*/
					module_info->module->module_instance->Module_Receive(msg);
#else
					/*Codes_SRS_MESSAGE_BUS_13_092: [The function shall deliver the message to the module's callback function via module_info->module_apis. ]*/
					module_info->module->module_apis->Module_Receive(module_info->module->module_handle, msg);
#endif // UWP_BINDING
					/*Codes_SRS_MESSAGE_BUS_13_093: [ The function shall destroy the message that was dequeued by calling Message_Destroy. ]*/
					Message_Destroy(msg);
				}
			}
			/*Codes_SRS_MESSAGE_BUS_17_019: [ The function shall free the buffer received on the receive_socket. ]*/
			nn_freemsg(buf);
		}	
	}

    return 0;
}

static MESSAGE_BUS_RESULT init_module(MESSAGE_BUS_MODULEINFO* module_info, const MODULE* module)
{
    MESSAGE_BUS_RESULT result;

    /*Codes_SRS_MESSAGE_BUS_13_107: The function shall assign the `module` handle to `MESSAGE_BUS_MODULEINFO::module`.*/
    module_info->module = (MODULE*)malloc(sizeof(MODULE));
    if (module_info->module == NULL)
    {
        LogError("Allocate module failed");
        result = MESSAGE_BUS_ERROR;
    }
	else
	{

#ifdef UWP_BINDING
		module_info->module->module_instance = module->module_instance;
#else
		module_info->module->module_apis = module->module_apis;
		module_info->module->module_handle = module->module_handle;
#endif // UWP_BINDING


		/*Codes_SRS_MESSAGE_BUS_13_099: [The function shall initialize MESSAGE_BUS_MODULEINFO::socket_lock with a valid lock handle.]*/
		module_info->socket_lock = Lock_Init();
		if (module_info->socket_lock == NULL)
		{
			/*Codes_SRS_MESSAGE_BUS_13_047: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
			LogError("Lock_Init for socket lock failed");
			result = MESSAGE_BUS_ERROR;
		}
		else
		{
			char uuid[MESSAGE_BUS_GUID_SIZE];
			memset(uuid, 0, MESSAGE_BUS_GUID_SIZE);
			/*Codes_SRS_MESSAGE_BUS_17_020: [ The function shall create a unique ID used as a quit signal. ]*/
			if (UniqueId_Generate(uuid, MESSAGE_BUS_GUID_SIZE) != UNIQUEID_OK)
			{
				/*Codes_SRS_MESSAGE_BUS_13_047: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
				LogError("Lock_Init for socket lock failed");
				Lock_Deinit(module_info->socket_lock);
				result = MESSAGE_BUS_ERROR;
			}
			else
			{
				module_info->quit_message_guid = STRING_construct(uuid);
				if (module_info->quit_message_guid == NULL)
				{
					/*Codes_SRS_MESSAGE_BUS_13_047: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
					LogError("String construct failed for module guid");
					Lock_Deinit(module_info->socket_lock);
					result = MESSAGE_BUS_ERROR;
				}
				else
				{
					result = MESSAGE_BUS_OK;
				}
			}
		}
	}
    return result;
}

static void deinit_module(MESSAGE_BUS_MODULEINFO* module_info)
{
    /*Codes_SRS_MESSAGE_BUS_13_057: [The function shall free all members of the MODULE_INFO object.]*/
	Lock_Deinit(module_info->socket_lock);
	STRING_delete(module_info->quit_message_guid);
	free(module_info->module);
}

static MESSAGE_BUS_RESULT start_module(MESSAGE_BUS_MODULEINFO* module_info, STRING_HANDLE url)
{
    MESSAGE_BUS_RESULT result;

	/* Connect to pub/sub */
	/*Codes_SRS_MESSAGE_BUS_17_013: [ The function shall create a nanomsg socket for reception. ]*/
	module_info->receive_socket = nn_socket(AF_SP, NN_SUB);
	if (module_info->receive_socket < 0)
	{
		/*Codes_SRS_MESSAGE_BUS_13_047: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
		LogError("module receive socket create failed");
		result = MESSAGE_BUS_ERROR;
	}
	else
	{
		/*Codes_SRS_MESSAGE_BUS_17_014: [ The function shall bind the socket to the the MESSAGE_BUS_HANDLE_DATA::url. ]*/
		int connect_result = nn_connect(module_info->receive_socket, STRING_c_str(url));
		if (connect_result < 0)
		{
			/*Codes_SRS_MESSAGE_BUS_13_047: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
			LogError("nn_connect failed");
			nn_close(module_info->receive_socket);
			module_info->receive_socket = -1;
			result = MESSAGE_BUS_ERROR;
		}
		else
		{
			/* We may want to subscribe to quit message guid, we may have to for topic subscriptions */
			if (nn_setsockopt(
				module_info->receive_socket, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) < 0)
			{
				/*Codes_SRS_MESSAGE_BUS_13_047: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
				LogError("nn_setsockopt failed");
				nn_close(module_info->receive_socket);
				module_info->receive_socket = -1;
				result = MESSAGE_BUS_ERROR;
			}
			else
			{
				/*Codes_SRS_MESSAGE_BUS_13_102: [The function shall create a new thread for the module by calling ThreadAPI_Create using module_publish_worker as the thread callback and using the newly allocated MESSAGE_BUS_MODULEINFO object as the thread context.*/
				if (ThreadAPI_Create(
					&(module_info->thread),
					module_publish_worker,
					(void*)module_info
				) != THREADAPI_OK)
				{
					/*Codes_SRS_MESSAGE_BUS_13_047: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
					LogError("ThreadAPI_Create failed");
					nn_close(module_info->receive_socket);
					result = MESSAGE_BUS_ERROR;
				}
				else
				{
					result = MESSAGE_BUS_OK;
				}
			}
		}
	}

    return result;
}

/*stop module means: stop the thread that feeds messages to Module_Receive function + deletion of all queued messages */
/*returns 0 if success, otherwise __LINE__*/
static int stop_module(int publish_socket, MESSAGE_BUS_MODULEINFO* module_info)
{
    int  quit_result, close_result, thread_result, result;

	/*Codes_SRS_MESSAGE_BUS_17_021: [ This function shall send a quit signal to the worker thread by sending MESSAGE_BUS_MODULEINFO::quit_message_guid to the publish_socket. ]*/
	/* send the unique quite id for this module */
	if ((quit_result = nn_send(publish_socket, STRING_c_str(module_info->quit_message_guid), MESSAGE_BUS_GUID_SIZE, 0)) < 0)
	{
		/*Codes_SRS_MESSAGE_BUS_17_015: [ This function shall close the MESSAGE_BUS_MODULEINFO::receive_socket. ]*/
		/* at the cost of a data race, we will close the socket to terminate the thread */
		nn_close(module_info->receive_socket);
		LogError("unable to peacefully close thread for module [%p], nn_send error [%d], taking harsher methods", module_info, quit_result);
	}
	else
	{
		/*Codes_SRS_MESSAGE_BUS_02_001: [ MessageBus_RemoveModule shall lock MESSAGE_BUS_MODULEINFO::socket_lock. ]*/
		if (Lock(module_info->socket_lock) != LOCK_OK)
		{
			/*Codes_SRS_MESSAGE_BUS_17_015: [ This function shall close the MESSAGE_BUS_MODULEINFO::receive_socket. ]*/
			/* at the cost of a data race, we will close the socket to terminate the thread */
			nn_close(module_info->receive_socket);
			LogError("unable to peacefully close thread for module [%p], Lock error, taking harsher methods", module_info );
		}
		else
		{
			/*Codes_SRS_MESSAGE_BUS_17_015: [ This function shall close the MESSAGE_BUS_MODULEINFO::receive_socket. ]*/
			close_result = nn_close(module_info->receive_socket);
			if (close_result < 0)
			{
				LogError("Receive socket close failed for module at  item [%p] failed", module_info);
			}
			else
			{
				/*all is fine, thread will eventually stop and be joined*/
			}
			/*Codes_SRS_MESSAGE_BUS_02_003: [ After closing the socket, MessageBus_RemoveModule shall unlock MESSAGE_BUS_MODULEINFO::info_lock. ]*/
			if (Unlock(module_info->socket_lock) != LOCK_OK)
			{
				LogError("unable to unlock socket lock");
			}
		}
	}
	/*Codes_SRS_MESSAGE_BUS_13_104: [The function shall wait for the module's thread to exit by joining MESSAGE_BUS_MODULEINFO::thread via ThreadAPI_Join. ]*/
	if (ThreadAPI_Join(module_info->thread, &thread_result) != THREADAPI_OK)
	{
		result = __LINE__;
		LogError("ThreadAPI_Join() returned an error.");
	}
	else
	{
		result = 0;
	}
    return result;
}

MESSAGE_BUS_RESULT MessageBus_AddModule(MESSAGE_BUS_HANDLE bus, const MODULE* module)
{
    MESSAGE_BUS_RESULT result;

    /*Codes_SRS_MESSAGE_BUS_99_013: [If `bus` or `module` is NULL the function shall return MESSAGE_BUS_INVALIDARG.]*/
    if (bus == NULL || module == NULL)
    {
        result = MESSAGE_BUS_INVALIDARG;
        LogError("invalid parameter (NULL).");
    }
#ifdef UWP_BINDING
	/*Codes_SRS_MESSAGE_BUS_99_015: [If `module_instance` is `NULL` the function shall return `MESSAGE_BUS_INVALIDARG`.]*/
	else if (module->module_instance == NULL)
	{
		result = MESSAGE_BUS_INVALIDARG;
		LogError("invalid parameter (NULL).");
	}
#else
	/*Codes_SRS_MESSAGE_BUS_99_014: [If `module_handle` or `module_apis` are `NULL` the function shall return `MESSAGE_BUS_INVALIDARG`.]*/
	else if (module->module_apis == NULL || module->module_handle == NULL)
	{
		result = MESSAGE_BUS_INVALIDARG;
		LogError("invalid parameter (NULL).");
	}
#endif // UWP_BINDING
    else
    {
        MESSAGE_BUS_MODULEINFO* module_info = (MESSAGE_BUS_MODULEINFO*)malloc(sizeof(MESSAGE_BUS_MODULEINFO));
        if (module_info == NULL)
        {
            LogError("Allocate module info failed");
            result = MESSAGE_BUS_ERROR;
        }
        else
        {
            if (init_module(module_info, module) != MESSAGE_BUS_OK)
            {
                /*Codes_SRS_MESSAGE_BUS_13_047: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
                LogError("start_module failed");
                free(module_info->module);
                free(module_info);
                result = MESSAGE_BUS_ERROR;
            }
            else
            {
                /*Codes_SRS_MESSAGE_BUS_13_039: [This function shall acquire the lock on MESSAGE_BUS_HANDLE_DATA::modules_lock.]*/
                MESSAGE_BUS_HANDLE_DATA* bus_data = (MESSAGE_BUS_HANDLE_DATA*)bus;
                if (Lock(bus_data->modules_lock) != LOCK_OK)
                {
                    /*Codes_SRS_MESSAGE_BUS_13_047: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
                    LogError("Lock on bus_data->modules_lock failed");
                    deinit_module(module_info);
                    free(module_info);
                    result = MESSAGE_BUS_ERROR;
                }
                else
                {
                    /*Codes_SRS_MESSAGE_BUS_13_045: [MessageBus_AddModule shall append the new instance of MESSAGE_BUS_MODULEINFO to MESSAGE_BUS_HANDLE_DATA::modules.]*/
                    LIST_ITEM_HANDLE moduleListItem = list_add(bus_data->modules, module_info);
                    if (moduleListItem == NULL)
                    {
                        /*Codes_SRS_MESSAGE_BUS_13_047: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
                        LogError("list_add failed");
                        deinit_module(module_info);
                        free(module_info);
                        result = MESSAGE_BUS_ERROR;
                    }
                    else
                    {
                        if (start_module(module_info, bus_data->url) != MESSAGE_BUS_OK)
                        {
                            LogError("start_module failed");
                            deinit_module(module_info);
                            list_remove(bus_data->modules, moduleListItem);
                            free(module_info);
                            result = MESSAGE_BUS_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_MESSAGE_BUS_13_047: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
                            result = MESSAGE_BUS_OK;
                        }
                    }

                    /*Codes_SRS_MESSAGE_BUS_13_046: [This function shall release the lock on MESSAGE_BUS_HANDLE_DATA::modules_lock.]*/
                    Unlock(bus_data->modules_lock);
                }
            }

        }
    }

    return result;
}

static bool find_module_predicate(LIST_ITEM_HANDLE list_item, const void* value)
{
    MESSAGE_BUS_MODULEINFO* element = (MESSAGE_BUS_MODULEINFO*)list_item_get_value(list_item);
#ifdef UWP_BINDING
	return element->module->module_instance == ((MODULE*)value)->module_instance;
#else
	return element->module->module_handle == ((MODULE*)value)->module_handle;
#endif // UWP_BINDING
}

MESSAGE_BUS_RESULT MessageBus_RemoveModule(MESSAGE_BUS_HANDLE bus, const MODULE* module)
{
    /*Codes_SRS_MESSAGE_BUS_13_048: [If `bus` or `module` is NULL the function shall return MESSAGE_BUS_INVALIDARG.]*/
    MESSAGE_BUS_RESULT result;
    if (bus == NULL || module == NULL)
    {
		/*Codes_SRS_MESSAGE_BUS_13_053: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
        result = MESSAGE_BUS_INVALIDARG;
        LogError("invalid parameter (NULL).");
    }
    else
    {
        /*Codes_SRS_MESSAGE_BUS_13_088: [This function shall acquire the lock on MESSAGE_BUS_HANDLE_DATA::modules_lock.]*/
        MESSAGE_BUS_HANDLE_DATA* bus_data = (MESSAGE_BUS_HANDLE_DATA*)bus;
        if (Lock(bus_data->modules_lock) != LOCK_OK)
        {
            /*Codes_SRS_MESSAGE_BUS_13_053: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
            LogError("Lock on bus_data->modules_lock failed");
            result = MESSAGE_BUS_ERROR;
        }
        else
        {
            /*Codes_SRS_MESSAGE_BUS_13_049: [MessageBus_RemoveModule shall perform a linear search for module in MESSAGE_BUS_HANDLE_DATA::modules.]*/
            LIST_ITEM_HANDLE module_info_item = list_find(bus_data->modules, find_module_predicate, module);

            if (module_info_item == NULL)
            {
				/*Codes_SRS_MESSAGE_BUS_13_050: [MessageBus_RemoveModule shall unlock MESSAGE_BUS_HANDLE_DATA::modules_lock and return MESSAGE_BUS_ERROR if the module is not found in MESSAGE_BUS_HANDLE_DATA::modules.]*/
                LogError("Supplied module was not found on the bus");
                result = MESSAGE_BUS_ERROR;
            }
            else
            {
                MESSAGE_BUS_MODULEINFO* module_info = (MESSAGE_BUS_MODULEINFO*)list_item_get_value(module_info_item);
                if (stop_module(bus_data->publish_socket, module_info) == 0)
                {
                    deinit_module(module_info);
                }
                else
                {
                    LogError("unable to stop module");
                }

                /*Codes_SRS_MESSAGE_BUS_13_052: [The function shall remove the module from MESSAGE_BUS_HANDLE_DATA::modules.]*/
                list_remove(bus_data->modules, module_info_item);
                free(module_info);

                /*Codes_SRS_MESSAGE_BUS_13_053: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
                result = MESSAGE_BUS_OK;
            }

            /*Codes_SRS_MESSAGE_BUS_13_054: [This function shall release the lock on MESSAGE_BUS_HANDLE_DATA::modules_lock.]*/
            Unlock(bus_data->modules_lock);
        }
    }

    return result;
}

static void bus_decrement_ref(MESSAGE_BUS_HANDLE bus)
{
    /*Codes_SRS_MESSAGE_BUS_13_058: [If `bus` is NULL the function shall do nothing.]*/
    if (bus != NULL)
    {
        /*Codes_SRS_MESSAGE_BUS_13_111: [Otherwise, MessageBus_Destroy shall decrement the internal ref count of the message.]*/
        /*Codes_SRS_MESSAGE_BUS_13_112: [If the ref count is zero then the allocated resources are freed.]*/
        if (DEC_REF(MESSAGE_BUS_HANDLE_DATA, bus) == DEC_RETURN_ZERO)
        {
            MESSAGE_BUS_HANDLE_DATA* bus_data = (MESSAGE_BUS_HANDLE_DATA*)bus; 
            if (list_get_head_item(bus_data->modules) != NULL)
            {
                LogError("WARNING: There are still active modules connected to the bus and the bus is being destroyed.");
            }
			/* May want to do nn_shutdown first for cleanliness. */
			nn_close(bus_data->publish_socket);
			STRING_delete(bus_data->url);
            list_destroy(bus_data->modules);
            Lock_Deinit(bus_data->modules_lock);
            free(bus_data);
        }
    }
    else
    {
        LogError("bus handle is NULL");
    }
}

extern void MessageBus_Destroy(MESSAGE_BUS_HANDLE bus)
{
    bus_decrement_ref(bus);
}

extern void MessageBus_DecRef(MESSAGE_BUS_HANDLE bus)
{
    /*Codes_SRS_MESSAGE_BUS_13_113: [This function shall implement all the requirements of the MessageBus_Destroy API.]*/
    bus_decrement_ref(bus);
}

MESSAGE_BUS_RESULT MessageBus_Publish(MESSAGE_BUS_HANDLE bus, MODULE_HANDLE source, MESSAGE_HANDLE message)
{
    MESSAGE_BUS_RESULT result;
    /*Codes_SRS_MESSAGE_BUS_13_030: [If bus or message is NULL the function shall return MESSAGE_BUS_INVALIDARG.]*/
    if (bus == NULL || message == NULL)
    {
        result = MESSAGE_BUS_INVALIDARG;
        LogError("Bus handle and/or message handle is NULL");
    }
    else
    {
		MESSAGE_BUS_HANDLE_DATA* bus_data = (MESSAGE_BUS_HANDLE_DATA*)bus;
		/*Codes_SRS_MESSAGE_BUS_17_022: [ MessageBus_Publish shall Lock the modules lock. ]*/
		if (Lock(bus_data->modules_lock) != LOCK_OK)
		{
			/*Codes_SRS_MESSAGE_BUS_13_053: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
			LogError("Lock on bus_data->modules_lock failed");
			result = MESSAGE_BUS_ERROR;
		}
		else
		{
			int32_t msg_size = 0;
			/*Codes_SRS_MESSAGE_BUS_17_007: [ MessageBus_Publish shall clone the message. ]*/
			MESSAGE_HANDLE msg = Message_Clone(message);
			/*Codes_SRS_MESSAGE_BUS_17_008: [ MessageBus_Publish shall serialize the message. ]*/
			const unsigned char* serial_message = Message_ToByteArray(msg, &msg_size);
			if (serial_message == NULL)
			{
				/*Codes_SRS_MESSAGE_BUS_13_053: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
				LogError("unable to serialize a message [%p]", msg);
				Message_Destroy(msg);
				result = MESSAGE_BUS_ERROR;
			}
			else
			{
				/*Codes_SRS_MESSAGE_BUS_17_009: [ MessageBus_Publish shall allocate a nanomsg buffer and copy the serialized message into it. ]*/
				void* nn_msg = nn_allocmsg(msg_size, 0);
				if (nn_msg == NULL)
				{
					/*Codes_SRS_MESSAGE_BUS_13_053: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
					LogError("unable to serialize a message [%p]", msg);
					result = MESSAGE_BUS_ERROR;
				}
				else
				{
					memcpy(nn_msg, serial_message, msg_size);
					/*Codes_SRS_MESSAGE_BUS_17_010: [ MessageBus_Publish shall send a message on the publish_socket. ]*/
					int nbytes = nn_send(bus_data->publish_socket, &nn_msg, NN_MSG, 0);
					if (nbytes != msg_size)
					{
						/*Codes_SRS_MESSAGE_BUS_13_053: [This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise.]*/
						LogError("unable to send a message [%p]", msg);
						/*Codes_SRS_MESSAGE_BUS_17_012: [ MessageBus_Publish shall free the message. ]*/
						nn_freemsg(nn_msg);
						result = MESSAGE_BUS_ERROR;
					}
					else
					{
						result = MESSAGE_BUS_OK;
					}
				}
				/*Codes_SRS_MESSAGE_BUS_17_012: [ MessageBus_Publish shall free the message. ]*/
				Message_Destroy(msg);
				/*Codes_SRS_MESSAGE_BUS_17_011: [ MessageBus_Publish shall free the serialized message data. ]*/
				free((void*)serial_message);
			}
			/*Codes_SRS_MESSAGE_BUS_17_023: [ MessageBus_Publish shall Unlock the modules lock. ]*/
			Unlock(bus_data->modules_lock);
		}

    }
	/*Codes_SRS_MESSAGE_BUS_13_037: [ This function shall return MESSAGE_BUS_ERROR if an underlying API call to the platform causes an error or MESSAGE_BUS_OK otherwise. ]*/
    return result;
}