#include "UI.h"

#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>

namespace {

class TerminalMode {
  public:
    TerminalMode() {
        if (tcgetattr(STDIN_FILENO, &original_) == -1)
            return;

        termios raw = original_;

        // Character-at-a-time input.
        raw.c_lflag &= ~(ICANON | ECHO);

        // read() returns immediately when input is available.
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;

        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        active_ = true;
    }

    ~TerminalMode() {
        if (active_)
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_);
    }

  private:
    termios original_{};
    bool active_ = false;
};

}  // namespace

UI::~UI() {
    stop();
}

void UI::start() {
    if (running_)
        return;

    running_ = true;
    thread_ = std::thread(&UI::run, this);
}

void UI::stop() {
    if (!running_)
        return;

    running_ = false;

    requestCV_.notify_all();

    if (thread_.joinable())
        thread_.join();
}

void UI::pushEvent(UIEvent event) {
    {
        std::lock_guard lock(eventMutex_);
        events_.push_back(std::move(event));
    }

    /*
     * At this point the UI thread is sleeping in read().
     *
     * For the simple implementation below, it will wake periodically.
     * A pipe/eventfd can be added later to wake it immediately.
     */
}

bool UI::tryGetRequest(ClientRequest& request) {
    std::unique_lock lock(requestMutex_);
    if (!running_ || requests_.empty()) 
        return false;

    request = std::move(requests_.front());
    requests_.pop_front();

    return true;
}

void UI::run() {
    TerminalMode terminal;

    redraw();

    while (running_) {
        processEvents();
        processInput();

        if (running_)
            redraw();

        // Avoid spinning at 100% CPU.
        usleep(10000);
    }

    // Restore a sane terminal state.
    std::cout << "\033[0m";
    std::cout << "\033[?25h";
    std::cout << "\033[2J\033[H";
    std::cout.flush();
}

void UI::processInput() {
    char c;

    while (true) {
        ssize_t n = read(STDIN_FILENO, &c, 1);

        if (n == 1) {
            handleKey(c);
            continue;
        }

        if (n == -1 && errno == EINTR)
            continue;

        break;
    }
}

void UI::handleKey(char c) {
    switch (c) {
        case '\n':
        case '\r':
            submitInput();
            return;

        case 127:
        case '\b':
            if (cursor_ > 0) {
                input_.erase(cursor_ - 1, 1);
                --cursor_;
            }
            return;

        // Ctrl-C.
        case 3:
            running_ = false;
            requestCV_.notify_all();
            return;

        default:
            break;
    }

    if (c >= 32 && c <= 126) {
        input_.insert(cursor_, 1, c);
        ++cursor_;
    }
}

void UI::submitInput() {
    if (input_.empty())
        return;

    std::string input = std::move(input_);
    input_.clear();
    cursor_ = 0;

    if (input[0] == '/')
        handleCommand(input);
    else if (input[0] == '@')
        handleMessage(input);
    else {
        // Treat a plain message as a message to the selected partner.
        if (!selectedPartner_.empty())
            handleMessage("@" + selectedPartner_ + " " + input);
        else {
            messages_.push_back({0, "", "No chat partner selected", false});
        }
    }
}

void UI::handleCommand(std::string_view input) {
    // /quit
    if (input == "/quit") {
        ClientRequest request;
        request.type = ClientRequest::Type::Quit;
        request.id = ClientRequest::nextRequestId++;

        {
            std::lock_guard lock(requestMutex_);
            requests_.push_back(std::move(request));
        }

        requestCV_.notify_one();

        running_ = false;
        return;
    }

    // /who
    if (input == "/who") {
        ClientRequest request;
        request.type = ClientRequest::Type::GetUsers;
        request.id = ClientRequest::nextRequestId++;

        {
            std::lock_guard lock(requestMutex_);
            requests_.push_back(std::move(request));
        }

        requestCV_.notify_one();

        commands_.push_back({std::string(input)});
        return;
    }

    // /login username
    if (input.starts_with("/login ")) {
        std::string username(input.substr(7));

        if (username.empty())
            return;

        username_ = username;

        ClientRequest request;
        request.type = ClientRequest::Type::Login;
        request.id = ClientRequest::nextRequestId++;
        request.username = std::move(username);

        {
            std::lock_guard lock(requestMutex_);
            requests_.push_back(std::move(request));
        }

        requestCV_.notify_one();

        commands_.push_back({std::string(input)});
        return;
    }

    // /chat username
    if (input.starts_with("/chat ")) {
        std::string username(input.substr(6));

        if (!username.empty())
            selectedPartner_ = std::move(username);

        commands_.push_back({std::string(input)});
        return;
    }

    messages_.push_back(
        {0, "", "Unknown command: " + std::string(input), false});
}

