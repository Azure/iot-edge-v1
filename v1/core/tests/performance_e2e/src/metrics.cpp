
#include <chrono>
#include <iostream>
#include <string>
#include <map>
#include <exception>

#include <parson.h>

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/map.h"
#include "message.h"
#include "module.h"

#include "simulator.h"

using HrClock = std::chrono::high_resolution_clock;
using MicroSeconds = std::chrono::microseconds;
using HrTime = std::chrono::time_point<HrClock, MicroSeconds>;
using Counter = long long;

template<typename ValueT>
struct SimpleAccumulator
{
    typedef ValueT result_type;

    ValueT accumulation;
    ValueT max;
    Counter accumulation_count;

    SimpleAccumulator() : accumulation(0), max(0), accumulation_count(0)
    {
    }
    
    result_type getSum()
    {
        return this->accumulation;
    }
    Counter getCount()
    {
        return this->accumulation_count;
    }
    result_type getMean()
    {
        ValueT mean(0);
        if (accumulation_count != 0)
        {
            mean = accumulation / accumulation_count;
        }
        return mean;
    }
    void add(ValueT v)
    {
        this->accumulation += v;
        this->accumulation_count++;
        if (v > max)
        {
            max = v;
        }
    }
};

struct METRICS_PER_DEVICE
{
    Counter messages_received;
    Counter seqence_number;
    Counter out_of_sequence_messages;
    Counter messages_lost;
    METRICS_PER_DEVICE() : messages_received(0), seqence_number(0), out_of_sequence_messages(0), messages_lost(0)
    {
    }
} ;

using PerDeviceMap = std::map<std::string, METRICS_PER_DEVICE>;

typedef struct METRICS_MODULE_HANDLE_TAG
{
    BROKER_HANDLE broker;
    bool started;
    HrTime start_time;
    Counter all_messages_received;
    Counter non_conforming_messages;
    SimpleAccumulator<MicroSeconds> latency;
    PerDeviceMap *per_device_metrics;
} METRICS_MODULE_HANDLE;


static void* MetricsModule_ParseConfigurationFromJson(const char* configuration)
{
    (void)configuration;
    return NULL;
}

static void MetricsModule_FreeConfiguration(void* configuration)
{
    (void)configuration;
}

static MODULE_HANDLE MetricsModule_Create(BROKER_HANDLE broker, const void* configuration)
{
    METRICS_MODULE_HANDLE * module;

    if (broker == NULL) 
    {
        LogError(" had a null input. broker: [%p], configuration: [%p]", broker, configuration);
        module = NULL;
    }
    else
    {
        module = (METRICS_MODULE_HANDLE*)malloc(sizeof(METRICS_MODULE_HANDLE));
        if (module == NULL)
        {
            LogError("Could not allocate memory for module handle");
        }
        else
        {
            HrTime init_time;
            Counter init_count(0);
            SimpleAccumulator<MicroSeconds> init_accumulator;

            module->broker = broker;
            module->started = false;
            module->start_time = init_time;
            module->all_messages_received = init_count;
            module->non_conforming_messages = init_count;
            module->latency = init_accumulator;
            module->per_device_metrics = new PerDeviceMap();
        }
    }
    return (MODULE_HANDLE)module;
}

static void MetricsModule_Start(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle != NULL)
    {
        METRICS_MODULE_HANDLE * module = (METRICS_MODULE_HANDLE *)moduleHandle;
        module->start_time = std::chrono::time_point_cast<MicroSeconds>(HrClock::now());
        module->started = true;
    }
}

