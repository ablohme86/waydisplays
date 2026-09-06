#include "monitormanager.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <algorithm>

namespace {
QString configDir() {
  return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}
QString profilesPath() {
  return configDir() + QStringLiteral("/profiles.json");
}
QString userSystemdDir() {
  return QDir::homePath() + QStringLiteral("/.config/systemd/user");
}
QString sunshineConfig() {
  return QDir::homePath() + QStringLiteral("/.config/sunshine/sunshine.conf");
}
} // namespace

MonitorManager::MonitorManager(QObject *parent) : QAbstractListModel(parent) {
  load();
  if (m_profiles.isEmpty())
    importExisting();
        this->refresh();
}

int MonitorManager::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : m_profiles.size();
}

QVariant MonitorManager::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_profiles.size())
    return {};
  const auto &p = m_profiles.at(index.row());
  switch (role) {
  case IdRole:
    return p.id;
  case NameRole:
    return p.name;
  case WidthRole:
    return p.width;
  case HeightRole:
    return p.height;
  case RefreshRole:
    return p.refresh;
  case ScaleRole:
    return p.scale;
  case PortRole:
    return p.port;
  case ActiveRole:
    return p.active;
  case OutputRole:
    return p.output();
  case SunshineRole:
    return p.sunshine;
  case ImportedRole:
    return p.imported;
  default:
    return {};
  }
}

QHash<int, QByteArray> MonitorManager::roleNames() const {
  return {{IdRole, "profileId"},      {NameRole, "name"},
          {WidthRole, "widthPx"},     {HeightRole, "heightPx"},
          {RefreshRole, "refreshHz"}, {ScaleRole, "displayScale"},
          {PortRole, "port"},         {ActiveRole, "active"},
          {OutputRole, "outputName"}, {SunshineRole, "sunshine"},
          {ImportedRole, "imported"}};
}

bool MonitorManager::backendAvailable() const {
  return !QStandardPaths::findExecutable(QStringLiteral("krfb-virtualmonitor"))
              .isEmpty() &&
         !QStandardPaths::findExecutable(QStringLiteral("systemctl")).isEmpty();
}

QString MonitorManager::backendDescription() const {
  return backendAvailable()
             ? QStringLiteral("KDE Wayland · krfb-virtualmonitor")
             : QStringLiteral("krfb-virtualmonitor mangler");
}

QVariantMap MonitorManager::profile(int row) const {
  if (row < 0 || row >= m_profiles.size())
    return {};
  const auto &p = m_profiles.at(row);
  return {
      {"id", p.id},          {"name", p.name},         {"width", p.width},
      {"height", p.height},  {"refresh", p.refresh},   {"scale", p.scale},
      {"port", p.port},      {"sunshine", p.sunshine}, {"password", p.password},
      {"output", p.output()}};
}

QString MonitorManager::safeId(const QString &name) {
  QString id = name.toLower();
  id.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")),
             QStringLiteral("-"));
  id.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
  return id.isEmpty() ? QStringLiteral("monitor") : id.left(48);
}

QString MonitorManager::generatePassword() const {
  QByteArray bytes(16, Qt::Uninitialized);
  for (auto &c : bytes)
    c = char(QRandomGenerator::system()->generate() & 0xff);
  return QString::fromLatin1(bytes.toHex());
}

