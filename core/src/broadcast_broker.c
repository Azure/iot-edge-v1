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
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/condition.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/refcount.h"
#include "azure_c_shared_utility/list.h"

#include "message.h"
#include "module.h"
#include "broker.h"

/*The structure backing the message broker handle*/
typedef struct BROKER_HANDLE_DATA_TAG
{
    LIST_HANDLE                modules;
    LOCK_HANDLE             modules_lock;
}BROKER_HANDLE_DATA;

DEFINE_REFCOUNT_TYPE(BROKER_HANDLE_DATA);

typedef struct BROKER_MODULEINFO_TAG
{
    /**
    * Handle to the module that's associated with the broker.
    */
    MODULE*             module;

    /**
    * Handle to the thread on which this module's message processing loop is
    * running.
    */
    THREAD_HANDLE           thread;

    /**
    * Handle to the queue of messages to be delivered to this module.
    */
    VECTOR_HANDLE           mq;

    /**
    * Lock used to synchronize access to the 'mq' field.
    */
    LOCK_HANDLE             mq_lock;

    /**
    * A condition variable that is signaled when there are new messages.
    */
    COND_HANDLE             mq_cond;

    /**
    * Message publish worker will keep running while this is false.
    */
    volatile sig_atomic_t   quit_worker;
}BROKER_MODULEINFO;

// This variable is used only for unit testing purposes.
size_t BROKER_offsetof_quit_worker = offsetof(BROKER_MODULEINFO, quit_worker);

BROKER_HANDLE Broker_Create(void)
{
    BROKER_HANDLE_DATA* result;

    /*Codes_SRS_BCAST_BROKER_13_067: [Broker_Create shall malloc a new instance of BROKER_HANDLE_DATA and return NULL if it fails.]*/
    result = REFCOUNT_TYPE_CREATE(BROKER_HANDLE_DATA);
    if (result == NULL)
    {
        LogError("malloc returned NULL");
        /*return as is*/
    }
    else
    {
        /*Codes_SRS_BCAST_BROKER_13_007: [Broker_Create shall initialize BROKER_HANDLE_DATA::modules with a valid VECTOR_HANDLE.]*/
        result->modules = list_create();
        if (result->modules == NULL)
        {
            /*Codes_SRS_BCAST_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]*/
            LogError("VECTOR_create failed");
            free(result);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_BCAST_BROKER_13_023: [Broker_Create shall initialize BROKER_HANDLE_DATA::modules_lock with a valid LOCK_HANDLE.]*/
            result->modules_lock = Lock_Init();
            if (result->modules_lock == NULL)
            {
                /*Codes_SRS_BCAST_BROKER_13_003: [This function shall return NULL if an underlying API call to the platform causes an error.]*/
                LogError("Lock_Init failed");
                list_destroy(result->modules);
                free(result);
                result = NULL;
            }
        }
    }

    /*Codes_SRS_BCAST_BROKER_13_001: [This API shall yield a BROKER_HANDLE representing the newly created message broker. This handle value shall not be equal to NULL when the API call is successful.]*/
    return result;
}

