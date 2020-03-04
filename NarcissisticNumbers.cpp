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
#include "utils.h"

#include "ui_NarcissisticNumbers.h"

#include <QStringListModel>
#include <QKeyEvent>
#include <QMessageBox>
#include <unordered_map>


class CStringListModel : public QStringListModel
{
public:
    CStringListModel( QObject* parent ) :
        QStringListModel( parent )
    {
    }

    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override
    {
        if ( role == Qt::TextAlignmentRole )
            return static_cast<int>( Qt::AlignVCenter | Qt::AlignRight );
        return QStringListModel::data( index, role );
    }
};

CNarcissisticNumbers::CNarcissisticNumbers( QWidget* parent )
    : QDialog( parent ),
    fImpl( new Ui::CNarcissisticNumbers )
{
    fImpl->setupUi( this );
    //setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );

    (void)connect( fModel, &QAbstractItemModel::dataChanged, this, [ this ]() { slotDataChanged(); } );
    (void)connect( fModel, &QAbstractItemModel::modelReset, this, [ this ]() { slotDataChanged(); } );
    (void)connect( fModel, &QAbstractItemModel::rowsInserted, this, [ this ]() { slotDataChanged(); } );
    (void)connect( fModel, &QAbstractItemModel::rowsRemoved, this, [ this ]() { slotDataChanged(); } );
    (void)connect( fModel, &QAbstractItemModel::rowsMoved, this, [ this ]() { slotDataChanged(); } );

    (void)connect( fImpl->btn_0, &QToolButton::clicked, this, [this](){ addValue( '0' ); } );
    (void)connect( fImpl->btn_1, &QToolButton::clicked, this, [this](){ addValue( '1' ); } );
    (void)connect( fImpl->btn_2, &QToolButton::clicked, this, [this](){ addValue( '2' ); } );
    (void)connect( fImpl->btn_3, &QToolButton::clicked, this, [this](){ addValue( '3' ); } );
    (void)connect( fImpl->btn_4, &QToolButton::clicked, this, [this](){ addValue( '4' ); } );
    (void)connect( fImpl->btn_5, &QToolButton::clicked, this, [this](){ addValue( '5' ); } );
    (void)connect( fImpl->btn_6, &QToolButton::clicked, this, [this](){ addValue( '6' ); } );
    (void)connect( fImpl->btn_7, &QToolButton::clicked, this, [this](){ addValue( '7' ); } );
    (void)connect( fImpl->btn_8, &QToolButton::clicked, this, [this](){ addValue( '8' ); } );
    (void)connect( fImpl->btn_9, &QToolButton::clicked, this, [this](){ addValue( '9' ); } );
    (void)connect( fImpl->btn_period, &QToolButton::clicked, this, [ this ]() { addValue( '.' ); } );

    (void)connect( fImpl->btn_enter, &QToolButton::clicked, this, &CNarcissisticNumbers::btnEnterClicked );
    (void)connect( fImpl->btn_C, &QToolButton::clicked, this, &CNarcissisticNumbers::btnCClicked );
    (void)connect( fImpl->btn_Del, &QToolButton::clicked, this, &CNarcissisticNumbers::btnDelClicked );
    (void)connect( fImpl->btn_BS, &QToolButton::clicked, this, &CNarcissisticNumbers::btnBSClicked );

    (void)connect( fImpl->btn_plus, &QToolButton::clicked, this, [this](){ binaryOperatorClicked( '+' ); } );
    (void)connect( fImpl->btn_minus, &QToolButton::clicked, this, [ this ]() { binaryOperatorClicked( '-' ); } );
    (void)connect( fImpl->btn_mult, &QToolButton::clicked, this, [ this ]() { binaryOperatorClicked( '*' ); } );
    (void)connect( fImpl->btn_div, &QToolButton::clicked, this, [ this ]() { binaryOperatorClicked( '/' ); } );

    (void)connect( fImpl->btn_Average, &QToolButton::clicked, this, &CNarcissisticNumbers::btnAverageClicked );
    (void)connect( fImpl->btn_Narcissistic, &QToolButton::clicked, this, &CNarcissisticNumbers::btnNarcissisticClicked );
    (void)connect( fImpl->btn_Factors, &QToolButton::clicked, this, [this](){ btnFactorsClicked( true ); } );
    (void)connect( fImpl->btn_ProperFactors, &QToolButton::clicked, this, [ this ]() { btnFactorsClicked( false ); } );
    (void)connect( fImpl->btn_PrimeFactors, &QToolButton::clicked, this, [ this ]() { btnPrimeFactorsClicked(); } );
    (void)connect( fImpl->btn_Perfect, &QToolButton::clicked, this, [ this ]() { btnPerfectClicked(); } );
    (void)connect( fImpl->btn_SemiPerfect, &QToolButton::clicked, this, [ this ]() { btnSemiPerfectClicked(); } );
    
    (void)connect( fImpl->btn_Abundant, &QToolButton::clicked, this, [ this ]() { btnAbundantClicked(); } );
    (void)connect( fImpl->btn_Weird, &QToolButton::clicked, this, [ this ]() { btnWeirdClicked(); } );
    (void)connect( fImpl->btn_Sublime, &QToolButton::clicked, this, [ this ]() { btnSublimeClicked(); } );


    fImpl->values->installEventFilter( this );
    setFocus( Qt::MouseFocusReason );
    initMaps();
    slotDataChanged();
}

