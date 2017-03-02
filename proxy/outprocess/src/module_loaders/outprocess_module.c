// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <errno.h>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/reqrep.h>

#include "module.h"
#include "message.h"
#include "control_message.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/threadapi.h"
#include "module_loaders/outprocess_module.h"
#include "azure_c_shared_utility/lock.h"

typedef struct THREAD_CONTROL_TAG
{
	LOCK_HANDLE thread_lock;
	THREAD_HANDLE thread_handle;
	int thread_flag;
} THREAD_CONTROL;

#define THREAD_FLAG_STOP 1

typedef struct OUTPROCESS_HANDLE_DATA_TAG
{
	LOCK_HANDLE handle_lock;
	int message_socket;
	int control_socket;
	STRING_HANDLE control_uri;
	STRING_HANDLE message_uri;
	STRING_HANDLE module_args;
	OUTPROCESS_MODULE_LIFECYCLE lifecyle_model;
	BROKER_HANDLE broker;

	THREAD_CONTROL message_thread;
	THREAD_CONTROL async_thread;
	THREAD_CONTROL control_thread;
} OUTPROCESS_HANDLE_DATA;

// forward definitions
static void* construct_create_message(OUTPROCESS_HANDLE_DATA* handleData, int32_t * creationMessageSize);


int outprocessMessageThread(void *param)
{
	/*Codes_SRS_OUTPROCESS_MODULE_17_037: [ This function shall receive the module handle data as the thread parameter. ]*/
	OUTPROCESS_HANDLE_DATA * handleData = (OUTPROCESS_HANDLE_DATA*)param;
	if (handleData == NULL)
	{
		LogError("outprocess thread: parameter is NULL");
	}
	else
	{
		int should_continue = 1;

		while (should_continue)
		{
			/*Codes_SRS_OUTPROCESS_MODULE_17_036: [ This function shall ensure thread safety on execution. ]*/
			if (Lock(handleData->handle_lock) != LOCK_OK)
			{
				LogError("unable to Lock handle data");
				should_continue = 0;
				break;
			}
			int nn_fd = handleData->message_socket;
			if (Unlock(handleData->handle_lock) != LOCK_OK)
			{
				should_continue = 0;
				break;
			}

			if (Lock(handleData->message_thread.thread_lock) != LOCK_OK)
			{
				LogError("unable to Lock");
				should_continue = 0;
				break;
			}
			if (handleData->message_thread.thread_flag == THREAD_FLAG_STOP)
			{
				should_continue = 0;
				(void)Unlock(handleData->message_thread.thread_lock);
				break;
			}
			if (Unlock(handleData->message_thread.thread_lock) != LOCK_OK)
			{
				should_continue = 0;
				break;
			}

			int nbytes;
			unsigned char *buf = NULL;
			errno = 0;
			/*Codes_SRS_OUTPROCESS_MODULE_17_038: [ This function shall read from the message channel for gateway messages from the module host. ]*/
			nbytes = nn_recv(nn_fd, (void *)&buf, NN_MSG, 0);
			int receive_error = nn_errno();
			if (nbytes < 0)
			{
				if (receive_error != ETIMEDOUT)
					should_continue = 0;
			}
			else
			{
				/*Codes_SRS_OUTPROCESS_MODULE_17_039: [ Upon successful receiving a gateway message, this function shall deserialize the message. ]*/
				const unsigned char*buf_bytes = (const unsigned char*)buf;
				MESSAGE_HANDLE msg = Message_CreateFromByteArray(buf_bytes, nbytes);
				if (msg != NULL)
				{
					/*Codes_SRS_OUTPROCESS_MODULE_17_040: [ This function shall publish any successfully created gateway message to the broker. ]*/
					Broker_Publish(handleData->broker, (MODULE_HANDLE)handleData, msg);
					Message_Destroy(msg);
				}
				nn_freemsg(buf);
			}

		}
	}
	return 0;
}

