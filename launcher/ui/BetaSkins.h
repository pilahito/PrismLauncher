// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QColor>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#include "ui/BetaShell.h"

// Modos de temporada para skins de Minecraft: aplica filtros y accesorios a una textura de 64 píxeles de ancho.
namespace BetaSkins {

inline void paintBlock(QImage& img, int x, int y, int w, int h, const QColor& color)
{
    for (int py = y; py < y + h && py < img.height(); ++py)
        for (int px = x; px < x + w && px < img.width(); ++px)
            img.setPixelColor(px, py, color);
}

// Devuelve una copia de la skin con el modo indicado ("normal", "christmas", "summer", "halloween").
inline QImage applyMode(const QImage& base, const QString& mode)
{
    QImage out = base.convertToFormat(QImage::Format_ARGB32);
    if (mode == QLatin1String("christmas")) {
        // Gorro rojo con ribete blanco y pompón sobre la cabeza (parte superior de la cabeza: x 8-16, y 0-8).
        paintBlock(out, 8, 0, 8, 2, QColor("#c62828"));
        paintBlock(out, 8, 2, 8, 1, QColor("#ffffff"));
        paintBlock(out, 13, 0, 2, 2, QColor("#ffffff"));
    } else if (mode == QLatin1String("summer")) {
        // Tono cálido y gafas de sol sobre los ojos de la cara (x 8-16, y 8-16).
        for (int y = 0; y < out.height(); ++y)
            for (int x = 0; x < out.width(); ++x) {
                QColor c = out.pixelColor(x, y);
                c.setRed(qMin(255, c.red() + 20));
                c.setGreen(qMin(255, c.green() + 8));
                c.setBlue(qMax(0, c.blue() - 15));
                out.setPixelColor(x, y, c);
            }
        paintBlock(out, 9, 12, 3, 2, QColor("#1a1a1a"));
        paintBlock(out, 12, 12, 3, 2, QColor("#1a1a1a"));
    } else if (mode == QLatin1String("halloween")) {
        // Oscurece la skin y le da un tono morado y naranja.
        for (int y = 0; y < out.height(); ++y)
            for (int x = 0; x < out.width(); ++x) {
                QColor c = out.pixelColor(x, y);
                c.setRed(qBound(0, int(c.red() * 0.7) + 30, 255));
                c.setGreen(qBound(0, int(c.green() * 0.5), 255));
                c.setBlue(qBound(0, int(c.blue() * 0.8) + 40, 255));
                out.setPixelColor(x, y, c);
            }
    }
    return out;
}

inline QWidget* page(QWidget* parent)
{
    auto content = new QWidget(parent);
    auto layout = new QVBoxLayout(content);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(12);
    layout->addWidget(BetaShell::heading(BetaShell::t("Skins", "Skins"), content));
    layout->addWidget(BetaShell::hint(BetaShell::t("Open a skin, apply a seasonal mode and save it as a new PNG.",
                                                   "Abre una skin, aplica un modo de temporada y guárdala como PNG nueva."),
                                      content));

    auto state = std::make_shared<QImage>();   // skin original
    auto current = std::make_shared<QImage>(); // skin con el modo aplicado

    auto preview = new QLabel(content);
    preview->setFixedSize(256, 256);
    preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet("background: #191e27; border: 1px solid #3b4553; border-radius: 10px; color: #a4afbf;");
    preview->setText(BetaShell::t("No skin open", "Ninguna skin abierta"));
    layout->addWidget(preview, 0, Qt::AlignLeft);

    auto show = [preview, current] {
        if (current->isNull())
            return;
        preview->setPixmap(QPixmap::fromImage(current->scaled(224, 224, Qt::KeepAspectRatio, Qt::FastTransformation)));
    };

    const QString buttonStyle = "QPushButton { background: #262d39; border: 1px solid #3b4553; border-radius: 10px; color: #dbe3ee; padding: 10px 18px; font-size: 13px; }"
                                "QPushButton:hover { background: #2e3948; border-color: #5bb7d3; }";

    auto row = new QHBoxLayout;
    row->setSpacing(10);
    auto open = new QPushButton(BetaShell::t("Open skin", "Abrir skin"), content);
    open->setStyleSheet(buttonStyle);
    row->addWidget(open);
    QObject::connect(open, &QPushButton::clicked, content, [content, state, current, show] {
        const QString path = QFileDialog::getOpenFileName(content, BetaShell::t("Open skin", "Abrir skin"), QString(), "PNG (*.png)");
        if (path.isEmpty())
            return;
        QImage img(path);
        if (img.isNull() || img.width() != 64) {
            QMessageBox::warning(content, BetaShell::t("Skin", "Skin"),
                                 BetaShell::t("The skin must be a 64-pixel-wide PNG.", "La skin debe ser una PNG de 64 píxeles de ancho."));
            return;
        }
        *state = img;
        *current = img;
        show();
    });

    const QList<std::pair<QString, QString>> modes = {
        { "normal", BetaShell::t("Normal", "Normal") },
        { "christmas", BetaShell::t("Christmas", "Navidad") },
        { "summer", BetaShell::t("Summer", "Verano") },
        { "halloween", BetaShell::t("Halloween", "Halloween") },
    };
    for (const auto& [mode, label] : modes) {
        auto button = new QPushButton(label, content);
        button->setStyleSheet(buttonStyle);
        row->addWidget(button);
        QObject::connect(button, &QPushButton::clicked, content, [state, current, show, mode = mode] {
            if (state->isNull())
                return;
            *current = applyMode(*state, mode);
            show();
        });
    }

    auto save = new QPushButton(BetaShell::t("Save as PNG", "Guardar como PNG"), content);
    save->setStyleSheet(buttonStyle);
    row->addWidget(save);
    QObject::connect(save, &QPushButton::clicked, content, [content, current] {
        if (current->isNull())
            return;
        const QString path = QFileDialog::getSaveFileName(content, BetaShell::t("Save as PNG", "Guardar como PNG"), "skin.png", "PNG (*.png)");
        if (!path.isEmpty())
            current->save(path, "PNG");
    });

    row->addStretch();
    layout->addLayout(row);
    layout->addStretch();
    return BetaShell::scrollPage(content, parent);
}

}  // namespace BetaSkins
