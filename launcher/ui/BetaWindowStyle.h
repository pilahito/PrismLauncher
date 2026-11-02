// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QEvent>
#include <QLibrary>
#include <QObject>
#include <QWidget>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

// Aplica el color del tema a las barras nativas de todas las ventanas de la beta.
class BetaWindowStyle : public QObject {
public:
    explicit BetaWindowStyle(QObject* parent) : QObject(parent) {}
protected:
    bool eventFilter(QObject* object, QEvent* event) override
    {
#ifdef Q_OS_WIN
        if (event->type() == QEvent::Show || event->type() == QEvent::PaletteChange) {
            auto widget = qobject_cast<QWidget*>(object);
            if (widget && widget->isWindow() && widget->isVisible() && widget->windowType() != Qt::ToolTip) {
                using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
                static auto setAttribute = reinterpret_cast<SetAttribute>(QLibrary::resolve(QStringLiteral("dwmapi"), "DwmSetWindowAttribute"));
                if (setAttribute) {
                    const auto background = widget->palette().color(QPalette::Window);
                    const auto foreground = widget->palette().color(QPalette::WindowText);
                    const BOOL dark = background.lightness() < 128;
                    const COLORREF caption = RGB(background.red(), background.green(), background.blue());
                    const COLORREF text = RGB(foreground.red(), foreground.green(), foreground.blue());
                    const auto handle = reinterpret_cast<HWND>(widget->winId());
                    setAttribute(handle, 20, &dark, sizeof(dark));
                    setAttribute(handle, 35, &caption, sizeof(caption));
                    setAttribute(handle, 36, &text, sizeof(text));
                }
            }
        }
#else
        Q_UNUSED(object);
        Q_UNUSED(event);
#endif
        return false;
    }
};