static int outprocessCreate(void *param)
{
	int thread_return;
	OUTPROCESS_HANDLE_DATA * handleData = (OUTPROCESS_HANDLE_DATA*)param;
	if (handleData == NULL)
	{
		LogError("async_create_receive_control thread: parameter is NULL");
		thread_return = -1;
	}
	else
	{
		if (Lock(handleData->handle_lock) != LOCK_OK)
		{
			LogError("Unable to acquire handle data lock");
			thread_return = -1;
		}
		else
		{
			int control_fd = handleData->control_socket;

			int32_t creationMessageSize = 0;
			void * creationMessage = construct_create_message(handleData, &creationMessageSize);
			(void)Unlock(handleData->handle_lock);
			if (creationMessage == NULL)
			{
				/*Codes_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
				LogError("Unable to create create control message");
				thread_return = -1;
			}
			else
			{
				/*Codes_SRS_OUTPROCESS_MODULE_17_013: [ This function shall send the Create Message on the control channel. ]*/
				int sendBytes = nn_send(control_fd, &creationMessage, NN_MSG, 0);
				if (sendBytes != creationMessageSize)
				{
					/*Codes_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
					LogError("unable to send create message [%p]", creationMessage);
					nn_freemsg(creationMessage);
					thread_return = -1;
				}
				else
				{
					unsigned char *buf = NULL;
					int recvBytes = nn_recv(control_fd, (void *)&buf, NN_MSG, 0);
					if (recvBytes < 0)
					{
						thread_return = -1;
					}
					else
					{
						CONTROL_MESSAGE * msg = ControlMessage_CreateFromByteArray((const unsigned char*)buf, recvBytes);
						nn_freemsg(buf);
						if (msg == NULL)
						{
							thread_return = -1;
						}
						else
						{
							if (msg->type != CONTROL_MESSAGE_TYPE_MODULE_REPLY)
							{
								thread_return = -1;
							}
							else
							{
								CONTROL_MESSAGE_MODULE_REPLY * resp_msg = (CONTROL_MESSAGE_MODULE_REPLY*)msg;
								if (resp_msg->status != 0)
								{
									thread_return = -1;
								}
								else
								{
									/*Codes_SRS_OUTPROCESS_MODULE_17_015: [ This function shall expect a successful result from the Create Response to consider the module creation a success. ]*/
									// complete success!
									thread_return = 1;
								}
							}
							ControlMessage_Destroy(msg);
						}
					}
				}
			}
		}
	}
	return thread_return;
}

int outprocessControlThread(void *param)
{
	OUTPROCESS_HANDLE_DATA * handleData = (OUTPROCESS_HANDLE_DATA*)param;
	if (handleData == NULL)
	{
		LogError("outprocessCreate thread: parameter is NULL");
	}
	else
	{
		int should_continue = 1;
		int needs_to_attach = 0;

		while (should_continue)
		{
			/*Codes_SRS_OUTPROCESS_MODULE_17_036: [ This function shall ensure thread safety on execution. ]*/

			if (Lock(handleData->control_thread.thread_lock) != LOCK_OK)
			{
				LogError("unable to Lock");
				should_continue = 0;
				break;
			}
			if (handleData->control_thread.thread_flag == THREAD_FLAG_STOP)
			{
				should_continue = 0;
				(void)Unlock(handleData->control_thread.thread_lock);
				break;
			}
			if (Unlock(handleData->control_thread.thread_lock) != LOCK_OK)
			{
				should_continue = 0;
				break;
			}

			if (needs_to_attach)
			{
				// our remote has detached.  Attempt to reattach.
				if (connection_reset_control(handleData) < 0)
				{
					LogError("attemping to reconnect to remote failed");
				}
				else
				{
					if (outprocessCreate(handleData) < 0)
					{
						LogError("attempting to reattach to remote failed");
					}
					else
					{
						needs_to_attach = 0;
					}
				}
			}

			if (Lock(handleData->handle_lock) != LOCK_OK)
			{
				LogError("unable to Lock handle data");
				should_continue = 0;
				break;
			}
			int nn_fd = handleData->control_socket;
			if (Unlock(handleData->handle_lock) != LOCK_OK)
			{
				should_continue = 0;
				break;
			}

			int nbytes;
			unsigned char *buf = NULL;
			errno = 0;
			nbytes = nn_recv(nn_fd, (void *)&buf, NN_MSG, NN_DONTWAIT);
			int receive_error = nn_errno();
			if (nbytes < 0)
			{
				if (receive_error != EAGAIN)
					should_continue = 0;
			}
			else
			{
				CONTROL_MESSAGE * msg = ControlMessage_CreateFromByteArray((const unsigned char*)buf, nbytes);
				nn_freemsg(buf);
				if (msg != NULL)
				{
					if (msg->type == CONTROL_MESSAGE_TYPE_MODULE_REPLY)
					{
						CONTROL_MESSAGE_MODULE_REPLY * resp_msg = (CONTROL_MESSAGE_MODULE_REPLY*)msg;
						if (resp_msg->status != 0)
						{
							needs_to_attach = 1;
						}
					}
					ControlMessage_Destroy(msg);
				}
			}
			ThreadAPI_Sleep(250);
		}
	}
	return 0;
}

