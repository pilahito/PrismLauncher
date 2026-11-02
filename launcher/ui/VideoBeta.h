// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsDropShadowEffect>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainterPath>
#include <QProgressBar>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#include <QMouseEvent>
#include <QWindow>
#include "settings/SettingsObject.h"

namespace VideoBeta {
class WindowFrame : public QObject {
public:
    explicit WindowFrame(QWidget* surface) : QObject(surface), surface(surface) { surface->setMouseTracking(true); surface->installEventFilter(this); }
protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if(event->type()==QEvent::Leave) { surface->unsetCursor(); return false; }
        if(event->type()!=QEvent::MouseMove && event->type()!=QEvent::MouseButtonPress) return false;
        auto mouse=static_cast<QMouseEvent*>(event); auto window=surface->window();
        if(window->isMaximized()) return false;
        const auto pos=mouse->position(); Qt::Edges edges;
        if(pos.x()<7)edges|=Qt::LeftEdge; if(pos.x()>surface->width()-7)edges|=Qt::RightEdge;
        if(pos.y()<7)edges|=Qt::TopEdge; if(pos.y()>surface->height()-7)edges|=Qt::BottomEdge;
        surface->setCursor(edges.testFlag(Qt::LeftEdge)||edges.testFlag(Qt::RightEdge) ? (edges.testFlag(Qt::TopEdge)||edges.testFlag(Qt::BottomEdge)?Qt::SizeFDiagCursor:Qt::SizeHorCursor) : (edges?Qt::SizeVerCursor:Qt::ArrowCursor));
        if(event->type()==QEvent::MouseButtonPress && mouse->button()==Qt::LeftButton && edges && window->windowHandle()) return window->windowHandle()->startSystemResize(edges);
        return false;
    }
private: QWidget* surface;
};
class TitleBar : public QWidget {
public:
    explicit TitleBar(QWidget* parent) : QWidget(parent) {
        setFixedHeight(27); auto row=new QHBoxLayout(this); row->setContentsMargins(4,0,0,0);
        auto logo=new QLabel(this); logo->setPixmap(APPLICATION->logo().pixmap(18,18)); row->addWidget(logo); row->addStretch();
        auto title=new QLabel("Prism Launcher",this); title->setAttribute(Qt::WA_TransparentForMouseEvents); title->setStyleSheet("color: #9fa8b5; font-size: 13px;"); row->addWidget(title); row->addStretch();
        for(int i=0;i<3;++i) { auto b=new QToolButton(this); b->setText(i==0?QString::fromUtf8("−"):i==1?QString::fromUtf8("□"):QString::fromUtf8("×")); b->setToolTip(i==0?"Minimizar":i==1?"Maximizar / restaurar":"Cerrar"); b->setFixedSize(30,25); b->setStyleSheet("QToolButton { border: 0; padding: 0; min-height: 0; color: #9da8b8; background: transparent; font-size: 17px; } QToolButton:hover { color: white; background: #394352; }"); row->addWidget(b); connect(b,&QToolButton::clicked,this,[this,i] { if(i==0)window()->showMinimized(); else if(i==1) { if(window()->isMaximized())window()->showNormal(); else window()->showMaximized(); } else window()->close(); }); }
    }
protected:
    void mousePressEvent(QMouseEvent* e) override { if(e->button()==Qt::LeftButton && window()->windowHandle())window()->windowHandle()->startSystemMove(); }
    void mouseDoubleClickEvent(QMouseEvent* e) override { if(e->button()==Qt::LeftButton) { if(window()->isMaximized())window()->showNormal(); else window()->showMaximized(); } }
};
class Tabs : public QTabBar {
public: using QTabBar::QTabBar;
protected:
    QSize tabSizeHint(int index) const override { return QSize(qMax(106,fontMetrics().horizontalAdvance(tabText(index))+34),34); }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const QList<QColor> colors={QColor("#354757"),QColor("#634b2b"),QColor("#2d583a"),QColor("#38446b"),QColor("#63353e")};
        for(int i=0;i<count();++i) { const auto r=tabRect(i).adjusted(0,0,-4,3); auto color=colors[i%colors.size()]; QLinearGradient fill(0,r.top(),0,r.bottom()); fill.setColorAt(0,color); fill.setColorAt(1,color.darker(140)); p.setBrush(fill); p.setPen(color.lighter(125)); p.drawRoundedRect(r,6,6); if(i==currentIndex()){p.setPen(QPen(QColor("#8ccbe2"),3));p.drawLine(r.left()+5,r.top()+1,r.right()-5,r.top()+1);} p.setPen(i==currentIndex()?QColor("#ffffff"):QColor("#cbd1db")); p.drawText(r.adjusted(0,0,0,-3),Qt::AlignCenter,tabText(i)); }
    }
};
inline QIcon icon(const QString& kind, QColor color = QColor("#c7cfda"))
{
    QPixmap pixmap(48, 48); pixmap.fill(Qt::transparent);
    QPainter p(&pixmap); p.setRenderHint(QPainter::Antialiasing); p.scale(2, 2);
    p.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (kind == "add") { p.drawEllipse(QRectF(3,3,18,18)); p.drawLine(12,7,12,17); p.drawLine(7,12,17,12); }
    else if (kind == "folder") { QPainterPath a; a.moveTo(3,6); a.lineTo(9,6); a.lineTo(11,9); a.lineTo(21,9); a.lineTo(21,20); a.lineTo(3,20); a.closeSubpath(); p.drawPath(a); }
    else if (kind == "play") { p.setBrush(QColor("#52cdeb")); p.setPen(Qt::NoPen); p.drawPolygon(QPolygonF{QPointF(6,3),QPointF(21,12),QPointF(6,21)}); }
    else if (kind == "edit") { p.drawPolygon(QPolygonF{QPointF(4,16),QPointF(16,4),QPointF(20,8),QPointF(8,20),QPointF(4,20)}); p.drawLine(14,6,18,10); }
    else if (kind == "user") { p.drawEllipse(QRectF(8,3,8,8)); p.drawArc(QRectF(4,13,16,14),0,180*16); p.drawLine(4,20,20,20); }
    else if (kind == "fork") { p.drawEllipse(QRectF(4,3,5,5)); p.drawEllipse(QRectF(15,3,5,5)); p.drawEllipse(QRectF(4,17,5,5)); p.drawLine(6,8,6,17); p.drawArc(QRectF(6,6,12,12),-90*16,90*16); }
    else if (kind == "list") { for(int y : {6,12,18}) { p.drawPoint(4,y); p.drawLine(9,y,21,y); } }
    else if (kind == "device") { p.drawRoundedRect(QRectF(7,3,13,17),2,2); p.drawLine(4,8,4,18); p.setBrush(QColor("#79d990")); p.setPen(Qt::NoPen); p.drawEllipse(QRectF(15,2,7,7)); }
    else { p.drawEllipse(QRectF(8,8,8,8)); for(int i=0;i<8;++i) { p.save(); p.translate(12,12); p.rotate(i*45); p.drawLine(0,8,0,10); p.restore(); } }
    return QIcon(pixmap);
}

inline void glow(QWidget* w)
{
    auto effect = new QGraphicsDropShadowEffect(w);
    effect->setBlurRadius(18); effect->setOffset(0,0); effect->setColor(QColor(58,190,232,100));
    w->setGraphicsEffect(effect);
    w->setStyleSheet("QToolButton, QPushButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #304958,stop:1 #24313d); border: 1px solid #5ab8d6; border-radius: 7px; color: #e2f8ff; padding: 7px 12px; } QToolButton:hover, QPushButton:hover { background: #365466; border-color: #8de5ff; } QToolButton:disabled { color: #9caab6; border-color: #456775; }");
}

inline void showSettings(QWidget* parent)
{
    APPLICATION->ShowGlobalSettings(parent, QStringLiteral("java-settings"));
}
} // namespace VideoBeta