#include <thread>

#include "UI.h"
#include "client.h"

int main() {
    UI ui;
    Client client;

    /*
     * The UI owns the terminal and runs on its own thread.
     */
    ui.start();

    /*
     * The Client owns the socket and runs entirely on this thread.
     */
    std::thread clientThread([&] { client::clientLoop(client, ui); });

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
