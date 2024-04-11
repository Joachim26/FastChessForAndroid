#include <engines/uci_engine.hpp>
#include <types/engine_config.hpp>

#include <chrono>
#include <string_view>
#include <thread>

#include "doctest/doctest.hpp"

using namespace fast_chess;

const std::string path = "./tests/mock/engine/";

TEST_SUITE("Uci Engine Communication Tests") {
    TEST_CASE("Test UciEngine Args Simple") {
        EngineConfiguration config;
#ifdef _WIN64
        config.cmd = path + "dummy_engine.exe";
#else
        config.cmd = path + "dummy_engine";
#endif
        config.args = "arg1 arg2 arg3";

        UciEngine uci_engine = UciEngine(config);

        for (const auto& line : uci_engine.output()) {
            std::cout << line << std::endl;
        }

        CHECK(uci_engine.output().size() == 6);
        CHECK(uci_engine.output()[0] == "argv[1]: arg1");
        CHECK(uci_engine.output()[1] == "argv[2]: arg2");
        CHECK(uci_engine.output()[2] == "argv[3]: arg3");
    }

    TEST_CASE("Test UciEngine Args Complex") {
        EngineConfiguration config;
#ifdef _WIN64
        config.cmd = path + "dummy_engine.exe";
#else
        config.cmd = path + "dummy_engine";
#endif
        config.args =
            "--backend=multiplexing "
            "--backend-opts=\"backend=cuda-fp16,(gpu=0),(gpu=1),(gpu=2),(gpu=3)\" "
            "--weights=lc0/BT4-1024x15x32h-swa-6147500.pb.gz --minibatch-size=132 "
            "--nncache=50000000 --threads=5";

        UciEngine uci_engine = UciEngine(config);

        for (const auto& line : uci_engine.output()) {
            std::cout << line << std::endl;
        }

        CHECK(uci_engine.output().size() == 9);
        CHECK(uci_engine.output()[0] == "argv[1]: --backend=multiplexing");
        CHECK(uci_engine.output()[1] ==
              "argv[2]: --backend-opts=backend=cuda-fp16,(gpu=0),(gpu=1),(gpu=2),(gpu=3)");
        CHECK(uci_engine.output()[2] == "argv[3]: --weights=lc0/BT4-1024x15x32h-swa-6147500.pb.gz");
        CHECK(uci_engine.output()[3] == "argv[4]: --minibatch-size=132");
        CHECK(uci_engine.output()[4] == "argv[5]: --nncache=50000000");
        CHECK(uci_engine.output()[5] == "argv[6]: --threads=5");
    }

    TEST_CASE("Testing the EngineProcess class") {
        EngineConfiguration config;
#ifdef _WIN64
        config.cmd = path + "dummy_engine.exe";
#else
        config.cmd = path + "dummy_engine";
#endif
        config.args = "arg1 arg2 arg3";

        UciEngine uci_engine = UciEngine(config);

        CHECK(uci_engine.output().size() == 6);
        CHECK(uci_engine.output()[0] == "argv[1]: arg1");
        CHECK(uci_engine.output()[1] == "argv[2]: arg2");
        CHECK(uci_engine.output()[2] == "argv[3]: arg3");

        uci_engine.uci();
        auto uci       = uci_engine.uciok();
        auto uciOutput = uci_engine.output();

        CHECK(uci);
        CHECK(uciOutput.size() == 3);
        CHECK(uciOutput[0] == "line0");
        CHECK(uciOutput[1] == "line1");
        CHECK(uciOutput[2] == "uciok");
        CHECK(uci_engine.isResponsive());

        uci_engine.writeEngine("sleep");
        const auto res = uci_engine.readEngine("done", std::chrono::milliseconds(100));
        CHECK(res == Process::Status::TIMEOUT);

        uci_engine.writeEngine("sleep");
        const auto res2 = uci_engine.readEngine("done", std::chrono::milliseconds(5000));
        CHECK(res2 == Process::Status::OK);
        CHECK(uci_engine.output().size() == 1);
        CHECK(uci_engine.output()[0] == "done");
    }

    TEST_CASE("Testing the EngineProcess class with lower level class functions") {
        EngineConfiguration config;
#ifdef _WIN64
        config.cmd = path + "dummy_engine.exe";
#else
        config.cmd = path + "dummy_engine";
#endif
        UciEngine uci_engine = UciEngine(config);

        uci_engine.writeEngine("uci");
        const auto res = uci_engine.readEngine("uciok");

        CHECK(res == Process::Status::OK);
        CHECK(uci_engine.output().size() == 3);
        CHECK(uci_engine.output()[0] == "line0");
        CHECK(uci_engine.output()[1] == "line1");
        CHECK(uci_engine.output()[2] == "uciok");

        uci_engine.writeEngine("isready");
        const auto res2 = uci_engine.readEngine("readyok");
        CHECK(res2 == Process::Status::OK);
        CHECK(uci_engine.output().size() == 1);
        CHECK(uci_engine.output()[0] == "readyok");

        uci_engine.writeEngine("sleep");
        const auto res3 = uci_engine.readEngine("done", std::chrono::milliseconds(100));
        CHECK(res3 == Process::Status::TIMEOUT);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        uci_engine.writeEngine("sleep");
        const auto res4 = uci_engine.readEngine("done", std::chrono::milliseconds(5000));
        CHECK(res4 == Process::Status::OK);
        CHECK(uci_engine.output().size() == 1);
        CHECK(uci_engine.output()[0] == "done");
    }
}