void CNarcissisticNumbers::initMaps()
{
    fOpMap =
    {
          { '+', [ this ]( double lhs, double rhs ) { return lhs + rhs; } }
         ,{ '-', [ this ]( double lhs, double rhs ) { return lhs - rhs; } }
         ,{ '*', [ this ]( double lhs, double rhs ) { return lhs * rhs; } }
         ,{ '/', [ this ]( double lhs, double rhs ) { return lhs / rhs; } }
    };

    fNumRowsPerFunctionMap =
    {
         { fImpl->btn_0, 0 }
        ,{ fImpl->btn_1, 0 }
        ,{ fImpl->btn_2, 0 }
        ,{ fImpl->btn_3, 0 }
        ,{ fImpl->btn_4, 0 }
        ,{ fImpl->btn_5, 0 }
        ,{ fImpl->btn_6, 0 }
        ,{ fImpl->btn_7, 0 }
        ,{ fImpl->btn_8, 0 }
        ,{ fImpl->btn_9, 0 }
        ,{ fImpl->btn_enter, 1 }
        ,{ fImpl->btn_plus, 2 }
        ,{ fImpl->btn_minus, 2 }
        ,{ fImpl->btn_mult, 2 }
        ,{ fImpl->btn_div, 2 }
        ,{ fImpl->btn_C, 1 }
        ,{ fImpl->btn_BS, 1 }
        ,{ fImpl->btn_Del, 1 }
        ,{ fImpl->btn_period, 0 }
        ,{ fImpl->btn_Average, 1 }
        ,{ fImpl->btn_Narcissistic, 1 }
        ,{ fImpl->btn_Factors, 1 }
        ,{ fImpl->btn_ProperFactors, 1 }
        ,{ fImpl->btn_PrimeFactors, 1 }
        ,{ fImpl->btn_Perfect, 1 }
        ,{ fImpl->btn_SemiPerfect, 1 }
        ,{ fImpl->btn_Abundant, 1 }
        ,{ fImpl->btn_Weird, 1 }
        ,{ fImpl->btn_Sublime, 1 }
    };

    fKeyMap =
    {
         { Qt::Key_0, [ this ]() { fImpl->btn_0->animateClick(); } }
        ,{ Qt::Key_1, [ this ]() { fImpl->btn_1->animateClick(); } }
        ,{ Qt::Key_2, [ this ]() { fImpl->btn_2->animateClick(); } }
        ,{ Qt::Key_3, [ this ]() { fImpl->btn_3->animateClick(); } }
        ,{ Qt::Key_4, [ this ]() { fImpl->btn_4->animateClick(); } }
        ,{ Qt::Key_5, [ this ]() { fImpl->btn_5->animateClick(); } }
        ,{ Qt::Key_6, [ this ]() { fImpl->btn_6->animateClick(); } }
        ,{ Qt::Key_7, [ this ]() { fImpl->btn_7->animateClick(); } }
        ,{ Qt::Key_8, [ this ]() { fImpl->btn_8->animateClick(); } }
        ,{ Qt::Key_9, [ this ]() { fImpl->btn_9->animateClick(); } }
        ,{ Qt::Key_Enter, [ this ]() { fImpl->btn_enter->animateClick(); } }
        ,{ Qt::Key_Return, [ this ]() { fImpl->btn_enter->animateClick(); } }
        ,{ Qt::Key_Plus, [ this ]() { fImpl->btn_plus->animateClick(); } }
        ,{ Qt::Key_Minus, [ this ]() { fImpl->btn_minus->animateClick(); } }
        ,{ Qt::Key_Asterisk, [ this ]() { fImpl->btn_mult->animateClick(); } }
        ,{ Qt::Key_Slash, [ this ]() { fImpl->btn_div->animateClick(); } }
        ,{ Qt::Key_Backspace, [ this ]() { fImpl->btn_BS->animateClick(); } }
        ,{ Qt::Key_Delete, [ this ]() { fImpl->btn_Del->animateClick(); } }
    };
}
bool CNarcissisticNumbers::eventFilter( QObject* obj, QEvent* event )
{
    if ( ( obj == fImpl->values ) || ( obj == this ) )
    {
        if ( event->type() == QEvent::KeyPress )
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>( event );
            keyPressEvent( keyEvent );
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        // pass the event on to the parent class
        return QDialog::eventFilter( obj, event );
    }
}

