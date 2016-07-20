#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <sys/signalfd.h>
#include <unistd.h>
#include <signal.h>

#include <glib.h>

#include "azure_c_shared_utility/xlogging.h"

#include "gateway.h"

static void print_usage();
static bool validate_args(int argc, char* argv[]);
static void handle_control_c(GMainLoop* loop);
static gboolean signal_handler(
    GIOChannel *channel,
    GIOCondition condition,
    gpointer user_data
);

guint g_event_source_id = 0;

int main(int argc, char*argv[])
{
    // create glib loop and register a ctrl+c handler
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    handle_control_c(loop);

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
            printf("Gateway is running.\r\n");
                    
            // run the glib loop
            g_main_loop_run(loop);

            printf("Gateway is quitting\r\n");
            Gateway_LL_Destroy(gateway);
        }
    }
    
    return result;
}

static bool validate_args(int argc, char* argv[])
{
    bool result;
    const size_t EXPECTED_PARAMS = 2;

    // we expect the path to the gateway JSON to be passed in
    if (argc < EXPECTED_PARAMS)
    {
        print_usage();
        result = false;
    }
    else
    {
        result = true;
        for (size_t i = 1; i < EXPECTED_PARAMS; i++)
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
           "    ble_gateway_hl <path to json>\r\n\r\n");
}

static void handle_control_c(GMainLoop* loop)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
    {
        LogError("Failed to set signal mask");
    }
    else
    {
        int fd = signalfd(-1, &mask, 0);
        if (fd < 0)
        {
            LogError("Failed to create signal descriptor");
        }
        else
        {
            GIOChannel *channel = g_io_channel_unix_new(fd);
            if (channel == NULL)
            {
                close(fd);
                LogError("Failed to create IO channel");
            }
            else
            {
                g_io_channel_set_close_on_unref(channel, TRUE);
                g_io_channel_set_encoding(channel, NULL, NULL);
                g_io_channel_set_buffered(channel, FALSE);

                g_event_source_id = g_io_add_watch(
                    channel,
                    G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
                    signal_handler,
                    loop
                );

                if (g_event_source_id == 0)
                {
                    LogError("g_io_add_watch failed");
                }

                g_main_loop_ref(loop);
                g_io_channel_unref(channel);
            }
        }
    }
}

static gboolean signal_handler(
    GIOChannel *channel,
    GIOCondition condition,
    gpointer user_data
)
{
    static unsigned int terminated = 0;
    struct signalfd_siginfo si;
    int fd;
    GMainLoop* loop = (GMainLoop*)user_data;
    gboolean result;

    if (condition & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
        LogInfo("Quitting...");
        g_main_loop_unref(loop);
        g_source_remove(g_event_source_id);
        g_main_loop_quit(loop);
        result = FALSE;
    }
    else
    {
        fd = g_io_channel_unix_get_fd(channel);

        if (read(fd, &si, sizeof(si)) != sizeof(si))
        {
            LogError("read from fd failed");
            result = FALSE;
        }
        else
        {
            switch (si.ssi_signo) {
                case SIGINT:
                    LogInfo("Caught ctrl+c - quitting...");
                    g_main_loop_unref(loop);
                    g_source_remove(g_event_source_id);
                    g_main_loop_quit(loop);
                    break;
                case SIGTERM:
                    if (terminated == 0) {
                        LogInfo("Caught SIGTERM - quitting...");
                        g_main_loop_unref(loop);
                        g_source_remove(g_event_source_id);
                        g_main_loop_quit(loop);
                    }

                    terminated = 1;
                    break;
            }

            result = TRUE;
        }
    }

    return result;
}