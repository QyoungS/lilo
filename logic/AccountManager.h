#pragma once
#include <QObject>
#include <QString>
#include "AccountModel.h"

class AccountManager : public QObject {
    Q_OBJECT
public:
    static AccountManager& instance();

    bool createAccount(int userId, const QString& name, const QString& number,
                       const QString& type, const QString& currency);
    bool updateAccount(int id, const QString& name, const QString& type, const QString& currency);
    bool deleteAccount(int id);
    Account getAccount(int id);

signals:
    void accountsChanged();

private:
    explicit AccountManager(QObject* parent = nullptr);
    AccountManager(const AccountManager&) = delete;
    AccountManager& operator=(const AccountManager&) = delete;
};
