// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
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
#include "BrightTheme.h"

#include <QObject>

QString BrightTheme::id()
{
    return "bright";
}

QString BrightTheme::name()
{
    return QObject::tr("Bright");
}

QPalette BrightTheme::colorScheme()
{
    QPalette brightPalette;
    brightPalette.setColor(QPalette::Window, QColor(245, 247, 251));
    brightPalette.setColor(QPalette::WindowText, QColor(28, 36, 51));
    brightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    brightPalette.setColor(QPalette::AlternateBase, QColor(242, 246, 252));
    brightPalette.setColor(QPalette::ToolTipBase, QColor(28, 36, 51));
    brightPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    brightPalette.setColor(QPalette::Text, QColor(28, 36, 51));
    brightPalette.setColor(QPalette::Button, QColor(255, 255, 255));
    brightPalette.setColor(QPalette::ButtonText, QColor(28, 36, 51));
    brightPalette.setColor(QPalette::BrightText, QColor(255, 88, 88));
    brightPalette.setColor(QPalette::Link, QColor(37, 137, 164));
    brightPalette.setColor(QPalette::Highlight, QColor(91, 124, 255));
    brightPalette.setColor(QPalette::HighlightedText, Qt::white);
    return fadeInactive(brightPalette, fadeAmount(), fadeColor());
}

double BrightTheme::fadeAmount()
{
    return 0.5;
}

QColor BrightTheme::fadeColor()
{
    return QColor(255, 255, 255);
}

bool BrightTheme::hasStyleSheet()
{
    return false;
}

QString BrightTheme::appStyleSheet()
{
    return R"(
        QMainWindow, QDialog, QDockWidget {
            background-color: #f5f7fb;
            color: #1c2433;
        }
        QWidget {
            color: #1c2433;
            background-color: transparent;
        }
        QToolBar {
            background-color: rgba(255, 255, 255, 0.92);
            border: 1px solid rgba(15, 23, 42, 0.08);
            border-radius: 14px;
            padding: 6px;
            spacing: 6px;
        }
        QToolButton, QPushButton {
            background-color: #ffffff;
            border: 1px solid rgba(15, 23, 42, 0.1);
            border-radius: 10px;
            padding: 8px 12px;
            min-height: 24px;
            color: #1c2433;
        }
        QToolButton:hover, QPushButton:hover {
            background-color: #eef3ff;
            border-color: rgba(91, 124, 255, 0.35);
        }
        QToolButton:pressed, QPushButton:pressed {
            background-color: #e2eaff;
        }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #ffffff;
            border: 1px solid rgba(15, 23, 42, 0.1);
            border-radius: 10px;
            padding: 7px 10px;
            color: #1c2433;
            selection-background-color: #5b7cff;
        }
        QComboBox::drop-down {
            border: none;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: rgba(15, 23, 42, 0.16);
            border-radius: 5px;
            min-height: 24px;
        }
        QListView, QTreeView, QTableView {
            background-color: #fcfdff;
            border: 1px solid rgba(15, 23, 42, 0.08);
            border-radius: 14px;
            padding: 6px;
            alternate-background-color: #f4f7fd;
        }
        QListView::item, QTreeView::item, QTableView::item {
            border-radius: 10px;
            padding: 8px 10px;
            margin: 2px 0px;
        }
        QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #5b7cff, stop:1 #41d3ae);
            color: #ffffff;
        }
        QMenu {
            background-color: #ffffff;
            border: 1px solid rgba(15, 23, 42, 0.08);
            border-radius: 12px;
            padding: 6px;
        }
        QMenu::item {
            border-radius: 8px;
            padding: 8px 12px;
        }
        QMenu::item:selected {
            background-color: #eef3ff;
        }
        QTabBar::tab {
            background-color: #ffffff;
            border: 1px solid rgba(15, 23, 42, 0.1);
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
            padding: 8px 12px;
            margin-right: 4px;
        }
        QTabBar::tab:selected {
            background-color: #eef3ff;
        }
        QToolTip {
            color: #1c2433;
            background-color: #f7faff;
            border: 1px solid rgba(15, 23, 42, 0.12);
            border-radius: 8px;
            padding: 6px;
        }
    )";
}
QString BrightTheme::tooltip()
{
    return QString();
}
