#include "BudgetWidget.h"
#include "DatabaseManager.h"
#include "NotificationManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QHeaderView>
#include <QLocale>
#include <QDebug>

BudgetWidget::BudgetWidget(int userId, QWidget* parent)
    : QWidget(parent), m_userId(userId)
{
    setupUi();
    loadBudgets();
}

void BudgetWidget::setupUi() {
    auto* root = new QVBoxLayout(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"ID", "카테고리", "월 한도", "지출액", "진행률"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->hide();
    m_table->setColumnHidden(0, true);
    root->addWidget(m_table);

    auto* btns   = new QHBoxLayout;
    auto* addBtn = new QPushButton("예산 추가", this);
    m_editBtn    = new QPushButton("수정",      this);
    m_deleteBtn  = new QPushButton("삭제",      this);
    btns->addWidget(addBtn);
    btns->addWidget(m_editBtn);
    btns->addWidget(m_deleteBtn);
    btns->addStretch();
    root->addLayout(btns);

    connect(addBtn,      &QPushButton::clicked, this, &BudgetWidget::onAdd);
    connect(m_editBtn,   &QPushButton::clicked, this, &BudgetWidget::onEdit);
    connect(m_deleteBtn, &QPushButton::clicked, this, &BudgetWidget::onDelete);
}

void BudgetWidget::loadBudgets() {
    m_table->setRowCount(0);

    QSqlQuery q(DatabaseManager::instance().database());
    q.prepare(R"(
        SELECT b.id, b.category, b.monthly_limit,
               COALESCE(SUM(t.amount), 0) as spent
        FROM budgets b
        LEFT JOIN accounts a ON a.user_id = b.user_id
        LEFT JOIN transactions t ON t.account_id = a.id
            AND t.type = '출금'
            AND t.category = b.category
            AND strftime('%Y-%m', t.created_at) = strftime('%Y-%m', 'now', 'localtime')
        WHERE b.user_id = :uid
        GROUP BY b.id, b.category, b.monthly_limit
        ORDER BY b.category
    )");
    q.bindValue(":uid", m_userId);

    if (!q.exec()) {
        qWarning() << "loadBudgets failed:" << q.lastError().text();
        return;
    }

    int row = 0;
    while (q.next()) {
        int     id    = q.value(0).toInt();
        QString cat   = q.value(1).toString();
        double  limit = q.value(2).toDouble();
        double  spent = q.value(3).toDouble();

        QLocale loc(QLocale::Korean, QLocale::SouthKorea);
        auto fmtNum = [&loc](double v) { return loc.toString((qlonglong)v); };

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
        m_table->setItem(row, 1, new QTableWidgetItem(cat));

        auto* limitItem = new QTableWidgetItem(fmtNum(limit));
        limitItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 2, limitItem);

        auto* spentItem = new QTableWidgetItem(fmtNum(spent));
        spentItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 3, spentItem);

        auto* bar = new QProgressBar(this);
        bar->setRange(0, 100);
        int pct = limit > 0 ? int(spent / limit * 100) : 0;
        bar->setValue(qMin(pct, 100));

        QString barColor;
        QString barText;
        if (pct >= 100) {
            barColor = "#B71C1C";
            barText  = QString("초과 (%1%)").arg(pct);
        } else if (pct >= 80) {
            barColor = "#F44336";
            barText  = QString("%1%").arg(pct);
        } else if (pct >= 60) {
            barColor = "#FF9800";
            barText  = QString("%1%").arg(pct);
        } else {
            barColor = "#4CAF50";
            barText  = QString("%1%").arg(pct);
        }
        bar->setFormat(barText);
        bar->setTextVisible(true);
        bar->setStyleSheet(QString(
            "QProgressBar { background:#F3F4F6; border:none; border-radius:99px; "
            "height:8px; text-align:right; padding-right:4px; "
            "font-size:7.5pt; color:#6B7280; }"
            "QProgressBar::chunk { background:%1; border-radius:99px; }").arg(barColor));
        m_table->setCellWidget(row, 4, bar);
        ++row;
    }
    auto* hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(QHeaderView::Fixed);
    hdr->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->setColumnWidth(1, 100);
    m_table->setColumnWidth(2, 130);
    m_table->setColumnWidth(3, 130);
}

