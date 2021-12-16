#include "QMLClipboardAdapter.hpp"
#include <QClipboard>
#include <QGuiApplication>

//==============================================================================

QMLClipboardAdapter::QMLClipboardAdapter(QObject* parent)
    : QObject(parent)
    , _clipboard(QGuiApplication::clipboard())
{
}

//==============================================================================

void QMLClipboardAdapter::setText(QString text)
{
    _clipboard->setText(text, QClipboard::Clipboard);
    _clipboard->setText(text, QClipboard::Selection);
}

//==============================================================================
