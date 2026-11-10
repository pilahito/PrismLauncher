// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QButtonGroup>
#include <QColor>
#include <QColorDialog>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVector3D>
#include <QWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include "ui/BetaShell.h"

// Sección Skins: editor de píxeles de 64×64 inspirado en XArt Skin Editor (Gangstos, MIT),
// modos de temporada y acceso al editor XArt completo.
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
        // Gorro rojo con ribete blanco y pompón sobre la cabeza.
        paintBlock(out, 8, 0, 8, 2, QColor("#c62828"));
        paintBlock(out, 8, 2, 8, 1, QColor("#ffffff"));
        paintBlock(out, 13, 0, 2, 2, QColor("#ffffff"));
    } else if (mode == QLatin1String("summer")) {
        // Tono cálido y gafas de sol sobre los ojos.
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
        // Oscurece la skin con tono morado y naranja.
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

// Lienzo de 64×64 con herramientas de pintura y deshacer (hasta 100 pasos).
class SkinCanvas : public QWidget {
public:
    static constexpr int kScale = 8;

    explicit SkinCanvas(QWidget* parent = nullptr) : QWidget(parent), m_image(64, 64, QImage::Format_ARGB32)
    {
        m_image.fill(Qt::transparent);
        setFixedSize(64 * kScale, 64 * kScale);
        setCursor(Qt::CrossCursor);
    }

    void setImage(const QImage& img)
    {
        m_image = img.convertToFormat(QImage::Format_ARGB32);
        m_undo.clear();
        changed();
    }
    QImage image() const { return m_image; }
    void setTool(const QString& tool) { m_tool = tool; }
    void setColor(const QColor& color) { m_color = color; }
    QColor color() const { return m_color; }
    void setPickHandler(std::function<void(QColor)> handler) { m_onPick = std::move(handler); }
    void setChangeHandler(std::function<void()> handler) { m_onChange = std::move(handler); }

    void undo()
    {
        if (m_undo.empty())
            return;
        m_image = m_undo.back();
        m_undo.pop_back();
        changed();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.fillRect(rect(), QColor("#2b313c"));
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                if ((x + y) % 2 == 0)
                    p.fillRect(x * kScale, y * kScale, kScale, kScale, QColor("#3a4150"));
        p.drawImage(rect(), m_image);
        p.setPen(QColor(255, 255, 255, 25));
        for (int i = 0; i <= 64; ++i) {
            p.drawLine(i * kScale, 0, i * kScale, height());
            p.drawLine(0, i * kScale, width(), i * kScale);
        }
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() != Qt::LeftButton)
            return;
        if (m_tool != QLatin1String("picker"))
            pushUndo();
        m_drawing = true;
        apply(e->position().toPoint());
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (m_drawing && (m_tool == QLatin1String("pencil") || m_tool == QLatin1String("eraser")))
            apply(e->position().toPoint());
    }

    void mouseReleaseEvent(QMouseEvent*) override { m_drawing = false; }

private:
    void changed()
    {
        update();
        if (m_onChange)
            m_onChange();
    }

    void pushUndo()
    {
        m_undo.push_back(m_image);
        if (m_undo.size() > 100)
            m_undo.erase(m_undo.begin());
    }

    void apply(const QPoint& pos)
    {
        const int x = pos.x() / kScale;
        const int y = pos.y() / kScale;
        if (x < 0 || y < 0 || x >= 64 || y >= 64)
            return;
        if (m_tool == QLatin1String("pencil")) {
            m_image.setPixelColor(x, y, m_color);
        } else if (m_tool == QLatin1String("eraser")) {
            m_image.setPixelColor(x, y, QColor(0, 0, 0, 0));
        } else if (m_tool == QLatin1String("picker")) {
            if (m_onPick)
                m_onPick(m_image.pixelColor(x, y));
            return;
        } else if (m_tool == QLatin1String("fill")) {
            floodFill(x, y);
        }
        changed();
    }

    void floodFill(int startX, int startY)
    {
        const QColor target = m_image.pixelColor(startX, startY);
        if (target == m_color)
            return;
        std::vector<QPoint> stack{ QPoint(startX, startY) };
        while (!stack.empty()) {
            const QPoint q = stack.back();
            stack.pop_back();
            if (q.x() < 0 || q.y() < 0 || q.x() >= 64 || q.y() >= 64)
                continue;
            if (m_image.pixelColor(q) != target)
                continue;
            m_image.setPixelColor(q, m_color);
            stack.push_back(QPoint(q.x() + 1, q.y()));
            stack.push_back(QPoint(q.x() - 1, q.y()));
            stack.push_back(QPoint(q.x(), q.y() + 1));
            stack.push_back(QPoint(q.x(), q.y() - 1));
        }
    }

    QImage m_image;
    std::vector<QImage> m_undo;
    QString m_tool = QStringLiteral("pencil");
    QColor m_color = QColor("#ffffff");
    bool m_drawing = false;
    std::function<void(QColor)> m_onPick;
    std::function<void()> m_onChange;
};