void CNarcissisticNumbers::slotDataChanged()
{
    auto numRows = fModel->rowCount();
    for( auto && ii : fNumRowsPerFunctionMap )
    {
        ii.first->setEnabled( numRows >= ii.second );
    }
}

void CNarcissisticNumbers::keyPressEvent( QKeyEvent* event )
{
    auto pos = fKeyMap.find( event->key() );
    if ( pos != fKeyMap.end() )
    {
        ((*pos).second)();
        return;
    }
    QDialog::keyPressEvent( event );
}

CNarcissisticNumbers::~CNarcissisticNumbers()
{
}

int CNarcissisticNumbers::numValues() const
{
    return fModel->rowCount();
}

template< typename T1 >
auto toNum( const QVariant& string, bool* aOK )
-> typename std::enable_if< std::is_floating_point< T1 >::value, T1 >::type
{
    return string.toDouble( aOK );
}

template< typename T1 >
auto toNum( const QVariant& string, bool * aOK )
-> typename std::enable_if< std::is_integral< T1 >::value, T1 >::type
{
    return string.toLongLong( aOK );
}

template< typename T >
T CNarcissisticNumbers::getLastValue( bool popLast )
{
    if ( fModel->rowCount() == 0 )
        return 0.0;

    QModelIndex mi = fModel->index( fModel->rowCount() - 1 );
    auto lastValue = mi.data();
    if ( lastValue.toString().isEmpty() )
    {
        if ( !popLast )
            return 0.0;
        fModel->removeRows( fModel->rowCount() - 1, 1 );
        mi = fModel->index( fModel->rowCount() - 1 );
    }

    bool aOK = false;
    T currValue;
    if ( lastValue.toString().toLower() == "nan" )
    {
        currValue = std::numeric_limits< T >::quiet_NaN();
        aOK = true;
    }
    else if ( lastValue.toString().toLower() == "inf" )
    {
        currValue = std::numeric_limits< T >::infinity();
        aOK = true;
    }
    else
        currValue = toNum< T >( lastValue, &aOK );

    if ( popLast )
        fModel->removeRows( fModel->rowCount() - 1, 1 );

    if ( !aOK )
    {
        return 0.0;
    }

    return currValue;
}


void CNarcissisticNumbers::addValue( char value )
{
    QModelIndex mi;
    QString currValue;
    if ( fModel->rowCount() )
    {
        mi = fModel->index( fModel->rowCount() - 1 );
        currValue = fModel->data( mi, Qt::EditRole ).toString();
    }
    else
    {
        fModel->insertRows( fModel->rowCount(), 1 );
        mi = fModel->index( 0, 0 );
    }
    currValue += value;
    fModel->setData( mi, currValue );
}

void CNarcissisticNumbers::addLastValue( double value )
{
    addLastValue( QString::number( value ) );
}

void CNarcissisticNumbers::addLastValue( int64_t value )
{
    addLastValue( QString::number( value ) );
}

