#include <stdio.h>
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/platform.h"

#include "gateway.h"

const int EXPECTED_PARAMS = 2;
const char* PLATFORM_INIT_FAILURE = "Failed to initialize the platform.\r\n";
const char* GATEWAY_CREATION_ERROR = "An error occurred while creating the gateway.\r\n";
const char* GATEWAY_STATUS_RUNNING = "Gateway is running. Press return to quit.\r\n";
const char* GATEWAY_STATUS_QUITTING = "Gateway is quitting.\r\n";
const char* USAGE_STRING = "\r\nUsage:\r\ngw <path to config json>\r\n\r\n";

void print_usage() {
  printf(USAGE_STRING);
}

bool validate_args(int argc, char* argv[])
{
  if (argc < EXPECTED_PARAMS)
  {
    print_usage();
    return false;
  }

  size_t i = 1;
  while (i < EXPECTED_PARAMS && strlen(argv[i]) != 0) {
    ++i;
  }

  return (i == EXPECTED_PARAMS);
}

int main(int argc, char* argv[]) {
  // Validate the arguments.
  if (!validate_args(argc, argv)) return 1;

  // initialize the IoTHub platform.
  if (platform_init() != 0) {
    LogError(PLATFORM_INIT_FAILURE);
    return 1;
  }

  // Create the GW instance from given configuration file.
  char* json_path = argv[1];
  GATEWAY_HANDLE gateway = Gateway_CreateFromJson(json_path);
  if(gateway == NULL) {
      LogError(GATEWAY_CREATION_ERROR);
      return 1;
  }

  printf(GATEWAY_STATUS_RUNNING);

  // Provide Key to break.
  // TODO: shall we remove it ?
  char enter = 0;
  while (enter != '\r' && enter != '\n') {
    enter = getchar();
  }

  printf(GATEWAY_STATUS_QUITTING);

  Gateway_Destroy(gateway);
  platform_deinit();

  return 0;
}
