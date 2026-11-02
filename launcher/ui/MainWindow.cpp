// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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
 *      Authors: Andrew Okin
 *               Peterix
 *               Orochimarufan <orochimarufan.x3@gmail.com>
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

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "HardwareInfo.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QButtonGroup>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QImage>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPainter>
#include <QPushButton>
#include <QShortcut>
#include <QShowEvent>
#include <QStatusBar>
#include <QTabBar>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>
#include <QWidgetAction>
#include <memory>

#include <BaseInstance.h>
#include <BuildConfig.h>
#include <DesktopServices.h>
#include <InstanceList.h>
#include <MMCZip.h>
#include <icons/IconList.h>
#include <java/JavaInstallList.h>
#include <java/JavaUtils.h>
#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <minecraft/auth/AccountList.h>
#include <net/ApiDownload.h>
#include <net/NetJob.h>
#include <news/NewsChecker.h>
#include <tools/BaseProfiler.h>
#include <updater/ExternalUpdater.h>
#include "InstanceWindow.h"

#include "ui/GuiUtil.h"
#include "ui/ViewLogWindow.h"
#include "ui/dialogs/AboutDialog.h"
#include "ui/dialogs/CopyInstanceDialog.h"
#include "ui/dialogs/CreateShortcutDialog.h"
#include "ui/dialogs/CustomMessageBox.h"
#include "ui/dialogs/ExportInstanceDialog.h"
#include "ui/dialogs/ExportPackDialog.h"
#include "ui/dialogs/IconPickerDialog.h"
#include "ui/dialogs/ImportResourceDialog.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/dialogs/NewsDialog.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/skins/SkinManageDialog.h"
#include "ui/instanceview/InstanceDelegate.h"
#include "ui/instanceview/InstanceProxyModel.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"
#include "ui/widgets/LabeledToolButton.h"

#include "minecraft/PackProfile.h"
#include "minecraft/VersionFile.h"
#include "minecraft/WorldList.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourcePackFolderModel.h"
#include "minecraft/mod/ShaderPackFolderModel.h"
#include "minecraft/mod/TexturePackFolderModel.h"
#include "minecraft/mod/tasks/LocalResourceParse.h"

#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"

#include "KonamiCode.h"

#include "InstanceCopyTask.h"
#include "InstanceDirUpdate.h"

#include "Json.h"

#include "MMCTime.h"

#include "VideoBeta.h"
#include "BetaShell.h"
#include "BetaSkins.h"
namespace {
class InstanceGroupFilterModel : public InstanceProxyModel {
public:
    using InstanceProxyModel::InstanceProxyModel;
    void setGroupFilter(QString group) { m_group = std::move(group); invalidateFilter(); }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& parent) const override
    {
        const auto sourceIndex = sourceModel()->index(sourceRow, 0, parent);
        if (!m_group.isEmpty() && sourceIndex.data(InstanceViewRoles::GroupRole).toString() != m_group)
            return false;
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, parent);
    }

private:
    QString m_group;
};

