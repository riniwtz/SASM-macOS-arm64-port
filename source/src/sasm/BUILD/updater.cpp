#include "updater.h"
#include "common.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFileInfo>

#include <QMessageBox>
#include <QNetworkReply>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QVersionNumber>
#include <memory>

static const QString repository = "riniwtz/SASM-macOS-arm64-port";
static QVersionNumber version(const QString &tag)
{
    const auto match = QRegularExpression("^v?(\\d+)\\.(\\d+)\\.(\\d+)(?:-macos\\.(\\d+))?$").match(tag);
    if (!match.hasMatch()) return {};
    QList<int> parts;
    for (int i = 1; i <= 4; ++i) {
        bool ok = true;
        int value = match.captured(i).isEmpty() ? 0 : match.captured(i).toInt(&ok);
        if (!ok) return {};
        parts << value;
    }
    return QVersionNumber(parts);
}
bool Updater::newer(const QString &candidate, const QString &current)
{
    const auto next = version(candidate), installed = version(current);
    return !next.isNull() && !installed.isNull() && QVersionNumber::compare(next, installed) > 0;
}
Release Updater::parse(const QJsonObject &json, const QString &current)
{
    Release result;
    const QString tag = json.value("tag_name").toString();
    if (json.value("draft").toBool() || json.value("prerelease").toBool() || !newer(tag, current)) return result;
    // Asset names use the full release tag to distinguish successive macOS builds.
    const QString expected = "SASM-" + QString(tag).remove(QRegularExpression("^v")) + "-macOS-arm64.zip";
    for (const auto &entry : json.value("assets").toArray()) {
        auto asset = entry.toObject();
        const QUrl url(asset.value("browser_download_url").toString());
        QString digest = asset.value("digest").toString();
        qint64 size = asset.value("size").toInteger();
        if (asset.value("name").toString() != expected || asset.value("state").toString() != "uploaded") continue;
        if (url.scheme() != "https" || url.host() != "github.com" ||
            !url.path().startsWith("/" + repository + "/releases/download/" + tag + "/") ||
            !url.userInfo().isEmpty() || url.port() != -1 || url.hasQuery() || url.hasFragment()) continue;
        if (!QRegularExpression("^sha256:[a-fA-F0-9]{64}$").match(digest).hasMatch() || size <= 0 || size > 500*1024*1024) continue;
        result = {tag, digest.mid(7).toLower(), url, size};
        break;
    }
    return result;
}
Updater::Updater(QWidget *parent) : QObject(parent), window(parent), network(this)
{
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] { check(false); });
    timer->start(60*60*1000);
    QTimer::singleShot(5000, this, [this] { check(false); });
}
void Updater::check(bool interactive)
{
    QSettings settings("SASM Project", "SASM");
    const auto now = QDateTime::currentSecsSinceEpoch();
    if (busy) return;
    if (!interactive && (!settings.value("updates/automatic", true).toBool() ||
        now - settings.value("updates/lastCheck", 0).toLongLong() < 24*60*60)) return;
    busy = true;
    QNetworkRequest request(QUrl("https://api.github.com/repos/" + repository + "/releases/latest"));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "SASM/" SASM_VERSION);
    request.setTransferTimeout(15000);
    auto reply = network.get(request);
    connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64) {
        if (received > 1024*1024) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, interactive, now] {
        busy = false;
        reply->deleteLater();
        auto fail = [this, interactive](const QString &message) {
            if (interactive) QMessageBox::warning(window, tr("Software Update"), message);
        };
        if (reply->error() != QNetworkReply::NoError) {
            fail(tr("Could not check GitHub for updates. %1").arg(reply->errorString())); return;
        }
        QJsonParseError error;
        auto document = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            fail(tr("GitHub returned an invalid release response.")); return;
        }
        QSettings settings("SASM Project", "SASM");
        settings.setValue("updates/lastCheck", now);
        auto json = document.object();
        const Release release = parse(json, SASM_RELEASE_TAG);
        if (release.tag.isEmpty()) {
            if (newer(json.value("tag_name").toString(), SASM_RELEASE_TAG))
                fail(tr("A newer release exists, but it has no verified macOS ARM64 update package yet."));
            else if (interactive) QMessageBox::information(window, tr("Software Update"), tr("You are up to date (%1).").arg(SASM_RELEASE_TAG));
            return;
        }
        if (!interactive && settings.value("updates/skipped").toString() == release.tag) return;
        QMessageBox prompt(QMessageBox::Information, tr("Software Update"),
            tr("SASM %1 is available. You have %2. Download the update?").arg(release.tag, SASM_RELEASE_TAG), QMessageBox::NoButton, window);
        auto get = prompt.addButton(tr("Download Update"), QMessageBox::AcceptRole);
        auto skip = prompt.addButton(tr("Skip This Version"), QMessageBox::RejectRole);
        prompt.addButton(tr("Later"), QMessageBox::RejectRole);
        prompt.exec();
        if (prompt.clickedButton() == skip) settings.setValue("updates/skipped", release.tag);
        if (prompt.clickedButton() == get) download(release);
    });
}
void Updater::download(const Release &release)
{
    QString destination = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../..");
    if (!destination.endsWith(".app") || !QFileInfo(QFileInfo(destination).absolutePath()).isWritable() ||
        destination.contains("/AppTranslocation/") || destination.startsWith("/Volumes/")) {
        QMessageBox::warning(window, tr("Software Update"), tr("Move SASM to a writable Applications folder before installing updates."));
        return;
    }
    auto directory = std::make_shared<QTemporaryDir>(QDir::tempPath() + "/SASM-update-XXXXXX");
    auto file = std::make_shared<QFile>(directory->filePath("update.zip"));
    if (!directory->isValid() || !file->open(QIODevice::WriteOnly)) {
        QMessageBox::warning(window, tr("Software Update"), tr("Could not create the update download.")); return;
    }
    busy = true;
    auto hash = std::make_shared<QCryptographicHash>(QCryptographicHash::Sha256);
    auto progress = new QProgressDialog(tr("Downloading SASM…"), tr("Cancel"), 0, 100, window);
    progress->setAttribute(Qt::WA_DeleteOnClose);
    progress->setAutoClose(false);
    progress->setAutoReset(false);
    progress->setWindowModality(Qt::WindowModal);
    progress->show();
    QNetworkRequest request(release.download);
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto reply = network.get(request);
    connect(progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
    connect(reply, &QNetworkReply::readyRead, reply, [reply, file, hash, release] {
        QByteArray bytes = reply->readAll();
        if (file->size() + bytes.size() > release.size || file->write(bytes) != bytes.size()) { reply->abort(); return; }
        hash->addData(bytes);
    });
    connect(reply, &QNetworkReply::downloadProgress, progress, [progress, release](qint64 received, qint64) {
        progress->setValue(int(qMin(qint64(100), received*100/release.size)));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, progress, directory, file, hash, release, destination] {
        busy = false;
        const bool cancelled = progress->wasCanceled();
        progress->close();
        reply->deleteLater();
        file->close();
        if (cancelled) return;
        if (reply->error() != QNetworkReply::NoError || file->size() != release.size ||
            QString::fromLatin1(hash->result().toHex()) != release.digest) {
            QMessageBox::warning(window, tr("Software Update"), tr("The download failed verification. Your installed app has not changed.")); return;
        }
        const QString helper = directory->filePath("install_update.py");
        if (!QFile::copy(Common::applicationDataPath() + "/install_update.py", helper)) {
            QMessageBox::warning(window, tr("Software Update"), tr("The update installer is missing.")); return;
        }
        if (QMessageBox::question(window, tr("Software Update"),
            tr("The update is ready. Install and relaunch SASM? You will be asked to save any open files."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
        if (!window->close()) return;
        const QString python = Common::executablePath("python3");
        const QStringList args = {helper, file->fileName(), release.digest, destination,
                                  QString::number(QCoreApplication::applicationPid()), release.tag};
        if (QProcess::startDetached(python, args)) {
            directory->setAutoRemove(false);
        } else {
            window->show();
            QMessageBox::warning(window, tr("Software Update"), tr("Could not start the update installer. Python 3 is required."));
        }
    });
}