void Broker_IncRef(BROKER_HANDLE broker)
{
    /*Codes_SRS_BCAST_BROKER_13_108: [If `broker` is NULL then Broker_IncRef shall do nothing.]*/
    if (broker == NULL)
    {
        LogError("invalid arg: broker is NULL");
    }
    else
    {
        /*Codes_SRS_BCAST_BROKER_13_109: [Otherwise, Broker_IncRef shall increment the internal ref count.]*/
        INC_REF(BROKER_HANDLE_DATA, broker);
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
    /*Codes_SRS_BCAST_BROKER_13_026: [This function shall assign `user_data` to a local variable called `module_info` of type `BROKER_MODULEINFO*`.]*/
    BROKER_MODULEINFO* module_info = (BROKER_MODULEINFO*)user_data;

    /*Codes_SRS_BCAST_BROKER_13_089: [This function shall acquire the lock on module_info->mq_lock.]*/
    if (Lock(module_info->mq_lock) != LOCK_OK)
    {
        /*Codes_SRS_BCAST_BROKER_02_004: [ If acquiring the lock fails, then module_publish_worker shall return. ]*/
        LogError("unable to lock");
    }
    else
    {
        /*Codes_SRS_BCAST_BROKER_13_068: [This function shall run a loop that keeps running while module_info->quit_worker is equal to 0.]*/
        while (module_info->quit_worker == 0)
        {
            /*Codes_SRS_BCAST_BROKER_13_071: [For every iteration of the loop the function will first wait on module_info->mq_cond using module_info->mq_lock as the corresponding mutex to be used by the condition variable.]*/
            /*Codes_SRS_BCAST_BROKER_04_001: [This function shall immediately start processing messages when `module->mq` is not empty without waiting on `module->mq_cond`.] */

            /*this condition accounts for the case where the message has been enqueued in the past, and the condition has been signalled in the past, and this thread */
            /*is still at static int module_publish_worker(void * user_data), that is, didn't get to execute Lock(...)*/
            if ((VECTOR_size(module_info->mq) > 0) || (Condition_Wait(module_info->mq_cond, module_info->mq_lock, 0) == COND_OK))
            {
                /*Codes_SRS_BCAST_BROKER_13_090: [When module_info->mq_cond has been signaled this function shall kick off another loop predicated on module_info->quit_worker being equal to 0 and module_info->mq not being empty.This thread has the lock on module_info->mq_lock at this point.]*/
                LOCK_RESULT lock_result = LOCK_OK;
                while ((module_info->quit_worker == 0) && VECTOR_size(module_info->mq) > 0)
                {
                    /*Codes_SRS_BCAST_BROKER_13_069: [The function shall dequeue a message from the module's message queue. ]*/
                    MESSAGE_HANDLE *pmsg = (MESSAGE_HANDLE*)VECTOR_front(module_info->mq);
                    MESSAGE_HANDLE msg = *pmsg;
                    VECTOR_erase(module_info->mq, pmsg, 1);

                    /*Codes_SRS_BCAST_BROKER_13_091: [The function shall unlock module_info->mq_lock.]*/
                    if (Unlock(module_info->mq_lock) != LOCK_OK)
                    {
                        LogError("unable to unlock");

                        /*Codes_SRS_BCAST_BROKER_13_093: [The function shall destroy the message that was dequeued by calling Message_Destroy.]*/
                        Message_Destroy(msg);

                        continue;
                    }
                    else
                    {
#ifdef UWP_BINDING
                        /*Codes_SRS_BCAST_BROKER_99_012: [The function shall deliver the message to the module's Receive function via the IInternalGatewayModule interface. ]*/
                        module_info->module->module_instance->Module_Receive(msg);
#else
                        /*Codes_SRS_BCAST_BROKER_13_092: [The function shall deliver the message to the module's callback function via module_info->module_apis. ]*/
                        module_info->module->module_apis->Module_Receive(module_info->module->module_handle, msg);
#endif // UWP_BINDING

                        /*Codes_SRS_BCAST_BROKER_13_093: [The function shall destroy the message that was dequeued by calling Message_Destroy.]*/
                        Message_Destroy(msg);

                        /*Codes_SRS_BCAST_BROKER_13_094: [The function shall re - acquire the lock on module_info->mq_lock.]*/
                        if ((lock_result = Lock(module_info->mq_lock)) != LOCK_OK)
                        {
                            LogError("unable to lock");
                            break;
                        }
                    }
                } 
            }
            else
            {
                LogError("Lock/Condition_Wait has failed. Bailing.");
                break;
            }
        }

        /*Codes_SRS_BCAST_BROKER_13_095: [When the function exits the outer loop predicated on module_info->quit_worker being 0 it shall unlock module_info->mq_lock before exiting from the function.]*/
        if (Unlock(module_info->mq_lock) != LOCK_OK)
        {
            LogError("unable to unlock - module worker was terminating");
        }
    }

    

    return 0;
}

static BROKER_RESULT init_module(BROKER_MODULEINFO* module_info, const MODULE* module)
{
    BROKER_RESULT result;

    /*Codes_SRS_BCAST_BROKER_13_107: The function shall assign the `module` handle to `BROKER_MODULEINFO::module`.*/
    module_info->module = (MODULE*)malloc(sizeof(MODULE));
    if (module_info->module == NULL)
    {
        LogError("Allocate module failed");
        result = BROKER_ERROR;
    }
    else
    {
#ifdef UWP_BINDING
		module_info->module->module_instance = module->module_instance;
#else
		module_info->module->module_apis = module->module_apis;
		module_info->module->module_handle = module->module_handle;
#endif // UWP_BINDING

        /*Codes_SRS_BCAST_BROKER_13_098: [The function shall initialize BROKER_MODULEINFO::mq with a valid vector handle.]*/
        module_info->mq = VECTOR_create(sizeof(MESSAGE_HANDLE));
        if (module_info->mq == NULL)
        {
            LogError("VECTOR_create failed");
            result = BROKER_ERROR;
        }
        else
        {
            /*Codes_SRS_BCAST_BROKER_13_099: [The function shall initialize BROKER_MODULEINFO::mq_lock with a valid lock handle.]*/
            module_info->mq_lock = Lock_Init();
            if (module_info->mq_lock == NULL)
            {
                LogError("Lock_Init failed");
                VECTOR_destroy(module_info->mq);
                result = BROKER_ERROR;
            }
            else
            {
                /*Codes_SRS_BCAST_BROKER_13_100: [The function shall initialize BROKER_MODULEINFO::mq_cond with a valid condition handle.]*/
                module_info->mq_cond = Condition_Init();
                if (module_info->mq_cond == NULL)
                {
                    LogError("Condition_Init failed");
                    Lock_Deinit(module_info->mq_lock);
                    VECTOR_destroy(module_info->mq);
                    result = BROKER_ERROR;
                }
                else
                {
                    /*Codes_SRS_BCAST_BROKER_13_101: [The function shall assign 0 to BROKER_MODULEINFO::quit_worker.]*/
                    module_info->quit_worker = 0;
                    result = BROKER_OK;
                }
            }
        }
    }

    return result;
}

static void deinit_module(BROKER_MODULEINFO* module_info)
{
    /*Codes_SRS_BCAST_BROKER_13_057: [The function shall free all members of the MODULE_INFO object.]*/
    VECTOR_destroy(module_info->mq);
    Condition_Deinit(module_info->mq_cond);
    Lock_Deinit(module_info->mq_lock);
    free(module_info->module);
}

static BROKER_RESULT start_module(BROKER_MODULEINFO* module_info)
{
    BROKER_RESULT result;

    /*Codes_SRS_BCAST_BROKER_13_102: [The function shall create a new thread for the module by calling ThreadAPI_Create using module_publish_worker as the thread callback and using the newly allocated BROKER_MODULEINFO object as the thread context.*/
    if (ThreadAPI_Create(
        &(module_info->thread),
        module_publish_worker,
        (void*)module_info
        ) != THREADAPI_OK)
    {
        LogError("ThreadAPI_Create failed");
        result = BROKER_ERROR;
    }
    else
    {
        result = BROKER_OK;
    }

    return result;
}

/*stop module means: stop the thread that feeds messages to Module_Receive function + deletion of all queued messages */
/*returns 0 if success, otherwise __LINE__*/
static int stop_module(BROKER_MODULEINFO* module_info)
{
    int thread_result, result;
    size_t len, i;
    /*Codes_SRS_BCAST_BROKER_02_001: [ Broker_RemoveModule shall lock `BROKER_MODULEINFO::mq_lock`. ]*/
    if (Lock(module_info->mq_lock) != LOCK_OK)
    {
        module_info->quit_worker = 1; /*at the cost of a data race, still try to stop the module*/
        /*Codes_SRS_BCAST_BROKER_02_002: [ If locking fails, then terminating the thread shall not be attempted (signalling the condition and joining the thread). ]*/
        LogError("unable to lock mq_lock");
    }
    else
    {
        /*Codes_SRS_BCAST_BROKER_13_103: [The function shall assign 1 to BROKER_MODULEINFO::quit_worker.]*/
        module_info->quit_worker = 1;

        /*Codes_SRS_BCAST_BROKER_17_001: [The function shall signal BROKER_MODULEINFO::mq_cond to release module from waiting.]*/
        if (Condition_Post(module_info->mq_cond) != COND_OK)
        {
            LogError("Condition_Post failed for module at  item [%p] failed", module_info);
        }
        else
        {
            /*all is fine, thread will eventually stop and be joined*/
        }

        /*Codes_SRS_BCAST_BROKER_02_003: [ After signaling the condition, Broker_RemoveModule shall unlock BROKER_MODULEINFO::mq_lock. ]*/
        if (Unlock(module_info->mq_lock) != LOCK_OK)
        {
            LogError("unable to unlock mq_lock");
        }
    }

    /*Codes_SRS_BCAST_BROKER_13_104: [The function shall wait for the module's thread to exit by joining BROKER_MODULEINFO::thread via ThreadAPI_Join. ]*/
    if (ThreadAPI_Join(module_info->thread, &thread_result) != THREADAPI_OK)
    {
        result = __LINE__;
        LogError("ThreadAPI_Join() returned an error.");
    }
    else
    {
        result = 0;
    }

    /*Codes_SRS_BCAST_BROKER_13_056: [If BROKER_MODULEINFO::mq is not empty then this function shall call Message_Destroy on every message still left in the collection.]*/
    len = VECTOR_size(module_info->mq);
    for (i = 0; i < len; i++)
    {
        // this MUST NOT be NULL
        MESSAGE_HANDLE* msg = (MESSAGE_HANDLE*)VECTOR_element(module_info->mq, i);
        Message_Destroy(*msg);
    }
    return result;
}

BROKER_RESULT Broker_AddModule(BROKER_HANDLE broker, const MODULE* module)
{
    BROKER_RESULT result;

    /*Codes_SRS_BCAST_BROKER_99_013: [If `broker` or `module` is NULL the function shall return BROKER_INVALIDARG.]*/
    if (broker == NULL || module == NULL)
    {
        result = BROKER_INVALIDARG;
        LogError("invalid parameter (NULL).");
    }
#ifdef UWP_BINDING
	/*Codes_SRS_BCAST_BROKER_99_015: [If `module_instance` is `NULL` the function shall return `BROKER_INVALIDARG`.]*/
	else if (module->module_instance == NULL)
	{
		result = BROKER_INVALIDARG;
		LogError("invalid parameter (NULL).");
	}
#else
	/*Codes_SRS_BCAST_BROKER_99_014: [If `module_handle` or `module_apis` are `NULL` the function shall return `BROKER_INVALIDARG`.]*/
	else if (module->module_apis == NULL || module->module_handle == NULL)
	{
		result = BROKER_INVALIDARG;
		LogError("invalid parameter (NULL).");
	}
#endif // UWP_BINDING
    else
    {
        BROKER_MODULEINFO* module_info = (BROKER_MODULEINFO*)malloc(sizeof(BROKER_MODULEINFO));
        if (module_info == NULL)
        {
            LogError("Allocate module info failed");
            result = BROKER_ERROR;
        }
        else
        {
            if (init_module(module_info, module) != BROKER_OK)
            {
                /*Codes_SRS_BCAST_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                LogError("start_module failed");
                free(module_info->module);
                free(module_info);
                result = BROKER_ERROR;
            }
            else
            {
                /*Codes_SRS_BCAST_BROKER_13_039: [This function shall acquire the lock on BROKER_HANDLE_DATA::modules_lock.]*/
                BROKER_HANDLE_DATA* broker_data = (BROKER_HANDLE_DATA*)broker;
                if (Lock(broker_data->modules_lock) != LOCK_OK)
                {
                    /*Codes_SRS_BCAST_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                    LogError("Lock on broker_data->modules_lock failed");
                    deinit_module(module_info);
                    free(module_info);
                    result = BROKER_ERROR;
                }
                else
                {
                    /*Codes_SRS_BCAST_BROKER_13_045: [Broker_AddModule shall append the new instance of BROKER_MODULEINFO to BROKER_HANDLE_DATA::modules.]*/
                    LIST_ITEM_HANDLE moduleListItem = list_add(broker_data->modules, module_info);
                    if (moduleListItem == NULL)
                    {
                        /*Codes_SRS_BCAST_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                        LogError("list_add failed");
                        deinit_module(module_info);
                        free(module_info);
                        result = BROKER_ERROR;
                    }
                    else
                    {
                        if (start_module(module_info) != BROKER_OK)
                        {
                            LogError("start_module failed");
                            deinit_module(module_info);
                            list_remove(broker_data->modules, moduleListItem);
                            free(module_info);
                            result = BROKER_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_BCAST_BROKER_13_047: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                            result = BROKER_OK;
                        }
                    }

                    /*Codes_SRS_BCAST_BROKER_13_046: [This function shall release the lock on BROKER_HANDLE_DATA::modules_lock.]*/
                    Unlock(broker_data->modules_lock);
                }
            }

        }
    }

    return result;
}

static bool find_module_predicate(LIST_ITEM_HANDLE list_item, const void* value)
{
    BROKER_MODULEINFO* element = (BROKER_MODULEINFO*)list_item_get_value(list_item);
#ifdef UWP_BINDING
	return element->module->module_instance == ((MODULE*)value)->module_instance;
#else
	return element->module->module_handle == ((MODULE*)value)->module_handle;
#endif // UWP_BINDING
}

BROKER_RESULT Broker_RemoveModule(BROKER_HANDLE broker, const MODULE* module)
{
    /*Codes_SRS_BCAST_BROKER_13_048: [If `broker` or `module` is NULL the function shall return BROKER_INVALIDARG.]*/
    BROKER_RESULT result;
    if (broker == NULL || module == NULL)
    {
        result = BROKER_INVALIDARG;
        LogError("invalid parameter (NULL).");
    }
    else
    {
        /*Codes_SRS_BCAST_BROKER_13_088: [This function shall acquire the lock on BROKER_HANDLE_DATA::modules_lock.]*/
        BROKER_HANDLE_DATA* broker_data = (BROKER_HANDLE_DATA*)broker;
        if (Lock(broker_data->modules_lock) != LOCK_OK)
        {
            /*Codes_SRS_BCAST_BROKER_13_053: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
            LogError("Lock on broker_data->modules_lock failed");
            result = BROKER_ERROR;
        }
        else
        {
            /*Codes_SRS_BCAST_BROKER_13_049: [Broker_RemoveModule shall perform a linear search for module in BROKER_HANDLE_DATA::modules.]*/
            LIST_ITEM_HANDLE module_info_item = list_find(broker_data->modules, find_module_predicate, module);

            if (module_info_item == NULL)
            {
                /*Codes_SRS_BCAST_BROKER_13_050: [Broker_RemoveModule shall unlock BROKER_HANDLE_DATA::modules_lock and return BROKER_ERROR if the module is not found in BROKER_HANDLE_DATA::modules.]*/
                LogError("Supplied module was not found on the broker");
                result = BROKER_ERROR;
            }
            else
            {
                BROKER_MODULEINFO* module_info = (BROKER_MODULEINFO*)list_item_get_value(module_info_item);
                if (stop_module(module_info) == 0)
                {
                    deinit_module(module_info);
                }
                else
                {
                    LogError("unable to stop module");
                }

                /*Codes_SRS_BCAST_BROKER_13_052: [The function shall remove the module from BROKER_HANDLE_DATA::modules.]*/
                list_remove(broker_data->modules, module_info_item);
                free(module_info);

                /*Codes_SRS_BCAST_BROKER_13_053: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                result = BROKER_OK;
            }

            /*Codes_SRS_BCAST_BROKER_13_054: [This function shall release the lock on BROKER_HANDLE_DATA::modules_lock.]*/
            Unlock(broker_data->modules_lock);
        }
    }

    return result;
}

static void broker_decrement_ref(BROKER_HANDLE broker)
{
    /*Codes_SRS_BCAST_BROKER_13_058: [If `broker` is NULL the function shall do nothing.]*/
    if (broker != NULL)
    {
        /*Codes_SRS_BCAST_BROKER_13_111: [Otherwise, Broker_Destroy shall decrement the internal ref count of the message.]*/
        /*Codes_SRS_BCAST_BROKER_13_112: [If the ref count is zero then the allocated resources are freed.]*/
        if (DEC_REF(BROKER_HANDLE_DATA, broker) == DEC_RETURN_ZERO)
        {
            BROKER_HANDLE_DATA* broker_data = (BROKER_HANDLE_DATA*)broker; 
            if (list_get_head_item(broker_data->modules) != NULL)
            {
                LogError("WARNING: There are still active modules connected to the broker and the broker is being destroyed.");
            }

            list_destroy(broker_data->modules);
            Lock_Deinit(broker_data->modules_lock);
            free(broker_data);
        }
    }
    else
    {
        LogError("broker handle is NULL");
    }
}
BROKER_RESULT Broker_AddLink(BROKER_HANDLE broker, const BROKER_LINK_DATA* link)
{
    /*Codes_SRS_BCAST_BROKER_17_003: [ Broker_AddLink shall return BROKER_OK. ]*/
    return BROKER_OK;
}

BROKER_RESULT Broker_RemoveLink(BROKER_HANDLE broker, const BROKER_LINK_DATA* link)
{
    /*Codes_SRS_BCAST_BROKER_17_004: [ Broker_RemoveLink shall return BROKER_OK. ]*/
    return BROKER_OK;    
}

extern void Broker_Destroy(BROKER_HANDLE broker)
{
    broker_decrement_ref(broker);
}

extern void Broker_DecRef(BROKER_HANDLE broker)
{
    /*Codes_SRS_BCAST_BROKER_13_113: [This function shall implement all the requirements of the Broker_Destroy API.]*/
    broker_decrement_ref(broker);
}

BROKER_RESULT Broker_Publish(BROKER_HANDLE broker, MODULE_HANDLE source, MESSAGE_HANDLE message)
{
    BROKER_RESULT result;
    /*Codes_SRS_BCAST_BROKER_13_030: [If broker or message is NULL the function shall return BROKER_INVALIDARG.]*/
    if (broker == NULL || message == NULL)
    {
        result = BROKER_INVALIDARG;
        LogError("Broker handle and/or message handle is NULL");
    }
    else
    {
        /*Codes_SRS_BCAST_BROKER_13_031: [Broker_Publish shall acquire the lock BROKER_HANDLE_DATA::modules_lock.]*/
        BROKER_HANDLE_DATA* broker_data = (BROKER_HANDLE_DATA*)broker;
        if (Lock(broker_data->modules_lock) != LOCK_OK)
        {
            LogError("Lock on broker_data->modules_lock failed");
            result = BROKER_ERROR;
        }
        else
        {
            /*Codes_SRS_BCAST_BROKER_13_032: [Broker_Publish shall start a processing loop for every module in BROKER_HANDLE_DATA::modules.]*/

            /*Codes_SRS_BCAST_BROKER_13_037: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
            result = BROKER_OK;

            // NOTE: This is a best-effort delivery bus which means that we offer no
            // delivery guarantees. If message delivery for a particular module fails,
            // we log the fact and go on our merry way trying to deliver messages to
            // other modules on the bus. We will however return BROKER_ERROR when this
            // happens.
            for (LIST_ITEM_HANDLE current_module = list_get_head_item(broker_data->modules);
                 current_module != NULL; 
                 current_module = list_get_next_item(current_module))
            {

                BROKER_MODULEINFO* module_info = (BROKER_MODULEINFO*)list_item_get_value(current_module);

                /*Codes_SRS_BCAST_BROKER_17_002: [ If source is not NULL, Broker_Publish shall not publish the message to the BROKER_MODULEINFO::module which matches source. ]*/
#ifdef UWP_BINDING
				if (source == NULL || module_info->module->module_instance != source)
#else
				if (source == NULL || module_info->module->module_handle != source)
#endif // UWP_BINDING
                {
                    /*Codes_SRS_BCAST_BROKER_13_033: [In the loop, the function shall first acquire the lock on BROKER_MODULEINFO::mq_lock.]*/
                    if (Lock(module_info->mq_lock) != LOCK_OK)
                    {
                        /*Codes_SRS_BCAST_BROKER_13_037: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                        LogError("Lock on module_info->mq_lock for module at item [%p] failed", current_module);
                        result = BROKER_ERROR;
                    }
                    else
                    {
                        /*Codes_SRS_BCAST_BROKER_13_034: [The function shall then append message to BROKER_MODULEINFO::mq by calling Message_Clone and VECTOR_push_back.]*/
                        MESSAGE_HANDLE msg = Message_Clone(message);
                        if (VECTOR_push_back(module_info->mq, &msg, 1) != 0)
                        {
                            /*Codes_SRS_BCAST_BROKER_13_037: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                            LogError("VECTOR_push_back failed for module at  item [%p] failed", current_module);
                            Message_Destroy(msg);
                            Unlock(module_info->mq_lock);
                            result = BROKER_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_BCAST_BROKER_13_096: [The function shall then signal BROKER_MODULEINFO::mq_cond.]*/
                            if (Condition_Post(module_info->mq_cond) != COND_OK)
                            {
                                /*Codes_SRS_BCAST_BROKER_13_037: [This function shall return BROKER_ERROR if an underlying API call to the platform causes an error or BROKER_OK otherwise.]*/
                                LogError("Condition_Post failed for module at  item [%p] failed", current_module);
                                result = BROKER_ERROR;
                            }

                            /*Codes_SRS_BCAST_BROKER_13_035: [The function shall then release BROKER_MODULEINFO::mq_lock.]*/
                            if (Unlock(module_info->mq_lock) != LOCK_OK)
                            {
                                LogError("unable to unlock");
                            }
                        }
                    }
                }
            }

            /*Codes_SRS_BCAST_BROKER_13_040: [Broker_Publish shall release the lock BROKER_HANDLE_DATA::modules_lock after the loop.]*/
            Unlock(broker_data->modules_lock);
        }
    }

    return result;
}