QString profileInUseFilter(const QString& profile, bool used)
{
    if (used) {
        return QObject::tr("%1 (in use)").arg(profile);
    } else {
        return profile;
    }
}
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    Q_INIT_RESOURCE(beta_covers);
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    ui->centralWidget->setContentsMargins(6, 6, 6, 6);
    new VideoBeta::WindowFrame(ui->centralWidget);
    QLineEdit* instanceSearch = nullptr;
    QTabBar* instanceGroups = nullptr;
    QStackedWidget* libraryStack = nullptr;
    QWidget* emptyDashboard = nullptr;
    ui->leftSidebar->hide();
    ui->centralWidget->setStyleSheet("QWidget#centralWidget { background: #191e27; }");
    auto page = new QWidget(this);
    auto shellLayout = new QVBoxLayout(page);
    shellLayout->setContentsMargins(6, 0, 6, 6);
    shellLayout->setSpacing(8);
    shellLayout->addWidget(new VideoBeta::TitleBar(page));
    auto shellRow = new QHBoxLayout;
    shellRow->setSpacing(12);
    shellLayout->addLayout(shellRow, 1);
    auto instancesPage = new QWidget(page);
    auto root = new QVBoxLayout(instancesPage);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);
    auto toolbar = new QHBoxLayout;
    toolbar->setSpacing(14);
    auto makeButton = [this](const QString& text, const QString& iconName, QWidget* parent) {
        auto b = new QToolButton(parent);
        b->setText(text); b->setIcon(VideoBeta::icon(iconName)); b->setIconSize(QSize(21,21));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setCursor(Qt::PointingHandCursor); b->setMinimumHeight(38);
        b->setStyleSheet("QToolButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #2d3440,stop:1 #242a35); border: 1px solid #3b4350; border-radius: 7px; padding: 5px 14px; font-size: 14px; color: #dce1e9; } QToolButton:hover { background: #343f4d; border-color: #6198ab; } QToolButton:disabled { color: #788491; }");
        return b;
    };
    auto add = makeButton(BetaShell::t("Add Instance","Añadir instancia"), "add", page);
    add->setObjectName("betaAddInstanceButton"); VideoBeta::glow(add);
    connect(add, &QToolButton::clicked, ui->actionAddInstance, &QAction::trigger);
    toolbar->addWidget(add);
    auto folders = makeButton(BetaShell::t("Folders","Carpetas"), "folder", page);
    folders->setMenu(ui->foldersMenu); folders->setPopupMode(QToolButton::InstantPopup); toolbar->addWidget(folders);
    auto settings = makeButton(BetaShell::t("Settings","Ajustes"), "settings", page); toolbar->addWidget(settings);
    connect(settings, &QToolButton::clicked, this, [this] { VideoBeta::showSettings(this); });
    auto accounts = makeButton(BetaShell::t("Accounts","Cuentas"), "user", page); toolbar->addWidget(accounts);
    connect(accounts, &QToolButton::clicked, this, [this, accounts] {
        if (auto menu = ui->actionAccountsButton->menu()) menu->exec(accounts->mapToGlobal(QPoint(0,accounts->height())));
        else APPLICATION->ShowGlobalSettings(this, "accounts");
    });
    toolbar->addStretch();
    // Buscador de la barra superior (como en el vídeo); filtra las instancias con el modelo de grupos.
    instanceSearch = new QLineEdit(page);
    instanceSearch->setPlaceholderText(BetaShell::t("Search", "Buscar"));
    instanceSearch->setClearButtonEnabled(true);
    instanceSearch->setMinimumWidth(260);
    instanceSearch->setStyleSheet("QLineEdit { background: #191e27; border: 1px solid #3b4553; border-radius: 8px; padding: 7px 12px; color: #dbe3ee; }"
                                  "QLineEdit:focus { border-color: #62d0e6; }");
    toolbar->addWidget(instanceSearch);
    auto device = makeButton("", "device", page); device->setToolTip(BetaShell::t("Check for Updates", "Comprobar actualizaciones del lanzador")); toolbar->addWidget(device);
    connect(device, &QToolButton::clicked, ui->actionCheckUpdate, &QAction::trigger);
    root->addLayout(toolbar);

    auto tabRow = new QHBoxLayout;
    instanceGroups = new VideoBeta::Tabs(page);
    instanceGroups->setExpanding(false); instanceGroups->setDrawBase(false);
    for (const auto& text : {"Instances","Mods","Clients","Modded","Textures"}) instanceGroups->addTab(QString::fromLatin1(text));
    instanceGroups->setStyleSheet("QTabBar::tab { color: #cbd1da; background: #232a35; border: 1px solid #3c4654; border-top: 3px solid #657284; border-bottom: 0; border-top-left-radius: 7px; border-top-right-radius: 7px; padding: 9px 25px; margin-right: 4px; font-size: 13px; } QTabBar::tab:selected { color: #ffffff; background: #303b48; border-top-color: #75c7e6; }");
    const QList<QColor> tabColors={QColor("#d6e6ed"),QColor("#e8b176"),QColor("#91c79b"),QColor("#a2b2eb"),QColor("#dd949e")};
    for(int i=0;i<tabColors.size();++i) instanceGroups->setTabTextColor(i,tabColors[i]);
    tabRow->addWidget(instanceGroups);
    tabRow->addStretch();
    auto viewMode = makeButton("", "list", page); viewMode->setToolTip(BetaShell::t("Toggle grid / list", "Alternar cuadrícula / lista")); tabRow->addWidget(viewMode);
    root->addLayout(tabRow);
    auto body = new QHBoxLayout; body->setSpacing(0);
    auto library = new QFrame(page);
    library->setStyleSheet("QFrame { background: #242c37; border: 1px solid #394451; border-radius: 10px; }");
    auto libraryLayout = new QHBoxLayout(library); libraryLayout->setContentsMargins(0,0,0,0); libraryLayout->setSpacing(0);
    libraryStack = new QStackedWidget(library);
    emptyDashboard = new QWidget(libraryStack);
    auto welcome = new QVBoxLayout(emptyDashboard);
    welcome->setContentsMargins(32, 40, 32, 40);
    welcome->addStretch();
    auto welcomeTitle = new QLabel(BetaShell::t("Your next adventure starts here", "Tu próxima aventura empieza aquí"), emptyDashboard);
    welcomeTitle->setAlignment(Qt::AlignCenter);
    welcomeTitle->setWordWrap(true);
    welcomeTitle->setStyleSheet("color: #e6edf5; font-size: 23px; font-weight: 600; border: none; background: transparent;");
    welcome->addWidget(welcomeTitle);
    auto welcomeText = new QLabel(BetaShell::t("Create a Minecraft instance or import your modpack.\nYour games will appear here with their own cover.", "Crea una instancia de Minecraft o importa tu modpack.\nTus partidas aparecerán aquí con su propia portada."), emptyDashboard);
    welcomeText->setAlignment(Qt::AlignCenter);
    welcomeText->setWordWrap(true);
    welcomeText->setStyleSheet("color: #a4afbf; font-size: 14px; padding: 12px; border: none; background: transparent;");
    welcome->addWidget(welcomeText);
    auto createInstance = makeButton(BetaShell::t("Create my first instance", "Crear mi primera instancia"), "add", emptyDashboard);
    VideoBeta::glow(createInstance);
    connect(createInstance, &QToolButton::clicked, ui->actionAddInstance, &QAction::trigger);
    welcome->addWidget(createInstance, 0, Qt::AlignHCenter);
    welcome->addStretch();
    libraryStack->addWidget(emptyDashboard); libraryLayout->addWidget(libraryStack,1);
    // Barra de acciones inferior: botones grandes en fila y el uso de memoria a la derecha.
    auto side = new QFrame(page); side->setFixedHeight(78);
    side->setObjectName("betaActionBar");
    side->setStyleSheet("QFrame#betaActionBar { background: #242c37; border: 1px solid #394451; border-radius: 10px; }");
    auto sideLayout = new QHBoxLayout(side); sideLayout->setContentsMargins(12,10,12,10); sideLayout->setSpacing(10);
    auto launch = makeButton(BetaShell::t("Launch","Lanzar"),"play",side); launch->setObjectName("betaLaunchButton"); launch->setMinimumHeight(52); VideoBeta::glow(launch);
    connect(launch,&QToolButton::clicked,ui->actionLaunchInstance,&QAction::trigger);
    connect(ui->actionLaunchInstance,&QAction::changed,launch,[launch,this] { launch->setEnabled(ui->actionLaunchInstance->isEnabled()); });
    launch->setEnabled(false); launch->setMinimumWidth(170); sideLayout->addWidget(launch);
    auto fork = makeButton(BetaShell::t("Copy instance","Copiar"),"fork",side); sideLayout->addWidget(fork);
    connect(fork, &QToolButton::clicked, ui->actionCopyInstance, &QAction::trigger);
    connect(ui->actionCopyInstance, &QAction::changed, fork, [this, fork] { fork->setEnabled(ui->actionCopyInstance->isEnabled()); });
    fork->setEnabled(false);
    auto edit = makeButton(BetaShell::t("Edit","Editar"),"edit",side); sideLayout->addWidget(edit);
    connect(edit, &QToolButton::clicked, ui->actionEditInstance, &QAction::trigger);
    connect(ui->actionEditInstance, &QAction::changed, edit, [this, edit] { edit->setEnabled(ui->actionEditInstance->isEnabled()); });
    edit->setEnabled(false);
    auto folder = makeButton(BetaShell::t("Folder","Carpeta"),"folder",side); sideLayout->addWidget(folder); folder->setEnabled(false);
    connect(folder,&QToolButton::clicked,ui->actionViewSelectedInstFolder,&QAction::trigger);
    connect(ui->actionViewSelectedInstFolder,&QAction::changed,folder,[this,folder] { folder->setEnabled(ui->actionViewSelectedInstFolder->isEnabled()); });
    sideLayout->addStretch();
    auto ramColumn = new QVBoxLayout; ramColumn->setSpacing(4);
    auto ramLabel = new QLabel(side); ramLabel->setStyleSheet("color: #c2cbd7; font-size: 10px; border: none;"); ramColumn->addWidget(ramLabel);
    auto ramBar = new QProgressBar(side); ramBar->setTextVisible(false); ramBar->setFixedHeight(5); ramBar->setRange(0,100);
    ramBar->setStyleSheet("QProgressBar { background: #151c25; border: none; border-radius: 2px; } QProgressBar::chunk { background: #58c4e5; border-radius: 2px; }");
    ramBar->setFixedWidth(180); ramColumn->addWidget(ramBar);
    sideLayout->addLayout(ramColumn);
    auto updateRam=[ramLabel,ramBar] { const auto total=HardwareInfo::totalRamMiB(); const auto available=qMin(total,HardwareInfo::availableRamMiB()); const auto used=total-available; ramLabel->setText(QString("RAM: %1 / %2 GB").arg(used/1024.0,0,'f',1).arg(total/1024.0,0,'f',1)); ramLabel->setToolTip("Uso de memoria del sistema"); ramBar->setValue(total?int(used*100/total):0); };
    updateRam(); auto timer=new QTimer(side); connect(timer,&QTimer::timeout,side,updateRam); timer->start(3000);
    body->addWidget(library); root->addLayout(body,1);
    root->addWidget(side);

    // Pie de la biblioteca como en el vídeo: carpetas de color a la izquierda y contador a la derecha.
    auto footer = new QHBoxLayout;
    footer->setSpacing(8);
    const QList<QString> footerColors = { "#354757", "#634b2b", "#2d583a", "#38446b", "#63353e" };
    const QStringList footerGroups = APPLICATION->instances()->getGroups();
    for (int i = 0; i < footerGroups.size() && i < footerColors.size(); ++i) {
        auto chip = new QLabel(footerGroups[i].isEmpty() ? BetaShell::t("Ungrouped", "Sin agrupar") : footerGroups[i], page);
        chip->setStyleSheet(QStringLiteral("QLabel { background: %1; color: #e6edf5; border-radius: 6px; padding: 4px 12px; font-size: 12px; }").arg(footerColors[i]));
        footer->addWidget(chip);
    }
    footer->addStretch();
    auto footerCount = new QLabel(page);
    footerCount->setStyleSheet("color: #a4afbf; font-size: 12px; background: transparent; border: none;");
    footer->addWidget(footerCount);
    auto updateFooter = [footerCount] {
        footerCount->setText(BetaShell::t("Instances: %1", "Instancias: %1").arg(APPLICATION->instances()->count()));
    };
    updateFooter();
    connect(APPLICATION->instances(), &InstanceList::dataIsInvalid, footerCount, updateFooter);
    root->addLayout(footer);

    // Barra lateral y páginas del rediseño (Instancias, Mis Mods, Interfaz, Temas y Colores, Temas, Gatos, Ajustes).
    auto navigation = new QFrame(page);
    navigation->setObjectName("betaSideNavigation");
    navigation->setFixedWidth(206);
    navigation->setStyleSheet("QFrame#betaSideNavigation { background: #202733; border: 1px solid #364351; border-radius: 12px; }");
    auto navLayout = new QVBoxLayout(navigation);
    navLayout->setContentsMargins(10, 14, 10, 12);
    navLayout->setSpacing(6);
    auto navBrand = new QLabel(QStringLiteral("Prism Launcher"), navigation);
    navBrand->setStyleSheet("font-size: 17px; font-weight: 600; color: #f0f4f9; padding: 4px 8px 12px; border: none; background: transparent;");
    navLayout->addWidget(navBrand);
    auto pages = new QStackedWidget(page);
    pages->addWidget(instancesPage);
    pages->addWidget(BetaShell::shortcutsPage(
        pages, BetaShell::t("Mods", "Mis Mods"),
        BetaShell::t("Mods are managed per instance: select one under Instances and open its editor.", "Los mods se gestionan por instancia: selecciona una en Instancias y abre su editor."),
        { { BetaShell::t("Edit instance", "Editar mods de la instancia"), [this] { ui->actionEditInstance->trigger(); } },
          { BetaShell::t("Add Instance", "Añadir instancia"), [this] { ui->actionAddInstance->trigger(); } },
          { BetaShell::t("View Central Mods Folder", "Abrir carpeta de mods central"), [this] { ui->actionViewCentralModsFolder->trigger(); } } }));
    pages->addWidget(BetaShell::shortcutsPage(
        pages, BetaShell::t("Interface settings", "Configuración de Interfaz"), BetaShell::t("Shortcuts to the launcher settings.", "Accesos directos a los ajustes del lanzador."),
        { { BetaShell::t("General", "General"), [this] { APPLICATION->ShowGlobalSettings(this, "launcher-settings"); } },
          { BetaShell::t("Language", "Idioma"), [this] { APPLICATION->ShowGlobalSettings(this, "language-settings"); } },
          { BetaShell::t("Appearance", "Apariencia"), [this] { APPLICATION->ShowGlobalSettings(this, "appearance-settings"); } },
          { QStringLiteral("Minecraft"), [this] { APPLICATION->ShowGlobalSettings(this, "minecraft-settings"); } },
          { BetaShell::t("Accounts", "Cuentas"), [this] { APPLICATION->ShowGlobalSettings(this, "accounts"); } },
          { QStringLiteral("Proxy"), [this] { APPLICATION->ShowGlobalSettings(this, "proxy-settings"); } } }));
    pages->addWidget(BetaShell::colorsPage(pages));
    pages->addWidget(BetaShell::themesPage(pages));
    pages->addWidget(BetaShell::catPacksPage(pages));
    pages->addWidget(BetaSkins::page(pages));
    auto navGroup = new QButtonGroup(navigation);
    navGroup->setExclusive(true);
    const QList<QPair<QString, int>> navItems = { { BetaShell::t("Instances", "Instancias"), 0 },
                                                  { BetaShell::t("Mods", "Mis Mods"), 1 },
                                                  { BetaShell::t("Interface settings", "Configuración de Interfaz"), 2 },
                                                  { BetaShell::t("Themes and colors", "Temas y Colores"), 3 },
                                                  { BetaShell::t("Themes", "Temas"), 4 },
                                                  { BetaShell::t("Cat packs", "Gatos (CatPacks)"), 5 },
                                                  { BetaShell::t("Skins", "Skins"), 6 } };
    const QStringList navIcons = { "folder", "fork", "list", "edit", "device", "user", "play" };  // icono de cada sección en modo plegado
    for (const auto& item : navItems) {
        auto button = new QPushButton(navigation);
        button->setText(item.first);
        button->setProperty("betaIconKind", navIcons.value(item.second));
        button->setCheckable(true);
        button->setChecked(item.second == 0);
        button->setCursor(Qt::PointingHandCursor);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setMinimumHeight(40);
        button->setStyleSheet("QPushButton { text-align: left; padding: 8px 12px; border: 1px solid transparent; border-radius: 9px; color: #cfd8e4; font-size: 13px; background: transparent; }"
                              "QPushButton:hover { background: #2a3543; }"
                              "QPushButton:checked { color: #ffffff; border: 1px solid #5ab8d6; background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #2f6f84,stop:1 #4f4a8f); }");
        navGroup->addButton(button, item.second);
        navLayout->addWidget(button);
        connect(button, &QToolButton::clicked, pages, [pages, index = item.second] { pages->setCurrentIndex(index); });
    }
    auto navSettings = new QPushButton(navigation);
    navSettings->setText(BetaShell::t("Settings", "Ajustes"));
    navSettings->setProperty("betaIconKind", QStringLiteral("settings"));
    navSettings->setCursor(Qt::PointingHandCursor);
    navSettings->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    navSettings->setMinimumHeight(40);
    navSettings->setStyleSheet("QPushButton { text-align: left; padding: 8px 12px; border: 1px solid transparent; border-radius: 9px; color: #cfd8e4; font-size: 13px; background: transparent; } QPushButton:hover { background: #2a3543; }");
    connect(navSettings, &QToolButton::clicked, this, [this] { VideoBeta::showSettings(this); });
    navLayout->addWidget(navSettings);
    navLayout->addStretch();
    // Novedades y actualizaciones: reutilizan las acciones que ya existen en el menú de ayuda.
    const QList<std::tuple<QString, QString, QAction*>> appLinks = {
        { BetaShell::t("News", "Novedades"), QStringLiteral("list"), ui->actionMoreNews },
        { BetaShell::t("Check for updates", "Actualizaciones"), QStringLiteral("device"), ui->actionCheckUpdate },
    };
    for (const auto& [linkText, linkIcon, linkAction] : appLinks) {
        QAction* action = linkAction;  // las lambdas no capturan enlaces estructurados en C++17
        auto link = new QPushButton(navigation);
        link->setText(linkText);
        link->setProperty("betaIconKind", linkIcon);
        link->setCursor(Qt::PointingHandCursor);
        link->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        link->setMinimumHeight(40);
        link->setStyleSheet("QPushButton { text-align: left; padding: 8px 12px; border: 1px solid transparent; border-radius: 9px; color: #cfd8e4; font-size: 13px; background: transparent; } QPushButton:hover { background: #2a3543; }");
        link->setEnabled(action->isEnabled());
        link->setVisible(action->isVisible());
        connect(action, &QAction::changed, link, [link, action] {
            link->setEnabled(action->isEnabled());
            link->setVisible(action->isVisible());
        });
        connect(link, &QPushButton::clicked, action, &QAction::trigger);
        navLayout->addWidget(link);
    }
    // Plegado de la barra lateral: botón « y plegado automático en ventanas estrechas.
    auto navToggle = new QToolButton(navigation);
    navToggle->setText(QString::fromUtf8("«"));
    navToggle->setCursor(Qt::PointingHandCursor);
    navToggle->setToolTip(BetaShell::t("Collapse sidebar", "Plegar barra lateral"));
    navToggle->setStyleSheet("QToolButton { border: 1px solid #3b4553; border-radius: 8px; color: #cfd8e4; background: #262d39; padding: 4px 10px; } QToolButton:hover { background: #2e3948; }");
    navLayout->insertWidget(0, navToggle, 0, Qt::AlignRight);
    auto navButtons = navigation->findChildren<QAbstractButton*>();
    auto setCollapsed = [navigation, navBrand, navToggle, navButtons](bool collapsed) {
        navigation->setFixedWidth(collapsed ? 66 : 206);
        navBrand->setVisible(!collapsed);
        navToggle->setText(QString::fromUtf8(collapsed ? "»" : "«"));
        for (auto button : navButtons) {
            if (button == navToggle) continue;
            if (button->property("betaFullText").isNull()) button->setProperty("betaFullText", button->text());
            const QString full = button->property("betaFullText").toString();
            button->setToolTip(full);
            const QString kind = button->property("betaIconKind").toString();
            if (collapsed && !kind.isEmpty()) {
                button->setText(QString());
                button->setIcon(VideoBeta::icon(kind));
                button->setIconSize(QSize(22, 22));
                button->setStyleSheet(button->styleSheet() + " QPushButton { text-align: center; padding: 8px 0; }");
            } else {
                button->setText(full);
                button->setIcon(QIcon());
                button->setStyleSheet(button->styleSheet().replace(" QPushButton { text-align: center; padding: 8px 0; }", QString()));
            }
        }
    };
    connect(navToggle, &QToolButton::clicked, navigation, [navigation, setCollapsed] { setCollapsed(navigation->width() > 100); });
    page->installEventFilter(new BetaShell::AutoCollapse(page, [setCollapsed, navigation](int width) {
        const bool narrow = width < 1180;
        if (narrow != (navigation->width() < 100)) setCollapsed(narrow);
    }));
    shellRow->addWidget(navigation);
    shellRow->addWidget(pages, 1);
    ui->mainContent->addWidget(page); ui->mainContent->setCurrentWidget(page);
    ui->mainContent->setMinimumWidth(980);
    setWindowIcon(APPLICATION->logo());
    setWindowTitle(APPLICATION->applicationDisplayName());
