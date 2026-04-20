#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QIcon>

class NotificationManager : public QObject {
    Q_OBJECT
public:
    static NotificationManager& instance();

    void initialize(QObject* parent = nullptr);
    void checkBudgets(int userId);

private:
    explicit NotificationManager(QObject* parent = nullptr);
    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    void notify(const QString& title, const QString& message);

    QSystemTrayIcon* m_tray = nullptr;
};
