#include "UI.h"

#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <thread>

#include "chat/client.h"

namespace {

using namespace std::chrono_literals;

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
    if (running)
        return;

    running = true;
    uiThread = std::thread(&UI::run, this);
}

void UI::stop() {
    if (!running)
        return;

    running = false;

    client::clientBackendSem.release();

    if (uiThread.joinable())
        uiThread.join();
}

void UI::addMessage(Message&& msg) {
    std::lock_guard lock(messageMutex);
    messages.emplace_back(std::move(msg));
}

void UI::addRequest(ClientRequest&& req) {
    std::lock_guard lock(requestMutex);
    requests.emplace_back(std::move(req));
    client::clientBackendSem.release();
}

bool UI::tryGetRequest(ClientRequest& request) {
    std::unique_lock lock(requestMutex);
    if (!running || requests.empty())
        return false;

    request = std::move(requests.front());
    requests.pop_front();

    return true;
}

void UI::run() {
    TerminalMode terminal;

    redraw();

    while (running) {
        processInput();

        if (running)
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
            if (cursor > 0) {
                input.erase(cursor - 1, 1);
                --cursor;
            }
            return;

        // Ctrl-C.
        case 3:
            running = false;
            client::clientBackendSem.release();
            return;

        default:
            break;
    }

    if (c >= 32 && c <= 126) {
        input.insert(cursor, 1, c);
        ++cursor;
    }
}

void UI::submitInput() {
    if (input.empty())
        return;

    std::string input = std::move(this->input);
    if (input[0] == '/')
        handleCommand(input);
    else if (input[0] == '@')
        handleMessage(input);
    else {
        if (!selectedPartner.empty())
            handleMessage("@" + selectedPartner + " " + input);
        else {
            addMessage(Message("No chat partner selected"));
        }
    }

    input.clear();
    cursor = 0;
}

void UI::handleCommand(std::string_view input) {
    // /quit
    if (input == "/quit") {
        ClientRequest request;
        request.type = ClientRequest::Type::Quit;
        request.id = ClientRequest::nextRequestId++;

        addMessage(Message({}, input));
        addRequest(std::move(request));

        running = false;
        return;
    }

    // /who
    if (input == "/who") {
        ClientRequest request;
        request.type = ClientRequest::Type::GetUsers;
        request.id = ClientRequest::nextRequestId++;

        addMessage(Message(request.id, input));
        addRequest(std::move(request));
        return;
    }

    // /login username
    if (input.starts_with("/login ")) {
        std::string username(input.substr(7));

        if (username.empty())
            return;

        username = username;

        ClientRequest request;
        request.type = ClientRequest::Type::Login;
        request.id = ClientRequest::nextRequestId++;
        request.username = std::move(username);

        addMessage(Message(request.id, input));
        addRequest(std::move(request));
        return;
    }

    // /chat username
    if (input.starts_with("/chat ")) {
        std::string username(input.substr(6));
        addMessage(Message({}, input));

        if (!username.empty())
            selectedPartner = std::move(username);
        return;
    }

    addMessage(Message({}, "\033[31m" + std::string(input) + "\033[0m"));
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
        selectedPartner = recipient;
        return;
    }
    std::string recipient(input.substr(1, space - 1));
    std::string message(input.substr(space + 1));

    if (message.empty())
        return;

    selectedPartner = recipient;

    ClientRequest request;
    request.type = ClientRequest::Type::SendMessage;
    request.id = ClientRequest::nextRequestId++;
    request.username = recipient;
    request.message = message;

    addMessage(Message(request.id, "You", message));
    addRequest(std::move(request));

    /*
     * We don't have a MessageId yet because Client::send() happens
     * asynchronously on the client thread.
     *
     * A proper implementation should have the client return the
     * MessageId and then send a MessageSent event back to the UI.
     */
}

void UI::render() {
    std::cout << "\033[2J\033[H";

    renderHistory();

    std::cout << "\n";
    renderInput();

    std::cout.flush();
}

void UI::renderHistory() {
    std::cout << "You have " << messages.size() << " messages\n";

    for (const auto& message : messages) {
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
    if (!selectedPartner.empty())
        std::cout << "[" << selectedPartner << "] ";

    std::cout << "> " << input;

    /*
     * Put the cursor back into the input buffer.
     *
     * This simple version assumes ASCII and a single-line input.
     */
    const std::size_t charsAfterCursor = input.size() - cursor;

    if (charsAfterCursor > 0)
        std::cout << "\033[" << charsAfterCursor << "D";

    // Hide cursor while drawing, then show it.
    std::cout << "\033[?25h";
}

void UI::redraw() {
    std::cout << "\033[?25l";
    render();
}