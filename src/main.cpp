#include <QCoreApplication>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "server.h"

const char *optstring = "c:h";
void usage()
{
    printf("Usage: rainbow -c <configuration file>\n");
    printf("rainbow -h\n");
    printf("configuration file: specifies the operational parameters of rainbow, such as the port and such.\n");
    printf("See the attached configuration.xml for more info");
}

int main(int argc, char *argv[])
{
    QCoreApplication *app = new QCoreApplication(argc, argv);
    Log *log = Log::instance();
    char *configuration_file = NULL;

    int result = 0;

    while ((result = getopt(argc, argv, optstring)) != -1)
    {
        switch (result)
        {
        case 'c':
            configuration_file = strdup(optarg);
            break;
        case 'h':
        default:
            usage();
            exit (0);
        }
    }

    Server *server = new Server(app);
    server->setConfigurationFile(QLatin1String(configuration_file));
    if (server->start()) {
        app->exec();
    }
    return 0;
}
