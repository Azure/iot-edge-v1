
#include <chrono>
#include <string>
#include <sstream>
#include <utility>
#include <algorithm>
#include <random>

#include <parson.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/map.h"
#include "message.h"
#include "module.h"

#include "simulator.h"


typedef struct SIMULATOR_MODULE_HANDLE_TAG
{
    BROKER_HANDLE broker;
    char * device_id;
    size_t message_delay;
    size_t properties_count;
    size_t properties_size;
    size_t message_size;
    char * psuedo_random_buffer;
    bool thread_flag;
    THREAD_HANDLE main_thread;
} SIMULATOR_MODULE_HANDLE;


static void* SimulatorModule_ParseConfigurationFromJson(const char* configuration)
{
    SIMULATOR_MODULE_CONFIG * result;
    if (configuration == NULL)
    {
        LogError("Simulator module expects configuration");
        result = NULL;
    }
    else
    {
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            LogError("unable to json_parse_string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                LogError("unable to json_value_get_object");
                result = NULL;
            }
            else
            {
                const char* deviceIdValue = json_object_get_string(obj, "deviceId");
                if (deviceIdValue == NULL)
                {
                    LogError("deviceId is a required field in configuration");
                    result = NULL;
                }
                else
                {
                    result = (SIMULATOR_MODULE_CONFIG *)malloc(sizeof(SIMULATOR_MODULE_CONFIG));
                    if (result == NULL)
                    {
                        LogError("Could not allocated Module data");
                    }
                    else
                    {
                        if (mallocAndStrcpy_s(&(result->device_id), deviceIdValue) != 0)
                        {
                            LogError("could not allocate memory for deviceID string");
                            free(result);
                            result = NULL;
                        }
                        else
                        {
                            result->message_delay = 0;
                            result->message_size = 256;
                            result->properties_count = 2;
                            result->properties_size = 16;

                            if (json_object_has_value_of_type(obj, "message.delay", JSONNumber))
                            {
                                result->message_delay = static_cast<unsigned int>(json_object_get_number(obj, "message.delay"));
                            }
                            double message_size_value = json_object_get_number(obj, "message.size");
                            if (message_size_value > 0)
                            {
                                result->message_size = static_cast<unsigned int>(message_size_value);
                            }
                            double properties_count_value = json_object_get_number(obj, "properties.count");
                            if (properties_count_value > 0)
                            {
                                result->properties_count = static_cast<unsigned int>(properties_count_value);
                            }
                            double properties_size_value = json_object_get_number(obj, "properties.size");
                            if (properties_size_value > 0)
                            {
                                result->properties_size = static_cast<unsigned int>(properties_size_value);
                            }
                        }
                    }
                }
            }
            json_value_free(json);
        }
    }
    return result;
}

static void SimulatorModule_FreeConfiguration(void* configuration)
{
    if (configuration != NULL)
    {
        SIMULATOR_MODULE_CONFIG * conf = (SIMULATOR_MODULE_CONFIG*)configuration;
        free(conf->device_id);
        free(conf);
    }
}

