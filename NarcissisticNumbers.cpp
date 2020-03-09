// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "NarcissisticNumbers.h"
#include "NarcissisticNumCalculator.h"
#include "utils.h"
#include "SpinBox64.h"
#include "SpinBox64U.h"

#include "ui_NarcissisticNumbers.h"

#include <QStringListModel>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <unordered_map>
#include <thread>
#include <limits>
#include <QLocale>
#include <QProgressDialog>
#include <QCloseEvent>
#include <QProgressBar>

CNarcissisticNumbers::CNarcissisticNumbers( QWidget* parent )
    : QDialog( parent ),
    fImpl( new Ui::CNarcissisticNumbers )
{
    fImpl->setupUi( this );
    fImpl->maxRange->setMaximum( CSpinBox64U::maxAllowed() );
    fImpl->minRange->setMaximum( CSpinBox64U::maxAllowed() );
    fImpl->numPerThread->setMaximum( CSpinBox64::maxAllowed() );
    fImpl->maxLabel->setText( tr( "Maximum: %1").arg( locale().toString( CSpinBox64U::maxAllowed() ) ) );
    //setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );

    (void)connect( fImpl->byRange, &QAbstractButton::clicked, this, [this](){ slotChanged(); } );
    (void)connect( fImpl->byNumbers, &QAbstractButton::clicked, this, [ this ]() { slotChanged(); } );
    (void)connect( fImpl->run, &QAbstractButton::clicked, this, [ this ]() { slotRun(); } );
    (void)connect( fImpl->reset, &QAbstractButton::clicked, this, [ this ]() { slotReset(); } );
    (void)connect( fImpl->maxRange, static_cast<void (CSpinBox64U::*)( uint64_t )>( &CSpinBox64U::valueChanged ), this, [ this ]() { slotRangeChanged(); } );
    (void)connect( fImpl->minRange, static_cast<void (CSpinBox64U::*)( uint64_t )>( &CSpinBox64U::valueChanged ), this, [ this ]() { slotRangeChanged(); } );
    (void)connect( fImpl->base, static_cast<void ( QSpinBox::* )( int )>( &QSpinBox::valueChanged ), fImpl->minRange, &CSpinBox64U::setDisplayIntegerBase );
    (void)connect( fImpl->base, static_cast<void ( QSpinBox::* )( int )>( &QSpinBox::valueChanged ), fImpl->maxRange, &CSpinBox64U::setDisplayIntegerBase ) ;

    fImpl->numCores->setText( QString::number( std::thread::hardware_concurrency() ) );
    loadSettings();
    setFocus( Qt::MouseFocusReason );
    slotChanged();
    fMonitorTimer = new QTimer( this );
    fMonitorTimer->setSingleShot( false );
    fMonitorTimer->setInterval( 50 );
    (void)connect( fMonitorTimer, &QTimer::timeout, this, [ this ](){ slotShowResults(); } );
}

CNarcissisticNumbers::~CNarcissisticNumbers()
{
    saveSettings();
}

void CNarcissisticNumbers::closeEvent( QCloseEvent * e )
{
    if ( fCalculator && !fCalculator->isFinished( nullptr ) )
    {
        e->ignore();
        fCalculator->setStopped( true );
        QTimer::singleShot( 0, this, &QDialog::close );
    }
    else
        e->accept();
}

void CNarcissisticNumbers::loadSettings()
{
    QSettings settings;
    fImpl->base->setValue( CNarcissisticNumCalculatorDefaults::base() );
    fImpl->numThreads->setValue( CNarcissisticNumCalculatorDefaults::numThreads() );
    fImpl->numPerThread->setValue( CNarcissisticNumCalculatorDefaults::numPerThread() );

    fImpl->byRange->setChecked( CNarcissisticNumCalculatorDefaults::byRange() );
    fImpl->byNumbers->setChecked( !CNarcissisticNumCalculatorDefaults::byRange() );
    fImpl->minRange->setValue( CNarcissisticNumCalculatorDefaults::range().first );
    fImpl->maxRange->setValue( CNarcissisticNumCalculatorDefaults::range().second );

    setNumbersList( CNarcissisticNumCalculatorDefaults::numbersList()  );
}

void CNarcissisticNumbers::saveSettings() const
{
    QSettings settings;
    CNarcissisticNumCalculatorDefaults::setBase( fImpl->base->value() );
    CNarcissisticNumCalculatorDefaults::setNumThreads( fImpl->numThreads->value() );
    CNarcissisticNumCalculatorDefaults::setNumPerThread( fImpl->numPerThread->value() );

    CNarcissisticNumCalculatorDefaults::setByRange( fImpl->byRange->isChecked() );
    CNarcissisticNumCalculatorDefaults::setRange( std::make_pair( fImpl->minRange->value(), fImpl->maxRange->value() ) );
    CNarcissisticNumCalculatorDefaults::setNumbersList( getNumbersList() );
}

void CNarcissisticNumbers::setNumbersList( const std::list< uint64_t >& numbers )
{
    QStringList tmp;
    for ( auto&& ii : numbers )
        tmp << QString::number( ii );
    fImpl->numList->setText( tmp.join( " " ) );
}

std::list< uint64_t > CNarcissisticNumbers::getNumbersList() const
{
    auto tmp = fImpl->numList->text().split( " ", QString::SkipEmptyParts );
    std::list< uint64_t > numbers;
    for ( auto&& ii : tmp )
    {
        bool aOK;
        auto curr = ii.toLongLong( &aOK );
        if ( aOK )
            numbers.push_back( curr );
    }
    return numbers;
}

