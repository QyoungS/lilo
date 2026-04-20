#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFont>
#include <QTextStream>
#include <QButtonGroup>
#include <QRadioButton>
#include <QFrame>
#include <QRegularExpression>
#include <QFileInfo>

// ─── 저장 경로 ────────────────────────────────────────────────
QString SettingsDialog::savePath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/custom_theme.qss";
}

QString SettingsDialog::loadSavedQss() {
    QFile f(savePath());
    if (!f.open(QFile::ReadOnly)) return {};
    return QString::fromUtf8(f.readAll());
}

void SettingsDialog::saveQss(const QString& qss) {
    QDir().mkpath(QFileInfo(savePath()).absolutePath());
    QFile f(savePath());
    if (f.open(QFile::WriteOnly | QFile::Text))
        QTextStream(&f) << qss;
}

// ─── 프리셋 테마 정의 ──────────────────────────────────────────
struct Preset { QString name; QString primary; QString accent; QString menuBg; };
static const Preset kPresets[] = {
    { "네이비 (기본)", "#003585", "#1D4ED8", "#003585" },
    { "에메랄드",      "#065F46", "#059669", "#064E3B" },
    { "바이올렛",      "#4C1D95", "#7C3AED", "#3B0764" },
    { "로즈",          "#9F1239", "#E11D48", "#881337" },
    { "슬레이트",      "#1E293B", "#475569", "#0F172A" },
    { "다크 모드",     "#0D1424", "#1D4ED8", "#0D1424" },
};

