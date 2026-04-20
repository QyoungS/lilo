#include "AccountModel.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

const QStringList AccountModel::s_headers = {
    "ID", "계좌명", "계좌번호", "잔액", "유형", "통화", "생성일"
};

AccountModel::AccountModel(QObject* parent) : QAbstractTableModel(parent) {}

int AccountModel::rowCount(const QModelIndex&) const { return m_accounts.size(); }
int AccountModel::columnCount(const QModelIndex&) const { return s_headers.size(); }

QVariant AccountModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_accounts.size())
        return {};
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return {};

    const Account& a = m_accounts[index.row()];
    switch (index.column()) {
    case 0: return a.id;
    case 1: return a.name;
    case 2: return a.number;
    case 3: return QString("%1 %2").arg(a.currency).arg(a.balance, 0, 'f', 2);
    case 4: return a.type;
    case 5: return a.currency;
    case 6: return a.createdAt;
    }
    return {};
}

QVariant AccountModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};
    return s_headers.value(section);
}

void AccountModel::loadAccounts(int userId) {
    beginResetModel();
    m_accounts.clear();

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare("SELECT id, user_id, name, number, balance, type, currency, created_at "
              "FROM accounts WHERE user_id = :uid ORDER BY created_at DESC");
    q.bindValue(":uid", userId);

    if (!q.exec()) {
        qWarning() << "loadAccounts failed:" << q.lastError().text();
    } else {
        while (q.next()) {
            Account a;
            a.id        = q.value(0).toInt();
            a.userId    = q.value(1).toInt();
            a.name      = q.value(2).toString();
            a.number    = q.value(3).toString();
            a.balance   = q.value(4).toDouble();
            a.type      = q.value(5).toString();
            a.currency  = q.value(6).toString();
            a.createdAt = q.value(7).toString();
            m_accounts.append(a);
        }
    }
    endResetModel();
}

Account AccountModel::accountAt(int row) const {
    if (row < 0 || row >= m_accounts.size()) return {};
    return m_accounts[row];
}

double AccountModel::totalBalance() const {
    double total = 0.0;
    for (const Account& a : m_accounts) total += a.balance;
    return total;
}