#ifndef QT_NO_ACCESSIBILITY
    setAccessibleName(BuildConfig.LAUNCHER_DISPLAYNAME);
#endif

    // instance toolbar stuff
    {
        // Qt doesn't like vertical moving toolbars, so we have to force them...
        // See https://github.com/PolyMC/PolyMC/issues/493
        connect(ui->instanceToolBar, &QToolBar::orientationChanged,
                [this](Qt::Orientation) { ui->instanceToolBar->setOrientation(Qt::Vertical); });

        // if you try to add a widget to a toolbar in a .ui file
        // qt designer will delete it when you save the file >:(
        changeIconButton = new LabeledToolButton(this);
        changeIconButton->setObjectName(QStringLiteral("changeIconButton"));
        changeIconButton->setIcon(QIcon::fromTheme("news"));
        changeIconButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(changeIconButton, &QToolButton::clicked, this, &MainWindow::on_actionChangeInstIcon_triggered);
        ui->instanceToolBar->insertWidgetBefore(ui->actionLaunchInstance, changeIconButton);

        renameButton = new LabeledToolButton(this);
        renameButton->setObjectName(QStringLiteral("renameButton"));
        renameButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(renameButton, &QToolButton::clicked, this, &MainWindow::on_actionRenameInstance_triggered);
        ui->instanceToolBar->insertWidgetBefore(ui->actionLaunchInstance, renameButton);

        ui->instanceToolBar->insertSeparator(ui->actionLaunchInstance);

        // restore the instance toolbar settings
        const auto setting_name = QString("WideBarVisibility_%1").arg(ui->instanceToolBar->objectName());
        instanceToolbarSetting = APPLICATION->settings()->getOrRegisterSetting(setting_name);

        ui->instanceToolBar->setVisibilityState(QByteArray::fromBase64(instanceToolbarSetting->get().toString().toUtf8()));

        ui->instanceToolBar->addContextMenuAction(ui->newsToolBar->toggleViewAction());
        ui->instanceToolBar->addContextMenuAction(ui->instanceToolBar->toggleViewAction());
        ui->instanceToolBar->addContextMenuAction(ui->actionToggleStatusBar);
        ui->instanceToolBar->addContextMenuAction(ui->actionLockToolbars);
    }

    // set the menu for the folders help, accounts, and export tool buttons
    {
        auto foldersMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionFoldersButton));
        ui->actionFoldersButton->setMenu(ui->foldersMenu);
        foldersMenuButton->setPopupMode(QToolButton::InstantPopup);

        helpMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionHelpButton));
        ui->actionHelpButton->setMenu(new QMenu(this));
        ui->actionHelpButton->menu()->addActions(ui->helpMenu->actions());
        ui->actionHelpButton->menu()->removeAction(ui->actionCheckUpdate);
        helpMenuButton->setPopupMode(QToolButton::InstantPopup);

        auto accountMenuButton = dynamic_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionAccountsButton));
        accountMenuButton->setPopupMode(QToolButton::InstantPopup);

        auto exportInstanceMenu = new QMenu(this);
        exportInstanceMenu->addAction(ui->actionExportInstanceZip);
        exportInstanceMenu->addAction(ui->actionExportInstanceMrPack);
        exportInstanceMenu->addAction(ui->actionExportInstanceFlamePack);
        ui->actionExportInstance->setMenu(exportInstanceMenu);
    }

    // hide, disable and show stuff
    {
        ui->actionReportBug->setVisible(!BuildConfig.BUG_TRACKER_URL.isEmpty());
        ui->actionMATRIX->setVisible(!BuildConfig.MATRIX_URL.isEmpty());
        ui->actionDISCORD->setVisible(!BuildConfig.DISCORD_URL.isEmpty());
        ui->actionREDDIT->setVisible(!BuildConfig.SUBREDDIT_URL.isEmpty());

        ui->actionCheckUpdate->setVisible(APPLICATION->updaterEnabled());

#ifndef Q_OS_MAC
        ui->actionAddToPATH->setVisible(false);
#endif

        // disabled until we have an instance selected
        ui->instanceToolBar->setEnabled(false);
        setInstanceActionsEnabled(false);

        // add a close button at the end of the main toolbar when running on gamescope / steam deck
        // this is only needed on gamescope because it defaults to an X11/XWayland session and
        // does not implement decorations
        if (qgetenv("XDG_CURRENT_DESKTOP") == "gamescope") {
            ui->mainToolBar->addAction(ui->actionCloseWindow);
        }

        ui->actionViewJavaFolder->setEnabled(BuildConfig.JAVA_DOWNLOADER_ENABLED);
    }

    {  // logs viewing
        connect(ui->actionViewLog, &QAction::triggered, this, [] { APPLICATION->showLogWindow(); });
    }

    // add the toolbar toggles to the view menu
    ui->viewMenu->addAction(ui->instanceToolBar->toggleViewAction());
    ui->viewMenu->addAction(ui->newsToolBar->toggleViewAction());

    updateThemeMenu();
    updateMainToolBar();
    if (auto addButton = qobject_cast<QToolButton*>(ui->mainToolBar->widgetForAction(ui->actionAddInstance))) {
        addButton->setStyleSheet(QStringLiteral("QToolButton { color: #67e8f9; border: 1px solid rgba(34,211,238,0.55); background: rgba(34,211,238,0.12); }"));
    }
    if (auto launchButton = qobject_cast<QToolButton*>(ui->instanceToolBar->widgetForAction(ui->actionLaunchInstance))) {
        launchButton->setStyleSheet(QStringLiteral("QToolButton { color: #67e8f9; border: 1px solid rgba(34,211,238,0.55); background: rgba(34,211,238,0.12); }"));
    }
    // OSX magic.
    setUnifiedTitleAndToolBarOnMac(true);

    // Global shortcuts
    {
        // you can't set QKeySequence::StandardKey shortcuts in qt designer >:(
        ui->actionAddInstance->setShortcut(QKeySequence::New);
        ui->actionSettings->setShortcut(QKeySequence::Preferences);
        ui->actionUndoTrashInstance->setShortcut(QKeySequence::Undo);
        ui->actionDeleteInstance->setShortcuts({ QKeySequence(tr("Backspace")), QKeySequence::Delete });
        ui->actionCloseWindow->setShortcut(QKeySequence::Close);
        connect(ui->actionCloseWindow, &QAction::triggered, APPLICATION, &Application::closeCurrentWindow);

        // FIXME: This is kinda weird. and bad. We need some kind of managed shutdown.
        auto q = new QShortcut(QKeySequence::Quit, this);
        connect(q, &QShortcut::activated, APPLICATION, &Application::quit);
    }

    // Konami Code
    {
        secretEventFilter = new KonamiCode(this);
        connect(secretEventFilter, &KonamiCode::triggered, this, &MainWindow::konamiTriggered);
    }

    // Add the news label to the news toolbar.
    {
        m_newsChecker.reset(new NewsChecker(APPLICATION->network(), BuildConfig.NEWS_RSS_URL));
        newsLabel = new QToolButton();
        newsLabel->setIcon(QIcon::fromTheme("news"));
        newsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        newsLabel->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        newsLabel->setFocusPolicy(Qt::NoFocus);
        ui->newsToolBar->insertWidget(ui->actionMoreNews, newsLabel);

        connect(newsLabel, &QAbstractButton::clicked, this, &MainWindow::newsButtonClicked);
        connect(m_newsChecker.get(), &NewsChecker::newsLoaded, this, &MainWindow::updateNewsLabel);
        updateNewsLabel();
    }

    // Create the instance list widget
    {
        view = new InstanceView(ui->centralWidget);

        view->setSelectionMode(QAbstractItemView::SingleSelection);
        // FIXME: leaks ListViewDelegate
        auto delegate = new ListViewDelegate(this);
        view->setItemDelegate(delegate);
        view->setFrameShape(QFrame::NoFrame);
        // do not show ugly blue border on the mac
        view->setAttribute(Qt::WA_MacShowFocusRect, false);
        connect(delegate, &ListViewDelegate::textChanged, this, [this](QString before, QString after) {
            if (auto newRoot = askToUpdateInstanceDirName(m_selectedInstance, before, after, this); !newRoot.isEmpty()) {
                auto oldID = m_selectedInstance->id();
                auto newID = QFileInfo(newRoot).fileName();
                QString origGroup(APPLICATION->instances()->getInstanceGroup(oldID));
                bool syncGroup = origGroup != GroupId() && oldID != newID;
                if (syncGroup)
                    APPLICATION->instances()->setInstanceGroup(oldID, GroupId());

                refreshInstances();
                setSelectedInstanceById(newID);

                if (syncGroup)
                    APPLICATION->instances()->setInstanceGroup(newID, origGroup);
            }
        });

        view->installEventFilter(this);
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, this, &MainWindow::showInstanceContextMenu);
        connect(view, &InstanceView::droppedURLs, this, &MainWindow::processURLs, Qt::QueuedConnection);

        auto groupModel = new InstanceGroupFilterModel(this);
        proxymodel = groupModel;
        proxymodel->setSourceModel(APPLICATION->instances());
        proxymodel->sort(0);
        if (instanceSearch) {
            connect(instanceSearch, &QLineEdit::textChanged, groupModel, [groupModel](const QString& text) {
                groupModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
                groupModel->setFilterFixedString(text);
            });
        }
        if (instanceGroups) {
            const auto groups = APPLICATION->instances()->getGroups();
            for (const auto& group : groups) {
                bool exists = false;
                for (int i = 0; i < instanceGroups->count(); ++i)
                    exists |= instanceGroups->tabText(i) == group;
                if (!exists && !group.isEmpty()) instanceGroups->addTab(group);
            }
            connect(APPLICATION->instances(), &InstanceList::groupsChanged, instanceGroups,
                    [instanceGroups](const QSet<QString>& updatedGroups) {
                const QString selected = instanceGroups->currentIndex() > 0 ? instanceGroups->tabText(instanceGroups->currentIndex()) : QString();
                while (instanceGroups->count() > 5) instanceGroups->removeTab(5);
                QStringList sorted = updatedGroups.values();
                sorted.sort(Qt::CaseInsensitive);
                for (const auto& group : sorted) {
                    bool exists = group.isEmpty();
                    for (int i = 0; i < instanceGroups->count(); ++i) exists |= instanceGroups->tabText(i) == group;
                    if (!exists) instanceGroups->addTab(group);
                }
                for (int i = 1; i < instanceGroups->count(); ++i)
                    if (instanceGroups->tabText(i) == selected) instanceGroups->setCurrentIndex(i);
            });
            connect(instanceGroups, &QTabBar::currentChanged, groupModel, [groupModel, instanceGroups](int index) {
                groupModel->setGroupFilter(index <= 0 ? QString() : instanceGroups->tabText(index));
            });
        }
        connect(proxymodel, &InstanceProxyModel::dataChanged, this, &MainWindow::instanceDataChanged);

        view->setModel(proxymodel);
        view->setSourceOfGroupCollapseStatus(
            [](const QString& groupName) -> bool { return APPLICATION->instances()->isGroupCollapsed(groupName); });
        connect(view, &InstanceView::groupStateChanged, APPLICATION->instances(), &InstanceList::on_GroupStateChanged);
        view->setStyleSheet(QStringLiteral("background: #171a21; border: 1px solid rgba(255,255,255,0.06); border-radius: 12px;"));
        libraryStack->addWidget(view);
        viewMode->setCheckable(true);
        connect(viewMode, &QToolButton::toggled, view, [this](bool list) { view->setCardListMode(list); });
        libraryStack->setCurrentWidget(APPLICATION->instances()->rowCount() == 0 ? emptyDashboard : view);
        auto updateLibraryPage = [this, libraryStack, emptyDashboard] {
            libraryStack->setCurrentWidget(APPLICATION->instances()->rowCount() == 0 ? emptyDashboard : view);
        };
        connect(APPLICATION->instances(), &QAbstractItemModel::rowsInserted, this,
                [updateLibraryPage](const QModelIndex&, int, int) { updateLibraryPage(); });
        connect(APPLICATION->instances(), &QAbstractItemModel::rowsRemoved, this,
                [updateLibraryPage](const QModelIndex&, int, int) { updateLibraryPage(); });
        connect(APPLICATION->instances(), &QAbstractItemModel::modelReset, this, updateLibraryPage);
    }
    // The cat background
    {
        // set the cat action priority here so you can still see the action in qt designer
        ui->actionCAT->setPriority(QAction::LowPriority);
        bool cat_enable = APPLICATION->settings()->get("TheCat").toBool();
        ui->actionCAT->setChecked(cat_enable);
        connect(ui->actionCAT, &QAction::toggled, this, &MainWindow::onCatToggled);
        connect(APPLICATION, &Application::currentCatChanged, this, &MainWindow::onCatChanged);
        setCatBackground(cat_enable);
    }

    // Togglable status bar
    {
        bool statusBarVisible = APPLICATION->settings()->get("StatusBarVisible").toBool();
        ui->actionToggleStatusBar->setChecked(statusBarVisible);
        connect(ui->actionToggleStatusBar, &QAction::toggled, this, &MainWindow::setStatusBarVisibility);
        setStatusBarVisibility(statusBarVisible);
    }

    // Lock toolbars
    {
        bool toolbarsLocked = APPLICATION->settings()->get("ToolbarsLocked").toBool();
        ui->actionLockToolbars->setChecked(toolbarsLocked);
        connect(ui->actionLockToolbars, &QAction::toggled, this, &MainWindow::lockToolbars);
        lockToolbars(toolbarsLocked);
    }
    // start instance when double-clicked
    connect(view, &InstanceView::activated, this, &MainWindow::instanceActivated);

    // track the selection -- update the instance toolbar
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::instanceChanged);

    // track icon changes and update the toolbar!
    connect(APPLICATION->icons(), &IconList::iconUpdated, this, &MainWindow::iconUpdated);

    // model reset -> selection is invalid. All the instance pointers are wrong.
    connect(APPLICATION->instances(), &InstanceList::dataIsInvalid, this, &MainWindow::selectionBad);

    // handle newly added instances
    connect(APPLICATION->instances(), &InstanceList::instanceSelectRequest, this, &MainWindow::instanceSelectRequest);

    // When the global settings page closes, we want to know about it and update our state
    connect(APPLICATION, &Application::globalSettingsApplied, this, &MainWindow::globalSettingsClosed);

    m_statusLeft = new QLabel(tr("No instance selected"), this);
    m_statusCenter = new QLabel(tr("Total playtime: 0s"), this);
    statusBar()->addPermanentWidget(m_statusLeft, 1);
    statusBar()->addPermanentWidget(m_statusCenter, 0);

    // Add "manage accounts" button, right align
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainToolBar->insertWidget(ui->actionAccountsButton, spacer);

    // Use undocumented property... https://stackoverflow.com/questions/7121718/create-a-scrollbar-in-a-submenu-qt
    ui->accountsMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");

    repopulateAccountsMenu();

    // Update the menu when the active account changes.
    // Shouldn't have to use lambdas here like this, but if I don't, the compiler throws a fit.
    // Template hell sucks...
    connect(APPLICATION->accounts(), &AccountList::defaultAccountChanged, [this] { defaultAccountChanged(); });
    connect(APPLICATION->accounts(), &AccountList::listActivityChanged, [this] { defaultAccountChanged(); });
    connect(APPLICATION->accounts(), &AccountList::listChanged, [this] { defaultAccountChanged(); });

    // Show initial account
    defaultAccountChanged();

    // TODO: refresh accounts here?
    // auto accounts = APPLICATION->accounts();

    // load the news
    {
        m_newsChecker->reloadNews();
        updateNewsLabel();
    }

    if (APPLICATION->updaterEnabled()) {
        bool updatesAllowed = APPLICATION->updatesAreAllowed();
        updatesAllowedChanged(updatesAllowed);

        connect(ui->actionCheckUpdate, &QAction::triggered, this, &MainWindow::checkForUpdates);

        // set up the updater object.
        auto updater = APPLICATION->updater();

        if (updater) {
            connect(updater, &ExternalUpdater::canCheckForUpdatesChanged, this, &MainWindow::updatesAllowedChanged);
        }
    }

    connect(ui->actionUndoTrashInstance, &QAction::triggered, this, &MainWindow::undoTrashInstance);

    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());

    // removing this looks stupid
    view->setFocus();

    retranslateUi();
    ui->btnInstances->setText(BetaShell::t("Instances", "Instancias"));
    ui->btnSettings->setText(BetaShell::t("Settings", "Ajustes"));
    ui->actionAddInstance->setText(QStringLiteral("Add Instance"));
    ui->actionChangeInstGroup->setText(BetaShell::t("New Folder", "Nueva carpeta"));
    ui->actionLaunchInstance->setText(BetaShell::t("Launch", "Lanzar"));
    ui->actionAccountsButton->setText(QString());
    if (auto button = findChild<QToolButton*>(QStringLiteral("betaAddInstanceButton")))
        button->setText(QStringLiteral("Add Instance"));
    if (auto button = findChild<QToolButton*>(QStringLiteral("betaNewFolderButton")))
        button->setText(BetaShell::t("New Folder", "Nueva carpeta"));
    if (auto button = findChild<QToolButton*>(QStringLiteral("betaLaunchButton")))
        button->setText(BetaShell::t("Launch", "Lanzar"));
    setWindowTitle(QStringLiteral("Prism Launcher"));
    ui->mainToolBar->hide();
    ui->instanceToolBar->hide();
    ui->newsToolBar->hide();
    ui->menuBar->hide();
    ui->statusBar->hide();
}

