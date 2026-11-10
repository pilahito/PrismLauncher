#include "NewsDialog.h"

#include "Application.h"
#include "settings/SettingsObject.h"

#include "ui_NewsDialog.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidgetItem>
#include <QLocale>
#include <QProcess>
#include <QTextDocument>

namespace {

// Idioma efectivo: el de Ajustes o, si está en automático, el del sistema.
bool wantsSpanish()
{
    QString lang = APPLICATION->settings()->get("Language").toString();
    if (lang.isEmpty())
        lang = QLocale::system().name();
    return lang.startsWith(QLatin1String("es"), Qt::CaseInsensitive);
}

// Traduce textos de inglés a español con el traductor sin conexión de la beta.
// Si no está disponible (sin Python, sin modelo o error), devuelve los textos originales.
QStringList translateToSpanish(const QStringList& texts)
{
    const QString script = QCoreApplication::applicationDirPath() + QStringLiteral("/traductor/traducir.py");
    if (texts.isEmpty() || !QFileInfo::exists(script))
        return texts;

    QJsonArray array;
    for (const QString& text : texts)
        array.append(text);
    const QJsonObject request{ { QStringLiteral("textos"), array } };

    QProcess process;
    process.start(QStringLiteral("python"), { script });
    if (!process.waitForStarted(5000))
        return texts;
    process.write(QJsonDocument(request).toJson(QJsonDocument::Compact));
    process.closeWriteChannel();
    if (!process.waitForFinished(120000) || process.exitCode() != 0)
        return texts;

    const QJsonArray translated = QJsonDocument::fromJson(process.readAllStandardOutput()).object().value(QStringLiteral("textos")).toArray();
    if (translated.size() != texts.size())
        return texts;

    QStringList result;
    for (const QJsonValue& value : translated)
        result << value.toString();
    return result;
}

// Contenido del artículo: en español se traduce párrafo a párrafo; en otro idioma se deja tal cual.
QString articleHtml(const QString& content)
{
    if (!wantsSpanish())
        return content;

    QTextDocument document;
    document.setHtml(content);
    const QStringList paragraphs = document.toPlainText().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const QStringList translated = translateToSpanish(paragraphs);
    if (translated == paragraphs)
        return content;

    QString html;
    for (const QString& paragraph : translated)
        html += QStringLiteral("<p>%1</p>").arg(paragraph.toHtmlEscaped());
    return html;
}

}  // namespace

NewsDialog::NewsDialog(QList<NewsEntryPtr> entries, QWidget* parent) : QDialog(parent), ui(new Ui::NewsDialog())
{
    ui->setupUi(this);

    // Los titulares se muestran traducidos, pero se buscan siempre por el título original.
    QStringList originals;
    for (const auto& entry : entries)
        originals << entry->title;
    const QStringList shown = wantsSpanish() ? translateToSpanish(originals) : originals;

    for (int i = 0; i < entries.size(); ++i) {
        auto item = new QListWidgetItem(shown[i]);
        item->setData(Qt::UserRole, originals[i]);
        ui->articleListWidget->addItem(item);
        m_entries.insert(originals[i], entries[i]);
    }

    connect(ui->articleListWidget, &QListWidget::currentTextChanged, this, &NewsDialog::selectedArticleChanged);
    connect(ui->toggleListButton, &QPushButton::clicked, this, &NewsDialog::toggleArticleList);

    m_article_list_hidden = ui->articleListWidget->isHidden();

    auto first_item = ui->articleListWidget->item(0);
    first_item->setSelected(true);

    auto article_entry = m_entries.constFind(first_item->data(Qt::UserRole).toString()).value();
    ui->articleTitleLabel->setText(QString("<a href='%1'>%2</a>").arg(article_entry->link, first_item->text()));

    ui->currentArticleContentBrowser->setText(articleHtml(article_entry->content));
    ui->currentArticleContentBrowser->flush();

    connect(this, &QDialog::finished, this, [this] {
        APPLICATION->settings()->set("NewsGeometry", QString::fromUtf8(saveGeometry().toBase64()));
    });
    const QByteArray base64Geometry = APPLICATION->settings()->get("NewsGeometry").toString().toUtf8();
    restoreGeometry(QByteArray::fromBase64(base64Geometry));
}

NewsDialog::~NewsDialog()
{
    delete ui;
}

void NewsDialog::selectedArticleChanged(const QString& new_title)
{
    // El título visible puede estar traducido; la entrada se busca por el original guardado en el ítem.
    const auto* current = ui->articleListWidget->currentItem();
    const QString original = current ? current->data(Qt::UserRole).toString() : new_title;
    auto article_entry = m_entries.constFind(original).value();

    ui->articleTitleLabel->setText(QString("<a href='%1'>%2</a>").arg(article_entry->link, new_title));

    ui->currentArticleContentBrowser->setText(articleHtml(article_entry->content));
    ui->currentArticleContentBrowser->flush();
}

void NewsDialog::toggleArticleList()
{
    m_article_list_hidden = !m_article_list_hidden;

    ui->articleListWidget->setHidden(m_article_list_hidden);

    if (m_article_list_hidden)
        ui->toggleListButton->setText(tr("Show article list"));
    else
        ui->toggleListButton->setText(tr("Hide article list"));
}