void CNarcissisticNumbers::slotChanged()
{
    fImpl->minRange->setEnabled( fImpl->byRange->isChecked() );
    fImpl->maxRange->setEnabled( fImpl->byRange->isChecked() );
    fImpl->numList->setEnabled( fImpl->byNumbers->isChecked() );
}

void CNarcissisticNumbers::slotReset()
{
    CNarcissisticNumCalculatorDefaults::reset();
    loadSettings();
}

void CNarcissisticNumbers::slotRun()
{
    fCalculator.reset( nullptr );
    fNumPartitions = 0;
    fFinishedCount = 0;
    slotShowResults();

    fCalculator.reset( new CNarcissisticNumCalculator( false ) );

    fCalculator->init();
    fCalculator->setBase( fImpl->base->value() );
    fCalculator->setNumThreads( fImpl->numThreads->value() );
    fCalculator->setNumPerThread( fImpl->numPerThread->value() );
    fCalculator->setByRange( fImpl->byRange->isChecked() );
    fCalculator->setRange( std::make_pair( fImpl->minRange->value(), fImpl->maxRange->value() ) );
    fCalculator->setNumbersList( getNumbersList() );

    slotShowResults();
    if ( !fProgress )
    {
        fProgress = new QProgressDialog( this );
        fProgress->setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );
        fProgress->setWindowTitle( tr( "Computing" ) );
        auto bar = new QProgressBar;
        bar->setFormat( "%p% (%v of %m)" );
        fProgress->setBar( bar );
        fProgress->setMinimumDuration( 100 );
    }

    fProgress->setLabelText( "Launching Threads" );
    fProgress->setCancelButtonText( "Cancel" );
    CNarcissisticNumCalculator::TReportFunctionType launchReport = [ this ](uint64_t min, uint64_t max, uint64_t curr )
        { 
            fProgress->setMinimum( min );
            fProgress->setMaximum( max );
            fProgress->setValue( curr );
            qApp->processEvents( QEventLoop::ProcessEventsFlag::AllEvents );
            return !fProgress->wasCanceled();
        };

    fCalculator->launch( launchReport, true );
    
    if ( !fProgress->wasCanceled() )
    {
        fProgress->setLabelText( "Partitioning Numbers" );
        fProgress->show();
        CNarcissisticNumCalculator::TReportFunctionType partitionReport = [ this ]( uint64_t min, uint64_t max, uint64_t curr )
        {
            fProgress->setMinimum( min );
            fProgress->setMaximum( max );
            fProgress->setValue( curr );
            qApp->processEvents( QEventLoop::ProcessEventsFlag::AllEvents );
            return !fProgress->wasCanceled();
        };
        fNumPartitions = fCalculator->partition( partitionReport, true );
    }

    if ( !fProgress->wasCanceled() )
    {
        fProgress->setLabelText( "Finding Narcissistic" );
        fProgress->setMinimum( 0 );
        fProgress->setMaximum( fNumPartitions );
        fProgress->setValue( static_cast< int >( fNumPartitions - fCalculator->numPartitions() ) );
        fProgress->show();

        fMonitorTimer->start();
        fFinishedCount = 0;
    }
    else
    {
        fCalculator->setStopped( true );
        updateUI( true );
    }
}

void CNarcissisticNumbers::slotShowResults()
{
    std::string results;
    bool finished = false;

    if ( fCalculator )
    {
        std::tie( results, finished ) = fCalculator->currentResults();
        if ( fProgress )
        {
            fProgress->setValue( static_cast<int>( fNumPartitions - fCalculator->numPartitions() ) );
            if ( fProgress->wasCanceled() )
            {
                fCalculator->setStopped( true );
            }
            else
            {
                auto runningResults = QString::fromStdString( fCalculator->getRunningResults() );
                fProgress->setLabelText( tr( "Finding Narcissistic\n%1" ).arg( runningResults ) );
            }
        }
    }
    fImpl->results->setText( QString::fromStdString( results ) );

    updateUI( finished );

    if ( finished )
    {
        fFinishedCount++;
        auto tmp = fProgress;
        fProgress = nullptr;
        delete tmp;
    }
    if ( fFinishedCount >= 3 )
    {
        fMonitorTimer->stop();
    }

    qApp->processEvents( QEventLoop::ProcessEventsFlag::AllEvents );
}

void CNarcissisticNumbers::updateUI( bool finished )
{
    fImpl->base->setEnabled( finished );
    fImpl->numThreads->setEnabled( finished );
    fImpl->numPerThread->setEnabled( finished );
    fImpl->byRange->setEnabled( finished );
    fImpl->byNumbers->setEnabled( finished );
    fImpl->minRange->setEnabled( finished );
    fImpl->maxRange->setEnabled( finished );
    fImpl->numList->setEnabled( finished );
    fImpl->run->setEnabled( finished );
    if ( finished )
        slotChanged();
}


void CNarcissisticNumbers::slotRangeChanged()
{
    auto rangeSize = fImpl->maxRange->value() - fImpl->minRange->value();
    auto currSender = dynamic_cast<QWidget*>( sender() );
    if ( rangeSize < 1 )
    {
        if ( currSender == fImpl->maxRange )
            fImpl->maxRange->setValue( fImpl->minRange->value() + 1 );
        else
            fImpl->minRange->setValue( fImpl->maxRange->value() - 1 );
    }
    else if ( rangeSize > 100000 )
    {
        fImpl->numPerThread->setValue( rangeSize / 10000 );
    }
}