void BudgetWidget::refresh() {
    loadBudgets();
    NotificationManager::instance().checkBudgets(m_userId);
}

void BudgetWidget::onAdd() {
    auto* dlg  = new QDialog(this);
    dlg->setWindowTitle("예산 추가");
    auto* form = new QFormLayout(dlg);

    auto* catBox = new QComboBox(dlg);
    catBox->addItems({"급여", "식비", "교통", "쇼핑", "의료", "여가", "이체", "기타"});
    auto* limitSpin = new QDoubleSpinBox(dlg);
    limitSpin->setRange(1, 1e12);
    limitSpin->setDecimals(0);
    limitSpin->setSingleStep(10000);
    limitSpin->setValue(100000);

    form->addRow("카테고리:", catBox);
    form->addRow("월 한도:", limitSpin);

    auto* btns = new QHBoxLayout;
    auto* ok   = new QPushButton("추가", dlg);
    auto* can  = new QPushButton("취소", dlg);
    btns->addWidget(ok); btns->addWidget(can);
    form->addRow(btns);

    connect(can, &QPushButton::clicked, dlg, &QDialog::reject);
    connect(ok,  &QPushButton::clicked, dlg, [=]() {
        QSqlQuery q(DatabaseManager::instance().database());
        q.prepare("INSERT OR REPLACE INTO budgets (user_id, category, monthly_limit) "
                  "VALUES (:uid, :cat, :lim)");
        q.bindValue(":uid", m_userId);
        q.bindValue(":cat", catBox->currentText());
        q.bindValue(":lim", limitSpin->value());
        if (!q.exec()) {
            QMessageBox::warning(dlg, "오류", "예산 저장에 실패했습니다: " + q.lastError().text());
            return;
        }
        dlg->accept();
        loadBudgets();
    });
    dlg->exec();
    dlg->deleteLater();
}

void BudgetWidget::onEdit() {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::information(this, "선택", "수정할 예산을 선택하세요."); return; }

    int     id    = m_table->item(row, 0)->text().toInt();
    QString cat   = m_table->item(row, 1)->text();
    double  limit = m_table->item(row, 2)->text().toDouble();

    auto* dlg  = new QDialog(this);
    dlg->setWindowTitle("예산 수정 — " + cat);
    auto* form = new QFormLayout(dlg);

    auto* limitSpin = new QDoubleSpinBox(dlg);
    limitSpin->setRange(1, 1e12);
    limitSpin->setDecimals(0);
    limitSpin->setSingleStep(10000);
    limitSpin->setValue(limit);
    form->addRow("월 한도:", limitSpin);

    auto* btns = new QHBoxLayout;
    auto* ok   = new QPushButton("저장", dlg);
    auto* can  = new QPushButton("취소", dlg);
    btns->addWidget(ok); btns->addWidget(can);
    form->addRow(btns);

    connect(can, &QPushButton::clicked, dlg, &QDialog::reject);
    connect(ok,  &QPushButton::clicked, dlg, [=]() {
        QSqlQuery q(DatabaseManager::instance().database());
        q.prepare("UPDATE budgets SET monthly_limit = :lim WHERE id = :id");
        q.bindValue(":lim", limitSpin->value());
        q.bindValue(":id",  id);
        if (!q.exec()) {
            QMessageBox::warning(dlg, "오류", "수정에 실패했습니다.");
            return;
        }
        dlg->accept();
        loadBudgets();
    });
    dlg->exec();
    dlg->deleteLater();
}

void BudgetWidget::onDelete() {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::information(this, "선택", "삭제할 예산을 선택하세요."); return; }
    int id = m_table->item(row, 0)->text().toInt();

    if (QMessageBox::question(this, "삭제", "이 예산을 삭제하시겠습니까?") == QMessageBox::Yes) {
        QSqlQuery q(DatabaseManager::instance().database());
        q.prepare("DELETE FROM budgets WHERE id = :id");
        q.bindValue(":id", id);
        q.exec();
        loadBudgets();
    }
}
