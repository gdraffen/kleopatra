/* -*- mode: c++; c-basic-offset:4 -*-
    crypto/gui/resultpage.cpp

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2008 Klarälvdalens Datakonsult AB

    Kleopatra is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config-kleopatra.h>

#include "resultpage.h"
#include "resultlistwidget.h"
#include "resultitemwidget.h"

#include <crypto/taskcollection.h>

#include <utils/scrollarea.h>

#include <KLocalizedString>

#include <QCheckBox>
#include <QHash>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>


using namespace Kleo;
using namespace Kleo::Crypto;
using namespace Kleo::Crypto::Gui;

class ResultPage::Private
{
    ResultPage *const q;
public:
    explicit Private(ResultPage *qq);

    void progress(const QString &msg, int progress, int total);
    void result(const std::shared_ptr<const Task::Result> &result);
    void started(const std::shared_ptr<Task> &result);
    void allDone();
    void keepOpenWhenDone(bool keep);
    QLabel *labelForTag(const QString &tag);

    std::shared_ptr<TaskCollection> m_tasks;
    QProgressBar *m_progressBar;
    QHash<QString, QLabel *> m_progressLabelByTag;
    QVBoxLayout *m_progressLabelLayout;
    int m_lastErrorItemIndex;
    ResultListWidget *m_resultList;
    QCheckBox *m_keepOpenCB;
};

ResultPage::Private::Private(ResultPage *qq) : q(qq), m_lastErrorItemIndex(0)
{
    QBoxLayout *const layout = new QVBoxLayout(q);
    QWidget *const labels = new QWidget;
    m_progressLabelLayout = new QVBoxLayout(labels);
    layout->addWidget(labels);
    m_progressBar = new QProgressBar;
    layout->addWidget(m_progressBar);
    m_resultList = new ResultListWidget;
    layout->addWidget(m_resultList);
    m_keepOpenCB = new QCheckBox;
    m_keepOpenCB->setText(i18n("Keep open after operation completed"));
    m_keepOpenCB->setChecked(true);
    connect(m_keepOpenCB, &QAbstractButton::toggled, q, &ResultPage::keepOpenWhenDone);
    layout->addWidget(m_keepOpenCB);
}

void ResultPage::Private::progress(const QString &msg, int progress, int total)
{
    Q_UNUSED(msg);
    Q_ASSERT(progress >= 0);
    Q_ASSERT(total >= 0);
    m_progressBar->setRange(0, total);
    m_progressBar->setValue(progress);
}

void ResultPage::Private::keepOpenWhenDone(bool)
{
}

void ResultPage::Private::allDone()
{
    Q_ASSERT(m_tasks);
    q->setAutoAdvance(!m_keepOpenCB->isChecked() && !m_tasks->errorOccurred());
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(100);
    m_tasks.reset();
    Q_FOREACH (const QString &i, m_progressLabelByTag.keys()) {
        if (!i.isEmpty()) {
            m_progressLabelByTag.value(i)->setText(i18n("%1: All operations completed.", i));
        } else {
            m_progressLabelByTag.value(i)->setText(i18n("All operations completed."));
        }
    }
    Q_EMIT q->completeChanged();
}

void ResultPage::Private::result(const std::shared_ptr<const Task::Result> &)
{
}

void ResultPage::Private::started(const std::shared_ptr<Task> &task)
{
    Q_ASSERT(task);
    const QString tag = task->tag();
    QLabel *const label = labelForTag(tag);
    Q_ASSERT(label);
    if (tag.isEmpty()) {
        label->setText(i18nc("number, operation description", "Operation %1: %2", m_tasks->numberOfCompletedTasks() + 1, task->label()));
    } else {
        label->setText(i18nc("tag( \"OpenPGP\" or \"CMS\"),  operation description", "%1: %2", tag, task->label()));
    }
}

ResultPage::ResultPage(QWidget *parent, Qt::WindowFlags flags) : WizardPage(parent, flags), d(new Private(this))
{
    setTitle(i18n("<b>Results</b>"));
}

ResultPage::~ResultPage()
{
}

bool ResultPage::keepOpenWhenDone() const
{
    return d->m_keepOpenCB->isChecked();
}

void ResultPage::setKeepOpenWhenDone(bool keep)
{
    d->m_keepOpenCB->setChecked(keep);
}

void ResultPage::setTaskCollection(const std::shared_ptr<TaskCollection> &coll)
{
    Q_ASSERT(!d->m_tasks);
    if (d->m_tasks == coll) {
        return;
    }
    d->m_tasks = coll;
    Q_ASSERT(d->m_tasks);
    d->m_resultList->setTaskCollection(coll);
    connect(d->m_tasks.get(), SIGNAL(progress(QString,int,int)),
            this, SLOT(progress(QString,int,int)));
    connect(d->m_tasks.get(), SIGNAL(done()),
            this, SLOT(allDone()));
    connect(d->m_tasks.get(), SIGNAL(result(std::shared_ptr<const Kleo::Crypto::Task::Result>)),
            this, SLOT(result(std::shared_ptr<const Kleo::Crypto::Task::Result>)));
    connect(d->m_tasks.get(), SIGNAL(started(std::shared_ptr<Kleo::Crypto::Task>)),
            this, SLOT(started(std::shared_ptr<Kleo::Crypto::Task>)));

    Q_FOREACH (const std::shared_ptr<Task> &i, d->m_tasks->tasks()) {    // create labels for all tags in collection
        Q_ASSERT(i && d->labelForTag(i->tag()));
        Q_UNUSED(i);
    }
    Q_EMIT completeChanged();
}

QLabel *ResultPage::Private::labelForTag(const QString &tag)
{
    if (QLabel *const label = m_progressLabelByTag.value(tag)) {
        return label;
    }
    QLabel *label = new QLabel;
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    m_progressLabelLayout->addWidget(label);
    m_progressLabelByTag.insert(tag, label);
    return label;
}

bool ResultPage::isComplete() const
{
    return d->m_tasks ? d->m_tasks->allTasksCompleted() : true;
}

#include "moc_resultpage.cpp"
