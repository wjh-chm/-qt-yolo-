#include "detailwidget.h"

#include <QDialog>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <functional>

namespace {

class ZoomGraphicsView : public QGraphicsView
{
public:
    explicit ZoomGraphicsView(QWidget *parent = nullptr)
        : QGraphicsView(parent)
    {
        setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setResizeAnchor(QGraphicsView::AnchorUnderMouse);
        setBackgroundBrush(QColor(28, 28, 28));
        setFrameShape(QFrame::NoFrame);
    }

    void resetZoom()
    {
        resetTransform();
        m_scaleFactor = 1.0;
    }

protected:
    void wheelEvent(QWheelEvent *event) override
    {
        const qreal step = event->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
        const qreal nextScale = m_scaleFactor * step;
        if (nextScale < 0.5 || nextScale > 2.0) {
            event->accept();
            return;
        }

        scale(step, step);
        m_scaleFactor = nextScale;
        event->accept();
    }

private:
    qreal m_scaleFactor = 1.0;
};

class ImagePreviewDialog : public QDialog
{
public:
    explicit ImagePreviewDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("图片预览"));
        resize(1100, 760);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(10);

        m_titleLabel = new QLabel(QStringLiteral("暂无图片"), this);
        m_titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_titleLabel);

        m_scene = new QGraphicsScene(this);
        m_view = new ZoomGraphicsView(this);
        m_view->setScene(m_scene);
        mainLayout->addWidget(m_view, 1);

        QHBoxLayout *controlLayout = new QHBoxLayout;
        m_prevButton = new QPushButton(QStringLiteral("上一张"), this);
        m_nextButton = new QPushButton(QStringLiteral("下一张"), this);
        m_relatedVideoButton = new QPushButton(QStringLiteral("查看对应视频"), this);
        QPushButton *closeButton = new QPushButton(QStringLiteral("关闭"), this);

        controlLayout->addWidget(m_prevButton);
        controlLayout->addWidget(m_nextButton);
        controlLayout->addStretch();
        controlLayout->addWidget(m_relatedVideoButton);
        controlLayout->addWidget(closeButton);
        mainLayout->addLayout(controlLayout);

        connect(m_prevButton, &QPushButton::clicked, this, [this]() {
            showIndex(m_currentIndex - 1);
        });
        connect(m_nextButton, &QPushButton::clicked, this, [this]() {
            showIndex(m_currentIndex + 1);
        });
        connect(m_relatedVideoButton, &QPushButton::clicked, this, [this]() {
            if (!m_images.isEmpty() && m_currentIndex >= 0 && m_currentIndex < m_images.size()
                && onOpenRelatedVideo) {
                const DetailImageItem &item = m_images.at(m_currentIndex);
                onOpenRelatedVideo(item.channelId, item.captureTime);
            }
        });
        connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    }

    void setImages(const QList<DetailImageItem> &images, int startIndex)
    {
        m_images = images;
        if (m_images.isEmpty()) {
            showIndex(-1);
            return;
        }

        const int boundedIndex = qBound(0, startIndex, m_images.size() - 1);
        showIndex(boundedIndex);
    }

    std::function<void(int, const QDateTime &)> onOpenRelatedVideo;

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Left) {
            showIndex(m_currentIndex - 1);
            return;
        }
        if (event->key() == Qt::Key_Right) {
            showIndex(m_currentIndex + 1);
            return;
        }

        QDialog::keyPressEvent(event);
    }

    void showEvent(QShowEvent *event) override
    {
        QDialog::showEvent(event);
        fitCurrentPixmap();
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QDialog::resizeEvent(event);
        fitCurrentPixmap();
    }

private:
    void fitCurrentPixmap()
    {
        if (m_currentIndex < 0 || m_currentIndex >= m_images.size() || m_scene->items().isEmpty()) {
            return;
        }

        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }

    void showIndex(int index)
    {
        m_scene->clear();
        m_view->resetZoom();

        if (m_images.isEmpty() || index < 0 || index >= m_images.size()) {
            m_currentIndex = -1;
            m_titleLabel->setText(QStringLiteral("暂无图片"));
            m_prevButton->setEnabled(false);
            m_nextButton->setEnabled(false);
            m_relatedVideoButton->setEnabled(false);
            return;
        }

        m_currentIndex = index;
        const DetailImageItem &item = m_images.at(m_currentIndex);
        const QPixmap pixmap(item.imagePath);
        if (pixmap.isNull()) {
            m_titleLabel->setText(QStringLiteral("%1 (图片加载失败)").arg(item.imageName));
        } else {
            m_titleLabel->setText(QStringLiteral("%1  %2")
                                      .arg(item.imageName)
                                      .arg(item.captureTime.toString("yyyy-MM-dd HH:mm:ss")));
            m_scene->addPixmap(pixmap);
            m_scene->setSceneRect(pixmap.rect());
            fitCurrentPixmap();
        }

        m_prevButton->setEnabled(m_currentIndex > 0);
        m_nextButton->setEnabled(m_currentIndex + 1 < m_images.size());
        m_relatedVideoButton->setEnabled(item.captureTime.isValid());
    }

    QList<DetailImageItem> m_images;
    int m_currentIndex = -1;
    QLabel *m_titleLabel = nullptr;
    ZoomGraphicsView *m_view = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QPushButton *m_prevButton = nullptr;
    QPushButton *m_nextButton = nullptr;
    QPushButton *m_relatedVideoButton = nullptr;
};

} // namespace

