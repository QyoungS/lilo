#include "TransactionWidget.h"
#include "SearchEngine.h"
#include "TransactionManager.h"
#include "AccountManager.h"
#include "NotificationManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QTextStream>
#include <QDate>
#include <QDialog>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLineEdit>

TransactionWidget::TransactionWidget(int userId, QWidget* parent)
    : QWidget(parent), m_userId(userId)
{
    setupUi();
    refresh();

    connect(&TransactionManager::instance(), &TransactionManager::transactionsChanged,
            this, [this](int) { onSearch(); });
    connect(&AccountManager::instance(), &AccountManager::accountsChanged,
            this, [this]() {
                int cur = m_accountCombo->currentIndex();
                m_accountModel->loadAccounts(m_userId);
                m_accountCombo->clear();
                m_accountCombo->addItem("전체 계좌", -1);
                for (const Account& a : m_accountModel->accounts())
                    m_accountCombo->addItem(a.name, a.id);
                m_accountCombo->setCurrentIndex(qMin(cur, m_accountCombo->count()-1));
            });
}

void TransactionWidget::setupUi() {
    auto* root = new QVBoxLayout(this);

    auto* filterBox = new QGroupBox("검색 / 필터", this);
    auto* grid = new QGridLayout(filterBox);

    m_accountCombo   = new QComboBox(this);
    m_keywordEdit    = new QLineEdit(this);
    m_keywordEdit->setPlaceholderText("키워드...");
    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->addItems({"전체", "급여", "식비", "교통", "쇼핑", "의료", "여가", "이체", "기타"});

    m_startDate = new QDateEdit(QDate::currentDate().addMonths(-1), this);
    m_endDate   = new QDateEdit(QDate::currentDate(), this);
    m_startDate->setCalendarPopup(true);
    m_endDate->setCalendarPopup(true);
    m_startDate->setDisplayFormat("yyyy-MM-dd");
    m_endDate->setDisplayFormat("yyyy-MM-dd");

    m_minAmt = new QDoubleSpinBox(this);
    m_maxAmt = new QDoubleSpinBox(this);
    m_minAmt->setRange(0, 1e12); m_minAmt->setSpecialValueText("전체");
    m_maxAmt->setRange(0, 1e12); m_maxAmt->setSpecialValueText("전체");
    m_minAmt->setValue(0); m_maxAmt->setValue(0);

    auto* searchBtn = new QPushButton("검색", this);
    auto* resetBtn  = new QPushButton("초기화", this);

    grid->addWidget(new QLabel("계좌:"),      0, 0); grid->addWidget(m_accountCombo,   0, 1);
    grid->addWidget(new QLabel("키워드:"),    0, 2); grid->addWidget(m_keywordEdit,     0, 3);
    grid->addWidget(new QLabel("카테고리:"),  0, 4); grid->addWidget(m_categoryFilter,  0, 5);
    grid->addWidget(new QLabel("시작일:"),    1, 0); grid->addWidget(m_startDate,       1, 1);
    grid->addWidget(new QLabel("종료일:"),    1, 2); grid->addWidget(m_endDate,         1, 3);
    grid->addWidget(new QLabel("최소 금액:"), 1, 4); grid->addWidget(m_minAmt,          1, 5);
    grid->addWidget(new QLabel("최대 금액:"), 1, 6); grid->addWidget(m_maxAmt,          1, 7);
    grid->addWidget(searchBtn,                1, 8); grid->addWidget(resetBtn,          1, 9);
    root->addWidget(filterBox);

    m_accountModel = new AccountModel(this);
    m_accountModel->loadAccounts(m_userId);
    m_accountCombo->addItem("전체 계좌", -1);
    for (const Account& a : m_accountModel->accounts())
        m_accountCombo->addItem(a.name, a.id);

    m_model = new TransactionModel(this);
    m_view  = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->verticalHeader()->hide();
    root->addWidget(m_view);

    auto* btns      = new QHBoxLayout;
    auto* editBtn   = new QPushButton("수정",        this);
    auto* deleteBtn = new QPushButton("삭제",        this);
    auto* exportBtn = new QPushButton("CSV 내보내기", this);
    btns->addWidget(editBtn);
    btns->addWidget(deleteBtn);
    btns->addStretch();
    btns->addWidget(exportBtn);
    root->addLayout(btns);

    connect(searchBtn, &QPushButton::clicked, this, &TransactionWidget::onSearch);
    connect(resetBtn,  &QPushButton::clicked, this, [this]() {
        m_keywordEdit->clear();
        m_categoryFilter->setCurrentIndex(0);
        m_startDate->setDate(QDate::currentDate().addMonths(-1));
        m_endDate->setDate(QDate::currentDate());
        m_minAmt->setValue(0); m_maxAmt->setValue(0);
        m_accountCombo->setCurrentIndex(0);
        onSearch();
    });
    connect(editBtn,   &QPushButton::clicked, this, &TransactionWidget::onEdit);
    connect(deleteBtn, &QPushButton::clicked, this, &TransactionWidget::onDelete);
    connect(exportBtn, &QPushButton::clicked, this, &TransactionWidget::onExportCsv);
    connect(m_accountCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TransactionWidget::onAccountChanged);
}