void MonitorManager::saveProfile(const QVariantMap &v) {
  const QString name = v.value("name").toString().trimmed();
  const int width = v.value("width").toInt(),
            height = v.value("height").toInt();
  const int refresh = v.value("refresh", 60).toInt(),
            port = v.value("port", 5901).toInt();
  const double scale = v.value("scale", 1.0).toDouble();
  if (!QRegularExpression(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9_-]{0,47}$"))
           .match(name)
           .hasMatch()) {
    setStatus(
        QStringLiteral("Navnet kan bare inneholde bokstaver, tall, _ og -"));
    emit operationFinished(false);
    return;
  }
  if (width < 320 || width > 16384 || height < 200 || height > 8640 ||
      refresh < 24 || refresh > 360 || port < 1024 || port > 65535 ||
      scale < 0.5 || scale > 4.0) {
    setStatus(
        QStringLiteral("Kontroller oppløsning, frekvens, skalering og port"));
    emit operationFinished(false);
    return;
  }

  const QString requestedId = v.value("id").toString();
  int row = -1;
  for (int i = 0; i < m_profiles.size(); ++i)
    if (m_profiles[i].id == requestedId) {
      row = i;
      break;
    }
  Profile p = row >= 0 ? m_profiles[row] : Profile{};
  p.id = requestedId.isEmpty() ? safeId(name) : requestedId;
  if (row < 0) {
    QString base = p.id;
    int suffix = 2;
    while (std::any_of(m_profiles.cbegin(), m_profiles.cend(),
                       [&](const Profile &x) { return x.id == p.id; }))
      p.id = base + QString::number(suffix++);
    p.serviceName =
        QStringLiteral("virtmonitors-") + p.id + QStringLiteral(".service");
    p.password = generatePassword();
  }
  p.name = name;
  p.width = width;
  p.height = height;
  p.refresh = refresh;
  p.port = port;
  p.scale = scale;
  p.sunshine = v.value("sunshine", true).toBool();
  if (!v.value("password").toString().isEmpty())
    p.password = v.value("password").toString();
  const bool wasActive = row >= 0 && m_profiles[row].active;
  QString error;
  if (wasActive)
    run(QStringLiteral("systemctl"),
        {QStringLiteral("--user"), QStringLiteral("stop"),
         m_profiles[row].serviceName});
  if (!writeService(p, &error)) {
    if (wasActive)
      run(QStringLiteral("systemctl"),
          {QStringLiteral("--user"), QStringLiteral("start"),
           m_profiles[row].serviceName});
    setStatus(error);
    emit operationFinished(false);
    return;
  }
  p.imported = false;
  if (row >= 0) {
    m_profiles[row] = p;
    emit dataChanged(index(row), index(row));
  } else {
    beginInsertRows({}, m_profiles.size(), m_profiles.size());
    m_profiles.append(p);
    endInsertRows();
  }
  persist();
  run(QStringLiteral("systemctl"),
      {QStringLiteral("--user"), QStringLiteral("daemon-reload")});
  if (wasActive) {
    run(QStringLiteral("systemctl"),
        {QStringLiteral("--user"), QStringLiteral("start"), p.serviceName});
    setBusy(true);
    configureWhenReady(p, 40);
  }
  setStatus(row >= 0 ? QStringLiteral("Profilen ble lagret")
                     : QStringLiteral("Skjermprofilen ble opprettet"));
  emit operationFinished(true);
  if (!wasActive)
    this->refresh();
}

bool MonitorManager::writeService(const Profile &p, QString *error) {
  QDir().mkpath(userSystemdDir());
  const QString path = userSystemdDir() + QLatin1Char('/') + p.serviceName;
  if (!p.serviceName.startsWith(QStringLiteral("virtmonitors-")) &&
      QFileInfo::exists(path) &&
      !QFileInfo::exists(path + QStringLiteral(".virtmonitors.bak")))
    QFile::copy(path, path + QStringLiteral(".virtmonitors.bak"));
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    *error =
        QStringLiteral("Kunne ikke skrive tjenesten: ") + file.errorString();
    return false;
  }
  const QString unit =
      QStringLiteral(
          "[Unit]\nDescription=Virtual monitor "
          "%1\nAfter=graphical-session.target\n\n"
          "[Service]\nType=simple\nExecStart=/usr/bin/krfb-virtualmonitor "
          "--name %1 --resolution %2x%3 --password %4 --port %5 --scale %6\n"
          "Restart=on-failure\nRestartSec=2\n")
          .arg(p.name)
          .arg(p.width)
          .arg(p.height)
          .arg(p.password)
          .arg(p.port)
          .arg(p.scale);
  file.write(unit.toUtf8());
  if (!file.commit()) {
    *error = QStringLiteral("Kunne ikke lagre tjenesten");
    return false;
  }
  QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  return true;
}

