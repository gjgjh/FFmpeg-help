#include <unistd.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <thread>

#include <readline/readline.h>
#include <readline/history.h>

extern "C" {
    #include <libffhelp/ffhelp.h>
}

void init_console() {
    rl_bind_key('\t', rl_insert);
}

size_t split(const std::string &txt, std::vector<std::string> &strs) {
    std::string word = "";
    for (auto x : txt) {
        if (x == ' ') {
            if ( word != "") {
                strs.push_back(word);
            }
            word = "";
        } else {
            word = word + x;
        }
    }
    if ( word != "") {
        strs.push_back(word);
    }
    return strs.size();
}

void run_ffmpeg(std::vector<std::string> words) {
    const char* args[128];

    for(size_t i = 0; i < words.size(); i++) {
        args[i] = words[i].c_str();
    }

    ffmain(words.size(), args);
}

int main() {
    char* buf;
    init_console();
    while ((buf = readline(">> ")) != nullptr) {
        if (strlen(buf) > 0) {
            add_history(buf);
        }
        std::string code = buf;

        std::vector<std::string> words;
        split(code, words);

        //run_ffmpeg(words);

        std::thread ff_thread(run_ffmpeg, words);
        ff_thread.join();

        // readline malloc's a new buffer every time.
        free(buf);
    }
}
