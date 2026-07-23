#ifndef SETTINGWIDGET_H
#define SETTINGWIDGET_H

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QWidget>

#include <array>

class SettingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingWidget(QWidget *parent = nullptr);

private:
    void initUi();
    void initQss();
    void loadSetting();
    void saveSetting();
    void refreshCameraLabels();
    QStringList detectedCameraNames() const;

    QLabel *lab_title;
    QPushButton *btn_browse;
    QLabel *lab_savepath;
    QLineEdit *edit_path;
    QComboBox *box_interval;
    std::array<QLabel *, 4> m_cameraLabels;
    std::array<QLineEdit *, 4> m_channelNameEdits;
    std::array<QComboBox *, 4> m_channelSelectCombos;
    QPushButton *btn_save;
    QPushButton *btn_cancel;

public slots:
    void browseRecordRoot();

signals:
    void requestBackToMain();
    void settingsSaved();
};

#endif // SETTINGWIDGET_H
