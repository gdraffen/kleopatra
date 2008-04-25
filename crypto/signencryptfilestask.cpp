/* -*- mode: c++; c-basic-offset:4 -*-
    crypto/signencryptfilestask.cpp

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

#include <config-kleopatra.h>

#include "signencryptfilestask.h"

#include <utils/input.h>
#include <utils/output.h>
#include <utils/stl_util.h>
#include <utils/kleo_assert.h>
#include <utils/exception.h>

#include <kleo/cryptobackendfactory.h>
#include <kleo/cryptobackend.h>
#include <kleo/signjob.h>
#include <kleo/signencryptjob.h>
#include <kleo/encryptjob.h>

#include <gpgme++/signingresult.h>
#include <gpgme++/encryptionresult.h>
#include <gpgme++/key.h>

#include <KLocalizedString>

#include <QFile>
#include <QPointer>
#include <QTextDocument> // for Qt::escape

#include <boost/bind.hpp>

using namespace Kleo;
using namespace Kleo::Crypto;
using namespace boost;
using namespace GpgME;

namespace {

    QString formatInputOutputLabel( const QString & input, const QString & output, bool inputDeleted, bool outputDeleted ) {
        return i18nc( "Input file --> Output file (rarr is arrow", "%1 &rarr; %2", 
                      inputDeleted ? QString("<s>%1</s>").arg( Qt::escape( input ) ) : Qt::escape( input ),
                      outputDeleted ? QString("<s>%1</s>").arg( Qt::escape( output ) ) : Qt::escape( output ) );
    }

    class ErrorResult : public Task::Result {
    public:
        ErrorResult( bool sign, bool encrypt, const Error & err, const QString & errStr, const QString & input, const QString & output ) 
            : Task::Result(), m_sign( sign ), m_encrypt( encrypt ), m_error( err ), m_errString( errStr ), m_inputLabel( input ), m_outputLabel( output ) {}

        /* reimp */ QString overview() const;
        /* reimp */ QString details() const;
        /* reimp */ int errorCode() const { return m_error.encodedError(); }
        /* reimp */ QString errorString() const { return m_errString; }
        /* reimp */ VisualCode code() const { return NeutralError; }

    private:
        const bool m_sign;
        const bool m_encrypt;
        const Error m_error;
        const QString m_errString;
        const QString m_inputLabel;
        const QString m_outputLabel;
    };
    
    class SignEncryptFilesResult : public Task::Result {
    public:
        SignEncryptFilesResult( const SigningResult & sr, const QString & input, const QString & output, bool inputRemoved, bool outputCreated )
            : Task::Result(), m_sresult( sr ), m_inputLabel( input ), m_outputLabel( output ), m_inputRemoved( inputRemoved ), m_outputCreated( outputCreated ) {
            
        }
        SignEncryptFilesResult( const EncryptionResult & er, const QString & input, const QString & output, bool inputRemoved, bool outputCreated  )
            : Task::Result(), m_eresult( er ), m_inputLabel( input ), m_outputLabel( output ), m_inputRemoved( inputRemoved ), m_outputCreated( outputCreated ) {}
        SignEncryptFilesResult( const SigningResult & sr, const EncryptionResult & er, const QString & input, const QString & output, bool inputRemoved, bool outputCreated )
            : Task::Result(), m_sresult( sr ), m_eresult( er ), m_inputLabel( input ), m_outputLabel( output ), m_inputRemoved( inputRemoved ), m_outputCreated( outputCreated ) {}

        /* reimp */ QString overview() const;
        /* reimp */ QString details() const;
        /* reimp */ int errorCode() const;
        /* reimp */ QString errorString() const;
        /* reimp */ VisualCode code() const;
        
    private:
        const SigningResult m_sresult;
        const EncryptionResult m_eresult;
        const QString m_inputLabel;
        const QString m_outputLabel;
        const bool m_inputRemoved;
        const bool m_outputCreated;
    };

    static QString makeSigningErrorOverview( const Error & err ) {
        if ( err.isCanceled() )
            return i18n("Signing canceled.");
        
        if ( err )
            return i18n("Signing failed." );
        return i18n("Signing succeeded.");
    }
    
    static QString makeErrorOverview( const SigningResult & result ) {
        return makeSigningErrorOverview( result.error() );
    }

    static QString makeEncryptionErrorOverview( const Error & err ) {
        if ( err.isCanceled() )
            return i18n("Encryption canceled.");
        
        if ( err )
            return i18n("Encryption failed." );

        return i18n("Encryption succeeded.");
    }

    static QString makeErrorOverview( const EncryptionResult & result ) {
        return makeEncryptionErrorOverview( result.error() );
    }

    static QString makeErrorDetails( const SigningResult & result ) {
        const Error err = result.error();
        if ( err )
            return Qt::escape( QString::fromLocal8Bit( err.asString() ) );
        return QString();
    }
    
    static QString makeErrorDetails( const EncryptionResult & result ) {
        const Error err = result.error();
        if ( err )
            return Qt::escape( QString::fromLocal8Bit( err.asString() ) );
        return i18n(" Encryption succeeded." );
    }
    
}