void MonitorManager::removeProfile(int row) {
  if (row < 0 || row >= m_profiles.size())
    return;
  const auto p = m_profiles.at(row);
  run(QStringLiteral("systemctl"),
      {QStringLiteral("--user"), QStringLiteral("stop"), p.serviceName});
  const QString servicePath =
      userSystemdDir() + QLatin1Char('/') + p.serviceName;
  QFile::rename(servicePath,
                servicePath + QStringLiteral(".virtmonitors.deleted-") +
                    QString::number(QDateTime::currentSecsSinceEpoch()));
  beginRemoveRows({}, row, row);
  m_profiles.removeAt(row);
  endRemoveRows();
  persist();
  run(QStringLiteral("systemctl"),
      {QStringLiteral("--user"), QStringLiteral("daemon-reload")});
  setStatus(QStringLiteral("Skjermprofilen ble slettet"));
}

void MonitorManager::startProfile(int row) {
  if (row < 0 || row >= m_profiles.size() || m_busy)
    return;
  if (!backendAvailable()) {
    setStatus(
        QStringLiteral("Installer krfb for å opprette virtuelle KDE-skjermer"));
    return;
  }
  setBusy(true);
  const Profile p = m_profiles.at(row);
  QString out;
  if (!run(QStringLiteral("systemctl"),
           {QStringLiteral("--user"), QStringLiteral("start"), p.serviceName},
           &out)) {
    setBusy(false);
    setStatus(QStringLiteral("Kunne ikke starte: ") + out.trimmed());
    return;
  }
  configureWhenReady(p, 40);
}

void MonitorManager::configureWhenReady(const Profile &p, int attemptsLeft) {
  QString outputs;
  run(QStringLiteral("kscreen-doctor"), {QStringLiteral("-o")}, &outputs);
  if (outputs.contains(p.output())) {
    run(QStringLiteral("kscreen-doctor"),
        {QStringLiteral("output.%1.enable").arg(p.output())});
    if (p.sunshine)
      updateSunshine(p);
    setBusy(false);
    setStatus(QStringLiteral("%1 er aktiv").arg(p.name));
    refresh();
    return;
  }
  if (attemptsLeft <= 1) {
    setBusy(false);
    setStatus(QStringLiteral("Tjenesten startet, men KWin fant ikke %1")
                  .arg(p.output()));
    refresh();
    return;
  }
  QTimer::singleShot(250, this, [this, p, attemptsLeft] {
    configureWhenReady(p, attemptsLeft - 1);
  });
}

void MonitorManager::stopProfile(int row) {
  if (row < 0 || row >= m_profiles.size())
    return;
  QString out;
  const auto p = m_profiles.at(row);
  if (run(QStringLiteral("systemctl"),
          {QStringLiteral("--user"), QStringLiteral("stop"), p.serviceName},
          &out))
    setStatus(QStringLiteral("%1 ble stoppet").arg(p.name));
  else
    setStatus(QStringLiteral("Kunne ikke stoppe: ") + out.trimmed());
  refresh();
}

void MonitorManager::refresh() {
  if (m_profiles.isEmpty())
    return;
  for (int i = 0; i < m_profiles.size(); ++i) {
    QString out;
    run(QStringLiteral("systemctl"),
        {QStringLiteral("--user"), QStringLiteral("is-active"),
         m_profiles[i].serviceName},
        &out);
    const bool active = out.trimmed() == QStringLiteral("active");
    if (active != m_profiles[i].active) {
      m_profiles[i].active = active;
      emit dataChanged(index(i), index(i), {ActiveRole});
    }
  }
}