// Cara de una pieza del modelo: esquinas 3D y región de la textura correspondiente.
struct SkinFace {
    std::array<QVector3D, 4> pts;
    QRectF uv;
    QVector3D normal;
};

// Vista 3D del modelo clásico (brazos de 4 píxeles). Se gira arrastrando con el ratón.
class SkinModel3D : public QWidget {
public:
    explicit SkinModel3D(QWidget* parent = nullptr) : QWidget(parent), m_tex(64, 64, QImage::Format_ARGB32)
    {
        m_tex.fill(Qt::transparent);
        setFixedSize(320, 420);
        setCursor(Qt::OpenHandCursor);
    }

    void setSlim(bool slim) { m_slim = slim; update(); }

    void setTexture(const QImage& img)
    {
        m_tex = img.convertToFormat(QImage::Format_ARGB32);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);
        p.fillRect(rect(), QColor("#1b2029"));

        std::vector<SkinFace> faces;
        // Piezas del modelo: base y capa exterior (overlay, 0.5 px más grande).
        addBox(faces, QVector3D(-4, 24, -4), QVector3D(8, 8, 8), 0, 0, 8, 8, 8, 0.0f);
        addBox(faces, QVector3D(-4, 24, -4), QVector3D(8, 8, 8), 32, 0, 8, 8, 8, 0.5f);
        addBox(faces, QVector3D(-4, 12, -2), QVector3D(8, 12, 4), 16, 16, 8, 12, 4, 0.0f);
        addBox(faces, QVector3D(-4, 12, -2), QVector3D(8, 12, 4), 16, 32, 8, 12, 4, 0.25f);
        const int arm = m_slim ? 3 : 4;
        addBox(faces, QVector3D(-4-arm, 12, -2), QVector3D(arm, 12, 4), 40, 16, arm, 12, 4, 0.0f);
        addBox(faces, QVector3D(-4-arm, 12, -2), QVector3D(arm, 12, 4), 40, 32, arm, 12, 4, 0.25f);
        addBox(faces, QVector3D(4, 12, -2), QVector3D(arm, 12, 4), 32, 48, arm, 12, 4, 0.0f);
        addBox(faces, QVector3D(4, 12, -2), QVector3D(arm, 12, 4), 48, 48, arm, 12, 4, 0.25f);
        addBox(faces, QVector3D(-4, 0, -2), QVector3D(4, 12, 4), 0, 16, 4, 12, 4, 0.0f);
        addBox(faces, QVector3D(-4, 0, -2), QVector3D(4, 12, 4), 0, 32, 4, 12, 4, 0.25f);
        addBox(faces, QVector3D(0, 0, -2), QVector3D(4, 12, 4), 16, 48, 4, 12, 4, 0.0f);
        addBox(faces, QVector3D(0, 0, -2), QVector3D(4, 12, 4), 0, 48, 4, 12, 4, 0.25f);

