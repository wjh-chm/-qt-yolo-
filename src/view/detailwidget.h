#ifndef DETAILWIDGET_H
#define DETAILWIDGET_H

#include <QDateTime>
#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QWidget>

struct DetailImageItem
{
    int imageId = 0;
    int channelId = 0;
    int exceptionId = -1;
    QString imageName;
    QString imagePath;
    QDateTime captureTime;
};

class DetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DetailWidget(QWidget *parent = nullptr);

    void setImages(const QList<DetailImageItem> &images);
    void clear();
    void showPreviewForPath(const QString &imagePath);
    QList<DetailImageItem> selectedImages() const;

signals:
    void requestOpenRelatedVideo(int channelId, const QDateTime &captureTime);
    void requestDeleteImages(const QList<DetailImageItem> &images);
    void requestExportImages(const QList<DetailImageItem> &images);

private:
    void initUi();
    void openPreviewAtIndex(int index);

    QListWidget *m_listWidget;
    QPushButton *m_btnDelete;
    QPushButton *m_btnExport;
    QList<DetailImageItem> m_images;
};

#endif // DETAILWIDGET_H