void CNarcissisticNumbers::addLastValue( bool value )
{
    addLastValue( value ? tr( "Yes" ) : tr( "No" ) );
}

void CNarcissisticNumbers::addLastValue( const QString& newValue )
{
    fModel->insertRows( fModel->rowCount(), 1 );
    QModelIndex mi = fModel->index( fModel->rowCount() - 1 );
    fModel->setData( mi, newValue );
}

std::pair< int64_t, std::list< int64_t > > CNarcissisticNumbers::getSumOfFactors( int64_t curr, bool properFactors ) const
{
    auto factors = computeFactors( curr );
    if ( properFactors && !factors.empty() )
        factors.pop_back();
    int64_t sum = 0;
    for ( auto ii : factors )
        sum += ii;
    return std::make_pair( sum, factors );
}

void CNarcissisticNumbers::btnEnterClicked()
{
    if ( fModel->rowCount() == 0 )
        return;

    QModelIndex mi = fModel->index( fModel->rowCount() - 1 );
    QString currValue = fModel->data( mi, Qt::EditRole ).toString();
    if ( currValue.isEmpty() )
        return;

    fModel->insertRows( fModel->rowCount(), 1 );
}

void CNarcissisticNumbers::btnDelClicked()
{
    if ( numValues() == 0 )
        return;

    fModel->removeRows( fModel->rowCount() - 1, 1 );
}

void CNarcissisticNumbers::btnBSClicked()
{
    if ( numValues() == 0 )
        return;

    QModelIndex mi = fModel->index( fModel->rowCount() - 1 );
    QString currValue = fModel->data( mi, Qt::EditRole ).toString();
    if ( currValue.isEmpty() )
        return;
    currValue = currValue.left( currValue.length() - 1 );
    fModel->setData( mi, currValue );
}

void CNarcissisticNumbers::btnCClicked() // clear all
{
    fModel->setStringList( QStringList() );
}

void CNarcissisticNumbers::binaryOperatorClicked( char op )
{
    if ( fModel->rowCount() < 2 )
        return;

    auto val2 = getLastValue< double >( true );
    auto val1 = getLastValue< double >( true );

    auto pos = fOpMap.find( op );
    if ( pos == fOpMap.end() )
        return;
    auto newValue = (*pos).second( val1, val2 );
    addLastValue( newValue );
}

void CNarcissisticNumbers::reportPrime( std::list<int64_t>& factors, int64_t curr, bool incNum, int numShowPrime )
{
    if ( factors.size() == numShowPrime )
    {
        addLastValue( tr( "%1 is a prime number" ).arg( curr ) );
        return;
    }
    if ( !incNum )
        factors.pop_back();
    for ( auto&& ii : factors )
        addLastValue( ii );
}

void CNarcissisticNumbers::btnAverageClicked()
{
    if ( numValues() < 1 )
        return;

    int num = 0;
    double total = 0;
    while ( numValues() > 0 )
    {
        num = num + 1;
        auto curr = getLastValue< double >( true );
        total += curr;
    }
    double newValue = total / num;
    addLastValue( newValue );
}

void CNarcissisticNumbers::btnNarcissisticClicked()
{
    if ( numValues() < 1 )
        return;

    auto curr = getLastValue< int64_t >( false );

    bool aOK;
    bool isNarc = NUtils::isNarcissistic( curr, 10, aOK );
    if ( aOK )
    {
        addLastValue( isNarc );
    }
}

std::list< int64_t > CNarcissisticNumbers::computeFactors( int64_t num ) const
{
    std::list< int64_t > retVal;
    std::list< int64_t > retVal2;
    retVal.push_back( 1 );
    retVal2.push_back( num );

    // only need to go to half way point
    auto lastNum = ( num / 2 ) + ( ( ( num % 2 ) == 0 ) ? 0 : 1 );
    for( int64_t ii = 2; ii < lastNum; ++ii )
    {
        if ( ( num % ii ) == 0 )
        {
            retVal.push_back( ii );
            auto other = num / ii;
            lastNum = std::min( lastNum, other );
            retVal2.push_front( other );
        }
    }

    if ( *retVal.rbegin() == *retVal2.begin() )
        retVal2.pop_front();
    retVal.insert( retVal.end(), retVal2.begin(), retVal2.end() );
    retVal.sort();
    return retVal;
}