// macOS always has a native menu bar, so these fixes are not applicable
// Other systems may or may not have a native menu bar (most do not - it seems like only Ubuntu Unity does)
#ifndef Q_OS_MAC
void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Alt && !APPLICATION->settings()->get("MenuBarInsteadOfToolBar").toBool())
        ui->menuBar->setVisible(!ui->menuBar->isVisible());
    else
        QMainWindow::keyReleaseEvent(event);
}
#endif

void MainWindow::retranslateUi()
{
    if (m_selectedInstance) {
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
    } else {
        m_statusLeft->setText(tr("No instance selected"));
    }

    ui->retranslateUi(this);
    ui->btnInstances->setText(BetaShell::t("Instances", "Instancias"));
    ui->btnAddInstance->setText(BetaShell::t("Add Instance", "Añadir instancia"));
    ui->btnSettings->setText(BetaShell::t("Settings", "Ajustes"));

    MinecraftAccountPtr defaultAccount = APPLICATION->accounts()->defaultAccount();
    if (defaultAccount) {
        auto profileLabel = profileInUseFilter(defaultAccount->displayName(), defaultAccount->isInUse());
        ui->actionAccountsButton->setText(profileLabel);
    }

    changeIconButton->setToolTip(ui->actionChangeInstIcon->toolTip());
    renameButton->setToolTip(ui->actionRenameInstance->toolTip());

    // replace the %1 with the launcher display name in some actions
    if (helpMenuButton->toolTip().contains("%1"))
        helpMenuButton->setToolTip(helpMenuButton->toolTip().arg(BuildConfig.LAUNCHER_DISPLAYNAME));

    for (auto action : ui->helpMenu->actions()) {
        if (action->text().contains("%1"))
            action->setText(action->text().arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        if (action->toolTip().contains("%1"))
            action->setToolTip(action->toolTip().arg(BuildConfig.LAUNCHER_DISPLAYNAME));
    }
}

MainWindow::~MainWindow() {}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    // Application restores translations/title from settings after construction;
    // reapply the video labels to the custom toolbar once the window is visible.
    ui->actionAddInstance->setText(QStringLiteral("Add Instance"));
    ui->actionChangeInstGroup->setText(BetaShell::t("New Folder", "Nueva carpeta"));
    ui->actionLaunchInstance->setText(BetaShell::t("Launch", "Lanzar"));
    ui->actionAccountsButton->setText(QString());
    setWindowTitle(QStringLiteral("Prism Launcher"));
    if (auto button = findChild<QToolButton*>(QStringLiteral("betaAddInstanceButton")))
        button->setText(QStringLiteral("Add Instance"));
    if (auto button = findChild<QToolButton*>(QStringLiteral("betaNewFolderButton")))
        button->setText(BetaShell::t("New Folder", "Nueva carpeta"));
    if (auto button = findChild<QToolButton*>(QStringLiteral("betaLaunchButton")))
        button->setText(BetaShell::t("Launch", "Lanzar"));
    // Application restores the saved QMainWindow state after construction. Keep
    // the legacy toolbars hidden so the custom Prism Beta layout stays in place.
    ui->mainToolBar->hide();
    ui->instanceToolBar->hide();
    ui->newsToolBar->hide();
    ui->menuBar->hide();
    ui->statusBar->hide();
    if (m_betaFirstShow) {
        m_betaFirstShow = false;
        const auto available = QGuiApplication::primaryScreen()->availableGeometry();
        const QSize target(qMin(1080, available.width() - 60), qMin(650, available.height() - 80));
        resize(target.expandedTo(QSize(800, 500)));
        move(available.center() - rect().center());
    }
}

