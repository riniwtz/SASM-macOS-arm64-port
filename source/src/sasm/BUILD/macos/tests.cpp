#include "../mainwindow.h"
#include "platform.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>

class MacOSTests : public QObject
{
    Q_OBJECT
    QString logs(MainWindow &window)
    {
        QString result;
        foreach (QTextEdit *edit, window.findChildren<QTextEdit *>())
            result += edit->toPlainText() + '\n';
        return result;
    }
    void source(MainWindow &window, const QString &text)
    {
        window.newFile();
        window.findChildren<Tab *>().last()->code->setPlainText(text);
    }
    QString read(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return QString();
        return QString::fromUtf8(file.readAll());
    }
private slots:
    void init()
    {
        QSettings settings("SASM Project", "SASM");
        settings.clear();
        settings.setValue("language", 1);
    }
    void supportFiles()
    {
        const QString data = Common::applicationDataPath();
        QVERIFY(QFile::exists(data + "/include/io64.inc"));
        QVERIFY(QFile::exists(data + "/include/macos.inc"));
        QVERIFY(QFile::exists(data + "/NASM/macro.c"));
        QVERIFY(QFile::exists(data + "/Projects/Hello.asm"));
        QVERIFY(QFile::exists(data + "/lldb_bridge.py"));
        QVERIFY(QFileInfo(Common::executablePath("nasm")).isExecutable());
    }
    void migrateLinuxDefaults()
    {
        QSettings settings("SASM Project", "SASM");
        settings.setValue("mode", "x86");
        settings.setValue("assemblyoptions", "-f elf32 $SOURCE$ -l $LSTOUTPUT$ -o $PROGRAM.OBJ$");
        settings.setValue("linkingoptions", "$PROGRAM.OBJ$ $MACRO.OBJ$ -g -o $PROGRAM$ -m32 -Wl,--no-warn-execstack");
        settings.setValue("linkerpath", "gcc");
        MacOS::initializeSettings(settings);
        QCOMPARE(settings.value("mode").toString(), QString("x64"));
        QVERIFY(!settings.contains("assemblyoptions"));
        QVERIFY(!settings.contains("linkingoptions"));
        QVERIFY(!settings.contains("linkerpath"));
        QCOMPARE(settings.value("macos/previous/mode").toString(), QString("x86"));
        settings.setValue("assemblyoptions", "custom flags");
        MacOS::initializeSettings(settings);
        QCOMPARE(settings.value("assemblyoptions").toString(), QString("custom flags"));
    }
    void newDocumentBuilds()
    {
        MainWindow window(QStringList() << "test");
        window.newFile();
        window.buildProgram();
        QVERIFY2(logs(window).contains("Built successfully."), qPrintable(logs(window)));
        QVERIFY(QFile::exists(Common::pathInTemp("SASMprog.exe")));
        QVERIFY2(!logs(window).contains("no platform load command"), qPrintable(logs(window)));
    }
    void examplesBuild_data()
    {
        QTest::addColumn<QString>("file");
        QTest::newRow("hello") << "Hello.asm";
        QTest::newRow("input") << "Sum.asm";
        QTest::newRow("main-alias") << "Return42.asm";
    }
    void examplesBuild()
    {
        QFETCH(QString, file);
        MainWindow window(QStringList() << "test");
        source(window, read(Common::applicationDataPath() + "/Projects/" + file));
        window.buildProgram();
        QVERIFY2(logs(window).contains("Built successfully."), qPrintable(logs(window)));
    }
    void explicitDarwinEntry()
    {
        MainWindow window(QStringList() << "test");
        source(window, "section .text\nglobal _main\n_main:\n mov eax, 42\n ret\n");
        window.buildProgram();
        QVERIFY2(logs(window).contains("Built successfully."), qPrintable(logs(window)));
    }
    void bundledExampleIsATemplate()
    {
        const QString path = Common::applicationDataPath() + "/Projects/Hello.asm";
        const QString original = read(path);
        MainWindow window(QStringList() << "test");
        window.openFile(path);
        Tab *tab = window.findChildren<Tab *>().last();
        QVERIFY(tab->getCurrentFilePath().isEmpty());
        tab->code->appendPlainText("; editing a template must not alter the application");
        window.buildProgram();
        QVERIFY2(logs(window).contains("Built successfully."), qPrintable(logs(window)));
        QCOMPARE(read(path), original);
    }
    void externalCSymbols()
    {
        MainWindow window(QStringList() << "test");
        source(window, "%include \"io64.inc\"\nCEXTERN strlen\nCEXTERN printf\n"
                       "section .data\ntext db 'abc',0\nsection .text\nglobal CMAIN\nCMAIN:\n"
                       " sub rsp, 8\n lea rdi, [rel text]\n call strlen\n add rsp, 8\n ret\n");
        window.buildProgram();
        QVERIFY2(logs(window).contains("Built successfully."), qPrintable(logs(window)));
    }
    void gasBuilds()
    {
        QSettings settings("SASM Project", "SASM");
        settings.setValue("assembler", "GAS");
        MainWindow window(QStringList() << "test");
        window.newFile();
        window.buildProgram();
        QVERIFY2(logs(window).contains("Built successfully."), qPrintable(logs(window)));
    }
    void missingAssembler()
    {
        QSettings settings("SASM Project", "SASM");
        settings.setValue("assemblerpath", "/nonexistent/sasm-test-nasm");
        MainWindow window(QStringList() << "test");
        window.newFile();
        window.buildProgram();
        QVERIFY(logs(window).contains("Unable to start"));
        QVERIFY(!logs(window).contains("Built successfully."));
        QVERIFY(!QFile::exists(Common::pathInTemp("SASMprog.exe")));
    }
    void invalidAssembly()
    {
        MainWindow window(QStringList() << "test");
        source(window, "not_an_instruction rax\n");
        window.buildProgram();
        QVERIFY(logs(window).contains("Build failed"));
        QVERIFY(!QFile::exists(Common::pathInTemp("macro.o")));
        QVERIFY(!QFile::exists(Common::pathInTemp("SASMprog.exe")));
    }
    void objectOnly()
    {
        QSettings settings("SASM Project", "SASM");
        settings.setValue("disablelinking", true);
        MainWindow window(QStringList() << "test");
        window.newFile();
        window.buildProgram();
        QVERIFY2(logs(window).contains("Object file built successfully"), qPrintable(logs(window)));
        QVERIFY(QFile::exists(Common::pathInTemp("program.o")));
        QVERIFY(!QFile::exists(Common::pathInTemp("macro.o")));
    }
    void runInputOutput()
    {
        MainWindow window(QStringList() << "test");
        source(window, read(Common::applicationDataPath() + "/Projects/Sum.asm"));
        window.buildProgram();
        QVERIFY2(logs(window).contains("Built successfully."), qPrintable(logs(window)));
        QProcess program;
        program.start(Common::pathInTemp("SASMprog.exe"), QStringList());
        if (!program.waitForStarted()) {
            if (qEnvironmentVariableIsSet("SASM_REQUIRE_EXECUTION"))
                QFAIL(qPrintable(program.errorString()));
            QSKIP("Intel execution is unavailable; install/enable Apple's Intel translation support.");
        }
        program.write("20 22\n");
        program.closeWriteChannel();
        QVERIFY(program.waitForFinished());
        QCOMPARE(program.exitCode(), 0);
        QCOMPARE(program.exitStatus(), QProcess::NormalExit);
        QCOMPARE(QString::fromUtf8(program.readAllStandardOutput()).trimmed(), QString("42"));
    }
    void runFromWindow_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("output");
        QTest::newRow("hello") << "Hello.asm" << "" << "Hello from SASM on macOS!";
        QTest::newRow("sum") << "Sum.asm" << "20 22\n" << "42";
    }
    void runFromWindow()
    {
        QFETCH(QString, file);
        QFETCH(QString, input);
        QFETCH(QString, output);
        if (QProcess::execute("/usr/bin/arch", QStringList() << "-x86_64" << "/usr/bin/true") != 0) {
            if (qEnvironmentVariableIsSet("SASM_REQUIRE_EXECUTION"))
                QFAIL("Intel execution is unavailable.");
            QSKIP("Intel execution is unavailable.");
        }
        MainWindow window(QStringList() << "test");
        window.openFile(Common::applicationDataPath() + "/Projects/" + file);
        Tab *tab = window.findChildren<Tab *>().last();
        QDockWidget *inputDock = tab->findChild<QDockWidget *>("inputDock");
        QDockWidget *outputDock = tab->findChild<QDockWidget *>("outputDock");
        QVERIFY(inputDock && outputDock);
        inputDock->findChild<RuQTextEdit *>()->setPlainText(input);
        window.buildProgram();
        QVERIFY2(logs(window).contains("Built successfully."), qPrintable(logs(window)));
        QVERIFY2(!logs(window).contains("no platform load command"), qPrintable(logs(window)));
        window.runProgram();
        QTRY_VERIFY_WITH_TIMEOUT(logs(window).contains("The program finished normally."), 10000);
        QCOMPARE(outputDock->findChild<RuQTextEdit *>()->toPlainText().trimmed(), output);
        QVERIFY2(!logs(window).contains("Unable to run"), qPrintable(logs(window)));
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir directory(QDir::tempPath() + "/SASM tests with spaces-XXXXXX");
    if (!directory.isValid()) return 1;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, directory.path());
    qputenv("TMPDIR", directory.path().toUtf8());
    MacOSTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tests.moc"
