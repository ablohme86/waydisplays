#include "sunshineintegration.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace SunshineIntegration {
QString configRoot() {
  return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
}
QString statePath() {
  const QString root = qEnvironmentVariable("XDG_STATE_HOME",
                                            QDir::homePath() + "/.local/state");
  return root + "/sunshine-display/desktop.json";
}
namespace {
QString selectionPath() { return configRoot() + "/virtmonitors-sunshine.json"; }
QString helperPath() { return QDir::homePath() + "/.local/lib/virtmonitors/sunshine-display.py"; }
QString shellQuote(QString value) {
  value.replace("'", "'\\''");
  return "'" + value + "'";
}
QString unitQuote(QString value) {
  value.replace('%', "%%");
  value.replace('\\', "\\\\");
  value.replace('"', "\\\"");
  return '"' + value + '"';
}
bool command(const QStringList &args) {
  QProcess process;
  process.start("systemctl", QStringList{"--user"} + args);
  if (!process.waitForStarted(3000) || !process.waitForFinished(15000)) {
    process.kill();
    process.waitForFinished();
    return false;
  }
  return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}
QJsonObject selection() {
  QFile file(selectionPath());
  if (!file.open(QIODevice::ReadOnly)) return {};
  return QJsonDocument::fromJson(file.readAll()).object();
}
QString service() {
  for (const auto &unit : {QStringLiteral("app-dev.lizardbyte.app.Sunshine.service"),
                           QStringLiteral("sunshine.service")}) {
    QProcess process;
    process.start("systemctl", {"--user", "show", unit, "-p", "LoadState", "--value"});
    if (process.waitForFinished(5000) && process.exitCode() == 0 &&
        process.readAllStandardOutput().trimmed() == "loaded") return unit;
  }
  return {};
}
QString setOption(QString text, const QString &key, const QString &value) {
  const QRegularExpression re("(?m)^[ \\t]*" + key + "[ \\t]*=[^\\n]*(?:\\n|$)");
  // Keep exactly one effective value, even if an older config has duplicates.
  text.remove(re);
  if (!text.endsWith('\n')) text += '\n';
  return text + key + " = " + value + '\n';
}
bool managedCommand(const QJsonObject &entry) {
  const QString cmd = entry.value("do").toString();
  // Replace only our helper and the two known predecessors; keep other hooks.
  for (const auto &path : {helperPath(), QDir::homePath() + "/.local/bin/sunshine-display",
                           QDir::homePath() + "/.local/bin/sunshine-display-mode"}) {
    if (cmd == path + " on" || cmd == shellQuote(path) + " on" ||
        cmd == "/usr/bin/python3 " + path + " on" ||
        cmd == "/usr/bin/python3 " + shellQuote(path) + " on") return true;
  }
  return false;
}
bool write(const QString &path, const QByteArray &data, QString *error) {
  if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
    *error = "Kunne ikke opprette mappe for " + path;
    return false;
  }
  if (QFile::exists(path)) {
    QFile old(path);
    if (!old.open(QIODevice::ReadOnly)) {
      *error = "Kunne ikke lese " + path;
      return false;
    }
    if (old.readAll() == data) return true;
    const QString backup = path + ".virtmonitors.bak";
    if (!QFile::exists(backup) && !QFile::copy(path, backup)) {
      *error = "Kunne ikke sikkerhetskopiere " + path;
      return false;
    }
    QFile::setPermissions(backup, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  }
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    *error = "Kunne ikke skrive " + path;
    return false;
  }
  file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  if (file.write(data) != data.size() || !file.commit()) {
    *error = "Kunne ikke lagre " + path;
    return false;
  }
  if (path == helperPath()) {
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
  }
  return true;
}
} // namespace

bool selected(const QString &monitorService) {
  return selection().value("monitor_service").toString() == monitorService;
}