QMenu* MainWindow::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->mainToolBar->toggleViewAction());

    filteredMenu->addAction(ui->actionToggleStatusBar);
    filteredMenu->addAction(ui->actionLockToolbars);

    return filteredMenu;
}
void MainWindow::setStatusBarVisibility(bool state)
{
    statusBar()->setVisible(state);
    APPLICATION->settings()->set("StatusBarVisible", state);
}
void MainWindow::lockToolbars(bool state)
{
    ui->mainToolBar->setMovable(!state);
    ui->instanceToolBar->setMovable(!state);
    ui->newsToolBar->setMovable(!state);
    APPLICATION->settings()->set("ToolbarsLocked", state);
}

void MainWindow::konamiTriggered()
{
    QString gradient =
        " stop:0 rgba(125, 0, 0, 255), stop:0.166 rgba(125, 125, 0, 255), stop:0.333 rgba(0, 125, 0, 255), stop:0.5 rgba(0, 125, 125, "
        "255), stop:0.666 rgba(0, 0, 125, 255), stop:0.833 rgba(125, 0, 125, 255), stop:1 rgba(125, 0, 0, 255));";
    QString stylesheet = "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0," + gradient;
    if (ui->mainToolBar->styleSheet() == stylesheet) {
        ui->mainToolBar->setStyleSheet("");
        ui->instanceToolBar->setStyleSheet("");
        ui->centralWidget->setStyleSheet("");
        ui->newsToolBar->setStyleSheet("");
        ui->statusBar->setStyleSheet("");
        qDebug() << "Super Secret Mode DEACTIVATED!";
    } else {
        ui->mainToolBar->setStyleSheet(stylesheet);
        ui->instanceToolBar->setStyleSheet("background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1," + gradient);
        ui->centralWidget->setStyleSheet("background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1," + gradient);
        ui->newsToolBar->setStyleSheet(stylesheet);
        ui->statusBar->setStyleSheet(stylesheet);
        qDebug() << "Super Secret Mode ACTIVATED!";
    }
}

void MainWindow::showInstanceContextMenu(const QPoint& pos)
{
    QList<QAction*> actions;

    QAction* actionSep = new QAction("", this);
    actionSep->setSeparator(true);

    bool onInstance = view->indexAt(pos).isValid();
    if (onInstance) {
        // reuse the file menu actions
        actions = ui->fileMenu->actions();

        // remove the add instance action, launcher settings action and close action
        actions.removeFirst();
        actions.removeLast();
        actions.removeLast();

        actions.prepend(ui->actionChangeInstIcon);
        actions.prepend(ui->actionRenameInstance);

        // add header
        actions.prepend(actionSep);
        QAction* actionVoid = new QAction(m_selectedInstance->name(), this);
        actionVoid->setEnabled(false);
        actions.prepend(actionVoid);
    } else {
        auto group = view->groupNameAt(pos);

        QAction* actionVoid = new QAction(group.isNull() ? BuildConfig.LAUNCHER_DISPLAYNAME : group, this);
        actionVoid->setEnabled(false);

        QAction* actionCreateInstance = new QAction(tr("&Create instance"), this);
        actionCreateInstance->setToolTip(ui->actionAddInstance->toolTip());
        if (!group.isNull()) {
            QVariantMap instance_action_data;
            instance_action_data["group"] = group;
            actionCreateInstance->setData(instance_action_data);
        }

        connect(actionCreateInstance, &QAction::triggered, this, &MainWindow::on_actionAddInstance_triggered);

        actions.prepend(actionSep);
        actions.prepend(actionVoid);
        actions.append(actionCreateInstance);
        if (!group.isNull()) {
            QAction* actionDeleteGroup = new QAction(tr("&Delete group"), this);
            connect(actionDeleteGroup, &QAction::triggered, this, [this, group] { deleteGroup(group); });
            actions.append(actionDeleteGroup);

            QAction* actionRenameGroup = new QAction(tr("&Rename group"), this);
            connect(actionRenameGroup, &QAction::triggered, this, [this, group] { renameGroup(group); });
            actions.append(actionRenameGroup);
        }
    }
    QMenu myMenu;
    myMenu.addActions(actions);
    if (onInstance && m_selectedInstance) {
        myMenu.addSeparator();
        auto coverAction = myMenu.addAction(QStringLiteral("Cambiar portada de la tarjeta…"));
        connect(coverAction, &QAction::triggered, this, [this] {
            if (!m_selectedInstance) return;
            const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("Portada de la instancia"), QString(), QStringLiteral("Imágenes (*.png *.jpg *.jpeg *.webp)"));
            if (path.isEmpty()) return;
            QImage image(path);
            if (image.isNull() || !image.save(m_selectedInstance->instanceRoot() + QStringLiteral("/cover.png"))) {
                QMessageBox::warning(this, QStringLiteral("Portada"), QStringLiteral("No se pudo guardar la imagen de portada."));
                return;
            }
            view->viewport()->update();
        });
    }
    /*
    if (onInstance)
        myMenu.setEnabled(m_selectedInstance->canLaunch());
    */
    myMenu.exec(view->mapToGlobal(pos));
}

void MainWindow::updateMainToolBar()
{
    ui->menuBar->hide();
    ui->mainToolBar->hide();
}

void MainWindow::updateLaunchButton()
{
    QMenu* launchMenu = ui->actionLaunchInstance->menu();
    if (launchMenu)
        launchMenu->clear();
    else
        launchMenu = new QMenu(this);
    if (m_selectedInstance)
        m_selectedInstance->populateLaunchMenu(launchMenu);
    ui->actionLaunchInstance->setMenu(launchMenu);
}

void MainWindow::updateThemeMenu()
{
    QMenu* themeMenu = ui->actionChangeTheme->menu();

    if (themeMenu) {
        themeMenu->clear();
    } else {
        themeMenu = new QMenu(this);
    }

    auto themes = APPLICATION->themeManager()->getValidApplicationThemes();

    QActionGroup* themesGroup = new QActionGroup(this);

    for (auto* theme : themes) {
        QAction* themeAction = themeMenu->addAction(theme->name());

        themeAction->setCheckable(true);
        if (APPLICATION->settings()->get("ApplicationTheme").toString() == theme->id()) {
            themeAction->setChecked(true);
        }
        themeAction->setActionGroup(themesGroup);

        connect(themeAction, &QAction::triggered, [theme]() {
            APPLICATION->themeManager()->setApplicationTheme(theme->id());
            APPLICATION->settings()->set("ApplicationTheme", theme->id());
        });
    }

    ui->actionChangeTheme->setMenu(themeMenu);
}

void MainWindow::repopulateAccountsMenu()
{
    ui->accountsMenu->clear();

    // NOTE: this is done so the accounts button text is not set to the accounts menu title
    QMenu* accountsButtonMenu = ui->actionAccountsButton->menu();
    if (accountsButtonMenu) {
        accountsButtonMenu->clear();
    } else {
        accountsButtonMenu = new QMenu(this);
        ui->actionAccountsButton->setMenu(accountsButtonMenu);
    }

    auto accounts = APPLICATION->accounts();
    MinecraftAccountPtr defaultAccount = accounts->defaultAccount();
    
    bool canChangeSkin = defaultAccount && (defaultAccount->accountType() == AccountType::MSA) && !defaultAccount->isActive();
    ui->actionManageSkins->setEnabled(canChangeSkin);

    QString active_profileId = "";
    if (defaultAccount) {
        // this can be called before accountMenuButton exists
        if (ui->actionAccountsButton) {
            auto profileLabel = profileInUseFilter(defaultAccount->displayName(), defaultAccount->isInUse());
            ui->actionAccountsButton->setText(profileLabel);
        }
    }

    QActionGroup* accountsGroup = new QActionGroup(this);

    if (accounts->count() <= 0) {
        ui->actionNoAccountsAdded->setEnabled(false);
        ui->accountsMenu->addAction(ui->actionNoAccountsAdded);
    } else {
        // TODO: Nicer way to iterate?
        for (int i = 0; i < accounts->count(); i++) {
            MinecraftAccountPtr account = accounts->at(i);
            auto profileLabel = profileInUseFilter(account->displayName(), account->isInUse());
            QAction* action = new QAction(profileLabel, this);
            action->setData(i);
            action->setCheckable(true);
            action->setActionGroup(accountsGroup);
            if (defaultAccount == account) {
                action->setChecked(true);
            }

            auto face = account->getFace();
            if (!face.isNull()) {
                action->setIcon(face);
            } else {
                action->setIcon(QIcon::fromTheme("noaccount"));
            }

            const int highestNumberKey = 9;
            if (i < highestNumberKey) {
                action->setShortcut(QKeySequence(tr("Ctrl+%1").arg(i + 1)));
            }

            ui->accountsMenu->addAction(action);
            connect(action, &QAction::triggered, this, &MainWindow::changeActiveAccount);
        }
    }

    ui->accountsMenu->addSeparator();

    ui->actionNoDefaultAccount->setData(-1);
    ui->actionNoDefaultAccount->setChecked(!defaultAccount);
    ui->actionNoDefaultAccount->setActionGroup(accountsGroup);

    ui->accountsMenu->addAction(ui->actionNoDefaultAccount);

    connect(ui->actionNoDefaultAccount, &QAction::triggered, this, &MainWindow::changeActiveAccount);

    ui->accountsMenu->addSeparator();
    ui->accountsMenu->addAction(ui->actionManageSkins);
    ui->accountsMenu->addAction(ui->actionManageAccounts);

    accountsButtonMenu->addActions(ui->accountsMenu->actions());
}

void MainWindow::updatesAllowedChanged(bool allowed)
{
    if (!APPLICATION->updaterEnabled()) {
        return;
    }
    ui->actionCheckUpdate->setEnabled(allowed);
}

/*
 * Assumes the sender is a QAction
 */
void MainWindow::changeActiveAccount()
{
    QAction* sAction = (QAction*)sender();

    // Profile's associated Mojang username
    if (sAction->data().typeId() != QMetaType::Int)
        return;

    QVariant action_data = sAction->data();
    bool valid = false;
    int index = action_data.toInt(&valid);
    if (!valid) {
        index = -1;
    }
    auto accounts = APPLICATION->accounts();
    accounts->setDefaultAccount(index == -1 ? nullptr : accounts->at(index));
    defaultAccountChanged();
}

