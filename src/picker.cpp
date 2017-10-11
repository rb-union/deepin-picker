#include "picker.h"
#include "settings.h"
#include "utils.h"
#include "colormenu.h"
#include <QPainter>
#include <QBitmap>
#include <QPixmap>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QStyleFactory>
#include <QScreen>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

Picker::Picker(QWidget *parent) : QWidget(parent)
{
    // Init window flags.
    setWindowFlags(Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    installEventFilter(this);

    // Init attributes.
    width = 220;
    height = 220;

    screenshotWidth = 11;
    screenshotHeight = 11;

    blockWidth = 20;
    blockHeight = 20;

    displayCursorDot = false;

    // Init update screenshot timer.
    updateScreenshotTimer = new QTimer(this);
    updateScreenshotTimer->setSingleShot(true);
    connect(updateScreenshotTimer, SIGNAL(timeout()), this, SLOT(updateScreenshot()));

    // Init window size and position.
    screenPixmap = QApplication::primaryScreen()->grabWindow(0);
    resize(screenPixmap.size());
    move(0, 0);

    // Show.
    show();

    // Update screenshot.
    updateScreenshot();
}

Picker::~Picker()
{
}

void Picker::paintEvent(QPaintEvent *)
{
}

void Picker::handleMouseMove(int, int)
{
    // Update screenshot.
    if (updateScreenshotTimer->isActive()) {
        updateScreenshotTimer->stop();
    }
    updateScreenshotTimer->start(5);
}

void Picker::updateScreenshot()
{
    if (!displayCursorDot) {
        // Get cursor coordinate.
        cursorX = QCursor::pos().x();
        cursorY = QCursor::pos().y();
        
        QPixmap cursorPixmap(Utils::getQrcPath("transparent.png"));

        // Get image under cursor.
        screenshotPixmap = QApplication::primaryScreen()->grabWindow(
            0,
            cursorX - screenshotWidth / 2,
            cursorY - screenshotHeight / 2,
            screenshotWidth,
            screenshotHeight).scaled(width, height);

        // Draw on screenshot.
        QPainter painter(&cursorPixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        
        QPainterPath circlePath;
        painter.setClipRegion(QRegion(0, 0, width, height, QRegion::Ellipse));
        painter.drawPixmap(0, 0, screenshotPixmap);

        // Draw circle bound.
        int outsidePenWidth = 1;
        QPen outsidePen("#00000");
        outsidePen.setWidth(outsidePenWidth);
        painter.setOpacity(0.05);
        painter.setPen(outsidePen);
        painter.drawEllipse(outsidePenWidth / 2 + 1, outsidePenWidth / 2 + 1, width - outsidePenWidth - 1, height - outsidePenWidth - 1);

        int insidePenWidth = 4;
        QPen insidePen("#ffffff");
        insidePen.setWidth(insidePenWidth);
        painter.setOpacity(0.5);
        painter.setPen(insidePen);
        painter.drawEllipse(insidePenWidth / 2 + 2, insidePenWidth / 2 + 2, width - insidePenWidth - 4, height - insidePenWidth - 4);
        
        // Draw focus block.
        painter.setOpacity(1);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setOpacity(0.2);
        painter.setPen("#000000");
        painter.drawRect(QRect(width / 2 - blockWidth / 2, height / 2 - blockHeight / 2, blockWidth, blockHeight));
        painter.setOpacity(1);
        painter.setPen("#ffffff");
        painter.drawRect(QRect(width / 2 - blockWidth / 2 + 1, height / 2 - blockHeight / 2 + 1, blockWidth - 2, blockHeight - 2));

        // Set screenshot as cursor.
        QApplication::setOverrideCursor(QCursor(cursorPixmap));
    }
}

void Picker::handleLeftButtonPress(int x, int y)
{
    if (!displayCursorDot) {
        QApplication::setOverrideCursor(Qt::ArrowCursor);
        hide();
        
        Settings *settings = new Settings();
        
        if (!Utils::fileExists(settings->configPath())) {
            settings->setOption("color_type", "HEX");
        }
        QString colorType = settings->getOption("color_type").toString();
        
        QColor color = getColorAtCursor(x, y);
        
        copyColor(color, colorType);
    }
}

void Picker::handleRightButtonRelease(int x, int y)
{
    if (!displayCursorDot) {
        displayCursorDot = true;

        ColorMenu *menu = new ColorMenu(x - blockWidth / 2, y - blockHeight / 2, blockWidth, getColorAtCursor(x, y));
        connect(menu, &ColorMenu::copyColor, this, &Picker::copyColor, Qt::QueuedConnection);
        connect(menu, &ColorMenu::exit, this, &Picker::exit, Qt::QueuedConnection);
        menu->show();
        menu->setFocus();
        
        QApplication::setOverrideCursor(Qt::ArrowCursor);
        hide();
        
        QTimer::singleShot(10, menu, &ColorMenu::showMenu);
    }
}

QColor Picker::getColorAtCursor(int x, int y)
{
    QImage img = screenPixmap.copy(x, y, 1, 1).toImage();
    return QColor(img.pixel(0, 0));
}