/* Connection related functions
*/
static int connection_reset_control(OUTPROCESS_HANDLE_DATA* handleData)
{
	int result;
	if (Lock(handleData->handle_lock) != LOCK_OK)
	{
		result = -1;
	}
	else
	{
		/*Codes_SRS_OUTPROCESS_MODULE_17_010: [ This function shall create a request/reply socket for sending control messages to the module host. ]*/
		if (handleData->control_socket != -1)
		{
			handleData->control_socket = nn_socket(AF_SP, NN_PAIR);
		}
		if (handleData->control_socket < 0)
		{
			result = handleData->control_socket;
			LogError("remote socket failed to connect to control URL, result = %d, errno = %d", result, nn_errno());
		}
		else
		{
			/*Codes_SRS_OUTPROCESS_MODULE_17_011: [ This function shall connect the request/reply socket to the control_id. ]*/
			int control_connect_id = nn_connect(handleData->control_socket, STRING_c_str(handleData->control_uri));
			if (control_connect_id < 0)
			{
				result = control_connect_id;
				LogError("remote socket failed to connect to control URL, result = %d, errno = %d", result, nn_errno());
			}
			else
			{
				result = 0;
			}
		}
	}
	(void)Unlock(handleData->handle_lock);
	return result;
}

static int connection_setup(OUTPROCESS_HANDLE_DATA* handleData, OUTPROCESS_MODULE_CONFIG * config)
{
	int result;
	handleData->control_socket = -1;
	/*
	* Start with messaging socket.
	*/
	/*Codes_SRS_OUTPROCESS_MODULE_17_008: [ This function shall create a pair socket for sending gateway messages to the module host. ]*/
	handleData->message_socket = nn_socket(AF_SP, NN_PAIR);
	if (handleData->message_socket < 0)
	{
		result = handleData->message_socket;
		LogError("message socket failed to create, result = %d, errno = %d", result, nn_errno());
	}
	else
	{
		/*Codes_SRS_OUTPROCESS_MODULE_17_009: [ This function shall bind and connect the pair socket to the message_uri. ]*/
		int message_bind_id = nn_connect(handleData->message_socket, STRING_c_str(config->message_uri));
		if (message_bind_id < 0)
		{
			result = message_bind_id;
			LogError("remote socket failed to bind to message URL, result = %d, errno = %d", result, nn_errno());
		}
		else
		{
			/*
			* Now, the control socket.
			*/
			/*Codes_SRS_OUTPROCESS_MODULE_17_010: [ This function shall create a request/reply socket for sending control messages to the module host. ]*/
			handleData->control_socket = nn_socket(AF_SP, NN_PAIR);
			if (handleData->control_socket < 0)
			{
				result = handleData->control_socket;
				LogError("remote socket failed to connect to control URL, result = %d, errno = %d", result, nn_errno());
			}
			else
			{
				/*Codes_SRS_OUTPROCESS_MODULE_17_011: [ This function shall connect the request/reply socket to the control_id. ]*/
				int control_connect_id = nn_connect(handleData->control_socket, STRING_c_str(config->control_uri));
				if (control_connect_id < 0)
				{
					result = control_connect_id;
					LogError("remote socket failed to connect to control URL, result = %d, errno = %d", result, nn_errno());
				}
				else
				{
					result = 0;
				}
			}
		}
	}
	return result;
}

