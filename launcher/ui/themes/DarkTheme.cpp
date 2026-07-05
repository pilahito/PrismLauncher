// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Tayou <git@tayou.org>
 *  Copyright (C) 2024 TheKodeToad <TheKodeToad@proton.me>
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
#include "DarkTheme.h"

#include <QObject>

QString DarkTheme::id()
{
    return "dark";
}

QString DarkTheme::name()
{
    return QObject::tr("Dark");
}

QPalette DarkTheme::colorScheme()
{
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(11, 16, 32));
    darkPalette.setColor(QPalette::WindowText, QColor(246, 248, 255));
    darkPalette.setColor(QPalette::Base, QColor(18, 25, 43));
    darkPalette.setColor(QPalette::AlternateBase, QColor(24, 35, 56));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(246, 248, 255));
    darkPalette.setColor(QPalette::ToolTipText, QColor(246, 248, 255));
    darkPalette.setColor(QPalette::Text, QColor(246, 248, 255));
    darkPalette.setColor(QPalette::Button, QColor(24, 35, 56));
    darkPalette.setColor(QPalette::ButtonText, QColor(246, 248, 255));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 88, 88));
    darkPalette.setColor(QPalette::Link, QColor(95, 211, 255));
    darkPalette.setColor(QPalette::Highlight, QColor(91, 124, 255));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::PlaceholderText, QColor(140, 160, 198));
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
    return 0.5;
}

QColor DarkTheme::fadeColor()
{
    return QColor(49, 49, 49);
}

bool DarkTheme::hasStyleSheet()
{
    return true;
}

QString DarkTheme::appStyleSheet()
{
    return R"(
        QMainWindow, QDialog, QDockWidget {
            background-color: #0b1020;
            color: #f6f8ff;
        }
        QWidget {
            color: #f6f8ff;
            background-color: transparent;
        }
        QToolBar {
            background-color: rgba(11, 16, 32, 0.92);
            border: 1px solid rgba(255, 255, 255, 0.06);
            border-radius: 14px;
            padding: 6px;
            spacing: 6px;
        }
        QToolButton, QPushButton {
            background-color: #182338;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 10px;
            padding: 8px 12px;
            min-height: 24px;
            color: #f6f8ff;
        }
        QToolButton:hover, QPushButton:hover {
            background-color: #22304a;
            border-color: rgba(95, 211, 255, 0.45);
        }
        QToolButton:pressed, QPushButton:pressed {
            background-color: #12192b;
        }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #12192b;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 10px;
            padding: 7px 10px;
            color: #f6f8ff;
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
            background: rgba(255, 255, 255, 0.18);
            border-radius: 5px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(255, 255, 255, 0.28);
        }
        QListView, QTreeView, QTableView {
            background-color: #0f1422;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 14px;
            padding: 6px;
            alternate-background-color: #131a2b;
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
            background-color: #141b2d;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 12px;
            padding: 6px;
        }
        QMenu::item {
            border-radius: 8px;
            padding: 8px 12px;
        }
        QMenu::item:selected {
            background-color: #22304a;
        }
        QTabBar::tab {
            background-color: #182338;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
            padding: 8px 12px;
            margin-right: 4px;
        }
        QTabBar::tab:selected {
            background-color: #22304a;
        }
        QScrollArea, QFrame {
            background-color: transparent;
            border: none;
        }
        QLabel {
            color: #f6f8ff;
        }
        QStatusBar {
            background-color: rgba(11, 16, 32, 0.95);
            color: #f6f8ff;
        }
        QToolTip {
            color: #f6f8ff;
            background-color: #1f2d47;
            border: 1px solid rgba(255, 255, 255, 0.12);
            border-radius: 8px;
            padding: 6px;
        }
    )";
}

QString DarkTheme::tooltip()
{
    return "";
}