std::list< int64_t > CNarcissisticNumbers::computePrimeFactors( int64_t num ) const
{
    std::list< int64_t > retVal;

    while( ( num % 2 ) == 0 )
    {
        retVal.push_back( 2 );
        num = num / 2 ;
    }

    int64_t lastNum = std::sqrt( num );

    for( int64_t ii = 3; ii < lastNum; ii = ii + 2 )
    {
        while( ( num % ii )  == 0 )
        {
            retVal.push_back( ii );
            num = num / ii;
        }
    }
    if ( num > 2 )
        retVal.push_back( num );
    return retVal;
}

bool CNarcissisticNumbers::isSemiPerfect( const std::vector< int64_t >& factors, size_t n, int64_t num ) const
{
    if ( num == 0 )
        return true;
    if ( n == 0 && num != 0 )
        return false;

    if ( factors[ n - 1 ] > num )
        return isSemiPerfect( factors, n - 1, num );
    return isSemiPerfect( factors, n - 1, num ) 
        || isSemiPerfect( factors, n - 1, num - factors[ n - 1 ] );
}

std::pair< bool, std::list< int64_t > > CNarcissisticNumbers::isPerfect( int64_t num ) const
{
    auto sum = getSumOfFactors( num, true );
    return std::make_pair( sum.first == num, sum.second );
}

std::pair< bool, std::list< int64_t > > CNarcissisticNumbers::isSemiPerfect( int64_t num ) const
{
    auto sum = getSumOfFactors( num, true );
    auto factors = std::vector< int64_t >( { sum.second.begin(), sum.second.end() } );
    auto isSemiPerfect = this->isSemiPerfect( factors, factors.size(), num );
    return std::make_pair( isSemiPerfect, sum.second );
}

std::pair< bool, std::list< int64_t > > CNarcissisticNumbers::isAbundant( int64_t num ) const
{
    auto sum = getSumOfFactors( num, true );
    return std::make_pair( sum.first > num, sum.second );
}


void CNarcissisticNumbers::btnFactorsClicked( bool incNum )
{
    if ( numValues() < 1 )
        return;
    auto curr = getLastValue< int64_t >( false );
    auto factors = computeFactors( curr );

    reportPrime( factors, curr, incNum, 2 );
}

void CNarcissisticNumbers::btnPrimeFactorsClicked()
{
    if ( numValues() < 1 )
        return;
    auto curr = getLastValue< int64_t >( false );
    auto factors = computePrimeFactors( curr );

    reportPrime( factors, curr, true, 1 );
}

void CNarcissisticNumbers::btnPerfectClicked()
{
    if ( numValues() < 1 )
        return;
    auto curr = getLastValue< int64_t >( false );

    addLastValue( isPerfect( curr ).first );
}

void CNarcissisticNumbers::btnSemiPerfectClicked()
{
    if ( numValues() < 1 )
        return;
    auto curr = getLastValue< int64_t >( false );
    addLastValue( isSemiPerfect( curr ).first );
}

void CNarcissisticNumbers::btnAbundantClicked()
{
    if ( numValues() < 1 )
        return;
    auto curr = getLastValue< int64_t >( false );
    addLastValue( isAbundant( curr ).first );
}

void CNarcissisticNumbers::btnWeirdClicked()
{
    if ( numValues() < 1 )
        return;
    auto curr = getLastValue< int64_t >( false );
    auto isAbundant = this->isAbundant( curr );
    if ( !isAbundant.first )
    {
        addLastValue( false );
        return;
    }

    std::vector< int64_t > factors( { isAbundant.second.begin(), isAbundant.second.end() } );
    addLastValue( !isSemiPerfect( factors, factors.size(), curr ) );
}

void CNarcissisticNumbers::btnSublimeClicked()
{
    if ( numValues() < 1 )
        return;
    auto curr = getLastValue< int64_t >( false );
    auto sumOfFactors = getSumOfFactors( curr, false );

    auto isNumFactorsPerfect = isPerfect( sumOfFactors.second.size() ).first;
    auto isSumPerfect = isPerfect( sumOfFactors.first ).first;
    addLastValue( isNumFactorsPerfect && isSumPerfect );
}