static void connection_teardown(OUTPROCESS_HANDLE_DATA* handleData)
{
	if (Lock(handleData->handle_lock) != LOCK_OK)
	{
		LogError("could not lock handle data - attempting to destroy module anyway");
	}
	if (handleData->message_socket >= 0)
		(void)nn_close(handleData->message_socket);
	if (handleData->control_socket >= 0)
		(void)nn_close(handleData->control_socket);
	(void)Unlock(handleData->handle_lock);
}



/**/

/* Control message functions */

static void* serialize_control_message(CONTROL_MESSAGE * msg, int32_t * theMessageSize)
{
	void * result;

	int32_t msg_size = ControlMessage_ToByteArray(msg, NULL, 0);
	if (msg_size < 0)
	{
		LogError("unable to serialize a control message");
		result = NULL;
	}
	else
	{
		result = nn_allocmsg(msg_size, 0);
		if (result == NULL)
		{
			LogError("unable to allocate a control message");
		}
		else
		{
			unsigned char *nn_msg_bytes = (unsigned char *)result;
			ControlMessage_ToByteArray(msg, nn_msg_bytes, msg_size);
			*theMessageSize = msg_size;
		}
	}
	return result;
}

static void* construct_create_message(OUTPROCESS_HANDLE_DATA* handleData, int32_t * creationMessageSize)
{
	void * result;
	uint32_t uri_length = STRING_length(handleData->message_uri);
	char * uri_string = (char*)STRING_c_str(handleData->message_uri);
	uint32_t args_length = STRING_length(handleData->module_args);
	char * args_string = (char*)STRING_c_str(handleData->module_args);
	if (uri_length == 0 || uri_string == NULL || 
		args_length == 0 || args_string == NULL)
	{
		result = NULL;
	}
	else
	{
		/*Codes_SRS_OUTPROCESS_MODULE_17_012: [ This function shall construct a Create Message from configuration. ]*/

		CONTROL_MESSAGE_MODULE_CREATE create_msg =
		{
			{
				CONTROL_MESSAGE_VERSION_CURRENT,	/*version*/
				CONTROL_MESSAGE_TYPE_MODULE_CREATE	/*type*/
			},
			GATEWAY_MESSAGE_VERSION_CURRENT,		/*gateway_message_version*/
			{
				uri_length + 1,						/*uri_size (+1 for null)*/
				(uint8_t)NN_PAIR,					/*uri_type*/
				uri_string							/*uri*/
			},
			args_length + 1,	/*args_size;(+1 for null)*/
			args_string			/*args;*/
		};
		result = serialize_control_message((CONTROL_MESSAGE *)&create_msg, creationMessageSize);
	}
	return result;
}

static void* construct_start_message(OUTPROCESS_HANDLE_DATA* handleData, int32_t * startMessageSize)
{
	void * result;

	CONTROL_MESSAGE start_msg =
	{
		CONTROL_MESSAGE_VERSION_CURRENT,	/*version*/
		CONTROL_MESSAGE_TYPE_MODULE_START	/*type*/
	};
	result = serialize_control_message(&start_msg, startMessageSize);
	return result;
}

static void * construct_destroy_message(OUTPROCESS_HANDLE_DATA* handleData, int32_t * destroyMessageSize)
{
	void * result;

	CONTROL_MESSAGE destroy_msg =
	{
		CONTROL_MESSAGE_VERSION_CURRENT,	/*version*/
		CONTROL_MESSAGE_TYPE_MODULE_DESTROY	/*type*/
	};
	result = serialize_control_message(&destroy_msg, destroyMessageSize);
	return result;
}

