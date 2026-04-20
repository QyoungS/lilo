#pragma once
#include <QObject>
#include <QString>

class AuthManager : public QObject {
    Q_OBJECT
public:
    static AuthManager& instance();

    bool login(const QString& username, const QString& password);
    bool registerUser(const QString& username, const QString& password);
    void logout();

    bool isLoggedIn() const { return m_userId > 0; }
    int  currentUserId() const { return m_userId; }
    QString currentUsername() const { return m_username; }

signals:
    void loggedIn(int userId, const QString& username);
    void loggedOut();

private:
    explicit AuthManager(QObject* parent = nullptr);
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    static QString hashPassword(const QString& password);

    int     m_userId   = 0;
    QString m_username;
};
