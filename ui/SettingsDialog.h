#pragma once
#include <QDialog>
#include <QPlainTextEdit>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QButtonGroup>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    // 저장된 사용자 QSS 불러오기 (없으면 빈 문자열)
    static QString loadSavedQss();
    // AppData에 QSS 저장
    static void saveQss(const QString& qss);
    // 저장 경로
    static QString savePath();

private slots:
    void onApply();
    void onSave();
    void onReset();
    void onPreset(int id);
    void onPickColor();
    void onFontSizeChanged(int value);
    void onImport();
    void onExport();

private:
    void setupUi();
    void setEditorText(const QString& qss);
    QString currentEditorText() const;
    QString applyFontSize(const QString& qss, int pt) const;
    QString applyPrimaryColor(const QString& qss, const QString& hex) const;

    QPlainTextEdit* m_editor;
    QLabel*         m_statusLabel;
    QLabel*         m_colorPreview;
    QSlider*        m_fontSlider;
    QLabel*         m_fontSizeLabel;
    QButtonGroup*   m_presetGroup;

    QString m_primaryColor = "#003585";
};
