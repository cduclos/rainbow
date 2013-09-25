#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[])
{
    QCoreApplication *app = new QCoreApplication(argc, argv);
    Log *log = Log::instance();

    if (argc < 2) {
        log->entry(Log::LogLevelCritical, "incorrect arguments");
        return 0;
    }
    Server *server = new Server(app);
    server->setConfigurationFile(QLatin1String(argv[1]));
    if (server->start()) {
        app->exec();
    }
    return 0;
}
