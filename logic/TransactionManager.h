#pragma once
#include <QObject>
#include <QString>

class TransactionManager : public QObject {
    Q_OBJECT
public:
    static TransactionManager& instance();

    bool deposit(int accountId, double amount, const QString& category,
                 const QString& description);
    bool withdraw(int accountId, double amount, const QString& category,
                  const QString& description);
    bool updateTransaction(int txId, double newAmount, const QString& category,
                           const QString& description);
    bool deleteTransaction(int txId);

signals:
    void transactionsChanged(int accountId);

private:
    explicit TransactionManager(QObject* parent = nullptr);
    TransactionManager(const TransactionManager&) = delete;
    TransactionManager& operator=(const TransactionManager&) = delete;

    bool recalculateBalance(int accountId);
};
