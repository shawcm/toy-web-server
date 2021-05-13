#include "serve.h"

int main(int argc, const char *argv[]) {
    uint16_t port = 2377;
    const char* rootdir = "/var/www/";

    if (argc >= 2) {
        port = std::stoi(argv[1]);
    }
    if (argc >= 3) {
        rootdir = argv[2];
    }

    Server server(port, rootdir);
    server.serve();  
}