/*Codes_SRS_OUTPROCESS_MODULE_17_001: [ This function shall return NULL if configuration is NULL ]*/
/*Codes_SRS_OUTPROCESS_MODULE_17_002: [ This function shall construct a STRING_HANDLE from the given configuration and return the result. ]*/
static void* Outprocess_ParseConfigurationFromJson(const char* configuration)
{
	return (void*)STRING_construct(configuration);
}

static void Outprocess_FreeConfiguration(void* configuration)
{
	if (configuration == NULL)
	{
		/*Codes_SRS_OUTPROCESS_MODULE_17_003: [ If configuration is NULL this function shall do nothing. ]*/
		LogError("configuration is NULL");
	}
	else
	{
		/*Codes_SRS_OUTPROCESS_MODULE_17_004: [ This function shall delete the STRING_HANDLE represented by configuration. ]*/
		STRING_delete((STRING_HANDLE)configuration);
	}
}

static int save_strings(OUTPROCESS_HANDLE_DATA * handleData, OUTPROCESS_MODULE_CONFIG * config)
{
	int result;
	handleData->control_uri = STRING_clone(config->control_uri);
	if (handleData->control_uri == NULL)
	{
		result = -1;
	}
	else
	{
		handleData->message_uri = STRING_clone(config->message_uri);
		if (handleData->message_uri == NULL)
		{
			STRING_delete(handleData->control_uri);
			result = -1;
		}
		else
		{
			handleData->module_args = STRING_clone(config->outprocess_module_args);
			if (handleData->module_args == NULL)
			{
				STRING_delete(handleData->control_uri);
				STRING_delete(handleData->message_uri);
				result = -1;
			}
			else
			{
				result = 0;
			}
		}
	}
	return result;
}

static void delete_strings(OUTPROCESS_HANDLE_DATA * handleData)
{
	STRING_delete(handleData->control_uri);
	STRING_delete(handleData->message_uri);
	STRING_delete(handleData->module_args);
}