void TransactionWidget::refresh() {
    onSearch();
}

void TransactionWidget::onSearch() {
    SearchCriteria c;
    c.userId    = m_userId;
    int aid = m_accountCombo->currentData().toInt();
    if (aid > 0) c.accountId = aid;
    c.keyword   = m_keywordEdit->text().trimmed();
    c.startDate = m_startDate->date().toString("yyyy-MM-dd");
    c.endDate   = m_endDate->date().toString("yyyy-MM-dd");
    if (m_minAmt->value() > 0) c.minAmount = m_minAmt->value();
    if (m_maxAmt->value() > 0) c.maxAmount = m_maxAmt->value();
    // "전체" means no category filter
    c.category  = (m_categoryFilter->currentText() == "전체") ? "All" : m_categoryFilter->currentText();

    m_model->setTransactions(SearchEngine::instance().search(c));
    auto* hdr = m_view->horizontalHeader();
    hdr->setSectionResizeMode(QHeaderView::Fixed);
    hdr->setSectionResizeMode(6, QHeaderView::Stretch);
    m_view->setColumnWidth(0, 50);
    m_view->setColumnWidth(1, 165);
    m_view->setColumnWidth(2, 120);
    m_view->setColumnWidth(3, 65);
    m_view->setColumnWidth(4, 90);
    m_view->setColumnWidth(5, 130);
}

void TransactionWidget::onAccountChanged(int) {
    onSearch();
}

int TransactionWidget::selectedRow() {
    auto idx = m_view->selectionModel()->currentIndex();
    return idx.isValid() ? idx.row() : -1;
}

int TransactionWidget::selectedTransactionId() {
    int row = selectedRow();
    return row >= 0 ? m_model->transactionAt(row).id : -1;
}

void TransactionWidget::onEdit() {
    int row = selectedRow();
    if (row < 0) { QMessageBox::information(this, "선택", "거래를 선택하세요."); return; }

    Transaction t = m_model->transactionAt(row);

    auto* dlg  = new QDialog(this);
    dlg->setWindowTitle("거래 수정");
    auto* form = new QFormLayout(dlg);

    auto* amtSpin = new QDoubleSpinBox(dlg);
    amtSpin->setRange(0.01, 1e12);
    amtSpin->setDecimals(2);
    amtSpin->setValue(t.amount);

    auto* catBox = new QComboBox(dlg);
    catBox->addItems({"급여", "식비", "교통", "쇼핑", "의료", "여가", "이체", "기타"});
    catBox->setCurrentText(t.category);

    auto* descEdit = new QLineEdit(t.description, dlg);
    form->addRow("금액:", amtSpin);
    form->addRow("카테고리:", catBox);
    form->addRow("설명:", descEdit);

    auto* btns = new QHBoxLayout;
    auto* ok  = new QPushButton("저장", dlg);
    auto* can = new QPushButton("취소", dlg);
    btns->addWidget(ok); btns->addWidget(can);
    form->addRow(btns);

    connect(can, &QPushButton::clicked, dlg, &QDialog::reject);
    connect(ok,  &QPushButton::clicked, dlg, [=]() {
        if (!TransactionManager::instance().updateTransaction(t.id, amtSpin->value(),
                catBox->currentText(), descEdit->text())) {
            QMessageBox::warning(dlg, "오류", "수정에 실패했습니다.");
            return;
        }
        NotificationManager::instance().checkBudgets(m_userId);
        dlg->accept();
    });
    dlg->exec();
    dlg->deleteLater();
}

void TransactionWidget::onDelete() {
    int id = selectedTransactionId();
    if (id < 0) { QMessageBox::information(this, "선택", "거래를 선택하세요."); return; }

    if (QMessageBox::question(this, "삭제", "이 거래를 삭제하시겠습니까? 잔액이 재계산됩니다.")
            == QMessageBox::Yes) {
        TransactionManager::instance().deleteTransaction(id);
    }
}

void TransactionWidget::onExportCsv() {
    QString path = QFileDialog::getSaveFileName(this, "CSV 내보내기", "거래내역.csv",
                                                "CSV 파일 (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "오류", "파일을 열 수 없습니다.");
        return;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF"; // UTF-8 BOM
    out << "ID,날짜,계좌,유형,카테고리,금액,설명\n";

    for (const Transaction& t : m_model->transactions()) {
        QString name = t.accountName;
        QString desc = t.description;

        out << t.id << ","
            << t.createdAt << ","
            << "\"" << name.replace("\"", "\"\"") << "\","
            << t.type << ","
            << t.category << ","
            << QString::number(t.amount, 'f', 2) << ","
            << "\"" << desc.replace("\"", "\"\"") << "\"\n";
    }

    QMessageBox::information(this, "내보내기", "내보내기가 완료되었습니다.");
}
