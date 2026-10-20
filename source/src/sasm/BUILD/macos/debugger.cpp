// macOS debugger backend. LLDB's Python API supplies structured events over
// a private pipe; the existing editor/debug widgets keep their public API.
#include "../debugger.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
QString quoted(const QString &value)
{
    const QByteArray json = QJsonDocument(QJsonArray() << value).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(json.mid(1, json.size() - 2));
}
void send(QProcess *process, const QJsonObject &message)
{
    process->write(QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n');
}
}

Debugger::Debugger(QTextEdit *edit, const QString &executable, const QString &directory,
                   const QString &input, Assembler *assembler, QWidget *parent)
    : QObject(parent), process(new QProcess(this)), textEdit(edit), offset(0), c(0),
      registersOk(true), gdbRun(false), exePath(executable), workingDirectoryPath(directory),
      inputPath(input), bufferTimer(0), checkGdbRunTimer(new QTimer(this)), watchesCount(0),
      firstAction(true), assembler(assembler), stopped(false), pid(0), dbgSymbols(true), entryPoint(0)
{
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutputToBuffer()));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readOutputToBuffer()));
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int, QProcess::ExitStatus) { emit finished(); });
    connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit printLog(tr("Unable to start LLDB: %1\n").arg(process->errorString()), Qt::red);
            emit finished();
        }
    });
    connect(checkGdbRunTimer, SIGNAL(timeout()), this, SLOT(checkGdbRun()));
    checkGdbRunTimer->setSingleShot(true);
    checkGdbRunTimer->start(20000);
    const QString script = "script import sys; sys.dont_write_bytecode=True; sys.path.insert(0, " + quoted(Common::applicationDataPath())
        + "); import lldb_bridge; lldb_bridge.run(lldb.debugger, " + quoted(exePath) + ", "
        + quoted(workingDirectoryPath) + ", " + quoted(inputPath) + ", "
        + quoted(Common::pathInTemp("program.asm")) + ")";
    process->start("/usr/bin/xcrun", QStringList() << "lldb" << "--no-lldbinit"
                   << "--no-use-colors" << "--batch" << "-o" << script);
}

Debugger::~Debugger()
{
    emit highlightLine(-1);
    disconnect(process, 0, this, 0);
    if (process->state() != QProcess::NotRunning) {
        send(process, QJsonObject{{"command", "quit"}});
        if (!process->waitForFinished(1500)) {
            process->kill();
            process->waitForFinished(1500);
        }
    }
}

void Debugger::readOutputToBuffer()
{
    allErrorBuffer += QString::fromLocal8Bit(process->readAllStandardError());
    buffer += QString::fromUtf8(process->readAllStandardOutput());
    int end;
    while ((end = buffer.indexOf('\n')) >= 0) {
        const QString line = buffer.left(end);
        buffer.remove(0, end + 1);
        if (line.startsWith("SASM:"))
            processMessage(line.mid(5), QString());
        else
            allBuffer += line + '\n';
    }
}

void Debugger::processMessage(QString output, QString)
{
    const QJsonObject message = QJsonDocument::fromJson(output.toUtf8()).object();
    const QString event = message.value("event").toString();
    if (event == "stopped") {
        stopped = true;
        gdbRun = true;
        checkGdbRunTimer->stop();
        pid = message.value("pid").toVariant().toULongLong();
        const int line = message.value("line").toInt(-2);
        emit highlightLine(line > 0 ? line : -2);
        if (firstAction) {
            firstAction = false;
            emit started();
        }
        emit wasStopped();
    } else if (event == "running") {
        stopped = false;
        emit highlightLine(-2);
    } else if (event == "output") {
        emit printOutput(message.value("text").toString());
    } else if (event == "registers") {
        QList<registersInfo> values;
        foreach (const QJsonValue &value, message.value("values").toArray()) {
            const QJsonObject item = value.toObject();
            registersInfo info;
            info.name = item.value("name").toString();
            info.hexValue = item.value("hex").toString();
            info.decValue = item.value("decimal").toString();
            values.append(info);
        }
        emit printRegisters(values);
    } else if (event == "memory") {
        memoryInfo info;
        info.isValid = message.value("valid").toBool();
        info.value = message.value("value").toString();
        watches.append(info);
        if (watches.size() == watchesCount) {
            emit printMemory(watches);
            watches.clear();
        }
    } else if (event == "error" || event == "log") {
        emit printLog(message.value("text").toString() + '\n', event == "error" ? Qt::red : Qt::darkGray);
    } else if (event == "exited") {
        emit printLog(tr("Debugged program exited with code %1.\n").arg(message.value("code").toInt()), Qt::darkGreen);
        emit finished();
    } else if (event == "finished") {
        emit finished();
    }
}

void Debugger::doInput(QString command, DebugActionType action)
{
    QString operation = "command";
    const QString text = command.trimmed();
    if (text == "c") operation = "continue";
    else if (text == "ni") operation = "next";
    else if (text == "si") operation = "step";
    else if (action == infoRegisters) operation = "registers";
    else if (action == infoMemory) operation = "memory";
    if (operation == "continue" || operation == "next" || operation == "step") {
        stopped = false;
        emit highlightLine(-2);
    }
    send(process, QJsonObject{{"command", operation}, {"text", text}});
}

bool Debugger::isStopped() { return stopped; }
void Debugger::pause() { send(process, QJsonObject{{"command", "pause"}}); }
void Debugger::setWatchesCount(int count) { watchesCount = count; watches.clear(); }
void Debugger::changeBreakpoint(quint64 line, bool added)
{
    send(process, QJsonObject{{"command", "breakpoint"}, {"line", int(line)}, {"added", added}});
}
void Debugger::checkGdbRun()
{
    if (!gdbRun) {
        emit printLog(tr("LLDB did not reach main. Check Command Line Tools and Intel translation support.\n")
                      + allBuffer + allErrorBuffer, Qt::red);
        emit finished();
    }
}

// Kept for the common QObject interface used by the GDB backend.
void Debugger::emitStarted() { emit started(); }
void Debugger::processOutput() { readOutputToBuffer(); }
void Debugger::processAction(QString output, QString error) { processMessage(output, error); }
