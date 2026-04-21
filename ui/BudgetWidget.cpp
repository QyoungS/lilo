#include "BudgetWidget.h"
#include "DatabaseManager.h"
#include "NotificationManager.h"
#include "AccountModel.h"
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
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(12);

    // ── 요약 카드 3개 ─────────────────────────────────────────
    auto* cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(12);

    auto makeCard = [&](const QString& title, QLabel*& valLbl, const QString& color) -> QGroupBox* {
        auto* box = new QGroupBox(this);
        box->setFixedHeight(80);
        box->setStyleSheet(QString(
            "QGroupBox { background:#FFFFFF; border:1px solid #E5E7EB; "
            "border-top:3px solid %1; border-radius:8px; margin-top:0; }").arg(color));
        auto* vl = new QVBoxLayout(box);
        vl->setContentsMargins(14, 10, 14, 10);
        vl->setSpacing(4);
        auto* titleLbl = new QLabel(title, box);
        titleLbl->setStyleSheet("color:#6B7280; font-size:8.5pt; font-weight:600; background:transparent;");
        valLbl = new QLabel("₩0", box);
        valLbl->setStyleSheet(QString(
            "color:%1; font-size:13pt; font-weight:700; background:transparent;").arg(color));
        vl->addWidget(titleLbl);
        vl->addWidget(valLbl);
        return box;
    };

    cardsRow->addWidget(makeCard("총 예산",   m_totalBudgetLabel, "#2563EB"));
    cardsRow->addWidget(makeCard("총 지출",   m_totalSpentLabel,  "#DC2626"));
    cardsRow->addWidget(makeCard("남은 예산", m_remainingLabel,   "#059669"));
    root->addLayout(cardsRow);

    // ── 테이블 ────────────────────────────────────────────────
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"ID", "카테고리", "월 한도", "지출액", "잔여", "상태", "진행률"});
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

    double totalLimit = 0, totalSpent = 0;
    int row = 0;
    while (q.next()) {
        int     id        = q.value(0).toInt();
        QString cat       = q.value(1).toString();
        double  limit     = q.value(2).toDouble();
        double  spent     = q.value(3).toDouble();
        double  remaining = limit - spent;
        int     pct       = limit > 0 ? (int)(spent * 100.0 / limit) : 0;

        totalLimit += limit;
        totalSpent += spent;

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(id)));
        m_table->setItem(row, 1, new QTableWidgetItem(cat));

        auto* limitItem = new QTableWidgetItem(AccountModel::formatKRW(limit));
        limitItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 2, limitItem);

        auto* spentItem = new QTableWidgetItem(AccountModel::formatKRW(spent));
        spentItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 3, spentItem);

        // 잔여 컬럼 (col 4)
        auto* remItem = new QTableWidgetItem(AccountModel::formatKRW(qAbs(remaining)));
        remItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        remItem->setForeground(remaining >= 0 ? QColor("#059669") : QColor("#DC2626"));
        m_table->setItem(row, 4, remItem);

        // 상태 배지 (col 5)
        QString badgeText, badgeBg, badgeFg;
        if (pct >= 100) {
            badgeText = "초과"; badgeBg = "#991B1B"; badgeFg = "#FFFFFF";
        } else if (pct >= 80) {
            badgeText = "위험"; badgeBg = "#FEE2E2"; badgeFg = "#991B1B";
        } else if (pct >= 60) {
            badgeText = "주의"; badgeBg = "#FEF9C3"; badgeFg = "#854D0E";
        } else {
            badgeText = "안전"; badgeBg = "#DCFCE7"; badgeFg = "#166534";
        }
        auto* badge = new QLabel(badgeText, m_table);
        badge->setAlignment(Qt::AlignCenter);
        badge->setStyleSheet(QString(
            "background:%1; color:%2; border-radius:4px; padding:2px 8px; "
            "font-size:8pt; font-weight:700;").arg(badgeBg, badgeFg));
        m_table->setCellWidget(row, 5, badge);

        // 진행률 바 (col 6)
        QString barColor;
        if (pct >= 100)     barColor = "#991B1B";
        else if (pct >= 80) barColor = "#EF4444";
        else if (pct >= 60) barColor = "#F59E0B";
        else                barColor = "#3B82F6";

        auto* bar = new QProgressBar(m_table);
        bar->setRange(0, 100);
        bar->setValue(qMin(pct, 100));
        bar->setTextVisible(true);
        bar->setFormat(QString::number(pct) + "%");
        bar->setFixedHeight(18);
        bar->setStyleSheet(QString(
            "QProgressBar { background:#F3F4F6; border:none; border-radius:9px; "
            "text-align:center; font-size:8pt; font-weight:700; color:#374151; }"
            "QProgressBar::chunk { background:%1; border-radius:9px; }").arg(barColor));
        m_table->setCellWidget(row, 6, bar);
        ++row;
    }

    // 컬럼 너비 설정
    auto* hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(QHeaderView::Fixed);
    hdr->setSectionResizeMode(6, QHeaderView::Stretch);
    m_table->setColumnWidth(1, 90);
    m_table->setColumnWidth(2, 120);
    m_table->setColumnWidth(3, 120);
    m_table->setColumnWidth(4, 120);
    m_table->setColumnWidth(5, 70);

    // 요약 카드 업데이트
    double totalRemaining = totalLimit - totalSpent;
    if (m_totalBudgetLabel)
        m_totalBudgetLabel->setText(AccountModel::formatKRW(totalLimit));
    if (m_totalSpentLabel)
        m_totalSpentLabel->setText(AccountModel::formatKRW(totalSpent));
    if (m_remainingLabel) {
        m_remainingLabel->setText(AccountModel::formatKRW(qAbs(totalRemaining)));
        m_remainingLabel->setStyleSheet(QString(
            "color:%1; font-size:13pt; font-weight:700; background:transparent;")
            .arg(totalRemaining >= 0 ? "#059669" : "#DC2626"));
    }
}

void BudgetWidget::refresh() {
    loadBudgets();
    NotificationManager::instance().checkBudgets(m_userId);
}

void BudgetWidget::onAdd() {
    auto* dlg  = new QDialog(this);
    dlg->setWindowTitle("예산 추가");
    dlg->setMinimumWidth(400);
    auto* form = new QFormLayout(dlg);
    form->setSpacing(14);
    form->setContentsMargins(24, 24, 24, 24);

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
    btns->setSpacing(8);
    auto* ok   = new QPushButton("추가", dlg);
    auto* can  = new QPushButton("취소", dlg);
    ok->setFixedHeight(36);
    can->setFixedHeight(36);
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
    dlg->setMinimumWidth(400);
    auto* form = new QFormLayout(dlg);
    form->setSpacing(14);
    form->setContentsMargins(24, 24, 24, 24);

    auto* limitSpin = new QDoubleSpinBox(dlg);
    limitSpin->setRange(1, 1e12);
    limitSpin->setDecimals(0);
    limitSpin->setSingleStep(10000);
    limitSpin->setValue(limit);
    form->addRow("월 한도:", limitSpin);

    auto* btns = new QHBoxLayout;
    btns->setSpacing(8);
    auto* ok   = new QPushButton("저장", dlg);
    auto* can  = new QPushButton("취소", dlg);
    ok->setFixedHeight(36);
    can->setFixedHeight(36);
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
