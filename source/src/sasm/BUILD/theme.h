#ifndef SASM_THEME_H
#define SASM_THEME_H
#include <QColor>
#include <QSettings>
namespace Theme {
void apply(QSettings &settings);
void select(QSettings &settings, const QString &mode);
QColor logColor(const QColor &color);
}
#endif
