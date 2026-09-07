#include "sunshineintegration.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>

void check(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
void put(const QString &path, const QByteArray &data) {
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path); check(file.open(QIODevice::WriteOnly), "write fixture"); file.write(data);
}
QByteArray read(const QString &path) {
  QFile file(path); check(file.open(QIODevice::ReadOnly), "read fixture"); return file.readAll();
}
int main(int argc, char **argv) {
  QTemporaryDir temporary;
  check(temporary.isValid(), "temporary directory");
  const QString home = temporary.path() + "/home";
  qputenv("HOME", home.toUtf8());
  qputenv("XDG_CONFIG_HOME", (home + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (home + "/state").toUtf8());
  QCoreApplication app(argc, argv);
  const QString bin = temporary.path() + "/bin";
  put(bin + "/systemctl", "#!/bin/sh\nif [ \"$2\" = show ]; then echo loaded; fi\nexit 0\n");
  QFile::setPermissions(bin + "/systemctl", QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
  qputenv("PATH", bin.toUtf8() + ':' + qgetenv("PATH"));
  const QString path = home + "/config/sunshine/sunshine.conf";
  const QByteArray original = "# keep this comment\nencoder = nvenc\ncapture = kms\noutput_name = old\n"
      "global_prep_cmd = [{\"do\":\"echo before\",\"undo\":\"echo after\",\"elevated\":false}]\n";
  put(path, original);
  QString error;
  check(SunshineIntegration::configure("Virtual-Test", "virtmonitors-test.service", &error), qPrintable(error));
  const auto configured = read(path);
  check(configured.contains("capture = kwin\n") && configured.contains("output_name = Virtual-Test\n"), "capture and output configured");
  check(configured.contains("encoder = nvenc") && configured.contains("# keep this comment"), "unrelated settings preserved");
  auto match = QRegularExpression("global_prep_cmd = ([^\\n]*)").match(QString::fromUtf8(configured));
  auto hooks = QJsonDocument::fromJson(match.captured(1).toUtf8()).array();
  check(hooks.size() == 2 && hooks[1].toObject()["do"] == "echo before", "other hooks preserved");
  const QString expectedCmd = "/usr/bin/python3 " + home + "/.local/lib/virtmonitors/sunshine-display.py on";
  check(hooks[0].toObject()["do"].toString() == expectedCmd, "prep command unquoted for boost process");
  check(QFile::permissions(home + "/.local/lib/virtmonitors/sunshine-display.py") & QFileDevice::ExeOwner, "helper is executable");
  check(read(path + ".virtmonitors.bak") == original, "original backup preserved");
  check(SunshineIntegration::configure("Virtual-Test", "virtmonitors-test.service", &error), qPrintable(error));
  check(read(path) == configured, "repeated setup is idempotent");
  // Ensure legacy quoted command from prior version is cleaned up on reconfigure
  put(path, "global_prep_cmd = [{\"do\":\"/usr/bin/python3 '" + home.toUtf8() + "/.local/lib/virtmonitors/sunshine-display.py' on\"},{\"do\":\"echo before\"}]\n");
  check(SunshineIntegration::configure("Virtual-Test", "virtmonitors-test.service", &error), qPrintable(error));
  auto matchLegacy = QRegularExpression("global_prep_cmd = ([^\\n]*)").match(QString::fromUtf8(read(path)));
  auto hooksLegacy = QJsonDocument::fromJson(matchLegacy.captured(1).toUtf8()).array();
  check(hooksLegacy.size() == 2, "legacy quoted hook replaced while preserving others");
  check(hooksLegacy[0].toObject()["do"].toString() == expectedCmd, "legacy quoted hook replaced with unquoted command");
  check(SunshineIntegration::configure("Virtual-Second", "virtmonitors-second.service", &error), qPrintable(error));
  check(SunshineIntegration::selected("virtmonitors-second.service"), "new profile selected");
  check(!SunshineIntegration::selected("virtmonitors-test.service"), "old profile deselected");
  check(read(path + ".virtmonitors.bak") == original, "backup not overwritten");
  check(!(QFile::permissions(path) & (QFileDevice::ReadGroup | QFileDevice::ReadOther)), "config kept private");
  put(SunshineIntegration::statePath(), "[]");
  const auto beforeBlocked = read(path);
  check(!SunshineIntegration::configure("Virtual-No", "no.service", &error), "active session setup blocked");
  check(!SunshineIntegration::restart(&error), "active session restart blocked");
  check(read(path) == beforeBlocked, "active config untouched");
  QFile::remove(SunshineIntegration::statePath());
  check(SunshineIntegration::disconnect("virtmonitors-second.service", &error), qPrintable(error));
  check(!read(path).contains("output_name") && read(path).contains("echo before"), "disconnect preserves other hooks");
  check(!SunshineIntegration::selected("virtmonitors-second.service"), "selection removed");
  check(SunshineIntegration::restart(&error), qPrintable(error));
  put(path, "global_prep_cmd = [broken\ncapture = kms\n");
  const auto invalid = read(path);
  check(!SunshineIntegration::configure("Virtual-No", "no.service", &error), "malformed hooks rejected");
  check(read(path) == invalid, "malformed config untouched");
  std::cout << "Sunshine integration checks passed\n";
}
