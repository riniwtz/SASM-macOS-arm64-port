#ifndef SASM_UPDATER_H
#define SASM_UPDATER_H
#include <QObject>
#include <QJsonObject>
#include <QUrl>
#include <QNetworkAccessManager>
class QWidget;
struct Release {
    QString tag, digest;
    QUrl download;
    qint64 size = 0;
};
class Updater : public QObject {
    Q_OBJECT
    QWidget *window;
    QNetworkAccessManager network;
    bool busy = false;
    void download(const Release &release);
public:
    explicit Updater(QWidget *parent);
    void check(bool interactive);
    static bool newer(const QString &candidate, const QString &current);
    static Release parse(const QJsonObject &json, const QString &current);
};
#endif