static MODULE_HANDLE Outprocess_Create(BROKER_HANDLE broker, const void* configuration)
{
	OUTPROCESS_HANDLE_DATA * module;
	if (
		(broker == NULL) ||
		(configuration == NULL)
		)
	{
		/*Codes_SRS_OUTPROCESS_MODULE_17_005: [ If broker or configuration are NULL, this function shall return NULL. ]*/
		LogError("invalid arguments for outprcess module. broker=[%p], configuration = [%p]", broker, configuration);
		module = NULL;
	}
	else
	{
		OUTPROCESS_MODULE_CONFIG * config = (OUTPROCESS_MODULE_CONFIG*)configuration;
		/*Codes_SRS_OUTPROCESS_MODULE_17_006: [ This function shall allocate memory for the MODULE_HANDLE. ]*/
		module = (OUTPROCESS_HANDLE_DATA*)malloc(sizeof(OUTPROCESS_HANDLE_DATA));
		if (module == NULL)
		{
			/*Codes_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
			LogError("allocation for module failed.");
		}
		else
		{
			/*Codes_SRS_OUTPROCESS_MODULE_17_007: [ This function shall intialize a lock for thread management. ]*/
			module->handle_lock = Lock_Init();
			if (module->handle_lock == NULL)
			{
				/*Codes_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
				LogError("unable to intialize module lock");
				free(module);
				module = NULL;
			}
			else
			{
				if (connection_setup(module, config) < 0)
				{
					/*Codes_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
					LogError("unable to Lock_Init");
					connection_teardown(module);
					Lock_Deinit(module->handle_lock);
					free(module);
					module = NULL;
				}
				else
				{
					THREAD_CONTROL default_thread =
					{
						NULL,
						NULL,
						0
					};
					module->broker = broker;
					module->message_thread = default_thread;
					module->control_thread = default_thread;
					module->async_thread = default_thread;
					module->lifecyle_model = config->lifecycle_model;

					if ((module->message_thread.thread_lock = Lock_Init()) == NULL)
					{
						connection_teardown(module);
						Lock_Deinit(module->handle_lock);
						free(module);
						module = NULL;
					}
					else if ((module->control_thread.thread_lock = Lock_Init()) == NULL)
					{
						connection_teardown(module);
						Lock_Deinit(module->message_thread.thread_lock);
						Lock_Deinit(module->handle_lock);
						free(module);
						module = NULL;
					}
					else if ((module->async_thread.thread_lock = Lock_Init()) == NULL)
					{
						connection_teardown(module);
						Lock_Deinit(module->control_thread.thread_lock);
						Lock_Deinit(module->message_thread.thread_lock);
						Lock_Deinit(module->handle_lock);
						free(module);
						module = NULL;
					}
					else if (save_strings(module, config) != 0)
					{
						connection_teardown(module);
						Lock_Deinit(module->async_thread.thread_lock);
						Lock_Deinit(module->control_thread.thread_lock);
						Lock_Deinit(module->message_thread.thread_lock);
						Lock_Deinit(module->handle_lock);
						free(module);
						module = NULL;
					}
					else
					{
						/*Codes_SRS_OUTPROCESS_MODULE_17_014: [ This function shall wait for a Create Response on the control channel. ]*/
						if (ThreadAPI_Create(&(module->async_thread.thread_handle), outprocessCreate, module) != THREADAPI_OK)
						{
							/*Codes_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
							connection_teardown(module);

							LogError("failed to spawn a thread");
							module->async_thread.thread_handle = NULL;
							connection_teardown(module);
							delete_strings(module);
							Lock_Deinit(module->async_thread.thread_lock);
							Lock_Deinit(module->control_thread.thread_lock);
							Lock_Deinit(module->message_thread.thread_lock);
							Lock_Deinit(module->handle_lock);
							free(module);
							module = NULL;
						}
						else
						{
							int thread_result = -1;
							if (module->lifecyle_model == OUTPROCESS_LIFECYCLE_SYNC)
							{
								if (ThreadAPI_Join(module->async_thread.thread_handle, &thread_result) != THREADAPI_OK)
								{
									/*Codes_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
									LogError("Async create thread failed result=[%d]", thread_result);
									thread_result = -1;
								}
								else
								{
									/*Codes_SRS_OUTPROCESS_MODULE_17_015: [ This function shall expect a successful result from the Create Response to consider the module creation a success. ]*/
									// async thread is done.
									module->async_thread.thread_handle = NULL;
								}
							}
							else
							{
								thread_result = 1;
							}
							if (thread_result < 0)
							{
								/*Codes_SRS_OUTPROCESS_MODULE_17_016: [ If any step in the creation fails, this function shall deallocate all resources and return NULL. ]*/
								connection_teardown(module);
								delete_strings(module);
								Lock_Deinit(module->async_thread.thread_lock);
								Lock_Deinit(module->control_thread.thread_lock);
								Lock_Deinit(module->message_thread.thread_lock);
								Lock_Deinit(module->handle_lock);
								free(module);
								module = NULL;
							}
						}
					}
				}
			}
		}
	}
	return module;
}

