/* -*- mode: c++; c-basic-offset:4 -*-
    conf/smimevalidationconfigurationpage.cpp

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

#include "smimevalidationconfigurationpage.h"
#include "smimevalidationconfigurationwidget.h"

#include <QVBoxLayout>
#include <kmessagebox.h>

using namespace Kleo::Config;

SMimeValidationConfigurationPage::SMimeValidationConfigurationPage(QWidget *parent, const QVariantList &args)
    : KCModule(parent, args)
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setMargin(0);

    mWidget = new SMimeValidationConfigurationWidget(this);
    lay->addWidget(mWidget);

    connect(mWidget, &SMimeValidationConfigurationWidget::changed, this, QOverload<>::of(&Kleo::Config::SMimeValidationConfigurationPage::changed));

    load();
}

void SMimeValidationConfigurationPage::load()
{
    mWidget->load();
}

void SMimeValidationConfigurationPage::save()
{
    mWidget->save();

}

void SMimeValidationConfigurationPage::defaults()
{
    mWidget->defaults();
}

extern "C" Q_DECL_EXPORT KCModule *create_kleopatra_config_smimevalidation(QWidget *parent, const QVariantList &args)
{
    SMimeValidationConfigurationPage *page =
        new SMimeValidationConfigurationPage(parent, args);
    page->setObjectName(QStringLiteral("kleopatra_config_smimevalidation"));
    return page;
}