// ─── 테마 생성 헬퍼 ────────────────────────────────────────────
static QString buildTheme(const QString& primary, const QString& accent,
                           const QString& menuBg, int fontSize,
                           bool dark = false) {
    QString bg      = dark ? "#0F1623"  : "#F0F4FA";
    QString cardBg  = dark ? "#1A2233"  : "#FFFFFF";
    QString text    = dark ? "#E2E8F0"  : "#1A2233";
    QString border  = dark ? "#334155"  : "#E2E8F0";
    QString inputBg = dark ? "#243044"  : "#FFFFFF";
    QString mutedTxt= dark ? "#64748B"  : "#64748B";
    QString tabSelColor = dark ? "#60A5FA" : accent;
    QString hoverRow = dark ? "#243450" : "#F0F7FF";
    QString selRow   = dark ? "#1E3A8A" : "#EFF6FF";
    QString selText  = dark ? "#FFFFFF" : accent;
    QString headerBg = dark ? "#243044" : "#F8FAFC";
    QString altRow   = dark ? "#1E2A3F" : "#FAFBFD";

    return QString(R"(
* { font-family:"맑은 고딕","Malgun Gothic","Segoe UI",Arial,sans-serif; font-size:%1pt; }

QMainWindow,QWidget { background:%2; color:%3; }
QDialog             { background:%4; color:%3; }

QMenuBar {
    background:%5; color:#FFFFFF; padding:2px 4px; spacing:2px;
    border-bottom:2px solid %6;
}
QMenuBar::item { padding:6px 14px; border-radius:4px; background:transparent; }
QMenuBar::item:selected, QMenuBar::item:pressed { background:%7; }

QMenu { background:%4; border:1px solid %8; border-radius:8px; padding:6px; }
QMenu::item { padding:8px 22px; border-radius:4px; color:%3; }
QMenu::item:selected { background:%7; color:#FFFFFF; }
QMenu::separator { height:1px; background:%8; margin:4px 12px; }

QToolBar { background:%5; border:none; padding:4px 10px; spacing:6px; }
QToolBar::separator { width:1px; background:rgba(255,255,255,0.25); margin:4px; }
QToolBar QToolButton {
    background:transparent; color:#FFFFFF; border:1px solid rgba(255,255,255,0.3);
    border-radius:5px; padding:5px 14px; font-weight:600;
}
QToolBar QToolButton:hover { background:rgba(255,255,255,0.15); }

QStatusBar { background:%4; border-top:1px solid %8; color:%9; font-size:9pt; }

QTabWidget::pane { border:none; background:%2; }
QTabBar { background:%4; border-bottom:1px solid %8; }
QTabBar::tab {
    background:transparent; color:%9; padding:13px 26px;
    font-weight:500; border:none; border-bottom:3px solid transparent;
}
QTabBar::tab:selected { color:%10; border-bottom:3px solid %10; font-weight:700; }
QTabBar::tab:hover:!selected { color:%7; background:rgba(59,130,246,0.06); }

QGroupBox {
    background:%4; border:1px solid %8; border-radius:12px;
    margin-top:14px; padding:14px 18px 18px 18px;
}
QGroupBox::title { left:18px; top:5px; color:%9; font-size:8.5pt; font-weight:700; }

QTableView,QTableWidget {
    background:%4; border:1px solid %8; border-radius:10px;
    gridline-color:%11; selection-background-color:%12;
    selection-color:%13; alternate-background-color:%14; color:%3;
}
QTableView::item,QTableWidget::item { padding:9px 14px; border:none; }
QTableView::item:hover { background:%15; }

QHeaderView::section {
    background:%16; color:%9; font-weight:700; font-size:9pt;
    padding:11px 14px; border:none;
    border-bottom:2px solid %8; border-right:1px solid %8;
}

QPushButton {
    background:%7; color:#FFFFFF; border:none; border-radius:6px;
    padding:8px 20px; font-weight:600; font-size:9.5pt; min-height:34px;
}
QPushButton:hover   { background:%6; }
QPushButton:pressed { background:%5; }
QPushButton:disabled{ background:%8; color:%9; }

QPushButton[text="삭제"]         { background:#EF4444; }
QPushButton[text="삭제"]:hover   { background:#DC2626; }
QPushButton[text="출금"]         { background:#F59E0B; }
QPushButton[text="출금"]:hover   { background:#D97706; }
QPushButton[text="입금"]         { background:#10B981; }
QPushButton[text="입금"]:hover   { background:#059669; }
QPushButton[text="이체"]         { background:#8B5CF6; }
QPushButton[text="이체"]:hover   { background:#7C3AED; }
QPushButton[text="이체..."]      { background:#8B5CF6; }
QPushButton[text="CSV 내보내기"] { background:#0EA5E9; }
QPushButton[text="취소"] { background:transparent; color:%9; border:1.5px solid %8; }
QPushButton[text="취소"]:hover { background:%11; }

QLineEdit,QDoubleSpinBox,QSpinBox,QDateEdit,QComboBox {
    background:%17; border:1.5px solid %8; border-radius:6px;
    padding:6px 12px; color:%3; font-size:9.5pt; min-height:34px;
}
QLineEdit:focus,QDoubleSpinBox:focus,
QDateEdit:focus,QComboBox:focus { border-color:%7; }
QLineEdit:hover,QDoubleSpinBox:hover,
QDateEdit:hover,QComboBox:hover { border-color:%9; }

QComboBox::drop-down { border:none; width:28px; }
QComboBox::down-arrow {
    width:0; height:0;
    border-left:5px solid transparent; border-right:5px solid transparent;
    border-top:6px solid %9; margin-right:8px;
}
QComboBox QAbstractItemView {
    background:%4; border:1px solid %8; border-radius:6px;
    selection-background-color:%7; selection-color:#FFFFFF; color:%3; padding:4px;
}

QProgressBar {
    background:%11; border:none; border-radius:5px;
    height:10px; color:%9; font-size:8pt;
}
QProgressBar::chunk {
    background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #60A5FA,stop:1 %7);
    border-radius:5px;
}

QScrollBar:vertical { background:%2; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:%8; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:%9; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QScrollBar:horizontal { background:%2; height:8px; border-radius:4px; }
QScrollBar::handle:horizontal { background:%8; border-radius:4px; min-width:30px; }
QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal { width:0; }

QLabel { color:%3; background:transparent; }
QFormLayout QLabel { color:%9; font-size:9pt; font-weight:600; }
QCalendarWidget QWidget { background:%4; color:%3; }
QCalendarWidget QAbstractItemView:enabled {
    background:%17; color:%3;
    selection-background-color:%7; selection-color:#FFFFFF;
}
QCalendarWidget QToolButton { background:%7; color:#FFFFFF; border-radius:4px; }
)")
    .arg(fontSize)          // %1
    .arg(bg)                // %2
    .arg(text)              // %3
    .arg(cardBg)            // %4
    .arg(menuBg)            // %5
    .arg(primary)           // %6
    .arg(accent)            // %7
    .arg(border)            // %8
    .arg(mutedTxt)          // %9
    .arg(tabSelColor)       // %10
    .arg(dark ? "#243044" : "#F1F5F9")  // %11 grid/scroll
    .arg(selRow)            // %12
    .arg(selText)           // %13
    .arg(altRow)            // %14
    .arg(hoverRow)          // %15
    .arg(headerBg)          // %16
    .arg(inputBg);          // %17
}

// ════════════════════════════════════════════════════════════
SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("테마 및 스타일 편집기");
    setMinimumSize(900, 620);
    setupUi();
}

void SettingsDialog::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // ── 왼쪽: 옵션 패널 ─────────────────────────────────────
    auto* leftWidget = new QWidget(this);
    leftWidget->setFixedWidth(240);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 8, 0);
    leftLayout->setSpacing(10);

    // 프리셋 테마 버튼
    auto* presetGroup = new QGroupBox("빠른 테마 선택", leftWidget);
    auto* presetLayout = new QVBoxLayout(presetGroup);
    presetLayout->setSpacing(6);
    m_presetGroup = new QButtonGroup(this);

    const QColor swatches[] = {
        QColor("#003585"), QColor("#065F46"), QColor("#4C1D95"),
        QColor("#9F1239"), QColor("#1E293B"), QColor("#0D1424")
    };
    for (int i = 0; i < 6; ++i) {
        auto* btn = new QPushButton(kPresets[i].name, presetGroup);
        btn->setCheckable(true);
        btn->setFixedHeight(34);
        QString hex = swatches[i].name();
        btn->setStyleSheet(QString(
            "QPushButton { background:%1; color:white; border-radius:6px; "
            "font-weight:600; text-align:left; padding-left:12px; }"
            "QPushButton:hover { opacity:0.85; }"
            "QPushButton:checked { border:2px solid white; }")
            .arg(hex));
        m_presetGroup->addButton(btn, i);
        presetLayout->addWidget(btn);
    }
    leftLayout->addWidget(presetGroup);

    // 기본 색상 선택
    auto* colorGroup = new QGroupBox("기본 색상 변경", leftWidget);
    auto* colorLayout = new QVBoxLayout(colorGroup);
    auto* colorRow = new QHBoxLayout;
    m_colorPreview = new QLabel(leftWidget);
    m_colorPreview->setFixedSize(36, 36);
    m_colorPreview->setStyleSheet("background:#003585; border-radius:6px; border:1px solid #CBD5E1;");
    auto* colorBtn = new QPushButton("색상 선택...", leftWidget);
    colorBtn->setFixedHeight(36);
    colorRow->addWidget(m_colorPreview);
    colorRow->addWidget(colorBtn);
    colorLayout->addLayout(colorRow);
    auto* colorHint = new QLabel("선택한 색상을 주요 색상으로 적용합니다.", leftWidget);
    colorHint->setWordWrap(true);
    colorHint->setStyleSheet("color:#64748B; font-size:8pt;");
    colorLayout->addWidget(colorHint);
    leftLayout->addWidget(colorGroup);

    // 폰트 크기
    auto* fontGroup = new QGroupBox("폰트 크기", leftWidget);
    auto* fontLayout = new QVBoxLayout(fontGroup);
    auto* fontRow = new QHBoxLayout;
    auto* smallLbl = new QLabel("8", leftWidget);
    m_fontSlider = new QSlider(Qt::Horizontal, leftWidget);
    m_fontSlider->setRange(8, 14);
    m_fontSlider->setValue(10);
    m_fontSlider->setTickInterval(1);
    m_fontSlider->setTickPosition(QSlider::TicksBelow);
    m_fontSizeLabel = new QLabel("10pt", leftWidget);
    m_fontSizeLabel->setFixedWidth(32);
    m_fontSizeLabel->setAlignment(Qt::AlignCenter);
    m_fontSizeLabel->setStyleSheet("font-weight:700; color:#1D4ED8;");
    fontRow->addWidget(smallLbl);
    fontRow->addWidget(m_fontSlider);
    fontRow->addWidget(new QLabel("14", leftWidget));
    fontLayout->addLayout(fontRow);
    fontLayout->addWidget(m_fontSizeLabel, 0, Qt::AlignCenter);
    leftLayout->addWidget(fontGroup);

    leftLayout->addStretch();

    // Import/Export
    auto* fileGroup = new QGroupBox("파일", leftWidget);
    auto* fileLayout = new QVBoxLayout(fileGroup);
    auto* importBtn = new QPushButton("📂 파일에서 불러오기", leftWidget);
    auto* exportBtn = new QPushButton("💾 파일로 내보내기", leftWidget);
    importBtn->setFixedHeight(32);
    exportBtn->setFixedHeight(32);
    fileLayout->addWidget(importBtn);
    fileLayout->addWidget(exportBtn);
    leftLayout->addWidget(fileGroup);

    // ── 오른쪽: QSS 에디터 ──────────────────────────────────
    auto* rightWidget  = new QWidget(this);
    auto* rightLayout  = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(8, 0, 0, 0);
    rightLayout->setSpacing(6);

    auto* editorLabel = new QLabel("QSS 직접 편집 (변경 후 [즉시 적용] 클릭)", rightWidget);
    editorLabel->setStyleSheet("font-weight:700; color:#1A2233; font-size:9.5pt;");
    rightLayout->addWidget(editorLabel);

    m_editor = new QPlainTextEdit(rightWidget);
    QFont mono("Consolas", 9);
    mono.setStyleHint(QFont::Monospace);
    m_editor->setFont(mono);
    m_editor->setTabStopDistance(28);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editor->setPlaceholderText("여기에 QSS 코드를 입력하세요...");
    rightLayout->addWidget(m_editor, 1);

    m_statusLabel = new QLabel("", rightWidget);
    m_statusLabel->setStyleSheet("color:#10B981; font-size:9pt; font-weight:600;");
    rightLayout->addWidget(m_statusLabel);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter, 1);

    // ── 하단 버튼 ────────────────────────────────────────────
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#E2E8F0;");
    root->addWidget(sep);

    auto* btnRow = new QHBoxLayout;
    auto* applyBtn  = new QPushButton("즉시 적용",    this);
    auto* saveBtn   = new QPushButton("저장",         this);
    auto* resetBtn  = new QPushButton("기본값 복원",  this);
    auto* closeBtn  = new QPushButton("닫기",         this);
    applyBtn->setFixedHeight(36);
    saveBtn->setFixedHeight(36);
    resetBtn->setFixedHeight(36);
    closeBtn->setFixedHeight(36);
    applyBtn->setStyleSheet("QPushButton{background:#10B981;color:white;border:none;border-radius:6px;font-weight:700;padding:0 20px;}"
                             "QPushButton:hover{background:#059669;}");
    saveBtn->setStyleSheet("QPushButton{background:#1D4ED8;color:white;border:none;border-radius:6px;font-weight:700;padding:0 20px;}"
                            "QPushButton:hover{background:#1E40AF;}");
    resetBtn->setStyleSheet("QPushButton{background:#F59E0B;color:white;border:none;border-radius:6px;font-weight:700;padding:0 20px;}"
                             "QPushButton:hover{background:#D97706;}");
    closeBtn->setStyleSheet("QPushButton{background:transparent;color:#475569;border:1.5px solid #CBD5E1;border-radius:6px;font-weight:600;padding:0 20px;}"
                             "QPushButton:hover{background:#F1F5F9;}");

    btnRow->addWidget(applyBtn);
    btnRow->addWidget(saveBtn);
    btnRow->addWidget(resetBtn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);

    // 현재 스타일시트를 에디터에 로드
    QString saved = loadSavedQss();
    m_editor->setPlainText(saved.isEmpty() ? qApp->styleSheet() : saved);

    // ── 시그널 연결 ──────────────────────────────────────────
    connect(m_presetGroup, &QButtonGroup::idClicked, this, &SettingsDialog::onPreset);
    connect(colorBtn,  &QPushButton::clicked, this, &SettingsDialog::onPickColor);
    connect(m_fontSlider, &QSlider::valueChanged, this, &SettingsDialog::onFontSizeChanged);
    connect(applyBtn,  &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(saveBtn,   &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(resetBtn,  &QPushButton::clicked, this, &SettingsDialog::onReset);
    connect(closeBtn,  &QPushButton::clicked, this, &QDialog::accept);
    connect(importBtn, &QPushButton::clicked, this, &SettingsDialog::onImport);
    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExport);
}

void SettingsDialog::onPreset(int id) {
    if (id < 0 || id >= 6) return;
    const Preset& p = kPresets[id];
    bool dark = (id == 5);
    int  fs   = m_fontSlider->value();
    QString qss = buildTheme(p.primary, p.accent, p.menuBg, fs, dark);
    m_editor->setPlainText(qss);
    m_primaryColor = p.primary;
    m_colorPreview->setStyleSheet(
        QString("background:%1; border-radius:6px; border:1px solid #CBD5E1;").arg(p.primary));
    qApp->setStyleSheet(qss);
    m_statusLabel->setText("✓ 테마 적용됨: " + p.name);
}

void SettingsDialog::onPickColor() {
    QColor c = QColorDialog::getColor(QColor(m_primaryColor), this, "기본 색상 선택");
    if (!c.isValid()) return;
    m_primaryColor = c.name();
    m_colorPreview->setStyleSheet(
        QString("background:%1; border-radius:6px; border:1px solid #CBD5E1;").arg(m_primaryColor));

    // 에디터의 현재 QSS에 색상 적용 후 즉시 반영
    QString qss = applyPrimaryColor(m_editor->toPlainText(), m_primaryColor);
    m_editor->setPlainText(qss);
    qApp->setStyleSheet(qss);
    m_statusLabel->setText("✓ 색상 적용됨: " + m_primaryColor);
}

void SettingsDialog::onFontSizeChanged(int value) {
    m_fontSizeLabel->setText(QString("%1pt").arg(value));
    QString qss = applyFontSize(m_editor->toPlainText(), value);
    m_editor->setPlainText(qss);
    qApp->setStyleSheet(qss);
}

void SettingsDialog::onApply() {
    qApp->setStyleSheet(m_editor->toPlainText());
    m_statusLabel->setText("✓ 즉시 적용되었습니다.");
}

void SettingsDialog::onSave() {
    QString qss = m_editor->toPlainText();
    saveQss(qss);
    qApp->setStyleSheet(qss);
    m_statusLabel->setText("✓ 저장 완료: " + savePath());
}

void SettingsDialog::onReset() {
    if (QMessageBox::question(this, "기본값 복원",
            "기본 테마로 복원하시겠습니까?\n저장된 사용자 테마는 삭제됩니다.")
            != QMessageBox::Yes) return;

    QFile::remove(savePath());
    QFile f(":/styles/light.qss");
    QString qss;
    if (f.open(QFile::ReadOnly)) qss = QString::fromUtf8(f.readAll());
    m_editor->setPlainText(qss);
    qApp->setStyleSheet(qss);
    m_statusLabel->setText("✓ 기본 테마로 복원되었습니다.");
}

void SettingsDialog::onImport() {
    QString path = QFileDialog::getOpenFileName(this, "QSS 파일 불러오기", {},
                                                "QSS 파일 (*.qss);;텍스트 파일 (*.txt)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QFile::ReadOnly))
        { QMessageBox::warning(this, "오류", "파일을 열 수 없습니다."); return; }
    m_editor->setPlainText(QString::fromUtf8(f.readAll()));
    m_statusLabel->setText("✓ 불러오기 완료: " + QFileInfo(path).fileName());
}

void SettingsDialog::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "QSS 파일 내보내기",
                                                "my_theme.qss", "QSS 파일 (*.qss)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QFile::WriteOnly | QFile::Text))
        { QMessageBox::warning(this, "오류", "파일을 저장할 수 없습니다."); return; }
    QTextStream(&f) << m_editor->toPlainText();
    m_statusLabel->setText("✓ 내보내기 완료: " + QFileInfo(path).fileName());
}

// ─── 유틸: 폰트 크기 교체 ─────────────────────────────────────
QString SettingsDialog::applyFontSize(const QString& qss, int pt) const {
    QString result = qss;
    static QRegularExpression re("font-size:\\s*\\d+pt;");
    result.replace(re, QString("font-size:%1pt;").arg(pt));
    // 첫 번째 * {} 블록에 font-size 없으면 추가
    if (!result.contains("font-size")) {
        result.prepend(QString("* { font-size:%1pt; }\n").arg(pt));
    }
    return result;
}

// ─── 유틸: 기본 색상 교체 ─────────────────────────────────────
QString SettingsDialog::applyPrimaryColor(const QString& qss, const QString& hex) const {
    QString result = qss;
    // 메뉴바/툴바 배경색 교체 (dark navy 계열 → 선택 색상)
    QStringList targets = { "#003585", "#004AB3", "#002060", "#0D1424", "#0F172A",
                             "#064E3B", "#3B0764", "#881337", "#0F172A" };
    for (const QString& t : targets)
        result.replace(t, hex, Qt::CaseInsensitive);
    return result;
}
