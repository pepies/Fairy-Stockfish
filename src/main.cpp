/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2022 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <sstream>
#include <cstring>
#include <mutex>
#include <queue>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

#include "bitboard.h"
#include "endgame.h"
#include "position.h"
#include "psqt.h"
#include "search.h"
#include "syzygy/tbprobe.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"

#include "piece.h"
#include "variant.h"
#include "xboard.h"


using namespace Stockfish;


int input_pipe[2];
int output_pipe[2];
std::mutex output_mutex;

class Redirector : public std::streambuf {
public:
    Redirector(std::streambuf *buf) : original(buf) {}
    int overflow(int c) override {
        if (c != EOF) {
            char ch = c;
            write(output_pipe[1], &ch, 1);
        }
        return original->sputc(c);
    }

private:
    std::streambuf *original;
};

Redirector redirector(std::cout.rdbuf());

extern "C" {
    void initialize_engine(int argc, char* argv[]) {
        std::cout.rdbuf(&redirector);

        pieceMap.init();
        variants.init();
        CommandLine::init(argc, argv);
        UCI::init(Options);
        Tune::init();
        PSQT::init(variants.find(Options["UCI_Variant"])->second);
        Bitboards::init();
        Position::init();
        Bitbases::init();
        Endgames::init();
        Threads.set(size_t(Options["Threads"]));
        Search::clear(); // After threads are up
        Eval::NNUE::init();
    }

    void run_engine_loop(int argc, char* argv[]) {
        UCI::loop(argc, argv);
    }

    void shutdown_engine() {
        Threads.set(0);
        variants.clear_all();
        pieceMap.clear_all();
        delete XBoard::stateMachine;
    }

    const char* get_output() {
        std::lock_guard<std::mutex> lock(output_mutex);
        static char buffer[4096];
        int n = read(output_pipe[0], buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            return buffer;
        }
        return nullptr;
    }

    void send_command(const char* command) {
        write(input_pipe[1], command, strlen(command));
        write(input_pipe[1], "\n", 1);
    }

    void prepare_input_redirection() {
        pipe(input_pipe);
        pipe(output_pipe);
        dup2(input_pipe[0], STDIN_FILENO);
        fcntl(output_pipe[0], F_SETFL, O_NONBLOCK); // Set output pipe to non-blocking mode
    }
}

int main(int argc, char* argv[]) {

  std::cout << engine_info() << std::endl;

  pieceMap.init();
  variants.init();
  CommandLine::init(argc, argv);
  UCI::init(Options);
  Tune::init();
  PSQT::init(variants.find(Options["UCI_Variant"])->second);
  Bitboards::init();
  Position::init();
  Bitbases::init();
  Endgames::init();
  Threads.set(size_t(Options["Threads"]));
  Search::clear(); // After threads are up
  Eval::NNUE::init();

  UCI::loop(argc, argv);

  Threads.set(0);
  variants.clear_all();
  pieceMap.clear_all();
  delete XBoard::stateMachine;
  return 0;
}
