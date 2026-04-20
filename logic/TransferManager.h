#pragma once
#include <QObject>
#include <QString>

class TransferManager : public QObject {
    Q_OBJECT
public:
    static TransferManager& instance();

    bool transfer(int fromAccountId, int toAccountId, double amount, const QString& memo);

signals:
    void transferCompleted(int fromAccountId, int toAccountId);

private:
    explicit TransferManager(QObject* parent = nullptr);
    TransferManager(const TransferManager&) = delete;
    TransferManager& operator=(const TransferManager&) = delete;
};
