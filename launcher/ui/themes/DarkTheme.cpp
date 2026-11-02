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
    darkPalette.setColor(QPalette::Window, QColor(19, 21, 26));
    darkPalette.setColor(QPalette::WindowText, QColor(235, 238, 244));
    darkPalette.setColor(QPalette::Base, QColor(25, 28, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(30, 34, 43));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(30, 34, 43));
    darkPalette.setColor(QPalette::ToolTipText, QColor(235, 238, 244));
    darkPalette.setColor(QPalette::Text, QColor(235, 238, 244));
    darkPalette.setColor(QPalette::Button, QColor(30, 34, 43));
    darkPalette.setColor(QPalette::ButtonText, QColor(235, 238, 244));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 88, 120));
    darkPalette.setColor(QPalette::Link, QColor(103, 232, 249));
    darkPalette.setColor(QPalette::Highlight, QColor(34, 211, 238));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::PlaceholderText, QColor(128, 137, 153));
    return fadeInactive(darkPalette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
    return 0.5;
}

QColor DarkTheme::fadeColor()
{
    return QColor(39, 34, 47);
}

bool DarkTheme::hasStyleSheet()
{
    return true;
}

QString DarkTheme::appStyleSheet()
{
    return R"(
        QWidget {
            color: #ebeff4;
            background-color: transparent;
        }
        QMainWindow, QDialog, QDockWidget {
            background-color: #13151a;
            color: #ebeff4;
        }
        QWidget#centralWidget, QStackedWidget#mainContent {
            background-color: #191d25;
        }
        QToolBar {
            background-color: rgba(25, 28, 35, 0.96);
            border: 1px solid rgba(255, 255, 255, 0.06);
            border-radius: 10px;
            padding: 4px;
            spacing: 6px;
        }
        QToolButton, QPushButton {
            background-color: #1e222b;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 7px;
            padding: 6px 10px;
            min-height: 24px;
            color: #ebeff4;
        }
        QToolButton:hover, QPushButton:hover {
            background-color: #292e39;
            border-color: rgba(34, 211, 238, 0.62);
        }
        QToolButton:pressed, QPushButton:pressed {
            background-color: #191c23;
        }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: #191c23;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 10px;
            padding: 7px 10px;
            color: #ebeff4;
            selection-background-color: #06b6d4;
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
            background-color: #1e222b;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 9px;
            padding: 4px;
            alternate-background-color: #222630;
        }
        QListView::item, QTreeView::item, QTableView::item {
            border-radius: 7px;
            padding: 6px 8px;
            margin: 2px 0px;
        }
        QListView::item:selected, QTreeView::item:selected, QTableView::item:selected {
            background-color: #304b5e;
            color: #ffffff;
        }
        QMenu {
            background-color: #1e222b;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 12px;
            padding: 6px;
        }
        QMenu::item {
            border-radius: 8px;
            padding: 8px 12px;
        }
        QMenu::item:selected {
            background-color: #292e39;
        }
        QTabBar::tab {
            background-color: #1e222b;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-top-left-radius: 10px;
            border-top-right-radius: 10px;
            padding: 8px 12px;
            margin-right: 4px;
        }
        QTabBar::tab:selected {
            background-color: #292e39;
        }
        QScrollArea, QFrame {
            background-color: transparent;
            border: none;
        }
        QLabel {
            color: #ebeff4;
        }
        QStatusBar {
            background-color: rgba(25, 28, 35, 0.98);
            color: #ebeff4;
        }
        QToolTip {
            color: #ebeff4;
            background-color: #242933;
            border: 1px solid rgba(255, 255, 255, 0.12);
            border-radius: 8px;
            padding: 6px;
        }
        QDialog { background: #232a35; }
        QWidget#betaNavigationPanel { background: #202733; border-right: 1px solid #364351; }
        QLabel#betaNavigationBrand { font-size: 17px; font-weight: 600; padding: 4px 8px 10px; }
        QLabel#betaPageHeading { font-size: 19px; font-weight: 600; padding: 14px 0 12px; border-bottom: 1px solid #394552; }
        QListView#betaPageNavigation { background: transparent; border: none; padding: 0; }
        QListView#betaPageNavigation::item { padding: 6px 8px; border: 1px solid transparent; border-radius: 7px; }
        QListView#betaPageNavigation::item:selected { background: #2d4555; border: 1px solid #57b5cf; color: #e5faff; }
        QListView#betaPageNavigation::item:hover { background: #2a3543; }
        QGroupBox { background: #272f3c; border: 1px solid #3a4554; border-radius: 9px; margin-top: 23px; padding: 14px 12px 12px; }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 2px 5px; color: #dae4ef; font-weight: 600; }
        QTabWidget::pane { background: #232b36; border: 1px solid #394553; border-radius: 8px; padding: 8px; top: -1px; }
        QTabBar::tab:selected { background: #2d414f; border-top: 2px solid #66c5e3; }
        QHeaderView::section { background: #252e3b; color: #aebccd; border: none; border-bottom: 1px solid #435162; padding: 8px 10px; }
        QTableView { gridline-color: #364250; }
        QAbstractItemView:disabled { color: #748293; }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1px solid #65bed8; }
        QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled { color: #768493; background: #202630; }
        QComboBox QAbstractItemView { background: #27313e; border: 1px solid #435368; selection-background-color: #33576b; }
        QComboBox::drop-down { width: 24px; }
        QComboBox::down-arrow { image: url(:/beta-covers/down.svg); width: 14px; height: 14px; }
        QCheckBox, QRadioButton { spacing: 8px; padding: 3px 0; }
        QCheckBox::indicator { width: 17px; height: 17px; border: 1px solid #647487; border-radius: 4px; background: #1d2530; }
        QCheckBox::indicator:checked { background: #328eaa; border-color: #65c8df; image: url(:/beta-covers/check.svg); }
        QCheckBox::indicator:indeterminate { background: #607b8d; }
        QCheckBox::indicator:disabled { border-color: #414e5d; background: #29313b; }
        QRadioButton::indicator { width: 16px; height: 16px; border-radius: 8px; border: 1px solid #677b8f; background: #1d2530; }
        QRadioButton::indicator:checked { border: 4px solid #65bdd4; background: #e4faff; }
        QPushButton:default, QPushButton[betaPrimary="true"] { background: #2a4b5e; border: 1px solid #5ab8d6; color: #e5faff; }
        QPushButton:default:hover, QPushButton[betaPrimary="true"]:hover { background: #345f74; }
        QPushButton:disabled, QToolButton:disabled { color: #768493; background: #222a34; border-color: #343e4b; }
        QProgressBar { background: #19222d; border: 1px solid #384859; border-radius: 6px; text-align: center; min-height: 12px; color: #e0f5ff; }
        QProgressBar::chunk { background: #3eacc7; border-radius: 5px; }
        QSlider::groove:horizontal { background: #3d4958; height: 5px; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #5bc4cf; border-radius: 2px; }
        QSlider::handle:horizontal { width: 17px; margin: -6px 0; background: #ddf6ff; border: 1px solid #89cddd; border-radius: 8px; }
        QScrollBar:horizontal { height: 10px; background: #1e2631; }
        QScrollBar::handle:horizontal { min-width: 24px; background: #526375; border-radius: 5px; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
        QSplitter::handle { background: #354252; }
        QPlainTextEdit { font-family: Consolas; background-color: #171c24; border: 1px solid #364351; border-radius: 10px; }
        QSpinBox, QDoubleSpinBox { padding-right: 26px; min-height: 22px; }
        QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 22px; border: none; }
        QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 22px; border: none; }
    )";
}

QString DarkTheme::tooltip()
{
    return "";
}
