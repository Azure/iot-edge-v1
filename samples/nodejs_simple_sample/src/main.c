#include <stdio.h>
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"

#include "gateway.h"

static void print_usage();
static bool validate_args(int argc, char* argv[]);

int main(int argc, char*argv[])
{
    int result;
    if (validate_args(argc, argv) == false)
    {
        result = 1;
    }
    else
    {
        char *json_path = argv[1];
        GATEWAY_HANDLE gateway = Gateway_Create_From_JSON(json_path);
        if(gateway == NULL)
        {
            LogError("An error ocurred while creating the gateway.");
            result = 1;
        }
        else
        {
            result = 0;
            printf("Gateway is running. Press return to quit.\r\n");

            char enter = 0;
            while (enter != '\r' && enter != '\n')
            {
                enter = getchar();
            }

            printf("Gateway is quitting\r\n");
            Gateway_LL_Destroy(gateway);
        }
    }

    return result;
}

static bool validate_args(int argc, char* argv[])
{
    bool result;
    const int EXPECTED_PARAMS = 2;

    // we expect the path to the gateway JSON to be passed in
    if (argc < EXPECTED_PARAMS)
    {
        print_usage();
        result = false;
    }
    else
    {
        result = true;
        for (int i = 1; i < EXPECTED_PARAMS; i++)
        {
            char* param = argv[i];
            if (strlen(param) == 0)
            {
                result = false;
                break;
            }
        }
    }

    return result;
}

static void print_usage()
{
    printf("\r\nUsage:\r\n"
           "    nodejs_simple_sample <path to json>\r\n\r\n");
}