static void Outprocess_Destroy(MODULE_HANDLE moduleHandle)
{
	OUTPROCESS_HANDLE_DATA* handleData = moduleHandle;
	/*Codes_SRS_OUTPROCESS_MODULE_17_026: [ If module is NULL, this function shall do nothing. ]*/
	if (handleData != NULL)
	{
		/*tell remote module to stop*/
		int32_t messageSize = 0;
		/*Codes_SRS_OUTPROCESS_MODULE_17_028: [ This function shall construct a Destroy Message. ]*/
		void * destroyMessage = construct_destroy_message(handleData, &messageSize);
		if (destroyMessage != NULL)
		{
			/*Codes_SRS_OUTPROCESS_MODULE_17_029: [ This function shall send the Destroy Message on the control channel. ]*/
			int sendBytes;
			int retry_count = 0;
			do
			{
				sendBytes = nn_send(handleData->control_socket, &destroyMessage, NN_MSG, NN_DONTWAIT);
				if (sendBytes != messageSize)
				{
					// best effort - it's mostly likely our remote module isn't attached.
					retry_count++;
					if (retry_count > 10)
					{
						break;
					}
				}
			} while (sendBytes != messageSize);

			if (sendBytes != messageSize)
			{
				LogError("unable to send destroy control message [%p], continuing with module destroy", destroyMessage);
				nn_freemsg(destroyMessage);
			}
		}
		else
		{
			LogError("unable to create destroy control message, continuing with module destroy");
		}

		/*Codes_SRS_OUTPROCESS_MODULE_17_030: [ This function shall close the message channel socket. ]*/
		/*Codes_SRS_OUTPROCESS_MODULE_17_031: [ This function shall close the control channel socket. ]*/
		connection_teardown(handleData);
		/*then stop the thread*/
		int notUsed;
		THREAD_HANDLE theCurrentThread;
		/*Codes_SRS_OUTPROCESS_MODULE_17_027: [ This function shall ensure thread safety on execution. ]*/
		if (Lock(handleData->message_thread.thread_lock) != LOCK_OK)
		{
			/*Codes_SRS_OUTPROCESS_MODULE_17_032: [ This function shall signal the messaging thread to close. ]*/
			LogError("not able to Lock, still setting the thread to finish");
			handleData->message_thread.thread_flag = THREAD_FLAG_STOP;
		}
		else
		{
			/*Codes_SRS_OUTPROCESS_MODULE_17_032: [ This function shall signal the messaging thread to close. ]*/
			handleData->message_thread.thread_flag = THREAD_FLAG_STOP;
			theCurrentThread = handleData->message_thread.thread_handle;
			Unlock(handleData->message_thread.thread_lock);
		}

		/*Codes_SRS_OUTPROCESS_MODULE_17_033: [ This function shall wait for the messaging thread to complete. ]*/
		if (theCurrentThread != NULL &&
			ThreadAPI_Join(theCurrentThread, &notUsed) != THREADAPI_OK)
		{
			LogError("unable to ThreadAPI_Join message thread, still proceeding in _Destroy");
		}

		/*Codes_SRS_OUTPROCESS_MODULE_17_031: [ This function shall close the control channel socket. ]*/
		if (Lock(handleData->async_thread.thread_lock) != LOCK_OK)
		{
			LogError("not able to Lock, still setting the thread to finish");
			handleData->async_thread.thread_flag = THREAD_FLAG_STOP;
		}
		else
		{
			handleData->async_thread.thread_flag = THREAD_FLAG_STOP;
			theCurrentThread = handleData->async_thread.thread_handle;
			Unlock(handleData->async_thread.thread_lock);
		}

		if (theCurrentThread != NULL &&
			ThreadAPI_Join(theCurrentThread, &notUsed) != THREADAPI_OK)
		{
			LogError("unable to ThreadAPI_Join async create thread, still proceeding in _Destroy");
		}

		if (Lock(handleData->control_thread.thread_lock) != LOCK_OK)
		{
			LogError("not able to Lock, still setting the thread to finish");
			handleData->control_thread.thread_flag = THREAD_FLAG_STOP;
		}
		else
		{
			handleData->control_thread.thread_flag = THREAD_FLAG_STOP;
			theCurrentThread = handleData->control_thread.thread_handle;
			Unlock(handleData->control_thread.thread_lock);
		}

		if (theCurrentThread != NULL &&
			ThreadAPI_Join(theCurrentThread, &notUsed) != THREADAPI_OK)
		{
			LogError("unable to ThreadAPI_Join control thread, still proceeding in _Destroy");
		}

		(void)Lock_Deinit(handleData->message_thread.thread_lock);
		(void)Lock_Deinit(handleData->async_thread.thread_lock);
		(void)Lock_Deinit(handleData->control_thread.thread_lock);
		/* Free remaining resources*/
		delete_strings(handleData);
		/*Codes_SRS_OUTPROCESS_MODULE_17_034: [ This function shall release all resources created by this module. ]*/
		(void)Lock_Deinit(handleData->handle_lock);
		free(handleData);
	}
}