QString ErrorResult::overview() const {
    assert( m_error || m_error.isCanceled() );
    assert( m_sign || m_encrypt );
    const QString label = formatInputOutputLabel( m_inputLabel, m_outputLabel, false, true );
    const bool canceled = m_error.isCanceled();
    if ( m_sign && m_encrypt )
        return canceled ? i18n( "%1: <b>Sign/encrypt canceled.</b>", label ) : i18n( " %1: Sign/encrypt failed.", label );
    return i18nc( "label: result. Example: foo -> foo.gpg: Encryption failed.", "%1: <b>%2</b>", label, 
                  m_sign ? makeSigningErrorOverview( m_error ) :makeEncryptionErrorOverview( m_error ) );
}

QString ErrorResult::details() const {
    return m_errString;
}

class SignEncryptFilesTask::Private {
    friend class ::Kleo::Crypto::SignEncryptFilesTask;
    SignEncryptFilesTask * const q;
public:
    explicit Private( SignEncryptFilesTask * qq );

private:
    std::auto_ptr<Kleo::SignJob> createSignJob( GpgME::Protocol proto );
    std::auto_ptr<Kleo::SignEncryptJob> createSignEncryptJob( GpgME::Protocol proto );
    std::auto_ptr<Kleo::EncryptJob> createEncryptJob( GpgME::Protocol proto );
    shared_ptr<const Task::Result> makeErrorResult( const Error & err, const QString & errStr );

private:
    void slotResult( const SigningResult & );
    void slotResult( const SigningResult &, const EncryptionResult & );
    void slotResult( const EncryptionResult & );

private:
    shared_ptr<Input> input;
    shared_ptr<Output> output;
    QString inputFileName, outputFileName;
    std::vector<Key> signers;
    std::vector<Key> recipients;

    bool sign     : 1;
    bool encrypt  : 1;
    bool ascii    : 1;
    bool detached : 1;
    bool removeInput : 1;
    
    QPointer<Kleo::Job> job;
};

SignEncryptFilesTask::Private::Private( SignEncryptFilesTask * qq )
    : q( qq ),
      input(),
      output(),
      inputFileName(),
      outputFileName(),
      signers(),
      recipients(),
      sign( true ),
      encrypt( true ),
      ascii( true ),
      detached( false ),
      removeInput( false ),
      job( 0 )
{

}

shared_ptr<const Task::Result> SignEncryptFilesTask::Private::makeErrorResult( const Error & err, const QString & errStr )
{
    return shared_ptr<const ErrorResult>( new ErrorResult( sign, encrypt, err, errStr, input->label(), output->label() ) );
}

SignEncryptFilesTask::SignEncryptFilesTask( QObject * p )
    : Task( p ), d( new Private( this ) )
{

}

SignEncryptFilesTask::~SignEncryptFilesTask() {}

void SignEncryptFilesTask::setInputFileName( const QString & fileName ) {
    kleo_assert( !d->job );
    kleo_assert( !fileName.isEmpty() );
    d->inputFileName = fileName;
}

void SignEncryptFilesTask::setOutputFileName( const QString & fileName ) {
    kleo_assert( !d->job );
    kleo_assert( !fileName.isEmpty() );
    d->outputFileName = fileName;
}

void SignEncryptFilesTask::setSigners( const std::vector<Key> & signers ) {
    kleo_assert( !d->job );
    d->signers = signers;
}

void SignEncryptFilesTask::setRecipients( const std::vector<Key> & recipients ) {
    kleo_assert( !d->job );
    d->recipients = recipients;
}

void SignEncryptFilesTask::setSign( bool sign ) {
    kleo_assert( !d->job );
    d->sign = sign;
}

void SignEncryptFilesTask::setEncrypt( bool encrypt ) {
    kleo_assert( !d->job );
    d->encrypt = encrypt;
}

