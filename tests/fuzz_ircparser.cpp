// libFuzzer harness for IrcParser::parse().
//
// Build modes:
//   Standalone replay (default, any compiler):
//     cmake -B build -DUPLINK_BUILD_TESTS=ON
//     ./build/fuzz_ircparser tests/corpus/*
//
//   libFuzzer (requires clang, ASan, libFuzzer):
//     cmake -B build-fuzz -DCMAKE_CXX_COMPILER=clang++ \
//           -DUPLINK_BUILD_TESTS=ON -DUPLINK_BUILD_FUZZ=ON
//     ./build-fuzz/fuzz_ircparser tests/corpus/
//
// Crash reproduction:
//   ./build/fuzz_ircparser crash-<id>

#include "irc/ircparser.h"

#include <QCoreApplication>
#include <cstddef>
#include <cstdint>

// The fuzzer entry point — called by libFuzzer or the standalone driver below.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const QString input = QString::fromUtf8(
        reinterpret_cast<const char *>(data), static_cast<qsizetype>(size));
    const IrcMessage msg = IrcParser::parse(input);
    (void)msg.isValid();
    return 0;
}

// ---------------------------------------------------------------------------
// Standalone driver — compiled when NOT building as a real libFuzzer binary.
// ---------------------------------------------------------------------------
#ifndef UPLINK_LIBFUZZER

#include <QFile>
#include <QTextStream>
#include <cstdio>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        // No files given — read one line from stdin and parse it.
        QFile f;
        (void)f.open(stdin, QIODevice::ReadOnly);
        const QByteArray data = f.readAll();
        LLVMFuzzerTestOneInput(
            reinterpret_cast<const uint8_t *>(data.constData()),
            static_cast<size_t>(data.size()));
        return 0;
    }

    int parsed = 0;
    for (int i = 1; i < argc; ++i) {
        QFile f(argv[i]);
        if (!f.open(QIODevice::ReadOnly)) {
            QTextStream(stderr) << "fuzz_ircparser: cannot open " << argv[i] << "\n";
            continue;
        }
        const QByteArray data = f.readAll();
        LLVMFuzzerTestOneInput(
            reinterpret_cast<const uint8_t *>(data.constData()),
            static_cast<size_t>(data.size()));
        ++parsed;
    }
    QTextStream(stdout) << "fuzz_ircparser: replayed " << parsed << " input(s) — no crashes\n";
    return 0;
}

#endif // UPLINK_LIBFUZZER
