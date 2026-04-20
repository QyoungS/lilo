#pragma once
#include <QAbstractTableModel>
#include <QList>
#include <QString>
#include <QSqlQuery>
#include <QSqlError>

struct Transaction {
    int id = 0;
    int accountId = 0;
    QString accountName;
    QString type;
    double amount = 0.0;
    QString category;
    QString description;
    QString createdAt;
};

class TransactionModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit TransactionModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void loadByAccount(int accountId);
    void loadByUser(int userId);
    void setTransactions(const QList<Transaction>& list);

    Transaction transactionAt(int row) const;
    QList<Transaction> transactions() const { return m_transactions; }

private:
    QList<Transaction> m_transactions;
    static const QStringList s_headers;

    void execAndLoad(QSqlQuery& q);
};