static void MetricsModule_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    if (moduleHandle != NULL && messageHandle != NULL)
    {
        HrTime received_time = std::chrono::time_point_cast<MicroSeconds>(HrClock::now());
        METRICS_MODULE_HANDLE * module = (METRICS_MODULE_HANDLE *)moduleHandle;
        module->all_messages_received++;

        CONSTMAP_HANDLE message_properties = Message_GetProperties(messageHandle);
        if (message_properties == NULL)
        {
            module->non_conforming_messages++;
        }
        else
        {
            const char * timestamp_property = ConstMap_GetValue(message_properties, "timestamp");
            const char * seq_num_property = ConstMap_GetValue(message_properties, "sequence number");
            const char * deviceId_property = ConstMap_GetValue(message_properties, "deviceId");
            if ((timestamp_property == NULL) || (seq_num_property == NULL) || (deviceId_property == NULL))
            {
                module->non_conforming_messages++;
            }
            else
            {
                try
                {
                    MicroSeconds timestamp_duration(std::stoll(timestamp_property));
                    HrTime timestamp(timestamp_duration);
                    MicroSeconds current_latency = received_time - timestamp;
                    module->latency.add(current_latency);

                    if (deviceId_property == NULL)
                    {
                        module->non_conforming_messages++;
                    }
                    else
                    {
                        std::string deviceId(deviceId_property);
                        METRICS_PER_DEVICE& per_device = (*module->per_device_metrics)[deviceId];
                        per_device.messages_received++;
                        per_device.seqence_number++;

                        Counter sequence_number(std::stoll(seq_num_property));
                        if (sequence_number != per_device.seqence_number)
                        {
                            per_device.out_of_sequence_messages++;
                            if (sequence_number > per_device.seqence_number)
                            {
                                per_device.messages_lost += (sequence_number - per_device.seqence_number);
                            }
                            per_device.seqence_number = sequence_number;
                        }
                    }
                }
                catch (std::exception & e)
                {
                    LogError("non-conforming message: exception caught: %s", e.what());
                    module->non_conforming_messages++;
                }
            }

            ConstMap_Destroy(message_properties);
        }
    }
}

static void MetricsModule_Destroy(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle == NULL)
    {
        LogError("Destroying a NULL module");
    }
    else
    {
        METRICS_MODULE_HANDLE * module = (METRICS_MODULE_HANDLE *)moduleHandle;
        if (module->started)
        {
            HrTime destroy_time = std::chrono::time_point_cast<MicroSeconds>(HrClock::now());
            MicroSeconds duration = destroy_time - module->start_time;
            std::cout
                << "Module Metrics:" << std::endl
                << "---------------" << std::endl
                << "Duration (ms): " << duration.count() / 1000 << std::endl
                << "Messages received: " << module->all_messages_received << std::endl
                << "Non-Conforming Messages: " << module->non_conforming_messages << std::endl
                << "Message Latency (average microseconds): " << module->latency.getMean().count() << std::endl
                << "Message Latency (max microseconds): " << module->latency.max.count() << std::endl
                << "Devices Discovered: " << module->per_device_metrics->size() << std::endl;
            for (PerDeviceMap::iterator d = module->per_device_metrics->begin();
                d != module->per_device_metrics->end();
                d++)
            {
                std::cout
                    << "Device: " << (*d).first << std::endl
                    << "Message count: " << (*d).second.messages_received << std::endl
                    << "Out of Sequence Count: " << (*d).second.out_of_sequence_messages << std::endl
                    << "Messages Lost: " << (*d).second.messages_lost << std::endl;
            }
        }    
        delete (module->per_device_metrics);
        free(module);
    }
}



static const MODULE_API_1 METRICS_APIS_all =
{
    {MODULE_API_VERSION_1},

    MetricsModule_ParseConfigurationFromJson,
    MetricsModule_FreeConfiguration,
    MetricsModule_Create,
    MetricsModule_Destroy,
    MetricsModule_Receive,
    MetricsModule_Start
};

#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_API* MODULE_STATIC_GETAPI(METRICS_MODULE)(MODULE_API_VERSION gateway_api_version)
#else
MODULE_EXPORT const MODULE_API* Module_GetApi(MODULE_API_VERSION gateway_api_version)
#endif
{
    (void)gateway_api_version;
    return reinterpret_cast< const MODULE_API *>(&METRICS_APIS_all);
}
