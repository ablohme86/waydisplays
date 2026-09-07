#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QObject>
#include <QVariantMap>

class MonitorManager final : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(
      QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(bool backendAvailable READ backendAvailable CONSTANT)
  Q_PROPERTY(QString backendDescription READ backendDescription CONSTANT)

public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    NameRole,
    WidthRole,
    HeightRole,
    RefreshRole,
    ScaleRole,
    PortRole,
    ActiveRole,
    OutputRole,
    SunshineRole,
    ImportedRole
  };

  explicit MonitorManager(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  QString statusMessage() const { return m_statusMessage; }
  bool busy() const { return m_busy; }
  bool backendAvailable() const;
  QString backendDescription() const;

  Q_INVOKABLE QVariantMap profile(int row) const;
  Q_INVOKABLE void saveProfile(const QVariantMap &values);
  Q_INVOKABLE void removeProfile(int row);
  Q_INVOKABLE void startProfile(int row);
  Q_INVOKABLE void stopProfile(int row);
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void restartSunshine();
  Q_INVOKABLE QString generatePassword() const;

signals:
  void statusMessageChanged();
  void busyChanged();
  void operationFinished(bool success);

private:
  struct Profile {
    QString id, name, password, serviceName;
    int width = 1920, height = 1080, refresh = 60, port = 5901;
    double scale = 1.0;
    bool active = false, sunshine = true, imported = false;
    QString output() const { return QStringLiteral("Virtual-") + name; }
  };

  void load();
  void persist();
  void importExisting();
  bool writeService(const Profile &profile, QString *error);
  bool run(const QString &program, const QStringList &arguments,
           QString *output = nullptr) const;
  void setStatus(const QString &message);
  void setBusy(bool value);
  bool updateSunshine(const Profile &profile);
  void configureWhenReady(const Profile &profile, int attemptsLeft);
  static QString safeId(const QString &name);

  QList<Profile> m_profiles;
  QString m_statusMessage;
  bool m_busy = false;
};