void MainWindow::defaultAccountChanged()
{
    repopulateAccountsMenu();

    MinecraftAccountPtr account = APPLICATION->accounts()->defaultAccount();

    // FIXME: this needs adjustment for MSA
    if (account && account->profileName() != "") {
        auto profileLabel = profileInUseFilter(account->displayName(), account->isInUse());
        ui->actionAccountsButton->setText(profileLabel);
        auto face = account->getFace();
        if (face.isNull()) {
            ui->actionAccountsButton->setIcon(QIcon::fromTheme("noaccount"));
        } else {
            ui->actionAccountsButton->setIcon(face);
        }
        return;
    }

    // Set the icon to the "no account" icon.
    ui->actionAccountsButton->setIcon(QIcon::fromTheme("noaccount"));
    ui->actionAccountsButton->setText(tr("Accounts"));
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == view) {
        if (ev->type() == QEvent::KeyPress) {
            secretEventFilter->input(ev);
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
            switch (keyEvent->key()) {
                    /*
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    activateInstance(m_selectedInstance);
                    return true;
                    */
                case Qt::Key_Delete:
                    on_actionDeleteInstance_triggered();
                    return true;
                case Qt::Key_F5:
                    refreshInstances();
                    return true;
                case Qt::Key_F2:
                    on_actionRenameInstance_triggered();
                    return true;
                default:
                    break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}

void MainWindow::updateNewsLabel()
{
    if (m_newsChecker->isLoadingNews()) {
        newsLabel->setText(tr("Loading news..."));
        newsLabel->setEnabled(false);
        ui->actionMoreNews->setVisible(false);
    } else {
        QList<NewsEntryPtr> entries = m_newsChecker->getNewsEntries();
        if (entries.length() > 0) {
            newsLabel->setText(entries[0]->title);
            newsLabel->setEnabled(true);
            ui->actionMoreNews->setVisible(true);
        } else {
            newsLabel->setText(tr("No news available."));
            newsLabel->setEnabled(false);
            ui->actionMoreNews->setVisible(false);
        }
    }
}

QList<int> stringToIntList(const QString& string)
{
    QStringList split = string.split(',', Qt::SkipEmptyParts);
    QList<int> out;
    for (int i = 0; i < split.size(); ++i) {
        out.append(split.at(i).toInt());
    }
    return out;
}
QString intListToString(const QList<int>& list)
{
    QStringList slist;
    for (int i = 0; i < list.size(); ++i) {
        slist.append(QString::number(list.at(i)));
    }
    return slist.join(',');
}

void MainWindow::onCatToggled(bool state)
{
    setCatBackground(state);
    APPLICATION->settings()->set("TheCat", state);
}

void MainWindow::setCatBackground(bool enabled)
{
    view->setPaintCat(enabled);
    view->viewport()->repaint();
}

void MainWindow::runModalTask(Task* task)
{
    connect(task, &Task::failed,
            [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
    connect(task, &Task::succeeded, [this, task]() {
        QStringList warnings = task->warnings();
        if (warnings.count()) {
            CustomMessageBox::selectable(this, tr("Warnings"), warnings.join('\n'), QMessageBox::Warning)->show();
        }
    });
    connect(task, &Task::aborted, [this] {
        CustomMessageBox::selectable(this, tr("Task aborted"), tr("The task has been aborted by the user."), QMessageBox::Information)
            ->show();
    });
    ProgressDialog loadDialog(this);
    loadDialog.setSkipButton(true, tr("Abort"));
    loadDialog.execWithTask(task);
}

void MainWindow::instanceFromInstanceTask(InstanceTask* rawTask)
{
    unique_qobject_ptr<Task> task(APPLICATION->instances()->wrapInstanceTask(rawTask));
    runModalTask(task.get());
}

void MainWindow::on_actionCopyInstance_triggered()
{
    if (!m_selectedInstance)
        return;

    CopyInstanceDialog copyInstDlg(m_selectedInstance, this);
    if (!copyInstDlg.exec())
        return;

    auto copyTask = new InstanceCopyTask(m_selectedInstance, copyInstDlg.getChosenOptions());
    copyTask->setName(copyInstDlg.instName());
    copyTask->setGroup(copyInstDlg.instGroup());
    copyTask->setIcon(copyInstDlg.iconKey());
    unique_qobject_ptr<Task> task(APPLICATION->instances()->wrapInstanceTask(copyTask));
    runModalTask(task.get());
}

void MainWindow::addInstance(const QString& url, const QMap<QString, QString>& extra_info, const QString& platformPage)
{
    QString groupName;
    do {
        QObject* obj = sender();
        if (!obj)
            break;
        QAction* action = qobject_cast<QAction*>(obj);
        if (!action)
            break;
        auto map = action->data().toMap();
        if (!map.contains("group"))
            break;
        groupName = map["group"].toString();
    } while (0);

    if (groupName.isEmpty()) {
        groupName = APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString();
    }

    NewInstanceDialog newInstDlg(groupName, url, extra_info, this);
    if (!platformPage.isEmpty())
        newInstDlg.selectPlatform(platformPage);
    if (!newInstDlg.exec())
        return;

    APPLICATION->settings()->set("LastUsedGroupForNewInstance", newInstDlg.instGroup());

    InstanceTask* creationTask = newInstDlg.extractTask();
    if (creationTask) {
        instanceFromInstanceTask(creationTask);
    }
}

void MainWindow::on_actionAddInstance_triggered()
{
    addInstance();
}

void MainWindow::processURLs(QList<QUrl> urls)
{
    // NOTE: This loop only processes one dropped file!
    for (auto& url : urls) {
        if (url.isEmpty() || url.toString().trimmed().isEmpty())
            continue;

        qDebug() << "Processing" << url;

        // The isLocalFile() check below doesn't work as intended without an explicit scheme.
        if (url.scheme().isEmpty())
            url.setScheme("file");

        ModPlatform::IndexedVersion version;
        QMap<QString, QString> extra_info;
        QUrl local_url;
        if (!url.isLocalFile()) {  // download the remote resource and identify

            const bool isExternalURLImport = (url.host().toLower() == "import") || (url.path().startsWith("/import", Qt::CaseInsensitive));

            QUrl dl_url;
            if (url.scheme() == "curseforge" || (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME && url.host() == "install")) {
                // need to find the download link for the modpack / resource
                // format of url curseforge://install?addonId=IDHERE&fileId=IDHERE
                // format of url binaryname://install?platform=curseforge&addonId=IDHERE&fileId=IDHERE
                QUrlQuery query(url);

                // check if this is a binaryname:// url
                if (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME) {
                    // check this is an curseforge platform request
                    if (query.queryItemValue("platform").toLower() != "curseforge") {
                        qDebug() << "Invalid mod distribution platform:" << query.queryItemValue("platform");
                        continue;
                    }
                }

                if (query.allQueryItemValues("addonId").isEmpty() || query.allQueryItemValues("fileId").isEmpty()) {
                    qDebug() << "Invalid curseforge link:" << url;
                    continue;
                }

                auto addonId = query.allQueryItemValues("addonId")[0];
                auto fileId = query.allQueryItemValues("fileId")[0];

                extra_info.insert("pack_id", addonId);
                extra_info.insert("pack_version_id", fileId);

                auto api = FlameAPI();
                auto [job, array] = api.getFile(addonId, fileId);

                connect(job.get(), &Task::failed, this,
                        [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
                connect(job.get(), &Task::succeeded, this, [this, array, addonId, fileId, &dl_url, &version] {
                    qDebug() << "Returned CFURL Json:\n" << array->toStdString().c_str();
                    auto doc = Json::requireDocument(*array);
                    auto data = doc.object()["data"].toObject();
                    // No way to find out if it's a mod or a modpack before here
                    // And also we need to check if it ends with .zip, instead of any better way
                    version = FlameMod::loadIndexedPackVersion(data);
                    auto fileName = version.fileName;

                    // Have to use ensureString then use QUrl to get proper url encoding
                    dl_url = QUrl(version.downloadUrl);
                    if (!dl_url.isValid()) {
                        CustomMessageBox::selectable(
                            this, tr("Error"),
                            tr("The modpack, mod, or resource %1 is blocked for third-parties! Please download it manually.").arg(fileName),
                            QMessageBox::Critical)
                            ->show();
                        return;
                    }

                    QFileInfo dl_file(dl_url.fileName());
                });

                {  // drop stack
                    ProgressDialog dlUrlDialod(this);
                    dlUrlDialod.setSkipButton(true, tr("Abort"));
                    dlUrlDialod.execWithTask(job.get());
                }

            } else if (url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME && !isExternalURLImport) {
                QVariantMap receivedData;
                const QUrlQuery query(url.query());
                const auto items = query.queryItems();
                for (auto it = items.begin(), end = items.end(); it != end; ++it)
                    receivedData.insert(it->first, it->second);
                emit APPLICATION->oauthReplyRecieved(receivedData);
                continue;
            } else if ((url.scheme() == "prismlauncher" || url.scheme() == BuildConfig.LAUNCHER_APP_BINARY_NAME) && isExternalURLImport) {
                // PrismLauncher URL protocol modpack import
                // works for any prism fork
                // preferred import format: prismlauncher://import?url=ENCODED
                const auto host = url.host().toLower();
                const auto path = url.path();

                QString encodedTarget;

                {
                    QUrlQuery query(url);
                    const auto values = query.allQueryItemValues("url");
                    if (!values.isEmpty()) {
                        encodedTarget = values.first();
                    }
                }

                // alternative import format: prismlauncher://import/ENCODED
                if (encodedTarget.isEmpty()) {
                    QString p = path;

                    if (p.startsWith("/import/", Qt::CaseInsensitive)) {
                        p = p.mid(QString("/import/").size());
                    } else if (host == "import" && p.startsWith("/")) {
                        p = p.mid(1);
                    }

                    if (!p.isEmpty() && p != "/import") {
                        encodedTarget = p;
                    }
                }

                if (encodedTarget.isEmpty()) {
                    CustomMessageBox::selectable(this, tr("Error"), tr("Invalid import link: missing 'url' parameter."),
                                                 QMessageBox::Critical)
                        ->show();
                    continue;
                }

                const QString decodedStr = QUrl::fromPercentEncoding(encodedTarget.toUtf8()).trimmed();

                QUrl target = QUrl::fromUserInput(decodedStr);

                // Validate: only allow http(s)
                if (!target.isValid() || (target.scheme() != "https" && target.scheme() != "http")) {
                    CustomMessageBox::selectable(this, tr("Error"), tr("Invalid import link: URL must be http(s)."), QMessageBox::Critical)
                        ->show();
                    continue;
                }

                const auto res = QMessageBox::question(
                    this, tr("Install modpack"),
                    tr("Do you want to download and import a modpack from:\n%1\n\nURL:\n%2").arg(target.host(), target.toString()),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                if (res != QMessageBox::Yes) {
                    continue;
                }

                dl_url = target;
            } else {
                dl_url = url;
            }

            if (!dl_url.isValid()) {
                continue;  // no valid url to download this resource
            }

            const QString path = dl_url.host() + '/' + dl_url.path();
            auto entry = APPLICATION->metacache()->resolveEntry("general", path);
            entry->setStale(true);
            auto dl_job = unique_qobject_ptr<NetJob>(new NetJob(tr("Modpack download"), APPLICATION->network()));
            dl_job->addNetAction(Net::ApiDownload::makeCached(dl_url, entry));
            auto archivePath = entry->getFullPath();

            bool dl_success = false;
            connect(dl_job.get(), &Task::failed, this,
                    [this](QString reason) { CustomMessageBox::selectable(this, tr("Error"), reason, QMessageBox::Critical)->show(); });
            connect(dl_job.get(), &Task::succeeded, this, [&dl_success] { dl_success = true; });

            {  // drop stack
                ProgressDialog dlUrlDialod(this);
                dlUrlDialod.setSkipButton(true, tr("Abort"));
                dlUrlDialod.execWithTask(dl_job.get());
            }

            if (!dl_success) {
                continue;  // no local file to identify
            }
            local_url = QUrl::fromLocalFile(archivePath);

        } else {
            local_url = url;
        }

        auto localFileName = QDir::toNativeSeparators(local_url.toLocalFile());
        QFileInfo localFileInfo(localFileName);

        if (localFileName.isEmpty() || !localFileInfo.exists()) {
            qDebug() << "Ignoring invalid path" << localFileName;
            continue;
        }

        auto type = ResourceUtils::identify(localFileInfo);

        if (ModPlatform::ResourceTypeUtils::VALID_RESOURCES.count(type) == 0) {  // probably instance/modpack
            addInstance(localFileName, extra_info);
            continue;
        }

        if (APPLICATION->instances()->count() <= 0) {
            CustomMessageBox::selectable(this, tr("No instance!"),
                                         tr("No instance available to add the resource to.\nPlease create a new instance before "
                                            "attempting to install this resource again."),
                                         QMessageBox::Critical)
                ->show();
            continue;
        }
        ImportResourceDialog dlg(localFileName, type, this);

        if (dlg.exec() != QDialog::Accepted)
            continue;

        qDebug() << "Adding resource" << localFileName << "to" << dlg.selectedInstanceKey;

        auto inst = APPLICATION->instances()->getInstanceById(dlg.selectedInstanceKey);
        auto minecraftInst = dynamic_cast<MinecraftInstance*>(inst);

        switch (type) {
            case ModPlatform::ResourceType::ResourcePack:
                minecraftInst->resourcePackList()->installResourceWithFlameMetadata(localFileName, version);
                break;
            case ModPlatform::ResourceType::TexturePack:
                minecraftInst->texturePackList()->installResourceWithFlameMetadata(localFileName, version);
                break;
            case ModPlatform::ResourceType::DataPack:
                qWarning() << "Importing of Data Packs not supported at this time. Ignoring" << localFileName;
                break;
            case ModPlatform::ResourceType::Mod:
                minecraftInst->loaderModList()->installResourceWithFlameMetadata(localFileName, version);
                break;
            case ModPlatform::ResourceType::ShaderPack:
                minecraftInst->shaderPackList()->installResourceWithFlameMetadata(localFileName, version);
                break;
            case ModPlatform::ResourceType::World:
                minecraftInst->worldList()->installWorld(localFileInfo);
                break;
            case ModPlatform::ResourceType::Unknown:
            default:
                qDebug() << "Can't Identify" << localFileName << "Ignoring it.";
                break;
        }
    }
}

void MainWindow::on_actionREDDIT_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.SUBREDDIT_URL));
}

void MainWindow::on_actionDISCORD_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.DISCORD_URL));
}

void MainWindow::on_actionMATRIX_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.MATRIX_URL));
}

