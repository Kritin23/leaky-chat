#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "utils/Packet.hh"

class ClientRequest {
  public:
    enum class Type { Login, SendMessage, GetUsers, Quit };

    Type type;

    SequenceNo id;
    std::string username;
    std::string message;

    inline static SequenceNo nextRequestId = 1;
};

struct Message {
    std::optional<SequenceNo> id;
    std::string sender;
    std::string text;
    bool failed = false;

    Message(std::string_view sv) : id{}, sender{}, text(sv) {}
    Message(SequenceNo id, std::string_view text)
        : id(id), text(text) {}
    Message(SequenceNo id, std::string_view sender, std::string_view text)
        : id(id), sender(sender), text(text) {}
    
    
};

class UI {
  public:
    UI() = default;
    ~UI();

    UI(const UI&) = delete;
    UI& operator=(const UI&) = delete;

    /*
     * Starts the UI rendering/input thread.
     */
    void start();

    /*
     * Stops the UI thread and restores the terminal.
     */
    void stop();

    /*
     * Called by the client thread or application shutdown code.
     */
    void disconnect();

    /*
     * Client thread consumes requests from here.
     *
     * Returns false when the UI has stopped and no more requests exist.
     */
    bool tryGetRequest(ClientRequest& request);

    void addMessage(Message&& msg);

    void editMessage(SequenceNo seq, auto transform) {
        std::lock_guard lock(messageMutex);
        for (auto it = messages.rbegin(); it != messages.rend(); it++) {
            if (it->id == seq) {
                transform(*it);
            }
        }
    }

    void addRequest(ClientRequest&& req);

  private:
    void run();

    void handleInput();
    void handleKey(char c);

    void executeInput();
    void executeCommand(std::string_view input);
    void sendMessage(std::string_view input);

    void processInput();

    void handleCommand(std::string_view);
    void handleMessage(std::string_view);

    void submitInput();

    void render();
    void renderMessages();
    void renderInput();
    void renderHistory();

    void redraw();

  private:
    std::thread uiThread;

    bool running = false;

    std::mutex requestMutex;
    std::deque<ClientRequest> requests;

    std::mutex messageMutex;
    std::deque<Message> messages;

    std::string username;
    std::string selectedPartner;

    std::string input;
    std::size_t cursor = 0;
};