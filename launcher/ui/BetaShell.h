// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QButtonGroup>
#include <QCoreApplication>
#include <QLocale>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <tuple>

#include "Application.h"
#include "settings/SettingsObject.h"
#include "ui/themes/CatPack.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"
#include "ui/widgets/AppearanceWidget.h"

// Barra lateral y páginas de la beta (Temas, Gatos, Temas y Colores, Mis Mods, Interfaz).
namespace BetaShell {

// Idioma efectivo: el de Ajustes o, si está en "automático" (vacío), el del sistema.
inline bool isSpanish()
{
    QString lang = APPLICATION->settings()->get("Language").toString();
    if (lang.isEmpty())
        lang = QLocale::system().name();
    return lang.startsWith("es", Qt::CaseInsensitive);
}

// Texto traducible: usa el catálogo de Prism del idioma elegido (clave en inglés). Si ese idioma no
// traduce la cadena, se usa el respaldo en español cuando el idioma es español, o el inglés.
inline QString t(const char* en, const char* es = nullptr)
{
    const QString source = QString::fromUtf8(en);
    const QString translated = QCoreApplication::translate("BetaShell", en);
    if (translated.size() > 2 && translated != source)
        return translated;
    const QString legacy = QCoreApplication::translate("MainWindow", en);
    if (legacy.size() > 2 && legacy != source)
        return legacy;
    if (es && isSpanish())
        return QString::fromUtf8(es);
    return source;
}

// Avisa del ancho del widget cada vez que cambia de tamaño.
class AutoCollapse : public QObject {
   public:
    AutoCollapse(QWidget* watched, std::function<void(int)> onWidth) : QObject(watched), m_onWidth(std::move(onWidth)) {}

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Resize)
            m_onWidth(static_cast<QWidget*>(watched)->width());
        return false;
    }

   private:
    std::function<void(int)> m_onWidth;
};

inline const char* cardStyle()
{
    return "QToolButton { background: palette(button); border: 1px solid palette(mid); border-radius: 12px; color: palette(button-text); padding: 10px; font-size: 13px; }"
           "QToolButton:hover { background: palette(highlight); border-color: palette(highlight); }"
           "QToolButton:checked { background: palette(highlight); border: 2px solid palette(highlight); color: palette(highlighted-text); }";
}

inline QLabel* heading(const QString& text, QWidget* parent)
{
    auto label = new QLabel(text, parent);
    label->setStyleSheet("font-size: 20px; font-weight: 600; color: palette(window-text); background: transparent; border: none;");
    return label;
}

inline QLabel* hint(const QString& text, QWidget* parent)
{
    auto label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setStyleSheet("font-size: 13px; color: palette(window-text); background: transparent; border: none;");
    return label;
}

inline QScrollArea* scrollPage(QWidget* content, QWidget* parent)
{
    auto area = new QScrollArea(parent);
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidget(content);
    area->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollArea > QWidget > QWidget { background: transparent; }");
    return area;
}

// Rejilla de tarjetas exclusivas; `select` se llama al elegir una.
inline QWidget* cardGrid(QWidget* parent,
                         const QString& title,
                         const QString& description,
                         const QList<std::tuple<QString, QString, QIcon, QString>>& entries,
                         const QString& current,
                         const std::function<void(int, const QString&)>& select)
{
    auto content = new QWidget(parent);
    auto layout = new QVBoxLayout(content);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(12);
    layout->addWidget(heading(title, content));
    layout->addWidget(hint(description, content));
    auto grid = new QGridLayout;
    grid->setSpacing(14);
    auto group = new QButtonGroup(content);
    group->setExclusive(true);
    const int columns = 4;
    for (int i = 0; i < entries.size(); ++i) {
        const auto& [id, name, icon, tooltip] = entries[i];
        auto card = new QToolButton(content);
        card->setCheckable(true);
        card->setChecked(id == current);
        card->setText(name);
        card->setToolTip(tooltip);
        if (!icon.isNull()) {
            card->setIcon(icon);
            card->setIconSize(QSize(72, 72));
            card->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        }
        card->setMinimumSize(150, icon.isNull() ? 84 : 128);
        card->setCursor(Qt::PointingHandCursor);
        card->setStyleSheet(cardStyle());
        group->addButton(card, i);
        grid->addWidget(card, i / columns, i % columns);
        QObject::connect(card, &QToolButton::clicked, content, [select, i, id = id] { select(i, id); });
    }
    layout->addLayout(grid);
    layout->addStretch();
    return scrollPage(content, parent);
}