void MainWindow::on_actionChangeInstIcon_triggered()
{
    if (!m_selectedInstance)
        return;

    IconPickerDialog dlg(this);
    dlg.execWithSelection(m_selectedInstance->iconKey());
    if (dlg.result() == QDialog::Accepted) {
        m_selectedInstance->setIconKey(dlg.selectedIconKey);
        auto icon = APPLICATION->icons()->getIcon(dlg.selectedIconKey);
        ui->actionChangeInstIcon->setIcon(icon);
        changeIconButton->setIcon(icon);
    }
}

void MainWindow::iconUpdated(QString icon)
{
    if (icon == m_currentInstIcon) {
        auto new_icon = APPLICATION->icons()->getIcon(m_currentInstIcon);
        ui->actionChangeInstIcon->setIcon(new_icon);
        changeIconButton->setIcon(new_icon);
    }
}

void MainWindow::updateInstanceToolIcon(QString new_icon)
{
    m_currentInstIcon = new_icon;
    auto icon = APPLICATION->icons()->getIcon(m_currentInstIcon);
    ui->actionChangeInstIcon->setIcon(icon);
    changeIconButton->setIcon(icon);
}

void MainWindow::setSelectedInstanceById(const QString& id)
{
    if (id.isNull())
        return;
    const QModelIndex index = APPLICATION->instances()->getInstanceIndexById(id);
    if (index.isValid()) {
        QModelIndex selectionIndex = proxymodel->mapFromSource(index);
        view->selectionModel()->setCurrentIndex(selectionIndex, QItemSelectionModel::ClearAndSelect);
        updateStatusCenter();
    }
}

void MainWindow::on_actionChangeInstGroup_triggered()
{
    if (!m_selectedInstance)
        return;

    InstanceId instId = m_selectedInstance->id();
    QString src(APPLICATION->instances()->getInstanceGroup(instId));

    QStringList groups = APPLICATION->instances()->getGroups();
    groups.prepend("");
    int index = groups.indexOf(src);
    bool ok = false;
    QString dst = QInputDialog::getItem(this, tr("Group name"), tr("Enter a new group name."), groups, index, true, &ok);
    dst = dst.simplified();

    if (ok) {
        APPLICATION->instances()->setInstanceGroup(instId, dst);
    }
}

void MainWindow::deleteGroup(QString group)
{
    Q_ASSERT(!group.isEmpty());

    const int reply = QMessageBox::question(this, tr("Delete group"), tr("Are you sure you want to delete the group '%1'?").arg(group),
                                            QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
        APPLICATION->instances()->deleteGroup(group);
}

void MainWindow::renameGroup(QString group)
{
    Q_ASSERT(!group.isEmpty());

    QString name = QInputDialog::getText(this, tr("Rename group"), tr("Enter a new group name."), QLineEdit::Normal, group);
    name = name.simplified();
    if (name.isNull() || name == group)
        return;

    const bool empty = name.isEmpty();
    const bool duplicate = APPLICATION->instances()->getGroups().contains(name, Qt::CaseInsensitive) && group.toLower() != name.toLower();

    if (empty || duplicate) {
        QMessageBox::warning(this, tr("Cannot rename group"), empty ? tr("Cannot set empty name.") : tr("Group already exists. :/"));
        return;
    }

    APPLICATION->instances()->renameGroup(group, name);
}

void MainWindow::undoTrashInstance()
{
    if (!APPLICATION->instances()->undoTrashInstance())
        QMessageBox::warning(
            this, tr("Failed to undo trashing instance"),
            tr("Some instances and shortcuts could not be restored.\nPlease check your trashbin to manually restore them."));
    ui->actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
}

void MainWindow::on_actionViewLauncherRootFolder_triggered()
{
    DesktopServices::openPath(".");
}

void MainWindow::on_actionViewInstanceFolder_triggered()
{
    QString str = APPLICATION->settings()->get("InstanceDir").toString();
    DesktopServices::openPath(str);
}

void MainWindow::on_actionViewCentralModsFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->settings()->get("CentralModsDir").toString(), true);
}

void MainWindow::on_actionViewSkinsFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->settings()->get("SkinsDir").toString(), true);
}

void MainWindow::on_actionViewIconThemeFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->themeManager()->getIconThemesFolder().path(), true);
}

void MainWindow::on_actionViewWidgetThemeFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->themeManager()->getApplicationThemesFolder().path(), true);
}

void MainWindow::on_actionViewCatPackFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->themeManager()->getCatPacksFolder().path(), true);
}

void MainWindow::on_actionViewIconsFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->icons()->getDirectory(), true);
}

void MainWindow::on_actionViewLogsFolder_triggered()
{
    DesktopServices::openPath("logs", true);
}

void MainWindow::on_actionViewJavaFolder_triggered()
{
    DesktopServices::openPath(APPLICATION->javaPath(), true);
}

void MainWindow::refreshInstances()
{
    APPLICATION->instances()->loadList();
}

void MainWindow::checkForUpdates()
{
    if (APPLICATION->updaterEnabled()) {
        APPLICATION->triggerUpdateCheck();
    } else {
        qWarning() << "Updater not set up. Cannot check for updates.";
    }
}

void MainWindow::on_actionSettings_triggered()
{
    VideoBeta::showSettings(this);
}

void MainWindow::globalSettingsClosed()
{
    // FIXME: quick HACK to make this work. improve, optimize.
    APPLICATION->instances()->loadList();
    proxymodel->invalidate();
    proxymodel->sort(0);
    updateMainToolBar();
    updateLaunchButton();
    updateThemeMenu();
    updateStatusCenter();
    // This needs to be done to prevent UI elements disappearing in the event the config is changed
    // but Prism Launcher exits abnormally, causing the window state to never be saved:
    APPLICATION->settings()->set("MainWindowState", QString::fromUtf8(saveState().toBase64()));
    update();
}

void MainWindow::on_actionEditInstance_triggered()
{
    if (!m_selectedInstance)
        return;

    if (m_selectedInstance->canEdit()) {
        APPLICATION->showInstanceWindow(m_selectedInstance, QStringLiteral("version"));
    } else {
        CustomMessageBox::selectable(this, tr("Instance not editable"),
                                     tr("This instance is not editable. It may be broken, invalid, or too old. Check logs for details."),
                                     QMessageBox::Critical)
            ->show();
    }
}

void MainWindow::on_actionManageSkins_triggered()
{
    auto account = APPLICATION->accounts()->defaultAccount();

    if (account && (account->accountType() == AccountType::MSA) && !account->isActive()) {
        SkinManageDialog dialog(this, account);
        dialog.exec();
    }
}

void MainWindow::on_actionManageAccounts_triggered()
{
    APPLICATION->ShowGlobalSettings(this, "accounts");
}

void MainWindow::on_actionReportBug_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.BUG_TRACKER_URL));
}

void MainWindow::on_actionClearMetadata_triggered()
{
    // This if contains side effects!
    if (!APPLICATION->metacache()->evictAll()) {
        CustomMessageBox::selectable(this, tr("Error"),
                                     tr("Metadata cache clear Failed!\nTo clear the metadata cache manually, press Folders -> View "
                                        "Launcher Root Folder, and after closing the launcher delete the folder named \"meta\"\n"),
                                     QMessageBox::Warning)
            ->show();
    }

    APPLICATION->metacache()->SaveNow();
}