void SignEncryptFilesTask::setRemoveInputFileOnSuccess( bool remove )
{
    kleo_assert( !d->job );
    d->removeInput = remove;
}

void SignEncryptFilesTask::setAsciiArmor( bool ascii ) {
    kleo_assert( !d->job );
    d->ascii = ascii;
}

void SignEncryptFilesTask::setDetachedSignature( bool detached ) {
    kleo_assert( !d->job );
    d->detached = detached;
}

Protocol SignEncryptFilesTask::protocol() const {
    if ( d->sign && !d->signers.empty() )
        return d->signers.front().protocol();
    if ( d->encrypt )
        if ( !d->recipients.empty() )
            return d->recipients.front().protocol();
        else
            return GpgME::OpenPGP; // symmetric OpenPGP encryption
    throw Kleo::Exception( gpg_error( GPG_ERR_INTERNAL ),
                           i18n("Cannot determine protocol for task") );
}

QString SignEncryptFilesTask::label() const {
    return d->input ? d->input->label() : QString();
}

void SignEncryptFilesTask::doStart() {
    kleo_assert( !d->job );
    if ( d->sign )
        kleo_assert( !d->signers.empty() );

    d->input = Input::createFromFile( d->inputFileName );
    d->output = Output::createFromFile( d->outputFileName, false );

    if ( d->encrypt )
        if ( d->sign ) {
            std::auto_ptr<Kleo::SignEncryptJob> job = d->createSignEncryptJob( protocol() );
            kleo_assert( job.get() );

            job->start( d->signers, d->recipients,
                        d->input->ioDevice(), d->output->ioDevice(), true );

            d->job = job.release();
        } else {
            std::auto_ptr<Kleo::EncryptJob> job = d->createEncryptJob( protocol() );
            kleo_assert( job.get() );

            job->start( d->recipients, d->input->ioDevice(), d->output->ioDevice(), true );

            d->job = job.release();
        }
    else
        if ( d->sign ) {
            std::auto_ptr<Kleo::SignJob> job = d->createSignJob( protocol() );
            kleo_assert( job.get() );

            job->start( d->signers,
                        d->input->ioDevice(), d->output->ioDevice(),
                        d->detached ? GpgME::Detached : GpgME::NormalSignatureMode );

            d->job = job.release();
        } else {
            kleo_assert( !"Either 'sign' or 'encrypt' must be set!" );
        }
}

void SignEncryptFilesTask::cancel() {
    if ( d->job )
        d->job->slotCancel();
}

std::auto_ptr<Kleo::SignJob> SignEncryptFilesTask::Private::createSignJob( GpgME::Protocol proto ) {
    const CryptoBackend::Protocol * const backend = CryptoBackendFactory::instance()->protocol( proto );
    kleo_assert( backend );
    std::auto_ptr<Kleo::SignJob> signJob( backend->signJob( /*armor=*/true, /*textmode=*/false ) );
    kleo_assert( signJob.get() );
    connect( signJob.get(), SIGNAL(progress(QString,int,int)),
             q, SLOT(setProgress(QString,int,int)) );
    connect( signJob.get(), SIGNAL(result(GpgME::SigningResult,QByteArray)),
             q, SLOT(slotResult(GpgME::SigningResult)) );
    return signJob;
}

std::auto_ptr<Kleo::SignEncryptJob> SignEncryptFilesTask::Private::createSignEncryptJob( GpgME::Protocol proto ) {
    const CryptoBackend::Protocol * const backend = CryptoBackendFactory::instance()->protocol( proto );
    kleo_assert( backend );
    std::auto_ptr<Kleo::SignEncryptJob> signEncryptJob( backend->signEncryptJob( /*armor=*/true, /*textmode=*/false ) );
    kleo_assert( signEncryptJob.get() );
    connect( signEncryptJob.get(), SIGNAL(progress(QString,int,int)),
             q, SLOT(setProgress(QString,int,int)) );
    connect( signEncryptJob.get(), SIGNAL(result(GpgME::SigningResult,GpgME::EncryptionResult,QByteArray)),
             q, SLOT(slotResult(GpgME::SigningResult,GpgME::EncryptionResult)) );
    return signEncryptJob;
}

