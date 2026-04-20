#pragma once
#include <QAbstractTableModel>
#include <QList>
#include <QString>

struct Account {
    int id = 0;
    int userId = 0;
    QString name;
    QString number;
    double balance = 0.0;
    QString type;
    QString currency;
    QString createdAt;
};

class AccountModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit AccountModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void loadAccounts(int userId);
    Account accountAt(int row) const;
    double totalBalance() const;
    QList<Account> accounts() const { return m_accounts; }

private:
    QList<Account> m_accounts;
    static const QStringList s_headers;
};