#ifdef Q_OS_MAC
void MainWindow::on_actionAddToPATH_triggered()
{
    auto binaryPath = APPLICATION->applicationFilePath();
    auto targetPath = QString("/usr/local/bin/%1").arg(BuildConfig.LAUNCHER_APP_BINARY_NAME);
    qDebug() << "Symlinking" << binaryPath << "to" << targetPath;

    QStringList args;
    args << "-e";
    args << QString("do shell script \"mkdir -p /usr/local/bin && ln -sf '%1' '%2'\" with administrator privileges")
                .arg(binaryPath, targetPath);
    auto outcome = QProcess::execute("/usr/bin/osascript", args);
    if (!outcome) {
        QMessageBox::information(this, tr("Successfully added %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME),
                                 tr("%1 was successfully added to your PATH. You can now start it by running `%2`.")
                                     .arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.LAUNCHER_APP_BINARY_NAME));
    } else {
        QMessageBox::critical(this, tr("Failed to add %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME),
                              tr("An error occurred while trying to add %1 to PATH").arg(BuildConfig.LAUNCHER_DISPLAYNAME));
    }
}
#endif

void MainWindow::on_actionOpenWiki_triggered()
{
    DesktopServices::openUrl(QUrl(BuildConfig.WIKI_URL));
}

void MainWindow::on_actionMoreNews_triggered()
{
    auto entries = m_newsChecker->getNewsEntries();
    NewsDialog news_dialog(entries, this);
    news_dialog.exec();
}

void MainWindow::newsButtonClicked()
{
    auto entries = m_newsChecker->getNewsEntries();
    NewsDialog news_dialog(entries, this);
    news_dialog.toggleArticleList();
    news_dialog.exec();
}

void MainWindow::onCatChanged(int)
{
    setCatBackground(APPLICATION->settings()->get("TheCat").toBool());
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionDeleteInstance_triggered()
{
    if (!m_selectedInstance) {
        return;
    }

    if (m_selectedInstance->isRunning()) {
        CustomMessageBox::selectable(this, tr("Cannot Delete Running Instance"),
                                     tr("The selected instance is currently running and cannot be deleted. Please stop the instance before "
                                        "attempting to delete it."),
                                     QMessageBox::Warning, QMessageBox::Ok)
            ->exec();
        return;
    }
    auto id = m_selectedInstance->id();

    QString shortcutStr;
    auto shortcuts = m_selectedInstance->shortcuts();
    if (!shortcuts.isEmpty())
        shortcutStr = tr(" and its %n registered shortcut(s)", "", shortcuts.size());
    auto response = CustomMessageBox::selectable(this, tr("Confirm Deletion"),
                                                 tr("You are about to delete \"%1\"%2.\n"
                                                    "This may be permanent and will completely delete the instance.\n\n"
                                                    "Are you sure?")
                                                     .arg(m_selectedInstance->name(), shortcutStr),
                                                 QMessageBox::Warning, QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                        ->exec();

    if (response != QMessageBox::Yes)
        return;

    if (!checkLinkedInstances(id, this, tr("Deleting")))
        return;

    if (APPLICATION->instances()->trashInstance(id)) {
        ui->actionUndoTrashInstance->setEnabled(APPLICATION->instances()->trashedSomething());
    } else {
        APPLICATION->instances()->deleteInstance(id);
    }
    APPLICATION->settings()->set("SelectedInstance", QString());
    selectionBad();
}

void MainWindow::on_actionExportInstanceZip_triggered()
{
    if (m_selectedInstance) {
        ExportInstanceDialog dlg(m_selectedInstance, this);
        dlg.exec();
    }
}

void MainWindow::on_actionExportInstanceMrPack_triggered()
{
    if (m_selectedInstance) {
        auto instance = dynamic_cast<MinecraftInstance*>(m_selectedInstance);
        if (instance != nullptr) {
            ExportPackDialog dlg(instance, this);
            dlg.exec();
        }
    }
}

void MainWindow::on_actionExportInstanceFlamePack_triggered()
{
    if (m_selectedInstance) {
        auto instance = dynamic_cast<MinecraftInstance*>(m_selectedInstance);
        if (instance) {
            if (auto cmp = instance->getPackProfile()->getComponent("net.minecraft");
                cmp && cmp->getVersionFile() && cmp->getVersionFile()->type == "snapshot") {
                QMessageBox msgBox(this);
                msgBox.setText("Snapshots are currently not supported by CurseForge modpacks.");
                msgBox.exec();
                return;
            }
            ExportPackDialog dlg(instance, this, ModPlatform::ResourceProvider::FLAME);
            dlg.exec();
        }
    }
}

void MainWindow::on_actionRenameInstance_triggered()
{
    if (m_selectedInstance) {
        view->edit(view->currentIndex());
    }
}

void MainWindow::on_actionViewSelectedInstFolder_triggered()
{
    if (m_selectedInstance) {
        QString str = m_selectedInstance->instanceRoot();
        DesktopServices::openPath(QFileInfo(str));
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Save the window state and geometry.
    APPLICATION->settings()->set("MainWindowState", QString::fromUtf8(saveState().toBase64()));
    APPLICATION->settings()->set("MainWindowGeometry", QString::fromUtf8(saveGeometry().toBase64()));
    instanceToolbarSetting->set(QString::fromUtf8(ui->instanceToolBar->getVisibilityState().toBase64()));
    event->accept();
    emit isClosing();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::instanceActivated(QModelIndex index)
{
    if (!index.isValid())
        return;
    QString id = index.data(InstanceList::InstanceIDRole).toString();
    BaseInstance* inst = APPLICATION->instances()->getInstanceById(id);
    if (!inst)
        return;

    activateInstance(inst);
}

void MainWindow::on_actionLaunchInstance_triggered()
{
    if (m_selectedInstance && !m_selectedInstance->isRunning()) {
        APPLICATION->launch(m_selectedInstance);
    }
}

void MainWindow::activateInstance(BaseInstance* instance)
{
    APPLICATION->launch(instance);
}

void MainWindow::on_actionKillInstance_triggered()
{
    if (m_selectedInstance && m_selectedInstance->isRunning()) {
        APPLICATION->kill(m_selectedInstance);
    }
}

void MainWindow::on_actionCreateInstanceShortcut_triggered()
{
    if (!m_selectedInstance)
        return;

    CreateShortcutDialog shortcutDlg(m_selectedInstance, this);
    if (!shortcutDlg.exec())
        return;
    shortcutDlg.createShortcut();
}

void MainWindow::taskEnd()
{
    QObject* sender = QObject::sender();
    if (sender == m_versionLoadTask)
        m_versionLoadTask = NULL;

    sender->deleteLater();
}

void MainWindow::startTask(Task* task)
{
    connect(task, &Task::succeeded, this, &MainWindow::taskEnd);
    connect(task, &Task::failed, this, &MainWindow::taskEnd);
    task->start();
}

void MainWindow::instanceChanged(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    if (!current.isValid()) {
        APPLICATION->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
    if (m_selectedInstance) {
        disconnect(m_selectedInstance, &BaseInstance::runningStatusChanged, this, &MainWindow::refreshCurrentInstance);
        disconnect(m_selectedInstance, &BaseInstance::profilerChanged, this, &MainWindow::refreshCurrentInstance);
    }
    QString id = current.data(InstanceList::InstanceIDRole).toString();
    m_selectedInstance = APPLICATION->instances()->getInstanceById(id);
    if (m_selectedInstance) {
        ui->instanceToolBar->setEnabled(true);
        setInstanceActionsEnabled(true);
        ui->actionLaunchInstance->setEnabled(m_selectedInstance->canLaunch());

        ui->actionKillInstance->setEnabled(m_selectedInstance->isRunning());
        ui->actionExportInstance->setEnabled(m_selectedInstance->canExport());
        renameButton->setText(m_selectedInstance->name());
        m_statusLeft->setText(m_selectedInstance->getStatusbarDescription());
        updateStatusCenter();
        updateInstanceToolIcon(m_selectedInstance->iconKey());

        updateLaunchButton();

        APPLICATION->settings()->set("SelectedInstance", m_selectedInstance->id());

        connect(m_selectedInstance, &BaseInstance::runningStatusChanged, this, &MainWindow::refreshCurrentInstance);
        connect(m_selectedInstance, &BaseInstance::profilerChanged, this, &MainWindow::refreshCurrentInstance);
    } else {
        APPLICATION->settings()->set("SelectedInstance", QString());
        selectionBad();
        return;
    }
}

void MainWindow::instanceSelectRequest(QString id)
{
    setSelectedInstanceById(id);
}

void MainWindow::instanceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    auto current = view->selectionModel()->currentIndex();
    QItemSelection test(topLeft, bottomRight);
    if (test.contains(current)) {
        instanceChanged(current, current);
    }
}

void MainWindow::selectionBad()
{
    // start by reseting everything...
    m_selectedInstance = nullptr;
    m_statusLeft->setText(tr("No instance selected"));

    statusBar()->clearMessage();
    ui->instanceToolBar->setEnabled(false);
    setInstanceActionsEnabled(false);
    updateLaunchButton();
    renameButton->setText(tr("Rename Instance"));
    updateInstanceToolIcon("grass");

    // ...and then see if we can enable the previously selected instance
    setSelectedInstanceById(APPLICATION->settings()->get("SelectedInstance").toString());
}

void MainWindow::checkInstancePathForProblems()
{
    QString instanceFolder = APPLICATION->settings()->get("InstanceDir").toString();
    if (FS::checkProblemticPathJava(QDir(instanceFolder))) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'!\' and this is known to cause Java problems!"));
        warning.setInformativeText(tr("You have now two options: <br/>"
                                      " - change the instance folder in the settings <br/>"
                                      " - move this installation of %1 to a different folder")
                                       .arg(BuildConfig.LAUNCHER_DISPLAYNAME));
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
    auto tempFolderText =
        tr("This is a problem: <br/>"
           " - The launcher will likely be deleted without warning by the operating system <br/>"
           " - close the launcher now and extract it to a real location, not a temporary folder");
    QString pathfoldername = QDir(instanceFolder).absolutePath();
    if (pathfoldername.contains("Rar$", Qt::CaseInsensitive)) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder contains \'Rar$\' - that means you haven't extracted the launcher archive!"));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    } else if (pathfoldername.startsWith(QDir::tempPath()) || pathfoldername.contains("/TempState/")) {
        QMessageBox warning(this);
        warning.setText(tr("Your instance folder is in a temporary folder: \'%1\'!").arg(QDir::tempPath()));
        warning.setInformativeText(tempFolderText);
        warning.setDefaultButton(QMessageBox::Ok);
        warning.exec();
    }
}

void MainWindow::updateStatusCenter()
{
    m_statusCenter->setVisible(APPLICATION->settings()->get("ShowGlobalGameTime").toBool());

    int timePlayed = APPLICATION->instances()->getTotalPlayTime();
    if (timePlayed > 0) {
        m_statusCenter->setText(
            tr("Total playtime: %1")
                .arg(Time::prettifyDuration(timePlayed, APPLICATION->settings()->get("ShowGameTimeWithoutDays").toBool())));
    }
}
// "Instance actions" are actions that require an instance to be selected (i.e. "new instance" is not here)
// Actions that also require other conditions (e.g. a running instance) won't be changed.
void MainWindow::setInstanceActionsEnabled(bool enabled)
{
    ui->actionLaunchInstance->setEnabled(enabled && m_selectedInstance && m_selectedInstance->canLaunch());
    ui->actionKillInstance->setEnabled(enabled && m_selectedInstance && m_selectedInstance->isRunning());
    ui->actionEditInstance->setEnabled(enabled);
    ui->actionChangeInstGroup->setEnabled(enabled);
    ui->actionViewSelectedInstFolder->setEnabled(enabled);
    ui->actionExportInstance->setEnabled(enabled);
    ui->actionDeleteInstance->setEnabled(enabled);
    ui->actionCopyInstance->setEnabled(enabled);
    ui->actionCreateInstanceShortcut->setEnabled(enabled);
}

void MainWindow::refreshCurrentInstance()
{
    auto current = view->selectionModel()->currentIndex();
    instanceChanged(current, current);
}