std::auto_ptr<Kleo::EncryptJob> SignEncryptFilesTask::Private::createEncryptJob( GpgME::Protocol proto ) {
    const CryptoBackend::Protocol * const backend = CryptoBackendFactory::instance()->protocol( proto );
    kleo_assert( backend );
    std::auto_ptr<Kleo::EncryptJob> encryptJob( backend->encryptJob( /*armor=*/true, /*textmode=*/false ) );
    kleo_assert( encryptJob.get() );
    connect( encryptJob.get(), SIGNAL(progress(QString,int,int)),
             q, SLOT(setProgress(QString,int,int)) );
    connect( encryptJob.get(), SIGNAL(result(GpgME::EncryptionResult,QByteArray)),
             q, SLOT(slotResult(GpgME::EncryptionResult)) );
    return encryptJob;
}

void SignEncryptFilesTask::Private::slotResult( const SigningResult & result ) {
    bool inputRemoved = false;
    bool outputCreated = false;
    if ( result.error().code() ) {
        output->cancel();
    } else {
        try {
            output->finalize();
            outputCreated = true;
            if ( removeInput ) {
                inputRemoved = QFile::remove( inputFileName );
            }
        } catch ( const GpgME::Exception & e ) {
            q->emitResult( makeErrorResult( e.error(), QString::fromLocal8Bit( e.what() ) ) );
            return;
        }
    }
   
    q->emitResult( shared_ptr<Result>( new SignEncryptFilesResult( result, input->label(), output->label(), inputRemoved, outputCreated ) ) );
}

void SignEncryptFilesTask::Private::slotResult( const SigningResult & sresult, const EncryptionResult & eresult ) {
    bool inputRemoved = false;
    bool outputCreated = false;
    if ( sresult.error().code() || eresult.error().code() ) {
        output->cancel();
    } else {
        try {
            output->finalize();
            outputCreated = true;
            if ( removeInput ) {
                inputRemoved = QFile::remove( inputFileName );
            }
        } catch ( const GpgME::Exception & e ) {
            q->emitResult( makeErrorResult( e.error(), QString::fromLocal8Bit( e.what() ) ) );
            return;
        }
    }
    
    q->emitResult( shared_ptr<Result>( new SignEncryptFilesResult( sresult, eresult, input->label(), output->label(), inputRemoved, outputCreated ) ) );
}

void SignEncryptFilesTask::Private::slotResult( const EncryptionResult & result ) {
    bool inputRemoved = false;
    bool outputCreated = false;
    if ( result.error().code() ) {
        output->cancel();
    } else {
        try {
            output->finalize();
            outputCreated = true;
            if ( removeInput ) {
                inputRemoved = QFile::remove( inputFileName );
            }
        } catch ( const GpgME::Exception & e ) {
            q->emitResult( makeErrorResult( e.error(), QString::fromLocal8Bit( e.what() ) ) );
            return;
        }
    }
    q->emitResult( shared_ptr<Result>( new SignEncryptFilesResult( result, input->label(), output->label(), inputRemoved, outputCreated ) ) );
}

QString SignEncryptFilesResult::overview() const {
    const bool sign = !m_sresult.isNull();
    const bool encrypt = !m_eresult.isNull();
 
    kleo_assert( sign || encrypt );

    const QString files = formatInputOutputLabel( m_inputLabel, m_outputLabel, m_inputRemoved, !m_outputCreated );
   
    return files + ": " + makeOverview( sign ? makeErrorOverview( m_sresult ) : makeErrorOverview( m_eresult ) );
}

QString SignEncryptFilesResult::details() const {
    return errorString();
}

int SignEncryptFilesResult::errorCode() const {
   if ( m_sresult.error().code() )
       return m_sresult.error().encodedError();
   if ( m_eresult.error().code() )
       return m_eresult.error().encodedError();
   return 0;
}

QString SignEncryptFilesResult::errorString() const {
    const bool sign = !m_sresult.isNull();
    const bool encrypt = !m_eresult.isNull();
    
    kleo_assert( sign || encrypt );

    if ( sign && encrypt ) {
        return 
            m_sresult.error().code() ? makeErrorDetails( m_sresult ) : 
            m_eresult.error().code() ? makeErrorDetails( m_eresult ) :
            QString();
    }
    
    return sign ? makeErrorDetails( m_sresult ) : makeErrorDetails( m_eresult );
}

Task::Result::VisualCode SignEncryptFilesResult::code() const
{
    if ( m_sresult.error().isCanceled() || m_eresult.error().isCanceled() )
        return Warning;
    return ( m_sresult.error().code() || m_eresult.error().code() ) ? NeutralError : NeutralSuccess;
}

#include "moc_signencryptfilestask.cpp"