static MODULE_HANDLE SimulatorModule_Create(BROKER_HANDLE broker, const void* configuration)
{
    SIMULATOR_MODULE_HANDLE * module;

    if ((broker == NULL) ||
        (configuration == NULL))
    {
        LogError("Simulator had a null input. broker: [%p], configuration: [%p]", broker, configuration);
        module = NULL;
    }
    else
    {
        SIMULATOR_MODULE_CONFIG * conf = (SIMULATOR_MODULE_CONFIG *)configuration;
        module = (SIMULATOR_MODULE_HANDLE*)malloc(sizeof(SIMULATOR_MODULE_HANDLE));
        if (module == NULL)
        {
            LogError("Could not allocate memory for module handle");
        }
        else
        {
            if (mallocAndStrcpy_s(&(module->device_id), conf->device_id) != 0)
            {
                LogError("could not allocate memory for deviceID string");
                free(module);
                module = NULL;
            }
            else
            {
                module->broker = broker;
                module->message_delay = conf->message_delay;
                module->message_size = conf->message_size;
                module->properties_count = conf->properties_count;
                module->properties_size = conf->properties_size;
                size_t max_buffer = std::max(module->properties_size, module->message_size);
                module->psuedo_random_buffer = (char*)malloc(max_buffer + 1);
                if (module->psuedo_random_buffer == NULL)
                {
                    LogError("could not allocate memory for deviceID string");
                    free(module->device_id);
                    free(module);
                    module = NULL;
                }
                else
                {
                    std::default_random_engine generator;
                    std::uniform_int_distribution<int> distribution(' ', 'z');
                    for (size_t i = 0; i < max_buffer; i++)
                    {
                        (module->psuedo_random_buffer)[i] = distribution(generator);
                    }
                    (module->psuedo_random_buffer)[max_buffer] = '\0';
                }
            }
        }
    }
    return (MODULE_HANDLE)module;
}

static int SimulatorModule_create_message(SIMULATOR_MODULE_HANDLE * module, MESSAGE_CONFIG* message)
{
    int thread_result;
    MAP_HANDLE property_map = Map_Create(NULL);
    if (property_map == NULL)
    {
        LogError("Allocation of properties map failed.");
        thread_result = -__LINE__;
    }
    else
    {
        std::string property_string(module->psuedo_random_buffer, module->properties_size);
        std::string property_count_string = std::to_string(module->properties_count);

        if (property_string.empty() || property_count_string.empty())
        {
            LogError("Allocation of properties string failed.");
            Map_Destroy(property_map);
            thread_result = -__LINE__;
        }
        else
        {
            if (Map_Add(property_map, "deviceId", module->device_id) != MAP_OK)
            {
                Map_Destroy(property_map);
                thread_result = -__LINE__;
            }
            else if (Map_Add(property_map, "property count", property_count_string.c_str()) != MAP_OK)
            {
                Map_Destroy(property_map);
                thread_result = -__LINE__;
            }
            else
            {
                thread_result = 0;
                for (size_t p = 0; p < module->properties_count; p++)
                {
                    std::ostringstream property_n;
                    property_n << "property" << p;
                    if (Map_Add(property_map, property_n.str().c_str(), property_string.c_str()))
                    {
                        thread_result = -__LINE__;
                        break;
                    }
                }
                if (thread_result != 0)
                {
                    Map_Destroy(property_map);
                    thread_result = -__LINE__;
                }
                else
                {
                    message->source = (const unsigned char*)malloc(module->message_size);
                    if (message->source == NULL)
                    {
                        LogError("unable to allocate message buffer");
                        Map_Destroy(property_map);
                        thread_result = -__LINE__;
                    }
                    else
                    {
                        message->sourceProperties = property_map;
                        message->size = module->message_size;
                        if (module->message_size == 0)
                        {
                            message->source = NULL;
                        }
                        else
                        {
                            memcpy((void*)message->source, module->psuedo_random_buffer, message->size - 1);
                            ((char*)(message->source))[message->size - 1] = '\0';
                        }
                        thread_result = 0;
                    }

                }
            }
        }
    }
    return thread_result;
}

