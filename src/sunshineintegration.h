#pragma once
#include <QString>

namespace SunshineIntegration {
QString configRoot();
QString statePath();
bool configure(const QString &output, const QString &monitorService,
               QString *error);
bool restart(QString *error);
bool disconnect(const QString &monitorService, QString *error);
bool selected(const QString &monitorService);
}
