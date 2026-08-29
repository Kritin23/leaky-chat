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

using MessageId = std::uint32_t;
using RequestId = std::uint32_t;

class ClientRequest {
  public:
    enum class Type { Login, SendMessage, GetUsers, Quit };

    Type type;

    RequestId id;
    std::string username;
    std::string message;

    inline static RequestId nextRequestId = 1;
};

class UIEvent {
  public:
    enum class Type {
        MessageReceived,
        MessageNacked,
        UsersReceived,
        LoginResult,
        Connected,
        Disconnected,
        Error
    };

    Type type;

    MessageId messageId{};
    std::string username;
    std::string message;
    std::vector<std::string> users;
    bool success{};
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
     * Called by the client thread.
     *
     * This function must be thread-safe. It does not render anything
     * directly; it merely queues an event for the UI thread.
     */
    void pushEvent(UIEvent event);

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

  private:
    void run();

    void handleInput();
    void handleKey(char c);

    void executeInput();
    void executeCommand(std::string_view input);
    void sendMessage(std::string_view input);

    void processEvents();
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
    std::thread thread_;

    bool running_ = false;

    /*
     * ------------------------------------------------------------------
     * UI -> Client
     * ------------------------------------------------------------------
     */

    std::mutex requestMutex_;
    std::condition_variable requestCV_;
    std::deque<ClientRequest> requests_;

    /*
     * ------------------------------------------------------------------
     * Client -> UI
     * ------------------------------------------------------------------
     */

    std::mutex eventMutex_;
    std::deque<UIEvent> events_;

    /*
     * ------------------------------------------------------------------
     * UI state -- accessed ONLY by UI thread
     * ------------------------------------------------------------------
     */

    struct Message {
        std::optional<MessageId> id;
        std::string sender;
        std::string text;
        bool failed = false;
    };

    struct Command {
        std::string text;
    };

    std::deque<Message> messages_;
    std::deque<Command> commands_;

    std::vector<std::string> onlineUsers_;

    std::string username_;
    std::string selectedPartner_;

    std::string input_;
    std::size_t cursor_ = 0;

    bool connected_ = false;
};