static int SimulatorModule_thread(void * context)
{
    int thread_result;
    SIMULATOR_MODULE_HANDLE * module = (SIMULATOR_MODULE_HANDLE *)context;
    MESSAGE_CONFIG message_to_send;

    thread_result = SimulatorModule_create_message(module, &message_to_send);
    if (thread_result != 0)
    {
        LogError("unable to continue with simulation");
        if (message_to_send.sourceProperties != NULL)
        {
            Map_Destroy(message_to_send.sourceProperties);
        }
        if (message_to_send.source != NULL)
        {
            free( (void*)message_to_send.source);
        }
    }
    else
    {
        using HrClock = std::chrono::high_resolution_clock;
        using MicroSeconds = std::chrono::microseconds;
        long long time_to_wait = module->message_delay * 1000;

        size_t messages_produced = 0;
        thread_result = 0;
        while (module->thread_flag)
        {
            std::chrono::time_point<HrClock, MicroSeconds> t1 = std::chrono::time_point_cast<MicroSeconds>(HrClock::now());
            auto t1_as_int = t1.time_since_epoch().count();
            std::string t1_as_string = std::to_string(t1_as_int);
            messages_produced++;
            std::string messages_produced_as_string = std::to_string(messages_produced);
            if (Map_AddOrUpdate(message_to_send.sourceProperties, "timestamp", t1_as_string.c_str()) != MAP_OK)
            {
                LogError("Unable to update timestamp in message");
                module->thread_flag = false;
                thread_result = -__LINE__;
                break;
            }
            else if (Map_AddOrUpdate(message_to_send.sourceProperties, "sequence number", messages_produced_as_string.c_str()) != MAP_OK)
            {
                LogError("Unable to update sequence number in message");
                module->thread_flag = false;
                thread_result = -__LINE__;
                break;
            }
            else
            {
                MESSAGE_HANDLE next_message = Message_Create(&message_to_send);
                if (next_message == NULL)
                {
                    LogError("Unable to create next message");
                    module->thread_flag = false;
                    thread_result = -__LINE__;
                    break;
                }
                else
                {
                    if (Broker_Publish(module->broker, module, next_message) != BROKER_OK)
                    {
                        LogError("Unable to publish message");
                        module->thread_flag = false;
                        thread_result = -__LINE__;
                        break;
                    }
                    else
                    {
                        Message_Destroy(next_message);
                        std::chrono::time_point<HrClock, MicroSeconds> t2 = std::chrono::time_point_cast<MicroSeconds>(HrClock::now());
                        auto time_to_publish = t2.time_since_epoch().count() - t1_as_int;
                        if (time_to_publish < time_to_wait)
                        {
                            unsigned int remaining_time = static_cast<unsigned int>((time_to_wait - time_to_publish)/1000);
                            ThreadAPI_Sleep(remaining_time);
                        }
                    }
                }
            }
        }
        if (message_to_send.sourceProperties != NULL)
        {
            Map_Destroy(message_to_send.sourceProperties);
        }
        if (message_to_send.source != NULL)
        {
            free((void*)message_to_send.source);
        }
    }
    return thread_result;
}

static void SimulatorModule_Start(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle != NULL)
    {
        SIMULATOR_MODULE_HANDLE * module = (SIMULATOR_MODULE_HANDLE *)moduleHandle;

        module->thread_flag = true;
        if (ThreadAPI_Create(&(module->main_thread), SimulatorModule_thread, module) != 0)
        {
            LogError("Thread Creation failed");
            module->main_thread = NULL;
        }
    }
}

static void SimulatorModule_Destroy(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle == NULL)
    {
        LogError("Destroying a NULL module");
    }
    else
    {
        SIMULATOR_MODULE_HANDLE * module = (SIMULATOR_MODULE_HANDLE *)moduleHandle;
        module->thread_flag = false;
        int thread_result;
        (void)ThreadAPI_Join(module->main_thread, &thread_result);
        if (thread_result != 0)
        {
            LogInfo("Thread ended with non-zero result: %d", thread_result);
        }
        free(module->device_id);
        free(module->psuedo_random_buffer);
        free(module);
    }
}

static void SimulatorModule_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    (void)moduleHandle;
    (void)messageHandle;
}

static const MODULE_API_1 SIMULATOR_APIS_all =
{
    {MODULE_API_VERSION_1},

    SimulatorModule_ParseConfigurationFromJson,
    SimulatorModule_FreeConfiguration,
    SimulatorModule_Create,
    SimulatorModule_Destroy,
    SimulatorModule_Receive,
    SimulatorModule_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(SIMULATOR_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    (void)gateway_api_version;
    return reinterpret_cast< const MODULE_API *>(&SIMULATOR_APIS_all);
}