        struct Projected {
            QPolygonF poly;
            QRectF uv;
            double depth;
        };
        std::vector<Projected> visible;
        const float cy = std::cos(m_yaw), sy = std::sin(m_yaw);
        const float cp = std::cos(m_pitch), sp = std::sin(m_pitch);
        auto rotate = [&](const QVector3D& v) {
            const float x1 = v.x() * cy + v.z() * sy;
            const float z1 = -v.x() * sy + v.z() * cy;
            const float y2 = v.y() * cp - z1 * sp;
            const float z2 = v.y() * sp + z1 * cp;
            return QVector3D(x1, y2, z2);
        };
        const float scale = 6.5f;
        auto project = [&](const QVector3D& v) {
            const QVector3D r = rotate(v);
            return std::pair<QPointF, float>(QPointF(width() / 2.0 + r.x() * scale, height() / 2.0 - (r.y() - 16.0) * scale), r.z());
        };
        for (const SkinFace& f : faces) {
            if (rotate(f.normal).z() <= 0)
                continue;
            Projected pr;
            float depth = 0;
            for (const QVector3D& corner : f.pts) {
                const auto [pt, z] = project(corner);
                pr.poly << pt;
                depth += z;
            }
            pr.uv = f.uv;
            pr.depth = depth / 4.0;
            visible.push_back(pr);
        }
        std::sort(visible.begin(), visible.end(), [](const Projected& a, const Projected& b) { return a.depth < b.depth; });

        for (const Projected& pr : visible) {
            QPolygonF src;
            src << QPointF(pr.uv.left(), pr.uv.top()) << QPointF(pr.uv.right(), pr.uv.top())
                << QPointF(pr.uv.right(), pr.uv.bottom()) << QPointF(pr.uv.left(), pr.uv.bottom());
            QTransform tr;
            if (!QTransform::quadToQuad(src, pr.poly, tr))
                continue;
            QPainterPath clip;
            clip.addPolygon(pr.poly);
            p.save();
            p.setClipPath(clip);
            p.setTransform(tr);
            p.drawImage(QPointF(0, 0), m_tex);
            p.restore();
        }
    }

    void mousePressEvent(QMouseEvent* e) override { m_last = e->position(); }
    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (!(e->buttons() & Qt::LeftButton))
            return;
        const QPointF d = e->position() - m_last;
        m_last = e->position();
        m_yaw += float(d.x()) * 0.01f;
        m_pitch = qBound(-1.2f, m_pitch + float(d.y()) * 0.01f, 1.2f);
        update();
    }

private:
    // Añade las seis caras de una caja. Las regiones siguen la distribución de la skin de 64 píxeles.
    static void addBox(std::vector<SkinFace>& out, const QVector3D& origin, const QVector3D& size, int u, int v, int w, int h, int d, float grow)
    {
        const float x0 = origin.x() - grow, y0 = origin.y() - grow, z0 = origin.z() - grow;
        const float dx = size.x() + 2 * grow, dy = size.y() + 2 * grow, dz = size.z() + 2 * grow;
        auto quad = [](auto point) {
            return std::array<QVector3D, 4>{ point(0.0f, 0.0f), point(1.0f, 0.0f), point(1.0f, 1.0f), point(0.0f, 1.0f) };
        };
        out.push_back({ quad([&](float s, float t) { return QVector3D(x0 + s * dx, y0 + (1 - t) * dy, z0 + dz); }),
                        QRectF(u + d, v + d, w, h), QVector3D(0, 0, 1) });
        out.push_back({ quad([&](float s, float t) { return QVector3D(x0 + (1 - s) * dx, y0 + (1 - t) * dy, z0); }),
                        QRectF(u + 2 * d + w, v + d, w, h), QVector3D(0, 0, -1) });
        out.push_back({ quad([&](float s, float t) { return QVector3D(x0, y0 + (1 - t) * dy, z0 + s * dz); }),
                        QRectF(u, v + d, d, h), QVector3D(-1, 0, 0) });
        out.push_back({ quad([&](float s, float t) { return QVector3D(x0 + dx, y0 + (1 - t) * dy, z0 + (1 - s) * dz); }),
                        QRectF(u + d + w, v + d, d, h), QVector3D(1, 0, 0) });
        out.push_back({ quad([&](float s, float t) { return QVector3D(x0 + s * dx, y0 + dy, z0 + t * dz); }),
                        QRectF(u + d, v, w, d), QVector3D(0, 1, 0) });
        out.push_back({ quad([&](float s, float t) { return QVector3D(x0 + s * dx, y0, z0 + (1 - t) * dz); }),
                        QRectF(u + d + w, v, w, d), QVector3D(0, -1, 0) });
    }

    QImage m_tex;
    bool m_slim = false;
    float m_yaw = -0.6f;
    float m_pitch = 0.25f;
    QPointF m_last;
};