void UI::handleMessage(std::string_view input) {
    // Expected format:
    //
    //     @username message
    //

    if (input.empty() || input[0] != '@')
        return;

    const auto space = input.find(' ');

    if (space <= 1) {
        return;
    } else if (space == std::string_view::npos) {
        std::string_view recipient(input.substr(1, space - 1));
        selectedPartner_ = recipient;
        return;
    }
    std::string recipient(input.substr(1, space - 1));
    std::string message(input.substr(space + 1));

    if (message.empty())
        return;

    selectedPartner_ = recipient;

    ClientRequest request;
    request.type = ClientRequest::Type::SendMessage;
    request.id = ClientRequest::nextRequestId++;
    request.username = recipient;
    request.message = message;

    {
        std::lock_guard lock(requestMutex_);
        requests_.push_back(request);
    }

    requestCV_.notify_one();

    /*
     * We don't have a MessageId yet because Client::send() happens
     * asynchronously on the client thread.
     *
     * A proper implementation should have the client return the
     * MessageId and then send a MessageSent event back to the UI.
     */
}

void UI::processEvents() {
    std::deque<UIEvent> events;

    {
        std::lock_guard lock(eventMutex_);
        events.swap(events_);
    }

    for (auto& event : events) {
        switch (event.type) {
            case UIEvent::Type::MessageReceived:
                messages_.push_back(
                    {event.messageId, event.username, event.message, false});
                break;

            case UIEvent::Type::MessageNacked:
                for (auto& message : messages_) {
                    if (message.id == event.messageId) {
                        message.failed = true;
                        break;
                    }
                }
                break;

            case UIEvent::Type::UsersReceived:
                onlineUsers_ = std::move(event.users);
                break;

            case UIEvent::Type::LoginResult:
                if (event.success) {
                    username_ = event.username;
                } else {
                    messages_.push_back({0,
                                         "Server",
                                         "Login failed: Please retry (possibly "
                                         "with a different username)",
                                         false});
                }
                break;

            case UIEvent::Type::Connected:
                messages_.push_back({0,
                                     "Server",
                                     "You are connected. Please set a username",
                                     false});

            case UIEvent::Type::Disconnected:
                running_ = false;
                break;

            case UIEvent::Type::Error:
                messages_.push_back({0, "", "Error", false});
                break;
        }
    }
}

void UI::render() {
    std::cout << "\033[2J\033[H";

    renderHistory();

    std::cout << "\n";
    renderInput();

    std::cout.flush();
}

void UI::renderHistory() {
    for (const auto& command : commands_)
        std::cout << "c: " << command.text << '\n';

    if (!onlineUsers_.empty()) {
        std::cout << "\nOnline users:\n";

        for (const auto& user : onlineUsers_)
            std::cout << "  " << user << '\n';

        std::cout << '\n';
    }

    for (const auto& message : messages_) {
        if (!message.sender.empty())
            std::cout << "[" << message.sender << "] ";
        else
            std::cout << "UI";

        std::cout << ": " << message.text;

        if (message.failed)
            std::cout << " [FAILED]";

        std::cout << '\n';
    }
}

void UI::renderInput() {
    if (!selectedPartner_.empty())
        std::cout << "[" << selectedPartner_ << "] ";

    std::cout << "> " << input_;

    /*
     * Put the cursor back into the input buffer.
     *
     * This simple version assumes ASCII and a single-line input.
     */
    const std::size_t charsAfterCursor = input_.size() - cursor_;

    if (charsAfterCursor > 0)
        std::cout << "\033[" << charsAfterCursor << "D";

    // Hide cursor while drawing, then show it.
    std::cout << "\033[?25h";
}

void UI::redraw() {
    std::cout << "\033[?25l";
    render();
}