bool MonitorManager::run(const QString &program, const QStringList &arguments,
                         QString *output) const {
  QProcess process;
  process.start(program, arguments);
  if (!process.waitForStarted(3000) || !process.waitForFinished(10000)) {
    if (output)
      *output = process.errorString();
    return false;
  }
  if (output)
    *output = QString::fromUtf8(process.readAllStandardOutput() +
                                process.readAllStandardError());
  return process.exitStatus() == QProcess::NormalExit &&
         process.exitCode() == 0;
}

void MonitorManager::updateSunshine(const Profile &p) {
  QFile input(sunshineConfig());
  if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
    return;
  QString text = QString::fromUtf8(input.readAll());
  input.close();
  QRegularExpression re(QStringLiteral("(?m)^\\s*output_name\\s*=.*$"));
  const QString line = QStringLiteral("output_name = ") + p.output();
  if (re.match(text).hasMatch())
    text.replace(re, line);
  else
    text += QStringLiteral("\n") + line + QStringLiteral("\n");
  QSaveFile output(sunshineConfig());
  if (output.open(QIODevice::WriteOnly | QIODevice::Text)) {
    output.write(text.toUtf8());
    output.commit();
  }
}

void MonitorManager::load() {
  QFile f(profilesPath());
  if (!f.open(QIODevice::ReadOnly))
    return;
  const auto array = QJsonDocument::fromJson(f.readAll()).array();
  for (const auto &value : array) {
    const auto o = value.toObject();
    Profile p;
    p.id = o["id"].toString();
    p.name = o["name"].toString();
    p.width = o["width"].toInt(1920);
    p.height = o["height"].toInt(1080);
    p.refresh = o["refresh"].toInt(60);
    p.scale = o["scale"].toDouble(1);
    p.port = o["port"].toInt(5901);
    p.password = o["password"].toString();
    p.serviceName = o["service"].toString(QStringLiteral("virtmonitors-") +
                                          p.id + QStringLiteral(".service"));
    p.sunshine = o["sunshine"].toBool(true);
    if (!p.id.isEmpty() && !p.name.isEmpty())
      m_profiles.append(p);
  }
}

void MonitorManager::persist() {
  QDir().mkpath(configDir());
  QJsonArray a;
  for (const auto &p : m_profiles) {
    QJsonObject o{{"id", p.id},
                  {"name", p.name},
                  {"width", p.width},
                  {"height", p.height},
                  {"refresh", p.refresh},
                  {"scale", p.scale},
                  {"port", p.port},
                  {"password", p.password},
                  {"service", p.serviceName},
                  {"sunshine", p.sunshine}};
    a.append(o);
  }
  QSaveFile f(profilesPath());
  if (f.open(QIODevice::WriteOnly)) {
    f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    f.write(QJsonDocument(a).toJson());
    f.commit();
  }
}

void MonitorManager::importExisting() {
  QDir dir(userSystemdDir());
  const auto files =
      dir.entryList({QStringLiteral("*virtual*monitor*.service")}, QDir::Files);
  QRegularExpression args(
      QStringLiteral("--name\\s+(\\S+)\\s+--resolution\\s+(\\d+)x(\\d+)\\s+--"
                     "password\\s+(\\S+)\\s+--port\\s+(\\d+)"));
  for (const auto &file : files) {
    QFile f(dir.filePath(file));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
      continue;
    const auto match = args.match(QString::fromUtf8(f.readAll()));
    if (!match.hasMatch())
      continue;
    Profile p;
    p.name = match.captured(1);
    p.id = safeId(p.name);
    p.width = match.captured(2).toInt();
    p.height = match.captured(3).toInt();
    p.password = match.captured(4);
    p.port = match.captured(5).toInt();
    p.serviceName = file;
    p.imported = true;
    m_profiles.append(p);
  }
  if (!m_profiles.isEmpty())
    persist();
}

void MonitorManager::setStatus(const QString &message) {
  if (m_statusMessage == message)
    return;
  m_statusMessage = message;
  emit statusMessageChanged();
}
void MonitorManager::setBusy(bool value) {
  if (m_busy == value)
    return;
  m_busy = value;
  emit busyChanged();
}
