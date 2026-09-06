#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "monitormanager.h"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  QQmlApplicationEngine engine;
  MonitorManager monitorManager;
  engine.rootContext()->setContextProperty("monitorManager", &monitorManager);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  engine.loadFromModule("VirtMonitors", "Main");

  return app.exec();
}