bool configure(const QString &output, const QString &monitorService, QString *error) {
  if (QFile::exists(statePath())) {
    *error = "Koble fra Moonlight før du endrer Sunshine-oppsettet";
    return false;
  }
  const QString unit = service();
  if (unit.isEmpty()) {
    *error = "Fant ingen Sunshine-brukertjeneste. Installer og aktiver Sunshine først";
    return false;
  }
  if (QStandardPaths::findExecutable("python3").isEmpty()) {
    *error = "Python 3 mangler; kan ikke konfigurere skjermbytte";
    return false;
  }
  const QString path = configRoot() + "/sunshine/sunshine.conf";
  QFile input(path);
  if (!input.open(QIODevice::ReadOnly)) {
    *error = "Kunne ikke lese Sunshine-oppsettet. Start Sunshine først";
    return false;
  }
  QString text = QString::fromUtf8(input.readAll());
  const QRegularExpression prepRe("(?m)^[ \\t]*global_prep_cmd[ \\t]*=[ \\t]*([^\\n]*)");
  QJsonArray hooks;
  auto matches = prepRe.globalMatch(text);
  int matchesCount = 0;
  while (matches.hasNext()) {
    const auto match = matches.next();
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(match.captured(1).toUtf8(), &parseError);
    if (++matchesCount > 1 || parseError.error != QJsonParseError::NoError || !doc.isArray()) {
      *error = "Sunshine har et uklart eller ugyldig global_prep_cmd-oppsett; ingen endringer gjort";
      return false;
    }
    for (const auto &value : doc.array()) {
      if (!value.isObject()) {
        *error = "Ugyldig forberedelseskommando i Sunshine; ingen endringer gjort";
        return false;
      }
      if (!managedCommand(value.toObject())) hooks.append(value);
    }
  }
  const QString invocation = "/usr/bin/python3 " + helperPath();
  // Switch before other prep commands, restore after them (Sunshine undoes in reverse).
  hooks.prepend(QJsonObject{{"do", invocation + " on"}, {"undo", invocation + " off"}});
  text = setOption(text, "capture", "kwin");
  text = setOption(text, "output_name", output);
  text = setOption(text, "global_prep_cmd", QString::fromUtf8(QJsonDocument(hooks).toJson(QJsonDocument::Compact)));
  QFile helper(":/sunshine/helpers/sunshine-display.py");
  if (!helper.open(QIODevice::ReadOnly)) {
    *error = "Skjermhjelperen mangler i programmet";
    return false;
  }
  const QString systemd = configRoot() + "/systemd/user/";
  // Use %h in systemd commands and unquoted helper invocation in Sunshine prep hooks.
  const QByteArray watcher = QString(
      "[Unit]\nDescription=Sunshine display switching\nAfter=%1\nPartOf=%1 graphical-session.target\nConditionPathExists=%2\n\n"
      "[Service]\nExecStart=/usr/bin/python3 \"%h/.local/lib/virtmonitors/sunshine-display.py\" watch\n"
      "ExecStopPost=/usr/bin/python3 \"%h/.local/lib/virtmonitors/sunshine-display.py\" off\n"
      "Restart=on-failure\nRestartSec=2\nTimeoutStopSec=30\n\n[Install]\nWantedBy=%1\n")
      .arg(unit, unitQuote(selectionPath())).toUtf8();
  const QJsonObject config{{"output", output}, {"monitor_service", monitorService}, {"sunshine_service", unit}};
  if (!write(helperPath(), helper.readAll(), error) ||
      !write(selectionPath(), QJsonDocument(config).toJson(), error) ||
      !write(systemd + "sunshine-display.service", watcher, error) ||
      !write(systemd + unit + ".d/virtmonitors.conf", "[Unit]\nWants=sunshine-display.service\n", error) ||
      !write(path, text.toUtf8(), error)) return false;
  // Load the units without interrupting Sunshine or an existing watcher.
  if (!command({"daemon-reload"})) {
    *error = "Oppsettet er lagret, men systemd kunne ikke lastes på nytt";
    return false;
  }
  return true;
}

bool restart(QString *error) {
  if (QFile::exists(statePath())) {
    *error = "Koble fra Moonlight før du starter Sunshine på nytt";
    return false;
  }
  const QString unit = selection().value("sunshine_service").toString(service());
  if (unit.isEmpty() || !command({"restart", unit})) {
    *error = "Kunne ikke starte Sunshine på nytt";
    return false;
  }
  return true;
}

bool disconnect(const QString &monitorService, QString *error) {
  if (!selected(monitorService)) return true;
  if (QFile::exists(statePath())) {
    *error = "Koble fra Moonlight før du fjerner Sunshine-koblingen";
    return false;
  }
  const QString path = configRoot() + "/sunshine/sunshine.conf";
  QFile input(path);
  if (!input.open(QIODevice::ReadOnly)) {
    *error = "Kunne ikke lese Sunshine-oppsettet";
    return false;
  }
  QString text = QString::fromUtf8(input.readAll());
  const QRegularExpression re("(?m)^[ \\t]*global_prep_cmd[ \\t]*=[ \\t]*([^\\n]*)");
  const auto match = re.match(text);
  if (match.hasMatch()) {
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(match.captured(1).toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
      *error = "Ugyldig global_prep_cmd; Sunshine-oppsettet ble ikke endret";
      return false;
    }
    QJsonArray hooks;
    for (const auto &entry : doc.array())
      if (!managedCommand(entry.toObject())) hooks.append(entry);
    text = setOption(text, "global_prep_cmd", QString::fromUtf8(QJsonDocument(hooks).toJson(QJsonDocument::Compact)));
  }
  text.remove(QRegularExpression("(?m)^[ \\t]*output_name[ \\t]*=[^\\n]*(?:\\n|$)"));
  if (!write(path, text.toUtf8(), error)) return false;
  if (!command({"stop", "sunshine-display.service"})) {
    *error = "Kunne ikke stoppe skjermovervåkeren";
    return false;
  }
  // Keep the service installed; ConditionPathExists skips it when unconfigured.
  const QString backup = selectionPath() + ".disabled";
  if (!write(backup, QJsonDocument(selection()).toJson(), error)) return false;
  if (!QFile::remove(selectionPath())) {
    *error = "Kunne ikke fjerne Sunshine-koblingen";
    return false;
  }
  return true;
}
} // namespace SunshineIntegration
