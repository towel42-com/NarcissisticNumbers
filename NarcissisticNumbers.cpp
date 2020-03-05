// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright noticeand this permission notice shall be included in
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

#include "ui_NarcissisticNumbers.h"

#include <QStringListModel>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <unordered_map>
#include <thread>

CNarcissisticNumbers::CNarcissisticNumbers( QWidget* parent )
    : QDialog( parent ),
    fImpl( new Ui::CNarcissisticNumbers )
{
    fImpl->setupUi( this );
    //setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );

    (void)connect( fImpl->byRange, &QAbstractButton::clicked, this, [this](){ slotChanged(); } );
    (void)connect( fImpl->byNumbers, &QAbstractButton::clicked, this, [ this ]() { slotChanged(); } );
    (void)connect( fImpl->run, &QAbstractButton::clicked, this, [ this ]() { slotRun(); } );

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

void CNarcissisticNumbers::setNumbersList( const std::list< int64_t >& numbers )
{
    QStringList tmp;
    for ( auto&& ii : numbers )
        tmp << QString::number( ii );
    fImpl->numList->setText( tmp.join( " " ) );
}

std::list< int64_t > CNarcissisticNumbers::getNumbersList() const
{
    auto tmp = fImpl->numList->text().split( " ", QString::SkipEmptyParts );
    std::list< int64_t > numbers;
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

void CNarcissisticNumbers::slotRun()
{
    fCalculator.reset( nullptr );
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
    fCalculator->launch( false );
    slotShowResults();
    fCalculator->partition( false );
    slotShowResults();

    fMonitorTimer->start();
    fFinishedCount = 0;
}

void CNarcissisticNumbers::slotShowResults()
{
    auto result = tr( "Results:" );
    if ( fCalculator )
    {
        result += "\n============================================\n";
        bool finished = fCalculator->isFinished( nullptr );

        result += tr( "Run Time: %1\n" ).arg( QString::fromStdString( NUtils::getTimeString( std::chrono::system_clock::now() - fCalculator->startTime(), true, true ) ) );
        result += tr( "Number of Partitions Remaining: %1\n" ).arg( fCalculator->numPartitions() );
        result += tr( "Number of Threads Remaining: %1\n" ).arg( fCalculator->numThreads() );

        auto numbers = fCalculator->results();
        result += tr( "Number of Narcissistic Numbers Found: %1\n" ).arg( numbers.size() );
        result += getNumberList( numbers );

        updateUI( finished );

        if ( finished )
            fFinishedCount++;
        if ( fFinishedCount >= 3 )
            fMonitorTimer->stop();
    }
    fImpl->results->setText( result );
    qApp->processEvents( QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents );
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

QString CNarcissisticNumbers::getNumberList( const std::list<int64_t>& numbers ) const
{
    QString retVal;
    bool first = true;
    size_t ii = 0;
    auto base = fImpl->base->value();
    for ( auto&& currVal : numbers )
    {
        if ( ii && ( ii % 5 == 0 ) )
        {
            retVal += "\n";
            first = true;
        }
        if ( !first )
            retVal += ", ";
        else
            retVal += "    ";
        first = false;
        retVal += QString::fromStdString( NUtils::toString( currVal, base ) );
        if ( base != 10 )
            retVal += tr( "(=%1)" ).arg( currVal );
        ii++;
    }
    return retVal;
}