inline QWidget* page(QWidget* parent, const QImage& initial = QImage(), bool initialSlim = false, std::function<void(const QString&)> saved = {})
{
    auto content = new QWidget(parent);
    auto layout = new QVBoxLayout(content);
    layout->setContentsMargins(26, 22, 26, 22);
    layout->setSpacing(12);
    layout->addWidget(BetaShell::heading(BetaShell::t("Skins", "Skins"), content));
    layout->addWidget(BetaShell::hint(BetaShell::t("Open a skin, paint it or apply a seasonal mode, and save it as a new PNG.",
                                                   "Abre una skin, píntala o aplica un modo de temporada, y guárdala como PNG nueva."),
                                      content));

    auto original = std::make_shared<QImage>();  // skin original para aplicar los modos
    auto canvas = new SkinCanvas(content);
    if (!initial.isNull()) { *original = initial; canvas->setImage(initial); }

    const QString buttonStyle = "QPushButton { background: #262d39; border: 1px solid #3b4553; border-radius: 10px; color: #dbe3ee; padding: 10px 18px; font-size: 13px; }"
                                "QPushButton:hover { background: #2e3948; border-color: #5bb7d3; }"
                                "QPushButton:checked { background: #26414f; border-color: #62d0e6; color: #ffffff; }";

    // Herramientas de pintura.
    auto toolColumn = new QVBoxLayout;
    toolColumn->setSpacing(8);
    auto tools = new QButtonGroup(content);
    tools->setExclusive(true);
    const QList<std::pair<QString, QString>> toolList = {
        { "pencil", BetaShell::t("Pencil", "Lápiz") },
        { "eraser", BetaShell::t("Eraser", "Borrador") },
        { "fill", BetaShell::t("Fill", "Bote") },
        { "picker", BetaShell::t("Picker", "Cuentagotas") },
    };
    for (const auto& [id, label] : toolList) {
        auto button = new QPushButton(label, content);
        button->setCheckable(true);
        button->setChecked(id == QLatin1String("pencil"));
        button->setStyleSheet(buttonStyle);
        tools->addButton(button);
        QObject::connect(button, &QPushButton::clicked, content, [canvas, id = id] { canvas->setTool(id); });
    }
    for (auto button : tools->buttons())
        toolColumn->addWidget(button);

    auto color = new QPushButton(BetaShell::t("Color…", "Color…"), content);
    color->setStyleSheet(buttonStyle);
    QObject::connect(color, &QPushButton::clicked, content, [content, canvas] {
        const QColor chosen = QColorDialog::getColor(canvas->color(), content, BetaShell::t("Color", "Color"));
        if (chosen.isValid())
            canvas->setColor(chosen);
    });
    toolColumn->addWidget(color);

    auto undo = new QPushButton(BetaShell::t("Undo", "Deshacer"), content);
    undo->setStyleSheet(buttonStyle);
    QObject::connect(undo, &QPushButton::clicked, content, [canvas] { canvas->undo(); });
    toolColumn->addWidget(undo);
    toolColumn->addStretch();

    canvas->setPickHandler([canvas, tools, toolList](QColor picked) {
        canvas->setColor(picked);
        // Vuelve al lápiz tras elegir un color.
        for (auto button : tools->buttons())
            if (button->text() == toolList[0].second)
                button->click();
    });

    auto model3d = new SkinModel3D(content);
    model3d->setTexture(canvas->image());
    model3d->setSlim(initialSlim);
    auto modelRow = new QHBoxLayout;
    auto models = new QButtonGroup(content);
    for (const auto& entry : {std::pair<QString, bool>{QStringLiteral("Clásico (Steve)"), false}, {QStringLiteral("Slim (Alex)"), true}}) {
        auto button = new QPushButton(entry.first, content);
        button->setCheckable(true); button->setChecked(entry.second == initialSlim); button->setStyleSheet(buttonStyle);
        models->addButton(button); modelRow->addWidget(button);
        QObject::connect(button, &QPushButton::clicked, content, [model3d, slim = entry.second] { model3d->setSlim(slim); });
    }
    modelRow->addStretch(); layout->addLayout(modelRow);
    canvas->setChangeHandler([canvas, model3d] { model3d->setTexture(canvas->image()); });

    auto editor = new QHBoxLayout;
    editor->setSpacing(14);
    editor->addWidget(canvas, 0, Qt::AlignTop);
    editor->addLayout(toolColumn);
    editor->addWidget(model3d, 0, Qt::AlignTop);
    editor->addStretch();
    layout->addLayout(editor);

    // Modos de temporada sobre la skin original.
    auto modes = new QHBoxLayout;
    modes->setSpacing(10);
    const QList<std::pair<QString, QString>> modeList = {
        { "normal", BetaShell::t("Normal", "Normal") },
        { "christmas", BetaShell::t("Christmas", "Navidad") },
        { "summer", BetaShell::t("Summer", "Verano") },
        { "halloween", BetaShell::t("Halloween", "Halloween") },
    };
    for (const auto& [mode, label] : modeList) {
        auto button = new QPushButton(label, content);
        button->setStyleSheet(buttonStyle);
        modes->addWidget(button);
        QObject::connect(button, &QPushButton::clicked, content, [original, canvas, mode = mode] {
            if (original->isNull())
                return;
            canvas->setImage(applyMode(*original, mode));
        });
    }
    modes->addStretch();
    layout->addLayout(modes);

    // Abrir, guardar y editor XArt completo.
    auto files = new QHBoxLayout;
    files->setSpacing(10);
    auto open = new QPushButton(BetaShell::t("Open skin", "Abrir skin"), content);
    open->setStyleSheet(buttonStyle);
    files->addWidget(open);
    QObject::connect(open, &QPushButton::clicked, content, [content, original, canvas] {
        const QString path = QFileDialog::getOpenFileName(content, BetaShell::t("Open skin", "Abrir skin"), QString(), "PNG (*.png)");
        if (path.isEmpty())
            return;
        QImage img(path);
        if (img.isNull() || img.width() != 64) {
            QMessageBox::warning(content, BetaShell::t("Skin", "Skin"),
                                 BetaShell::t("The skin must be a 64-pixel-wide PNG.", "La skin debe ser una PNG de 64 píxeles de ancho."));
            return;
        }
        *original = img;
        canvas->setImage(img);
    });

    auto save = new QPushButton(BetaShell::t("Save as PNG", "Guardar como PNG"), content);
    save->setStyleSheet(buttonStyle);
    files->addWidget(save);
    QObject::connect(save, &QPushButton::clicked, content, [content, canvas, saved] {
        const QString path = QFileDialog::getSaveFileName(content, BetaShell::t("Save as PNG", "Guardar como PNG"), "skin.png", "PNG (*.png)");
        if (!path.isEmpty()) {
            if (!canvas->image().save(path, "PNG"))
                QMessageBox::warning(content, QStringLiteral("Skin"), QStringLiteral("No se pudo guardar la imagen."));
            else if (saved) saved(path);
        }
    });

    auto xart = new QPushButton(BetaShell::t("Open XArt editor", "Abrir editor XArt"), content);
    xart->setStyleSheet(buttonStyle);
    files->addWidget(xart);
    QObject::connect(xart, &QPushButton::clicked, content, [content] {
        const QString exe = QCoreApplication::applicationDirPath() + QStringLiteral("/xart/XArtSkinEditor.exe");
        if (!QFileInfo::exists(exe)) {
            QMessageBox::warning(content, BetaShell::t("XArt editor", "Editor XArt"),
                                 BetaShell::t("The XArt editor is not installed in this build.", "El editor XArt no está instalado en esta compilación."));
            return;
        }
        QProcess::startDetached(exe, {});
    });

    files->addStretch();
    layout->addLayout(files);
    layout->addWidget(BetaShell::hint(BetaShell::t("Editor and XArt by Gangstos (MIT license).",
                                                   "Editor y XArt de Gangstos (licencia MIT)."),
                                      content));
    layout->addStretch();
    return BetaShell::scrollPage(content, parent);
}

}  // namespace BetaSkins
