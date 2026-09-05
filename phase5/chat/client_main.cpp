#include <thread>
#include <iostream>

#include "UI.h"
#include "client.h"

int main(int argc, char* argv[]) {

    client_impl::UI ui;
    client_impl::Client client;

    int ServerPort = 10101;
    std::string ServerIP = "127.0.0.1";

    if(argc > 1) {
        if(argc!=3){
            std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
            return 1;
        }
        ServerIP = argv[1];
        ServerPort = std::stoi(argv[2]);
    }

    /*
     * The UI owns the terminal and runs on its own thread.
     */
    ui.start();

    /*
     * The Client owns the socket and runs entirely on this thread.
     */
    std::thread clientThread([&] { client_impl::clientLoop(client, ui, ServerIP, ServerPort); });

    /*
     * Wait for the client thread to finish.
     *
     * The UI thread is managed internally by UI::start()/UI::stop().
     */
    clientThread.join();

    /*
     * Make sure the UI has finished before exiting.
     */
    ui.stop();

    return 0;
}
