#include "platform.h"
#include "../common.h"
#include <QRegularExpression>

void MacOS::initializeSettings(QSettings &settings)
{
    if (!settings.value("macos/toolchainVersion", 0).toInt()) {
        // Keep the previous configuration available while migrating Linux defaults.
        const QStringList keys = QStringList() << "mode" << "assembler" << "assemblyoptions"
            << "linkingoptions" << "assemblerpath" << "linkerpath" << "starttext";
        foreach (const QString &key, keys) {
            if (settings.contains(key))
                settings.setValue("macos/previous/" + key, settings.value(key));
        }
        const QString name = settings.value("assembler", "NASM").toString();
        const bool unsupported = name != "NASM" && name != "GAS";
        if (unsupported) {
            settings.setValue("assembler", "NASM");
            foreach (const QString &key, QStringList() << "assemblyoptions" << "linkingoptions"
                     << "assemblerpath" << "linkerpath" << "starttext")
                settings.remove(key);
        }
        const QString options = settings.value("assemblyoptions").toString();
        if (options.contains(QRegularExpression("\\belf(32|64)\\b")) ||
            options.contains("--32") || options.contains("--64"))
            settings.remove("assemblyoptions");
        const QString linking = settings.value("linkingoptions").toString();
        if (linking.contains("--no-warn-execstack") || linking.contains("-m32") ||
            linking.contains("-m64"))
            settings.remove("linkingoptions");
        const QString linker = settings.value("linkerpath").toString();
        if (linker == "gcc" || linker == "/usr/bin/gcc")
            settings.remove("linkerpath");
        const QString executable = settings.value("assemblerpath").toString();
        if (executable == "nasm" || executable == "as" || executable == "/usr/bin/as" ||
            (executable == "/usr/local/bin/nasm" && !QFile::exists(executable)))
            settings.remove("assemblerpath");
        settings.setValue("macos/toolchainVersion", 1);
    }
    // Modern macOS cannot link or run 32-bit x86 applications.
    settings.setValue("mode", "x64");
}
