/* -*- mode: c++; c-basic-offset:4 -*-
    crypto/gui/decryptverifyresultwidget.h

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2007 Klarälvdalens Datakonsult AB

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

#ifndef __KLEOPATRA_CRYPTO_GUI_DECRYPTVERIFYRESULTWIDGET_H__
#define __KLEOPATRA_CRYPTO_GUI_DECRYPTVERIFYRESULTWIDGET_H__

#include <crypto/gui/resultdisplaywidget.h>

#include <vector>

namespace GpgME {
    class DecryptionResult;
    class VerificationResult;
    class Signature;
    class Key;
}

class QVBoxLayout;
class QLabel;

namespace Kleo {
namespace Crypto {
namespace Gui {

    class DecryptVerifyResultWidget : public ResultDisplayWidget {
        Q_OBJECT
    public:
        explicit DecryptVerifyResultWidget( QWidget * parent );
        ~DecryptVerifyResultWidget();

        void setResult( const GpgME::DecryptionResult & decryptionResult, const GpgME::VerificationResult & verificationResult );

    private:
        QString formatDecryptionResult( const GpgME::DecryptionResult &, const std::vector<GpgME::Key> & );
        QString formatVerificationResult( const GpgME::VerificationResult & ) const;
        QString formatSignature( const GpgME::Signature &, const GpgME::Key & );
        QLabel * formatSignatureWidget( QLabel *, const GpgME::Signature &, const GpgME::Key & );

    private:
        QVBoxLayout * m_box;
    };

}
}
}

#endif /* __KLEOPATRA_CRYPTO_GUI_DECRYPTVERIFYRESULTWIDGET_H__ */