DetailWidget::DetailWidget(QWidget *parent)
    : QWidget(parent)
{
    initUi();
}

void DetailWidget::setImages(const QList<DetailImageItem> &images)
{
    m_images = images;
    m_listWidget->clear();

    for (int index = 0; index < m_images.size(); ++index) {
        const DetailImageItem &image = m_images.at(index);

        QListWidgetItem *item = new QListWidgetItem(m_listWidget);
        item->setText(image.imageName);
        item->setData(Qt::UserRole, index);
        item->setSizeHint(QSize(210, 170));

        const QPixmap sourcePixmap(image.imagePath);
        if (!sourcePixmap.isNull()) {
            item->setIcon(QIcon(sourcePixmap.scaled(180,
                                                    120,
                                                    Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation)));
        } else {
            item->setText(QStringLiteral("%1\n(图片不存在)").arg(image.imageName));
        }
    }
}

void DetailWidget::clear()
{
    m_images.clear();
    m_listWidget->clear();
}

QList<DetailImageItem> DetailWidget::selectedImages() const
{
    QList<DetailImageItem> images;
    const QList<QListWidgetItem *> selectedItems = m_listWidget->selectedItems();
    for (QListWidgetItem *item : selectedItems) {
        if (item == nullptr) {
            continue;
        }

        const int index = item->data(Qt::UserRole).toInt();
        if (index >= 0 && index < m_images.size()) {
            images.append(m_images.at(index));
        }
    }

    return images;
}

void DetailWidget::showPreviewForPath(const QString &imagePath)
{
    for (int index = 0; index < m_images.size(); ++index) {
        if (m_images.at(index).imagePath == imagePath) {
            QListWidgetItem *item = m_listWidget->item(index);
            if (item != nullptr) {
                m_listWidget->setCurrentItem(item);
                m_listWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            }
            return;
        }
    }
}

void DetailWidget::initUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *toolLayout = new QHBoxLayout;
    toolLayout->setContentsMargins(0, 0, 0, 0);
    m_btnDelete = new QPushButton(QStringLiteral("删除"), this);
    m_btnExport = new QPushButton(QStringLiteral("导出"), this);
    toolLayout->addStretch();
    toolLayout->addWidget(m_btnDelete);
    toolLayout->addWidget(m_btnExport);
    layout->addLayout(toolLayout);

    m_listWidget = new QListWidget(this);
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setMovement(QListView::Static);
    m_listWidget->setWrapping(true);
    m_listWidget->setSpacing(12);
    m_listWidget->setWordWrap(true);
    m_listWidget->setIconSize(QSize(180, 120));
    m_listWidget->setGridSize(QSize(220, 180));
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if (item == nullptr) {
            return;
        }

        openPreviewAtIndex(item->data(Qt::UserRole).toInt());
    });
    connect(m_btnDelete, &QPushButton::clicked, this, [this]() {
        const QList<DetailImageItem> images = selectedImages();
        if (images.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择图片"));
            return;
        }

        emit requestDeleteImages(images);
    });
    connect(m_btnExport, &QPushButton::clicked, this, [this]() {
        const QList<DetailImageItem> images = selectedImages();
        if (images.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先选择图片"));
            return;
        }

        emit requestExportImages(images);
    });
}

void DetailWidget::openPreviewAtIndex(int index)
{
    if (index < 0 || index >= m_images.size()) {
        return;
    }

    ImagePreviewDialog dialog(this);
    dialog.setImages(m_images, index);
    dialog.onOpenRelatedVideo = [this](int channelId, const QDateTime &captureTime) {
        emit requestOpenRelatedVideo(channelId, captureTime);
    };
    dialog.exec();
}
