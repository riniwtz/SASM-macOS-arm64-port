#include "theme.h"
#include <QApplication>
#include <QStyleHints>
#include <QWidget>
#include <QAction>

namespace Theme {
static const QStringList syntax = {"keywords", "registers", "numbers", "memory", "labels",
                                  "comments", "system", "iomacro", "quotation"};
static QStringList colorKeys()
{
    QStringList keys = {"backgroundcolor", "fontcolor", "linenumberpanelcolor", "linenumberfontcolor",
                        "currentlinecolor", "debuglinecolor"};
    for (const auto &name : syntax) keys << name + "color" << name + "colorbg";
    return keys;
}
void select(QSettings &settings, const QString &mode)
{
    if (settings.value("appearance/mode", "custom").toString() == "custom") {
        for (const auto &key : colorKeys())
            if (settings.contains(key)) settings.setValue("appearance/custom/" + key, settings.value(key));
    }
    settings.setValue("appearance/mode", mode);
    if (mode == "system") qApp->styleHints()->unsetColorScheme();
    if (mode == "custom") {
        for (const auto &key : colorKeys()) {
            if (settings.contains("appearance/custom/" + key))
                settings.setValue(key, settings.value("appearance/custom/" + key));
            else settings.remove(key);
        }
    }
    apply(settings);
}
void apply(QSettings &settings)
{
    // Preserve an existing custom editor palette on migration.
    if (!settings.contains("appearance/mode"))
        settings.setValue("appearance/mode", settings.contains("backgroundcolor") ? "custom" : "system");
    const QString mode = settings.value("appearance/mode").toString();
    const bool dark = mode == "dark" || (mode == "system" &&
        qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark) ||
        (mode == "custom" && settings.value("backgroundcolor", QColor(Qt::white)).value<QColor>().lightness() < 128);
    if (mode != "system") qApp->styleHints()->setColorScheme(dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
    const QColor base = dark ? QColor("#1e2025") : QColor("#ffffff");
    const QColor text = dark ? QColor("#e5e7eb") : QColor("#202124");
    if (mode != "custom") {
        const QList<QColor> foreground = dark
            ? QList<QColor>{"#82aaff", "#c792ea", "#f7ba7e", "#89ddff", "#f07178", "#9aa4b2", "#80cbc4", "#c3e88d", "#ecc48d"}
            : QList<QColor>{"#174ea6", "#7b1fa2", "#a34b00", "#006b7d", "#a61c35", "#57636e", "#00695c", "#315e13", "#805500"};
        for (int i = 0; i < syntax.size(); ++i) {
            settings.setValue(syntax[i] + "color", foreground[i]);
            settings.setValue(syntax[i] + "colorbg", base);
        }
        settings.setValue("backgroundcolor", base);
        settings.setValue("fontcolor", text);
        settings.setValue("linenumberpanelcolor", dark ? QColor("#272a31") : QColor("#f0f2f5"));
        settings.setValue("linenumberfontcolor", dark ? QColor("#aeb6c3") : QColor("#526071"));
        settings.setValue("currentlinecolor", dark ? QColor("#303642") : QColor("#edf3ff"));
        settings.setValue("debuglinecolor", dark ? QColor("#62521f") : QColor("#ffe18a"));
    }
    QPalette palette;
    palette.setColor(QPalette::Window, dark ? QColor("#292c33") : QColor("#f4f5f7"));
    palette.setColor(QPalette::Base, settings.value("backgroundcolor", base).value<QColor>());
    palette.setColor(QPalette::AlternateBase, dark ? QColor("#252830") : QColor("#eef1f5"));
    for (auto role : {QPalette::Text, QPalette::WindowText, QPalette::ButtonText, QPalette::ToolTipText}) {
        palette.setColor(role, settings.value("fontcolor", text).value<QColor>());
        palette.setColor(QPalette::Disabled, role, dark ? QColor("#858b98") : QColor("#777e89"));
    }
    palette.setColor(QPalette::Button, dark ? QColor("#343840") : QColor("#e6e9ee"));
    palette.setColor(QPalette::ToolTipBase, dark ? QColor("#343840") : QColor("#ffffff"));
    palette.setColor(QPalette::Highlight, QColor("#2764bd"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Link, dark ? QColor("#82aaff") : QColor("#174ea6"));
    palette.setColor(QPalette::Light, dark ? QColor("#49505d") : QColor("#ffffff"));
    palette.setColor(QPalette::Mid, dark ? QColor("#414752") : QColor("#b9c0cb"));
    palette.setColor(QPalette::Dark, dark ? QColor("#17191e") : QColor("#8b95a5"));
    palette.setColor(QPalette::BrightText, QColor("#ff8080"));
    qApp->setPalette(palette);
    // Older editors set an explicit palette; update open documents and dialogs too.
    for (QWidget *widget : QApplication::allWidgets()) {
        for (QAction *action : widget->findChildren<QAction *>()) {
            if (action->objectName().startsWith("theme-")) action->setChecked(action->objectName() == "theme-" + mode);
        }
        widget->setPalette(palette);
        widget->update();
    }
}
QColor logColor(const QColor &color)
{
    if (qApp->palette().color(QPalette::Base).lightness() >= 128) return color;
    if (color == Qt::red) return QColor("#ff8585");
    if (color == Qt::darkGreen) return QColor("#9cdda2");
    if (color == Qt::darkGray) return QColor("#bec6d4");
    return color.lightness() < 100 ? color.lighter(230) : color;
}
}