inline QWidget* themesPage(QWidget* parent)
{
    QList<std::tuple<QString, QString, QIcon, QString>> entries;
    for (auto* theme : APPLICATION->themeManager()->getValidApplicationThemes())
        entries.append({ theme->id(), theme->name(), QIcon(), theme->tooltip() });
    return cardGrid(parent, t("Themes", "Temas"),
                    t("Choose the look of the whole application. Changes apply instantly.", "Elige el aspecto de toda la aplicación. El cambio se aplica al momento."), entries,
                    APPLICATION->settings()->get("ApplicationTheme").toString(), [](int, const QString& id) {
                        if (APPLICATION->settings()->get("ApplicationTheme").toString() != id) {
                            APPLICATION->settings()->set("ApplicationTheme", id);
                            APPLICATION->themeManager()->applyCurrentlySelectedTheme();
                        }
                    });
}

inline QWidget* catPacksPage(QWidget* parent)
{
    QList<std::tuple<QString, QString, QIcon, QString>> entries;
    for (auto* cat : APPLICATION->themeManager()->getValidCatPacks())
        entries.append({ cat->id(), cat->name(), QIcon(cat->path()), QString() });
    return cardGrid(parent, t("Cat packs", "Gatos (CatPacks)"),
                    t("The cat that keeps you company in the launcher background. Add more in the cat packs folder.", "El gato que te acompaña en el fondo del lanzador. Añade más en la carpeta de CatPacks."), entries,
                    APPLICATION->settings()->get("BackgroundCat").toString(), [](int index, const QString& id) {
                        APPLICATION->settings()->set("BackgroundCat", id);
                        APPLICATION->currentCatChanged(index);
                    });
}

inline QWidget* colorsPage(QWidget* parent)
{
    auto content = new QWidget(parent);
    auto layout = new QVBoxLayout(content);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(12);
    layout->addWidget(heading(t("Themes and colors", "Temas y Colores"), content));
    layout->addWidget(hint(t("Application theme, icons, cat and console colors with preview.", "Tema de la aplicación, iconos, gato y colores de la consola con vista previa."), content));
    auto appearance = new AppearanceWidget(false, content);
    appearance->loadSettings();
    layout->addWidget(appearance);
    layout->addStretch();
    return scrollPage(content, parent);
}

// Página con botones que abren las páginas de ajustes globales.
inline QWidget* shortcutsPage(QWidget* parent,
                              const QString& title,
                              const QString& description,
                              const QList<std::pair<QString, std::function<void()>>>& actions)
{
    auto content = new QWidget(parent);
    auto layout = new QVBoxLayout(content);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(12);
    layout->addWidget(heading(title, content));
    layout->addWidget(hint(description, content));
    auto grid = new QGridLayout;
    grid->setSpacing(14);
    for (int i = 0; i < actions.size(); ++i) {
        auto button = new QToolButton(content);
        button->setText(actions[i].first);
        button->setMinimumSize(190, 64);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(cardStyle());
        QObject::connect(button, &QToolButton::clicked, content, actions[i].second);
        grid->addWidget(button, i / 3, i % 3);
    }
    layout->addLayout(grid);
    layout->addStretch();
    return scrollPage(content, parent);
}

}  // namespace BetaShell