static void Outprocess_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
	OUTPROCESS_HANDLE_DATA* handleData = moduleHandle;
	/*Codes_SRS_OUTPROCESS_MODULE_17_022: [ If module or message_handle is NULL, this function shall do nothing. ]*/
	if (handleData != NULL && messageHandle != NULL)
	{
		/* forward message to remote */
		/*Codes_SRS_OUTPROCESS_MODULE_17_023: [ This function shall serialize the message for transmission on the message channel. ]*/
		int32_t msg_size = Message_ToByteArray(messageHandle, NULL, 0);
		if (msg_size < 0)
		{
			LogError("unable to serialize received message [%p]", messageHandle);
		}
		else
		{
			void* result = nn_allocmsg(msg_size, 0);
			if (result == NULL)
			{
				LogError("unable to allocate buffer for received message [%p]", messageHandle);
			}
			else
			{
				unsigned char *nn_msg_bytes = (unsigned char *)result;
				Message_ToByteArray(messageHandle, nn_msg_bytes, msg_size);
				/*Codes_SRS_OUTPROCESS_MODULE_17_024: [ This function shall send the message on the message channel. ]*/
				int nbytes = nn_send(handleData->message_socket, &result, NN_MSG, 0);
				if (nbytes != msg_size)
				{
					LogError("unable to send buffer to remote for message [%p]", messageHandle);
					/*Codes_SRS_OUTPROCESS_MODULE_17_025: [ This function shall free any resources created. ]*/
					nn_freemsg(result);
				}
			}
		}
	}
}

static void Outprocess_Start(MODULE_HANDLE moduleHandle)
{
	OUTPROCESS_HANDLE_DATA* handleData = moduleHandle;
	/*Codes_SRS_OUTPROCESS_MODULE_17_020: [ This function shall do nothing if module is NULL. ]*/
	if (handleData != NULL)
	{
		/*Codes_SRS_OUTPROCESS_MODULE_17_017: [ This function shall ensure thread safety on execution. ]*/
		/*Codes_SRS_OUTPROCESS_MODULE_17_018: [ This function shall create a thread to handle receiving messages from module host. ]*/
		if (ThreadAPI_Create(&(handleData->message_thread.thread_handle), outprocessMessageThread, handleData) != THREADAPI_OK)
		{
			LogError("failed to spawn message handling thread");
			handleData->message_thread.thread_handle = NULL;
		}
		else if (ThreadAPI_Create(&(handleData->control_thread.thread_handle), outprocessControlThread, handleData) != THREADAPI_OK)
		{
			LogError("failed to spawn control handling thread");
			handleData->control_thread.thread_handle = NULL;
		}
		else
		{
			int32_t startMessageSize = 0;
			void * startmessage = construct_start_message(handleData, &startMessageSize);
			/*Codes_SRS_OUTPROCESS_MODULE_17_019: [ This function shall send a Start Message on the control channel. ]*/
			int sendBytes = nn_send(handleData->control_socket, &startmessage, NN_MSG, 0);
			if (sendBytes != startMessageSize)
			{
				/*Codes_SRS_OUTPROCESS_MODULE_17_021: [ This function shall free any resources created. ]*/
				LogError("unable to send start message [%p]", startmessage);
				nn_freemsg(startmessage);
			}
		}
	}
}


const MODULE_API_1 Outprocess_Module_API_all =
{
	{MODULE_API_VERSION_1},
	Outprocess_ParseConfigurationFromJson,
	Outprocess_FreeConfiguration,
	Outprocess_Create,
	Outprocess_Destroy,
	Outprocess_Receive,
	Outprocess_Start
};
 
