// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "InstanceDelegate.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QTextLayout>
#include <QTextOption>
#include <QtMath>

#include <QIcon>
#include <QTextEdit>
#include "BaseInstance.h"
#include "InstanceList.h"
#include "InstanceView.h"

ListViewDelegate::ListViewDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void drawSelectionRect(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (option.state & QStyle::State_Selected) {
        QColor accent = QColor(34, 211, 238);
        accent.setAlpha(220);
        painter->setPen(QPen(accent, 1));
        painter->setBrush(QColor(18, 42, 52));
        painter->drawRoundedRect(rect.adjusted(2, 2, -2, -2), 12, 12);
        painter->setBrush(QColor(34, 211, 238));
        painter->drawRoundedRect(QRect(rect.left() + 3, rect.top() + 10, 3, rect.height() - 20), 2, 2);
    } else {
        QColor backgroundColor = option.palette.color(QPalette::Window);
        backgroundColor.setAlpha(180);
        painter->setPen(QPen(QColor(255, 255, 255, 32), 1));
        painter->setBrush(backgroundColor);
        painter->drawRoundedRect(rect.adjusted(2, 2, -2, -2), 12, 12);
    }

    painter->restore();
}

void drawFocusRect(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect)
{
    if (!(option.state & QStyle::State_HasFocus))
        return;
    QStyleOptionFocusRect opt;
    opt.direction = option.direction;
    opt.fontMetrics = option.fontMetrics;
    opt.palette = option.palette;
    opt.rect = rect;
    // opt.state           = option.state | QStyle::State_KeyboardFocusChange |
    // QStyle::State_Item;
    auto col = option.state & QStyle::State_Selected ? QPalette::Highlight : QPalette::Base;
    opt.backgroundColor = option.palette.color(col);
    // Apparently some widget styles expect this hint to not be set
    painter->setRenderHint(QPainter::Antialiasing, false);

    QStyle* style = option.widget ? option.widget->style() : QApplication::style();

    style->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, painter, option.widget);

    painter->setRenderHint(QPainter::Antialiasing);
}

// TODO this can be made a lot prettier
void drawProgressOverlay(QPainter* painter, const QStyleOptionViewItem& option, const int value, const int maximum)
{
    if (maximum == 0 || value == maximum) {
        return;
    }

    painter->save();

    qreal percent = (qreal)value / (qreal)maximum;
    QColor color = option.palette.color(QPalette::Dark);
    color.setAlphaF(0.70f);
    painter->setBrush(color);
    painter->setPen(QPen(QBrush(), 0));
    painter->drawPie(option.rect, 90 * 16, -percent * 360 * 16);

    painter->restore();
}

void drawBadges(QPainter* painter, const QStyleOptionViewItem& option, BaseInstance* instance, QIcon::Mode mode, QIcon::State state)
{
    QList<QString> pixmaps;
    if (instance->isRunning()) {
        pixmaps.append("status-running");
    } else if (instance->hasCrashed() || instance->hasVersionBroken()) {
        pixmaps.append("status-bad");
    }
    if (instance->hasUpdateAvailable()) {
        pixmaps.append("checkupdate");
    }

    static const int itemSide = 24;
    static const int spacing = 1;
    const int itemsPerRow = qMax(1, qFloor(double(option.rect.width() + spacing) / double(itemSide + spacing)));
    const int rows = qCeil((double)pixmaps.size() / (double)itemsPerRow);
    QListIterator<QString> it(pixmaps);
    painter->translate(option.rect.topLeft());
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < itemsPerRow; ++x) {
            if (!it.hasNext()) {
                return;
            }
            // FIXME: inject this.
            auto icon = QIcon::fromTheme(it.next());
            // opt.icon.paint(painter, iconbox, Qt::AlignCenter, mode, state);
            const QPixmap pixmap;
            // itemSide
            QRect badgeRect(option.rect.width() - x * itemSide + qMax(x - 1, 0) * spacing - itemSide,
                            y * itemSide + qMax(y - 1, 0) * spacing, itemSide, itemSide);
            icon.paint(painter, badgeRect, Qt::AlignCenter, mode, state);
        }
    }
    painter->translate(-option.rect.topLeft());
}

void ListViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    auto instance = static_cast<BaseInstance*>(index.data(InstanceList::InstancePointerRole).value<void*>());
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    const QRect card = opt.rect.adjusted(5,5,-5,-5);
    QPainterPath clip; clip.addRoundedRect(card,9,9);
    painter->setClipPath(clip);
    painter->fillRect(card,QColor("#2a303b"));
    QRect coverRect = card.adjusted(0,0,0,-58);
    QPixmap cover;
    if(instance) cover.load(instance->instanceRoot()+"/cover.png");
    if(cover.isNull()) cover.load(QString(":/beta-covers/cover%1.png").arg(instance ? qHash(instance->id())%5 : 0));
    const auto scaled = cover.scaled(coverRect.size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
    painter->drawPixmap(coverRect,scaled,QRect((scaled.width()-coverRect.width())/2,(scaled.height()-coverRect.height())/2,coverRect.width(),coverRect.height()));
    QLinearGradient shade(coverRect.topLeft(),coverRect.bottomLeft());
    shade.setColorAt(0,QColor(10,18,26,15)); shade.setColorAt(1,QColor(10,18,26,130)); painter->fillRect(coverRect,shade);
    opt.icon.paint(painter,QRect(card.right()-44,coverRect.bottom()-35,32,32));
    painter->setPen(QColor("#f2f4f7")); painter->setFont(QFont("Segoe UI",11));
    painter->drawText(QRect(card.left()+10,coverRect.bottom()+7,card.width()-20,23),Qt::AlignVCenter,painter->fontMetrics().elidedText(index.data().toString(),Qt::ElideRight,card.width()-20));
    painter->setFont(QFont("Segoe UI",9)); painter->setPen(QColor("#a7b0be"));
    const auto detail = instance ? instance->getStatusbarDescription() : QString();
    painter->drawText(QRect(card.left()+10,coverRect.bottom()+31,card.width()-20,18),Qt::AlignVCenter,painter->fontMetrics().elidedText(detail,Qt::ElideRight,card.width()-20));
    // Etiqueta de la tarjeta: nombre del grupo en un color fijo por grupo (solo visual; sin grupo no se muestra).
    const QString group = index.data(InstanceList::GroupRole).toString();
    if (!group.isEmpty()) {
        static const QList<QColor> chipColors = { QColor("#354757"), QColor("#634b2b"), QColor("#2d583a"), QColor("#38446b"), QColor("#63353e") };
        const QRect chip(coverRect.left() + 7, coverRect.bottom() - 23, painter->fontMetrics().horizontalAdvance(group) + 16, 18);
        painter->setPen(Qt::NoPen);
        painter->setBrush(chipColors[qHash(group) % chipColors.size()]);
        painter->drawRoundedRect(chip, 5, 5);
        painter->setBrush(Qt::NoBrush);
        painter->setFont(QFont("Segoe UI",8)); painter->setPen(QColor("#f2f7fb")); painter->drawText(chip, Qt::AlignCenter, group);
    }
    painter->setClipping(false); painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(opt.state & QStyle::State_Selected ? QColor("#65c8e5") : QColor("#444e5d"),opt.state & QStyle::State_Selected ? 2 : 1));
    painter->drawRoundedRect(card,9,9);
    if(instance) drawBadges(painter,opt,instance,QIcon::Normal,QIcon::Off);
    drawProgressOverlay(painter,opt,index.data(InstanceViewRoles::ProgressValueRole).toInt(),index.data(InstanceViewRoles::ProgressMaximumRole).toInt());
    painter->restore();
}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const
{
    const auto view = qobject_cast<const InstanceView*>(option.widget);
    return QSize(view ? view->itemWidth() : 240,210);
}
class NoReturnTextEdit : public QTextEdit {
    Q_OBJECT
   public:
    explicit NoReturnTextEdit(QWidget* parent) : QTextEdit(parent)
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    }
    bool event(QEvent* event) override
    {
        auto eventType = event->type();
        if (eventType == QEvent::KeyPress || eventType == QEvent::KeyRelease) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            auto key = keyEvent->key();
            if ((key == Qt::Key_Return || key == Qt::Key_Enter) && eventType == QEvent::KeyPress) {
                emit editingDone();
                return true;
            }
            if (key == Qt::Key_Tab) {
                return true;
            }
        }
        return QTextEdit::event(event);
    }
   signals:
    void editingDone();
};

void ListViewDelegate::updateEditorGeometry(QWidget* editor,
                                            const QStyleOptionViewItem& option,
                                            [[maybe_unused]] const QModelIndex& index) const
{
    const int iconSize = 48;
    QRect textRect = option.rect;
    // QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    textRect.adjust(0, iconSize + 5, 0, 0);
    editor->setGeometry(textRect);
}

void ListViewDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto text = index.data(Qt::EditRole).toString();
    QTextEdit* realEditor = qobject_cast<NoReturnTextEdit*>(editor);
    realEditor->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    realEditor->append(text);
    realEditor->selectAll();
    realEditor->document()->clearUndoRedoStacks();
}

void ListViewDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QTextEdit* realEditor = qobject_cast<NoReturnTextEdit*>(editor);
    QString text = realEditor->toPlainText();
    text.replace(QChar('\n'), QChar(' '));
    text = text.trimmed();
    // Prevent instance names longer than 128 chars
    text.truncate(128);
    if (text.size() != 0) {
        emit textChanged(model->data(index).toString(), text);
        model->setData(index, text);
    }
}

QWidget* ListViewDelegate::createEditor(QWidget* parent,
                                        [[maybe_unused]] const QStyleOptionViewItem& option,
                                        [[maybe_unused]] const QModelIndex& index) const
{
    auto editor = new NoReturnTextEdit(parent);
    connect(editor, &NoReturnTextEdit::editingDone, this, &ListViewDelegate::editingDone);
    return editor;
}

void ListViewDelegate::editingDone()
{
    NoReturnTextEdit* editor = qobject_cast<NoReturnTextEdit*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

#include "InstanceDelegate.moc"
